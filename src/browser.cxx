#include "browser/client.hxx"
#include "browser.hxx"
#include "include/cef_life_span_handler.h"
#include "include/cef_task.h"
#include "include/cef_base.h"
#include "include/views/cef_window.h"

#include <algorithm>

#include <fmt/core.h>

struct CloseTask: public CefTask {
	CloseTask(CefRefPtr<Browser::Window> window): window(window) {}
	void Execute() override { window->Close(); }
	private:
		CefRefPtr<Browser::Window> window;
		IMPLEMENT_REFCOUNTING(CloseTask);
		DISALLOW_COPY_AND_ASSIGN(CloseTask);
};

struct DevtoolsTask: public CefTask {
	DevtoolsTask(CefRefPtr<Browser::Window> window): window(window) {}
	void Execute() override { window->ShowDevTools(); }
	private:
		CefRefPtr<Browser::Window> window;
		IMPLEMENT_REFCOUNTING(DevtoolsTask);
		DISALLOW_COPY_AND_ASSIGN(DevtoolsTask);
};

Browser::Window::Window(CefRefPtr<Browser::Client> client, Browser::Details details, CefString url, bool show_devtools):
	browser_count(0), client(client.get()), show_devtools(show_devtools), details(details), window(nullptr), browser_view(nullptr), browser(nullptr), devtools(nullptr), pending_delete(false)
{
	fmt::print("[B] Browser::Window direct constructor, this={}\n", reinterpret_cast<uintptr_t>(this));
	CefRefPtr<CefDictionaryValue> dict = nullptr;
	if (details.has_custom_js) {
		dict = CefDictionaryValue::Create();
		dict->SetString("customjs", details.custom_js);
	}
	this->Init(url, dict);
}

Browser::Window::Window(CefRefPtr<Browser::Client> client, Browser::Details details, bool show_devtools):
	browser_count(0), client(client.get()), show_devtools(show_devtools), details(details), window(nullptr), browser_view(nullptr), browser(nullptr), devtools(nullptr), pending_delete(false)
{
	fmt::print("[B] Browser::Window indirect constructor, this={}\n", reinterpret_cast<uintptr_t>(this));
}

void Browser::Window::Init(CefString url, CefRefPtr<CefDictionaryValue> extra_info) {
	CefBrowserSettings browser_settings;
	browser_settings.background_color = CefColorSetARGB(0, 0, 0, 0);
	this->browser_view = CefBrowserView::CreateBrowserView(this, url, browser_settings, extra_info, nullptr, this);
	CefWindow::CreateTopLevelWindow(this);
}

void Browser::Window::OnWindowCreated(CefRefPtr<CefWindow> window) {
	fmt::print("[B] OnWindowCreated {} this={}\n", window->GetID(), reinterpret_cast<uintptr_t>(this));
	this->client->OnBoltWindowCreated(window);
	this->window = std::move(window);
	this->window->AddChildView(this->browser_view);
	if (this->details.center_on_open) {
		this->window->CenterWindow(CefSize(this->details.preferred_width, this->details.preferred_height));
	}
	this->window->SetTitle("Tideline");
	this->window->Show();
}

void Browser::Window::OnWindowClosing(CefRefPtr<CefWindow> window) {
	fmt::print("[B] OnWindowClosing {} this={}\n", window->GetID(), reinterpret_cast<uintptr_t>(this));
	this->window = nullptr;
	this->browser_view = nullptr;
}

CefRect Browser::Window::GetInitialBounds(CefRefPtr<CefWindow> window) {
	return cef_rect_t {
		.x = this->details.startx,
		.y = this->details.starty,
		.width = this->details.preferred_width,
		.height = this->details.preferred_height,
	};
}

cef_show_state_t Browser::Window::GetInitialShowState(CefRefPtr<CefWindow>) {
	return CEF_SHOW_STATE_NORMAL;
}

bool Browser::Window::IsFrameless(CefRefPtr<CefWindow>) {
	return !this->details.frame;
}

bool Browser::Window::CanResize(CefRefPtr<CefWindow>) {
	return this->details.resizeable;
}

bool Browser::Window::CanMaximize(CefRefPtr<CefWindow>) {
	return true;
}

bool Browser::Window::CanMinimize(CefRefPtr<CefWindow>) {
	return true;
}

bool Browser::Window::CanClose(CefRefPtr<CefWindow> win) {
	fmt::print("[B] CanClose for window {}, this={}\n", window->GetID(), reinterpret_cast<uintptr_t>(this));
	if (this->devtools) {
		this->devtools->browser->GetHost()->CloseBrowser(true);
		this->devtools = nullptr;
	}

	return true;
}

CefSize Browser::Window::GetPreferredSize(CefRefPtr<CefView>) {
	return CefSize(this->details.preferred_width, this->details.preferred_height);
}

#if defined(__linux__)
bool Browser::Window::GetLinuxWindowProperties(CefRefPtr<CefWindow>, CefLinuxWindowProperties& properties) {
	cef_string_from_utf8(BOLT_WINDOW_CLASS, sizeof(BOLT_WINDOW_CLASS) - sizeof(*BOLT_WINDOW_CLASS), &properties.wayland_app_id);
	properties.wm_class_class = { .str = properties.wayland_app_id.str, .length = properties.wayland_app_id.length, .dtor = nullptr };
	cef_string_from_utf8(BOLT_EXE_NAME, sizeof(BOLT_EXE_NAME) - sizeof(*BOLT_EXE_NAME), &properties.wm_class_name);
	return true;
}
#endif

CefRefPtr<CefBrowserViewDelegate> Browser::Window::GetDelegateForPopupBrowserView(CefRefPtr<CefBrowserView>, const CefBrowserSettings&, CefRefPtr<CefClient>, bool is_devtools) {
	fmt::print("[B] GetDelegateForPopupBrowserView this={}\n", reinterpret_cast<uintptr_t>(this));
	if (!is_devtools) {
		return nullptr;
	}
	if (this->devtools) {
		this->devtools->Focus();
		return nullptr;
	}
	Browser::Details details = {
		.preferred_width = this->popup_features.widthSet ? this->popup_features.width : 800,
		.preferred_height = this->popup_features.heightSet ? this->popup_features.height : 608,
		.startx = this->popup_features.x,
		.starty = this->popup_features.y,
		.center_on_open = !this->popup_features.xSet || !this->popup_features.ySet,
		.resizeable = true,
		.frame = true,
		.is_devtools = true,
	};
	this->devtools = new Browser::Window(this->client, details, this->show_devtools);
	return this->devtools;
}

bool Browser::Window::OnPopupBrowserViewCreated(CefRefPtr<CefBrowserView> browser_view, CefRefPtr<CefBrowserView> popup_browser_view, bool is_devtools) {
	fmt::print("[B] OnPopupBrowserViewCreated this={}\n", reinterpret_cast<uintptr_t>(this));
	this->devtools->browser_view = popup_browser_view;
	CefWindow::CreateTopLevelWindow(this->devtools);
	return true;
}

void Browser::Window::OnBrowserCreated(CefRefPtr<CefBrowserView> browser_view, CefRefPtr<CefBrowser> browser) {
	fmt::print("[B] OnBrowserCreated this={} {}\n", reinterpret_cast<uintptr_t>(this), browser->GetIdentifier());
	this->browser = browser;
	if (this->show_devtools && !this->details.is_devtools) {
		this->ShowDevTools();
	}
}

void Browser::Window::OnBrowserDestroyed(CefRefPtr<CefBrowserView>, CefRefPtr<CefBrowser>) {
	fmt::print("[B] OnBrowserDestroyed this={}\n", reinterpret_cast<uintptr_t>(this));
}

bool Browser::Window::IsSameBrowser(CefRefPtr<CefBrowser> browser) const {
	return this->browser->IsSame(browser);
}

void Browser::Window::Focus() const {
	CefRefPtr<CefBrowserHost> host = this->browser->GetHost();
	if (host) {
		host->SetFocus(true);
	}
}

void Browser::Window::Close() {
	fmt::print("[B] Close this={}\n", reinterpret_cast<uintptr_t>(this));
	if (CefCurrentlyOn(TID_UI)) {
		if (this->browser) {
			this->browser->GetHost()->TryCloseBrowser();
		} else {
			this->pending_delete = true;
		}
	} else {
		CefPostTask(TID_UI, new CloseTask(this));
	}
}

void Browser::Window::ShowDevTools() {
	if (CefCurrentlyOn(TID_UI)) {
		if (this->browser) {
			this->browser->GetHost()->ShowDevTools(CefWindowInfo(), this, CefBrowserSettings(), CefPoint());
		} else {
			this->show_devtools = true;
		}
	} else {
		CefPostTask(TID_UI, new DevtoolsTask(this));
	}
}

void Browser::Window::NotifyClosed() { }

void Browser::Window::SendMessage(CefString str) {
	this->browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, CefProcessMessage::Create(str));
}

CefRefPtr<CefLifeSpanHandler> Browser::Window::GetLifeSpanHandler() {
	return this;
}

bool Browser::Window::OnBeforePopup(
	CefRefPtr<CefBrowser> browser,
	CefRefPtr<CefFrame> frame,
	int popup_id,
	const CefString& target_url,
	const CefString& target_frame_name,
	CefLifeSpanHandler::WindowOpenDisposition target_disposition,
	bool user_gesture,
	const CefPopupFeatures& popup_features,
	CefWindowInfo& window_info,
	CefRefPtr<CefClient>& client,
	CefBrowserSettings& settings,
	CefRefPtr<CefDictionaryValue>& extra_info,
	bool* no_javascript_access
) {
	// block popup creation (window.open)
	return true;
}

void Browser::Window::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
	fmt::print("[B] Browser::OnAfterCreated for browser {}\n", browser->GetIdentifier());
	this->browser_count += 1;
	if (this->pending_delete) {
		// calling CloseBrowser here would lead to a segmentation fault in CEF because we're still
		// technically in the create function, which is going to assume the browser still exists.
		CefPostTask(TID_UI, new CloseTask(this));
	}
}

void Browser::Window::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
	fmt::print("[B] Browser::OnBeforeClose for browser {}\n", browser->GetIdentifier());
	if (this->browser && browser->IsSame(this->browser)) {
		this->browser = nullptr;
	}
	this->browser_count -= 1;
	if (this->browser_count == 0) {
		this->NotifyClosed();
	}
}
