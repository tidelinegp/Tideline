#include "resource_handler.hxx"
#include "include/cef_urlrequest.h"
#include "include/internal/cef_types.h"

// returns content-length or -1 if it's malformed or nonexistent
// CEF interprets -1 as "unknown length"
static int64_t GetContentLength(CefRefPtr<CefResponse> response) {
	const std::string content_length = response->GetHeaderByName("Content-Length").ToString();
	if (content_length.size() == 0) {
		return -1;
	}
	int64_t length = 0;
	for (auto it = content_length.begin(); it != content_length.end(); it++) {
		if (*it < '0' || *it > '9') return -1;
		length = (length * 10) + ((int64_t)(*it) - '0');
	}
	return length;
}

bool Browser::ResourceHandler::Open(CefRefPtr<CefRequest>, bool& handle_request, CefRefPtr<CefCallback>) {
	handle_request = true;
	return true;
}

void Browser::ResourceHandler::GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t& response_length, CefString& redirectUrl) {
	response->SetStatus(this->status);
	response->SetMimeType(this->mime);

	// there's never a reason to apply access control to files served from memory
	response->SetHeaderByName("Access-Control-Allow-Origin", "*", true);

	if (this->has_location) {
		redirectUrl = this->location;
		response->SetHeaderByName("Location", this->location, false);
	}
	response_length = this->data_len;
}

bool Browser::ResourceHandler::Read(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefResourceReadCallback>) {
	if (this->cursor == this->data_len) {
		// "To indicate response completion set |bytes_read| to 0 and return false."
		bytes_read = 0;
		return false;
	}
	if (this->cursor + bytes_to_read <= this->data_len) {
		// requested read is entirely in bounds
		bytes_read = bytes_to_read;
		memcpy(data_out, this->data + this->cursor, bytes_read);
		this->cursor += bytes_to_read;
	} else {
		// read only to end of string
		bytes_read = this->data_len - this->cursor;
		memcpy(data_out, this->data + this->cursor, bytes_read);
		this->cursor = this->data_len;
	}
	if (this->cursor == this->data_len) this->finish();
	return true;
}

bool Browser::ResourceHandler::Skip(int64_t bytes_to_skip, int64_t& bytes_skipped, CefRefPtr<CefResourceSkipCallback>) {
	if (this->cursor + bytes_to_skip <= this->data_len) {
		// skip in bounds
		bytes_skipped = bytes_to_skip;
		this->cursor += bytes_to_skip;
	} else {
		// skip to end of string
		bytes_skipped = this->data_len - this->cursor;
		this->cursor = this->data_len;
	}
	return true;
}

void Browser::ResourceHandler::Cancel() {
	if (this->cursor != this->data_len) {
		this->cursor = this->data_len;
		this->finish();
	}
}

CefRefPtr<CefResourceHandler> Browser::ResourceHandler::GetResourceHandler(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefRefPtr<CefRequest>) {
	return this;
}

void Browser::ResourceHandler::finish() {
	if (this->file_manager) {
		this->file_manager->free(FileManager::File { .contents = this->data, .size = this->data_len, .mime_type = this->mime });
		this->file_manager = nullptr;
	}
}

Browser::URLHandler::URLHandler(CefRefPtr<CefRequest> request, CefRefPtr<CefRequestContext> context): urlrequest_complete(false), headers_checked(false), urlrequest_callback(nullptr), cursor(0), return_data_ptr(nullptr), status_override(-1), mime_type_override(nullptr), location_override(nullptr) {
	this->url_request = CefURLRequest::Create(request, this, context);
}

CefResourceRequestHandler::ReturnValue Browser::URLHandler::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
	// use CefResourceRequestHandler::OnBeforeResourceLoad to stall asynchronously on the IO thread
	// until the request completes, if it hasn't already
	if (urlrequest_complete) {
		return RV_CONTINUE;
	} else {
		this->urlrequest_callback = callback;
		return RV_CONTINUE_ASYNC;
	}
}

CefRefPtr<CefResourceHandler> Browser::URLHandler::GetResourceHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request) {
	return this;
}

bool Browser::URLHandler::Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback) {
	handle_request = true;
	return true;
}

void Browser::URLHandler::GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t& response_length, CefString& redirectUrl) {
	CefRefPtr<CefResponse> url_response = this->url_request->GetResponse();
	CefResponse::HeaderMap headers;
	url_response->GetHeaderMap(headers);
	response->SetHeaderMap(headers);
	response->SetCharset(url_response->GetCharset());

	if (this->status_override >= 0) {
		response->SetStatus(this->status_override);
	} else {
		response->SetStatus(url_response->GetStatus());
		response->SetStatusText(url_response->GetStatusText());
	}

	if (this->mime_type_override) {
		const CefString mime(this->mime_type_override);
		response->SetMimeType(mime);
		response->SetHeaderByName("Content-Type", mime, true);
	} else {
		response->SetMimeType(url_response->GetMimeType());
	}

	if (this->location_override) {
		const CefString location(this->location_override);
		redirectUrl = location;
		response->SetHeaderByName("Location", location, true);
	} else {
		redirectUrl = url_response->GetHeaderByName("Location");
	}

	response_length = GetContentLength(url_response);
}

bool Browser::URLHandler::Read(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefResourceReadCallback> callback) {
	const void* data = this->return_data_ptr;
	size_t data_length = this->return_data_length;
	if (!data) {
		data = this->data.data();
		data_length = this->data.size();
	}

	if (this->cursor == data_length) {
		// "To indicate response completion set |bytes_read| to 0 and return false."
		bytes_read = 0;
		return false;
	}
	if (this->cursor + bytes_to_read <= data_length) {
		// requested read is entirely in bounds
		bytes_read = bytes_to_read;
		memcpy(data_out, reinterpret_cast<const char*>(data) + this->cursor, bytes_read);
		this->cursor += bytes_to_read;
	} else {
		// read only to end of string
		bytes_read = data_length - this->cursor;
		memcpy(data_out, reinterpret_cast<const char*>(data) + this->cursor, bytes_read);
		this->cursor = data_length;
	}
	return true;
}

bool Browser::URLHandler::Skip(int64_t bytes_to_skip, int64_t& bytes_skipped, CefRefPtr<CefResourceSkipCallback> callback) {
	const size_t data_length = this->return_data_ptr ? this->return_data_length : this->data.size();
	if (this->cursor + bytes_to_skip <= data_length) {
		// skip in bounds
		bytes_skipped = bytes_to_skip;
		this->cursor += bytes_to_skip;
	} else {
		// skip to end of string
		bytes_skipped = data_length - this->cursor;
		this->cursor = data_length;
	}
	return true;
}

void Browser::URLHandler::Cancel() {
	this->cursor = this->data.size();
}

void Browser::URLHandler::OnRequestComplete(CefRefPtr<CefURLRequest> request) {
	this->urlrequest_complete = true;
	if (this->urlrequest_callback) {
		this->urlrequest_callback->Continue();
	}
}

void Browser::URLHandler::OnUploadProgress(CefRefPtr<CefURLRequest> request, int64_t current, int64_t total) {}
void Browser::URLHandler::OnDownloadProgress(CefRefPtr<CefURLRequest> request, int64_t current, int64_t total) {}

void Browser::URLHandler::OnDownloadData(CefRefPtr<CefURLRequest> request, const void* data, size_t data_length) {
	CefRefPtr<CefResponse> response = request->GetResponse();
	if (!this->headers_checked && response) {
		const int64_t content_length = GetContentLength(response);
		if (content_length != -1) this->data.reserve(content_length);
		this->headers_checked = true;
	}
	size_t write_offset = this->data.size();
	this->data.resize(write_offset + data_length);
	memcpy(this->data.data() + write_offset, data, data_length);
}

bool Browser::URLHandler::GetAuthCredentials(bool isProxy, const CefString& host, int port, const CefString& realm, const CefString& scheme, CefRefPtr<CefAuthCallback> callback) {
	return false;
}

void Browser::URLHandlerNoCORS::GetResponseHeaders(CefRefPtr<CefResponse> response, int64_t& response_length, CefString& redirectUrl) {
	Browser::URLHandler::GetResponseHeaders(response, response_length, redirectUrl);
	response->SetHeaderByName("Access-Control-Allow-Origin", "*", true);
}
