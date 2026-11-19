#include "client.hxx"
#include "window_launcher.hxx"
#include "include/cef_parser.h"
#include "include/cef_resource_request_handler.h"
#include "include/cef_values.h"
#include "include/internal/cef_types.h"
#include "resource_handler.hxx"
#include "request.hxx"

#include <fcntl.h>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <sstream>

#if defined(_WIN32)
#include <windows.h>
#include <wininet.h>
#include <bcrypt.h>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "bcrypt.lib")
#endif

#include <atomic>
#include <mutex>
#include <thread>
#include "../sha256/sha256.h"
#include "update_key.h"

#if defined(HAS_LIBARCHIVE) && defined(BOLT_PLUGINS)
#include <archive.h>
#include <archive_entry.h>
#endif

#define BOLTDOMAIN "bolt-internal"
#define BOLTURI "https://" BOLTDOMAIN
#define DEFAULTURL BOLTURI "/index.html"

#define ENDPOINT_ACCOUNTS "/game-session/v1/accounts"

#if defined(__linux__)
#define URLPLATFORM "linux"
#endif

#if defined(_WIN32)
#define URLPLATFORM "windows"
#endif

#if defined(__APPLE__)
#define URLPLATFORM "mac"
#endif

// window controls for the frameless launcher window; CefWindow must be used from the UI thread
struct WindowControlTask: public CefTask {
	enum class Action { Minimize, ToggleMaximize };
	WindowControlTask(CefRefPtr<CefWindow> window, Action action): window(window), action(action) {}
	void Execute() override {
		if (!window) return;
		switch (action) {
			case Action::Minimize:
				window->Minimize();
				break;
			case Action::ToggleMaximize:
				if (window->IsMaximized()) {
					window->Restore();
				} else {
					window->Maximize();
				}
				break;
		}
	}
	private:
		CefRefPtr<CefWindow> window;
		Action action;
		IMPLEMENT_REFCOUNTING(WindowControlTask);
		DISALLOW_COPY_AND_ASSIGN(WindowControlTask);
};

struct OpenLoginWindowTask: public CefTask {
	OpenLoginWindowTask(CefRefPtr<Browser::Client> client): client(client) {}
	void Execute() override { client->OpenLoginWindow(); }
	private:
		CefRefPtr<Browser::Client> client;
		IMPLEMENT_REFCOUNTING(OpenLoginWindowTask);
		DISALLOW_COPY_AND_ASSIGN(OpenLoginWindowTask);
};

struct CharacterLookupHandler: public Browser::URLHandlerNoCORS {
	CharacterLookupHandler(CefRefPtr<CefRequest> request, CefRefPtr<CefRequestContext> context, CefRefPtr<Browser::Client> client, CefString session_id, bool discard): Browser::URLHandlerNoCORS(request, context), client(client), session_id(session_id), discard(discard) {}
	void OnRequestComplete(CefRefPtr<CefURLRequest> request) override {
		if (this->discard && this->url_request->GetResponse()->GetStatus() == 401) {
			const std::string sid = this->session_id.ToString();
			this->client->DropSession(sid);
		}
		Browser::URLHandlerNoCORS::OnRequestComplete(request);
	}
	private:
		CefRefPtr<Browser::Client> client;
		CefString session_id;
		bool discard;
};

struct FilePicker: public CefRunFileDialogCallback, Browser::ResourceHandler {
	FilePicker(CefRefPtr<CefBrowser> browser, std::vector<CefString> accept_filters):
		accept_filters(accept_filters), callback(nullptr), browser_host(browser->GetHost()), ResourceHandler("text/plain") { }

	bool Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback) override {
		this->callback = callback;
		CefString title = "Select a JAR file...";
		CefString default_file_path = "";
		this->browser_host->RunFileDialog(FILE_DIALOG_OPEN, title, "", this->accept_filters, this);
		this->browser_host = nullptr;
		handle_request = false;
		return true;
	}

	void Cancel() override {
		this->browser_host = nullptr;
		this->callback = nullptr;
		Browser::ResourceHandler::Cancel();
	}

	void OnFileDialogDismissed(const std::vector<CefString>& file_paths) override {
		if (file_paths.size() == 0) {
			this->status = 204;
			this->data_len = 0;
			this->data = nullptr;
		} else {
			this->status = 200;
			this->data_ = file_paths[0].ToString();
			this->data_len = this->data_.size();
			this->data = reinterpret_cast<const unsigned char*>(this->data_.data());
		}

		if (this->callback) {
			this->callback->Continue();
			this->callback = nullptr;
		}
	}

	private:
		std::vector<CefString> accept_filters;
		CefRefPtr<CefCallback> callback;
		std::string data_;
		CefRefPtr<CefBrowserHost> browser_host;
		
	IMPLEMENT_REFCOUNTING(FilePicker);
	DISALLOW_COPY_AND_ASSIGN(FilePicker);
};

Browser::Launcher::Launcher(
	CefRefPtr<Browser::Client> client,
	Details details,
	bool show_devtools,
	CefRefPtr<FileManager::FileManager> file_manager,
	std::filesystem::path config_dir,
	std::filesystem::path data_dir,
	CefRefPtr<CefListValue> session_list
): Window(client, details, show_devtools), last_x(details.startx), last_y(details.starty), data_dir(data_dir), file_manager(file_manager) {
	this->config_path = config_dir;
	this->config_path.append("launcher.json");

	this->launcher_bin_path = config_dir;
	this->launcher_bin_path.append(LAUNCHER_BIN_FILENAME);

	this->rs3_elf_path = data_dir;
	this->rs3_elf_path.append("rs3linux");

	this->rs3_elf_hash_path = data_dir;
	this->rs3_elf_hash_path.append("rs3linux.sha256");

	this->rs3_exe_path = data_dir;
	this->rs3_exe_path.append("rs3windows.exe");

	this->rs3_exe_hash_path = data_dir;
	this->rs3_exe_hash_path.append("rs3windows.sha256");

	this->rs3_app_path = data_dir;
	this->rs3_app_path.append("rs3mac");

	this->rs3_app_hash_path = data_dir;
	this->rs3_app_hash_path.append("rs3mac.sha256");

	this->osrs_exe_path = data_dir;
	this->osrs_exe_path.append("osrswindows.exe");

	this->osrs_exe_hash_path = data_dir;
	this->osrs_exe_hash_path.append("osrswindows.sha256");

	this->osrs_app_path = data_dir;
	this->osrs_app_path.append("osrsmac");

	this->osrs_app_hash_path = data_dir;
	this->osrs_app_hash_path.append("osrsmac.sha256");

	this->runelite_path = data_dir;
	this->runelite_path.append("runelite.jar");

	this->runelite_id_path = data_dir;
	this->runelite_id_path.append("runelite_id.bin");

	this->hdos_path = data_dir;
	this->hdos_path.append("hdos.jar");

	this->hdos_version_path = data_dir;
	this->hdos_version_path.append("hdos_version.bin");

#if defined(BOLT_PLUGINS)
	this->plugin_config_path = config_dir;
	this->plugin_config_path.append("plugins.json");

	this->plugins_data_dir = data_dir;
	this->plugins_data_dir.append("plugins");

	this->plugins_config_dir = config_dir;
	this->plugins_config_dir.append("plugins");
#endif

	CefRefPtr<CefDictionaryValue> dict = CefDictionaryValue::Create();
	dict->SetBool("launcher", true);
	if (session_list) {
		dict->SetList(L"sessions", session_list);
	}
#if defined(BOLT_PLUGINS)
	std::ifstream plugin_file(this->plugin_config_path.c_str(), std::ios::in | std::ios::binary);
	CefRefPtr<CefDictionaryValue> plugins_dict;
	if (!plugin_file.fail()) {
		std::stringstream ss;
		ss << plugin_file.rdbuf();
		// CefParseJSON returns null if the file doesn't contain valid JSON
		CefRefPtr<CefValue> json = CefParseJSON(ss.str(), JSON_PARSER_RFC);
		if (json) {
			// GetDictionary returns null if the top-level object in the file is not a dictionary
			plugins_dict = json->GetDictionary();
		}
		plugin_file.close();
	}
	if (!plugins_dict) {
		// since the svelte frontend doesn't have conditional compilation, it instead needs to pick up at runtime
		// whether plugin support is enabled in this build or not. if this variable is undefined, that will be
		// interpreted as having no plugin support, whereas any dict/object will be interpreted as having plugin
		// support. therefore, even if we have nothing to send, we must still send an empty dict.
		plugins_dict = CefDictionaryValue::Create();
	}
	dict->SetDictionary(L"plugins", plugins_dict);
#endif
	this->Init(this->BuildURL(), dict);
}

CefRefPtr<CefRequestHandler> Browser::Launcher::GetRequestHandler() {
	return this;
}

#define ROUTE(API, FUNC) if (path == "/" API) { return this->FUNC(request, browser, query); }
#define ROUTEUNSUPPORTED(API) if (path == "/" API) { QSENDNOTSUPPORTED(); }

#if defined(BOLT_PLUGINS)
#define ROUTEIFPLUGINS(API, FUNC) ROUTE(API, FUNC)
#if defined(HAS_LIBARCHIVE)
#define ROUTEIFLIBARCHIVE(API, FUNC) ROUTEIFPLUGINS(API, FUNC)
#else
#define ROUTEIFLIBARCHIVE(API, FUNC) ROUTEUNSUPPORTED(API)
#endif
#else
#define ROUTEIFPLUGINS(API, FUNC) ROUTEUNSUPPORTED(API)
#define ROUTEIFLIBARCHIVE(API, FUNC) ROUTEUNSUPPORTED(API)
#endif

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::GetResourceRequestHandler(
	CefRefPtr<CefBrowser> browser,
	CefRefPtr<CefFrame> frame,
	CefRefPtr<CefRequest> request,
	bool is_navigation,
	bool is_download,
	const CefString& request_initiator,
	bool& disable_default_handling
) {
	// Parse URL
	const std::string request_url = request->GetURL().ToString();
	const std::string::size_type colon = request_url.find_first_of(':');
	if (colon == std::string::npos) {
		return nullptr;
	}
	const std::string_view schema(request_url.begin(), request_url.begin() + colon);

	// if the request URL doesn't start with `http(s)://`, don't try to process it any further
	if ((schema != "http" && schema != "https") || request_url.size() < colon + 4
		|| request_url.at(colon + 1) != '/' || request_url.at(colon + 2) != '/')
	{
		return nullptr;
	}

	// parse all the important parts of the URL as string_views
	const std::string::size_type next_hash = request_url.find_first_of('#', colon + 3);
	const auto url_end = next_hash == std::string::npos ? request_url.end() : request_url.begin() + next_hash;
	const std::string::size_type next_sep = request_url.find_first_of('/', colon + 3);
	const std::string::size_type next_question = request_url.find_first_of('?', colon + 3);
	const auto domain_end = next_sep == std::string::npos && next_question == std::string::npos ? url_end : request_url.begin() + (next_sep < next_question ? next_sep : next_question);
	const std::string_view domain(request_url.begin() + colon + 3, domain_end);
	const std::string_view path(domain_end, next_question == std::string::npos ? url_end : request_url.begin() + next_question);
	const std::string_view query(request_url.begin() + next_question + 1, next_question == std::string::npos ? request_url.begin() + next_question + 1 : url_end);
	const std::string_view comment(next_hash == std::string::npos ? request_url.end() : request_url.begin() + next_hash + 1, request_url.end());

	const bool is_internal_target = domain == BOLTDOMAIN;
	const bool is_internal_initiator = request_initiator == BOLTURI;

	if (is_internal_target) {
		disable_default_handling = true;
	}

	// internal API endpoints - only allowed if it's a request from internal URL to internal URL
	if (is_internal_target && is_internal_initiator) {
		ROUTE("login", OpenLoginWindow)
		ROUTE("logout", Logout)
		ROUTE("characters", LookupCharacters)
		ROUTE("launch-rs3-deb", LaunchRs3Deb)
		ROUTE("launch-rs3-exe", LaunchRs3Exe)
		ROUTE("launch-rs3-app", LaunchRs3App)
		ROUTE("launch-osrs-exe", LaunchOsrsExe)
		ROUTE("launch-osrs-app", LaunchOsrsApp)
		ROUTE("launch-runelite-jar", LaunchRuneliteJarNormal)
		ROUTE("launch-runelite-jar-configure", LaunchRuneliteJarConfigure)
		ROUTE("launch-hdos-jar", LaunchHdosJar)
		ROUTE("save-config", SaveConfig)
		ROUTE("open-external-url", OpenExternalUrl)
		ROUTE("browse-directory", BrowseDirectory)
		ROUTE("browse-data", BrowseData)
		ROUTE("jar-file-picker", JarFilePicker)
		ROUTE("json-file-picker", JsonFilePicker)
		ROUTE("close", Close)
		ROUTE("minimize", Minimize)
		ROUTE("toggle-maximize", ToggleMaximize)
		ROUTE("running-clients", RunningClients)
		ROUTE("dashboard-installed", DashboardInstalled)
		ROUTE("start-dashboard", StartDashboard)
		ROUTE("apply-update", ApplyUpdate)
		ROUTE("update-status", UpdateStatus)
		ROUTE("cancel-update", CancelUpdate)
		ROUTEIFPLUGINS("save-plugin-config", SavePluginConfig)
		ROUTEIFPLUGINS("list-game-clients", ListGameClients)
		ROUTEIFPLUGINS("read-json-file", ReadJsonFile)
		ROUTEIFPLUGINS("start-plugin", StartPlugin)
		ROUTEIFPLUGINS("stop-plugin", StopPlugin)
		ROUTEIFPLUGINS("install-plugin", InstallPlugin)
		ROUTEIFPLUGINS("uninstall-plugin", UninstallPlugin)
		ROUTEIFPLUGINS("get-plugindir-json", GetPluginDirJson)
		ROUTEIFPLUGINS("browse-plugin-data", BrowsePluginData)
		ROUTEIFPLUGINS("browse-plugin-config", BrowsePluginConfig)
	}

	// internal hashmap of filenames - allowed to fetch these either if the request is from an internal origin,
	// or is_navigation is true, since navigations have no origin
	if (is_internal_target && (is_internal_initiator || is_navigation)) {
		FileManager::File file = this->file_manager->get(path);
		if (file.contents) {
			return new ResourceHandler(file, this->file_manager);
		} else {
			this->file_manager->free(file);
			QSENDNOTFOUND();
		}
	}

	if (!is_internal_initiator) {
		return nullptr;
	}

	// default way to handle web requests from the main window is to make a CefURLRequest that's not subject to CORS safety
	disable_default_handling = true;
	CefRefPtr<CefRequest> new_request = CefRequest::Create();
	CefRequest::HeaderMap headers;
	request->GetHeaderMap(headers); // doesn't include Referer header
	new_request->Set(request->GetURL(), request->GetMethod(), request->GetPostData(), headers);
	return new URLHandlerNoCORS(new_request, this->browser->GetHost()->GetRequestContext());
}

void Browser::Launcher::OnBrowserDestroyed(CefRefPtr<CefBrowserView> view, CefRefPtr<CefBrowser> browser) {
	Window::OnBrowserDestroyed(view, browser);
	this->file_manager = nullptr;

#define INT16CFG(N) static_cast<uint8_t>(static_cast<uint16_t>(N) & 0xFF), static_cast<uint8_t>((static_cast<uint16_t>(N) >> 8) & 0xFF)
	const uint8_t config_bytes[] = {INT16CFG(1), INT16CFG(this->last_x), INT16CFG(this->last_y)};
#undef INT16CFG
	std::ofstream f(this->launcher_bin_path, std::ios::binary | std::ios::out);
	if (!f.fail()) {
		f.write(reinterpret_cast<const char*>(config_bytes), sizeof(config_bytes));
		f.close();
	}
}

CefString Browser::Launcher::BuildURL() const {
	std::stringstream url;
	url << DEFAULTURL "?platform=" URLPLATFORM "&bolt_version=" BOLT_VERSION
#if defined(BOLT_FLATHUB_BUILD)
	"&flathub=1"
#endif
#if defined(HAS_LIBARCHIVE)
	"&has_libarchive=1"
#endif
	;

	std::ifstream rs_elf_hashfile(this->rs3_elf_hash_path.c_str(), std::ios::in | std::ios::binary);
	if (!rs_elf_hashfile.fail()) {
		url << "&rs3_deb_installed_hash=" << rs_elf_hashfile.rdbuf();
	}

	std::ifstream rs_exe_hashfile(this->rs3_exe_hash_path.c_str(), std::ios::in | std::ios::binary);
	if (!rs_exe_hashfile.fail()) {
		url << "&rs3_exe_installed_hash=" << rs_exe_hashfile.rdbuf();
	}

	std::ifstream rs_app_hashfile(this->rs3_app_hash_path.c_str(), std::ios::in | std::ios::binary);
	if (!rs_app_hashfile.fail()) {
		url << "&rs3_app_installed_hash=" << rs_app_hashfile.rdbuf();
	}

	std::ifstream osrs_exe_hashfile(this->osrs_exe_hash_path.c_str(), std::ios::in | std::ios::binary);
	if (!osrs_exe_hashfile.fail()) {
		url << "&osrs_exe_installed_hash=" << osrs_exe_hashfile.rdbuf();
	}

	std::ifstream osrs_app_hashfile(this->osrs_app_hash_path.c_str(), std::ios::in | std::ios::binary);
	if (!osrs_app_hashfile.fail()) {
		url << "&osrs_app_installed_hash=" << osrs_app_hashfile.rdbuf();
	}

	std::ifstream rl_hashfile(this->runelite_id_path.c_str(), std::ios::in | std::ios::binary);
	if (!rl_hashfile.fail()) {
		url << "&runelite_installed_id=" << rl_hashfile.rdbuf();
	}

	std::ifstream hdos_hashfile(this->hdos_version_path.c_str(), std::ios::in | std::ios::binary);
	if (!hdos_hashfile.fail()) {
		url << "&hdos_installed_version=" << hdos_hashfile.rdbuf();
	}

	std::ifstream config_file(this->config_path.c_str(), std::ios::in | std::ios::binary);
	std::string config_str;
	if (!config_file.fail()) {
		std::stringstream ss;
		ss << config_file.rdbuf();
		config_str = CefURIEncode(CefString(ss.str()), true).ToString();
		url << "&config=" << config_str;
		config_file.close();
	}

	return url.str();
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::OpenLoginWindow(CefRefPtr<CefRequest>, CefRefPtr<CefBrowser>, std::string_view) {
	CefPostTask(TID_UI, new OpenLoginWindowTask(this->client));
	QSENDOK();
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::Logout(CefRefPtr<CefRequest>, CefRefPtr<CefBrowser>, std::string_view query) {
	std::string_view session_id;
	bool has_session_id;
	ParseQuery(query, [&](const std::string_view& key, const std::string_view& val) {
		PQSTRINGVIEW(session_id)
	});
	QREQPARAM(session_id);
	this->client->DropSession(session_id);
	// TODO: is there any way to actually invalidate a session ID? Currently our way of "logging out"
	// is just forgetting about the session and pretending it doesn't exist anymore, but it would
	// still be usable if someone had a copy of it...
	QSENDOK();
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::LookupCharacters(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser>, std::string_view query) {
	CefString session_id;
	std::string_view discard;
	bool has_session_id = false;
	bool has_discard = false;
	ParseQuery(query, [&](const std::string_view& key, const std::string_view& val) {
		PQCEFSTRING(session_id)
		PQSTRINGVIEW(discard)
	});
	QREQPARAM(session_id);
	const bool discard_expired = has_discard && !discard.empty() && discard[0] != '0';

	std::stringstream ss;
	ss << "Bearer " << session_id.ToString();
	const std::string auth_header = ss.str();

	CefRefPtr<CefRequest> new_request = CefRequest::Create();
	CefRequest::HeaderMap headers;
	request->GetHeaderMap(headers); // doesn't include Referer header
	headers.insert(std::make_pair(CefString(L"Authorization"), CefString(auth_header)));
	new_request->Set(BOLT_AUTH_ORIGIN ENDPOINT_ACCOUNTS, L"GET", nullptr, headers);
	return new CharacterLookupHandler(new_request, this->browser->GetHost()->GetRequestContext(), this->client, session_id, discard_expired);
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::LaunchRs3Deb(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser>, std::string_view query) {
	return this->LaunchRs3Deb(request, query);
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::LaunchRs3Exe(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser>, std::string_view query) {
	return this->LaunchRs3Exe(request, query);
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::LaunchRs3App(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser>, std::string_view query) {
	return this->LaunchRs3App(request, query);
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::LaunchOsrsExe(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser>, std::string_view query) {
	return this->LaunchOsrsExe(request, query);
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::LaunchOsrsApp(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser>, std::string_view query) {
	return this->LaunchOsrsApp(request, query);
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::LaunchHdosJar(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser>, std::string_view query) {
	return this->LaunchHdosJar(request, query);
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::LaunchRuneliteJarNormal(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	return this->LaunchRuneliteJar(request, query, false);
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::LaunchRuneliteJarConfigure(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	return this->LaunchRuneliteJar(request, query, true);
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::SaveConfig(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view _) {
	return SaveFileFromPost(request, this->config_path.c_str());
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::OpenExternalUrl(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view _) {
	CefRefPtr<CefPostData> post_data = request->GetPostData();
	QSENDBADREQUESTIF(!post_data || post_data->GetElementCount() != 1);
	CefPostData::ElementVector elements;
	post_data->GetElements(elements);
	size_t byte_count = elements[0]->GetBytesCount();
	char* url = new char[byte_count + 1];
	elements[0]->GetBytes(byte_count, url);
	url[byte_count] = '\0';
	this->OpenExternalUrl(url);
	delete[] url;
	QSENDOK();
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::BrowseDirectory(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	QSTRING path;
	bool has_path = false;
	ParseQuery(query, [&](const std::string_view& key, const std::string_view& val) {
		PQSTRING(path)
	});
	QREQPARAM(path);
	const std::filesystem::path p(path);
	QSENDSYSTEMERRORIF(BrowseFile(p));
	QSENDOK();
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::BrowseData(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	QSENDSYSTEMERRORIF(BrowseFile(this->data_dir));
	QSENDOK();
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::JarFilePicker(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	return new FilePicker(browser, {".jar"});
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::JsonFilePicker(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	return new FilePicker(browser, {".json"});
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::Close(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	browser->GetHost()->CloseBrowser(false);
	QSENDOK();
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::Minimize(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	CefPostTask(TID_UI, new WindowControlTask(this->window, WindowControlTask::Action::Minimize));
	QSENDOK();
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::ToggleMaximize(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	CefPostTask(TID_UI, new WindowControlTask(this->window, WindowControlTask::Action::ToggleMaximize));
	QSENDOK();
}

#if defined(_WIN32)
// collects the titles of visible top-level windows that look like a game client, e.g. "RuneLite - Name"
static BOOL CALLBACK CollectClientTitles(HWND hwnd, LPARAM param) {
	if (!IsWindowVisible(hwnd)) return TRUE;
	wchar_t buf[256];
	const int len = GetWindowTextW(hwnd, buf, 256);
	if (len <= 0) return TRUE;
	const std::wstring title(buf, static_cast<size_t>(len));
	if (title.find(L"RuneLite") == std::wstring::npos
		&& title.find(L"RuneScape") == std::wstring::npos
		&& title.find(L"HDOS") == std::wstring::npos) {
		return TRUE;
	}
	reinterpret_cast<std::vector<std::string>*>(param)->push_back(CefString(title).ToString());
	return TRUE;
}
#endif

// Lets the UI see which game clients are open right now, so it can show which characters are running
// even after the launcher itself was closed and reopened. Only titles of game windows are returned.
CefRefPtr<CefResourceRequestHandler> Browser::Launcher::RunningClients(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	std::string json = "[";
#if defined(_WIN32)
	std::vector<std::string> titles;
	EnumWindows(CollectClientTitles, reinterpret_cast<LPARAM>(&titles));
	for (size_t i = 0; i < titles.size(); i += 1) {
		if (i > 0) json += ",";
		json += "\"";
		for (const char c : titles[i]) {
			if (c == '"' || c == '\\') json += '\\';
			if (static_cast<unsigned char>(c) >= 0x20) json += c;
		}
		json += "\"";
	}
#endif
	json += "]";
	return new Browser::ResourceHandler(json, 200, "application/json");
}

#if defined(_WIN32)
// the local dashboard app that ships next to this program: <folder of this exe>\web\tideline-web.exe
static std::filesystem::path DashboardExePath() {
	wchar_t buf[MAX_PATH];
	const DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
	if (len == 0 || len >= MAX_PATH) return std::filesystem::path();
	return std::filesystem::path(buf).parent_path() / L"web" / L"tideline-web.exe";
}
#endif

// Says whether the bundled dashboard app is installed, so the UI can offer it without asking the user to set anything up.
CefRefPtr<CefResourceRequestHandler> Browser::Launcher::DashboardInstalled(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
#if defined(_WIN32)
	std::error_code ec;
	const std::filesystem::path exe = DashboardExePath();
	if (!exe.empty() && std::filesystem::exists(exe, ec)) {
		QSENDOK();
	}
#endif
	QSENDNOTFOUND();
}

// Starts the bundled dashboard app. It takes no input from the page: the program path is fixed, so this cannot be
// used to run anything else. The app itself does nothing if a copy is already running.
CefRefPtr<CefResourceRequestHandler> Browser::Launcher::StartDashboard(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
#if defined(_WIN32)
	std::error_code ec;
	const std::filesystem::path exe = DashboardExePath();
	if (exe.empty() || !std::filesystem::exists(exe, ec)) {
		QSENDNOTFOUND();
	}
	std::wstring cmd = L"\"" + exe.wstring() + L"\"";
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	const std::wstring dir = exe.parent_path().wstring();
	if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW | DETACHED_PROCESS, nullptr, dir.c_str(), &si, &pi)) {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		QSENDOK();
	}
	QSENDSYSTEMERRORIF(true);
#else
	QSENDNOTSUPPORTED();
#endif
}

#if defined(_WIN32)
// ---- Updates -------------------------------------------------------------------------------------------------------
// The page finds out that a newer version exists (it reads latest.json) and passes the details here. Nothing is
// trusted until it is checked: the installer's SHA-256 must match, and the signature over
// "tideline-update|version|sha256|url" must verify against the public key compiled into this program. Only the
// holder of the private key (the developer) can produce such a signature, so a hijacked download or manifest
// cannot make this program run anything else.
static std::mutex update_mutex;
static std::string update_state = "idle";  // idle | downloading | verifying | installing | error
static std::string update_message;
static void SetUpdateState(const std::string& state, const std::string& message = "") {
	std::lock_guard<std::mutex> lock(update_mutex);
	update_state = state;
	update_message = message;
}

static std::atomic<unsigned long long> update_done{0};
static std::atomic<unsigned long long> update_total{0};
static std::atomic<bool> update_cancel{false};

static bool HexToBytes(const std::string& hex, std::vector<unsigned char>& out) {
	if (hex.size() % 2 != 0) return false;
	out.clear();
	for (size_t i = 0; i < hex.size(); i += 2) {
		unsigned int v = 0;
		for (int j = 0; j < 2; j += 1) {
			const char c = hex[i + j];
			v <<= 4;
			if (c >= '0' && c <= '9') v |= c - '0';
			else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
			else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
			else return false;
		}
		out.push_back(static_cast<unsigned char>(v));
	}
	return true;
}

static bool Sha256OfFile(const std::filesystem::path& path, std::string& hex_out) {
	std::ifstream f(path, std::ios::binary);
	if (!f) return false;
	SHA256_CTX ctx;
	sha256_init(&ctx);
	std::vector<char> buf(1 << 16);
	while (f) {
		f.read(buf.data(), buf.size());
		const std::streamsize n = f.gcount();
		if (n > 0) sha256_update(&ctx, reinterpret_cast<const unsigned char*>(buf.data()), static_cast<size_t>(n));
	}
	unsigned char hash[SHA256_BLOCK_SIZE];
	sha256_final(&ctx, hash);
	static const char digits[] = "0123456789abcdef";
	hex_out.clear();
	for (size_t i = 0; i < SHA256_BLOCK_SIZE; i += 1) {
		hex_out += digits[hash[i] >> 4];
		hex_out += digits[hash[i] & 15];
	}
	return true;
}

// ECDSA P-256 over SHA-256 of `message`, signature as 64 bytes r||s.
static bool VerifyUpdateSignature(const std::string& message, const std::vector<unsigned char>& sig) {
	if (sig.size() != 64) return false;
	BCRYPT_ALG_HANDLE alg = nullptr;
	BCRYPT_KEY_HANDLE key = nullptr;
	bool ok = false;
	if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDSA_P256_ALGORITHM, nullptr, 0) < 0) return false;
	std::vector<unsigned char> blob(sizeof(BCRYPT_ECCKEY_BLOB) + 64);
	BCRYPT_ECCKEY_BLOB* header = reinterpret_cast<BCRYPT_ECCKEY_BLOB*>(blob.data());
	header->dwMagic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
	header->cbKey = 32;
	memcpy(blob.data() + sizeof(BCRYPT_ECCKEY_BLOB), UPDATE_PUBLIC_KEY, 64);
	if (BCryptImportKeyPair(alg, nullptr, BCRYPT_ECCPUBLIC_BLOB, &key, blob.data(), static_cast<ULONG>(blob.size()), 0) >= 0) {
		SHA256_CTX ctx;
		sha256_init(&ctx);
		sha256_update(&ctx, reinterpret_cast<const unsigned char*>(message.data()), message.size());
		unsigned char hash[SHA256_BLOCK_SIZE];
		sha256_final(&ctx, hash);
		ok = BCryptVerifySignature(key, nullptr, hash, sizeof(hash), const_cast<unsigned char*>(sig.data()), static_cast<ULONG>(sig.size()), 0) >= 0;
		BCryptDestroyKey(key);
	}
	BCryptCloseAlgorithmProvider(alg, 0);
	return ok;
}

// Downloads `url` to `file`, updating the progress counters and stopping early when Cancel is pressed. Goes straight to the
// network and never reads or writes Windows' download cache, so a cancelled download really starts over next time.
// Returns 0 = done, 1 = cancelled, 2 = failed (error code in `err`).
static int DownloadFile(const std::wstring& url, const std::filesystem::path& file, DWORD& err) {
	err = 0;
	HINTERNET net = InternetOpenW(L"Tideline", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	if (!net) { err = GetLastError(); return 2; }
	HINTERNET req = InternetOpenUrlW(net, url.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_SECURE, 0);
	if (!req) { err = GetLastError(); InternetCloseHandle(net); return 2; }
	DWORD status = 0, status_len = sizeof(status);
	if (!HttpQueryInfoW(req, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &status_len, nullptr) || status != 200) {
		err = status ? status : GetLastError();
		InternetCloseHandle(req);
		InternetCloseHandle(net);
		return 2;
	}
	DWORD len = 0, len_size = sizeof(len);
	if (HttpQueryInfoW(req, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &len, &len_size, nullptr)) update_total = len;
	std::ofstream out(file, std::ios::binary | std::ios::trunc);
	int result = 0;
	if (!out) {
		err = ERROR_ACCESS_DENIED;
		result = 2;
	}
	std::vector<char> buf(1 << 16);
	unsigned long long total_read = 0;
	while (result == 0) {
		if (update_cancel.load()) { result = 1; break; }
		DWORD got = 0;
		if (!InternetReadFile(req, buf.data(), static_cast<DWORD>(buf.size()), &got)) { err = GetLastError(); result = 2; break; }
		if (got == 0) break;
		out.write(buf.data(), got);
		if (!out) { err = ERROR_DISK_FULL; result = 2; break; }
		total_read += got;
		update_done = total_read;
	}
	out.close();
	InternetCloseHandle(req);
	InternetCloseHandle(net);
	return result;
}

static void RunUpdate(std::string version, std::string url, std::string sha, std::string sig_hex, bool add_dashboard) {
	std::vector<unsigned char> sig;
	if (sha.size() != 64 || !HexToBytes(sig_hex, sig) || url.empty() || version.empty()) {
		SetUpdateState("error", "The update details are invalid.");
		return;
	}
	for (const char c : version) {
		if (!((c >= '0' && c <= '9') || c == '.' || c == '-' || (c >= 'a' && c <= 'z'))) {
			SetUpdateState("error", "The update version is invalid.");
			return;
		}
	}
	// check the signature before touching the network: a forged manifest is rejected without downloading anything
	if (!VerifyUpdateSignature("tideline-update|" + version + "|" + sha + "|" + url, sig)) {
		SetUpdateState("error", "This update is not signed by Tideline, so it was not installed.");
		return;
	}
	wchar_t tmp[MAX_PATH];
	const DWORD tl = GetTempPathW(MAX_PATH, tmp);
	if (tl == 0 || tl >= MAX_PATH) {
		SetUpdateState("error", "Could not find a temporary folder.");
		return;
	}
	const std::filesystem::path file = std::filesystem::path(tmp) / ("Tideline-Update-" + version + ".exe");
	std::error_code ec;
	std::filesystem::remove(file, ec);
	SetUpdateState("downloading", "Downloading the update...");
	const int wlen = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, nullptr, 0);
	std::wstring wurl(wlen > 0 ? wlen - 1 : 0, L'\0');
	if (wlen > 1) MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, wurl.data(), wlen);
	DWORD dl_err = 0;
	const int dl = DownloadFile(wurl, file, dl_err);
	if (dl == 1) {
		std::filesystem::remove(file, ec);
		SetUpdateState("cancelled", "Cancelled.");
		return;
	}
	if (dl != 0) {
		std::filesystem::remove(file, ec);
		SetUpdateState("error", "The download failed (code " + std::to_string(dl_err) + "). Check your connection and try again.");
		return;
	}
	SetUpdateState("verifying", "Checking the download...");
	std::string actual;
	if (!Sha256OfFile(file, actual) || actual != sha) {
		std::filesystem::remove(file, ec);
		SetUpdateState("error", "The download did not match its checksum, so it was discarded.");
		return;
	}
	// a Cancel that arrived while the file was being checked still counts: nothing has been installed yet
	if (update_cancel.load()) {
		std::filesystem::remove(file, ec);
		SetUpdateState("cancelled", "Cancelled.");
		return;
	}
	SetUpdateState("installing", "Installing...");
	// /SILENT shows a small progress window; the installer closes this program, replaces the files and starts it again
	std::wstring cmd = L"\"" + file.wstring() + L"\" /SILENT /SUPPRESSMSGBOXES /NORESTART /CLOSEAPPLICATIONS /RELAUNCH=1";
		// adding the dashboard to a launcher-only install: the same signed installer, with the dashboard box ticked
		if (add_dashboard) cmd += L" /TYPE=custom /COMPONENTS=main,dashboard";
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, DETACHED_PROCESS, nullptr, tmp, &si, &pi)) {
		SetUpdateState("error", "Could not start the installer.");
		return;
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
}
#endif

// Starts installing an update the page found. Query: version, url, sha256, sig (all URL-encoded). Everything is
// verified before anything runs; progress is read from update-status.
CefRefPtr<CefResourceRequestHandler> Browser::Launcher::ApplyUpdate(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
#if defined(_WIN32)
	std::string version, url, sha, sig;
	bool add_dashboard = false;
	ParseQuery(query, [&](const std::string_view& key, const std::string_view& val) {
		if (key == "dashboard") { add_dashboard = (val == "1"); return; }
		const std::string decoded = CefURIDecode(std::string(val), true, static_cast<cef_uri_unescape_rule_t>(UU_SPACES | UU_PATH_SEPARATORS | UU_URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS | UU_REPLACE_PLUS_WITH_SPACE)).ToString();
		if (key == "version") version = decoded;
		else if (key == "url") url = decoded;
		else if (key == "sha256") sha = decoded;
		else if (key == "sig") sig = decoded;
	});
	{
		std::lock_guard<std::mutex> lock(update_mutex);
		if (update_state == "downloading" || update_state == "verifying" || update_state == "installing") {
			QSENDOK();
		}
		update_state = "downloading";
		update_message = "Starting...";
		update_done = 0;
		update_total = 0;
		update_cancel = false;
	}
	std::thread(RunUpdate, version, url, sha, sig, add_dashboard).detach();
	QSENDOK();
#else
	QSENDNOTSUPPORTED();
#endif
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::UpdateStatus(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	std::string json = "{\"state\":\"idle\",\"message\":\"\"}";
#if defined(_WIN32)
	std::lock_guard<std::mutex> lock(update_mutex);
	json = "{\"done\":" + std::to_string(update_done.load()) + ",\"total\":" + std::to_string(update_total.load()) + ",\"state\":\"" + update_state + "\",\"message\":\"";
	for (const char c : update_message) {
		if (c == '"' || c == '\\') json += '\\';
		if (static_cast<unsigned char>(c) >= 0x20) json += c;
	}
	json += "\"}";
#endif
	return new Browser::ResourceHandler(json, 200, "application/json");
}

// Stops a download in progress (the Cancel button). Once the installer itself has started it cannot be stopped from here.
CefRefPtr<CefResourceRequestHandler> Browser::Launcher::CancelUpdate(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
#if defined(_WIN32)
	update_cancel = true;
	QSENDOK();
#else
	QSENDNOTSUPPORTED();
#endif
}

CefRefPtr<CefDragHandler> Browser::Launcher::GetDragHandler() {
	return this;
}

void Browser::Launcher::OnDraggableRegionsChanged(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, const std::vector<CefDraggableRegion>& regions) {
	if (this->window) {
		this->window->SetDraggableRegions(regions);
	}
}


#if defined(BOLT_PLUGINS)
CefRefPtr<CefResourceRequestHandler> Browser::Launcher::SavePluginConfig(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view _) {
	return SaveFileFromPost(request, this->plugin_config_path.c_str());
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::ListGameClients(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view _) {
	this->UpdateClientList(true);
	QSENDOK();
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::ReadJsonFile(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	QSTRING path;
	bool has_path = false;
	ParseQuery(query, [&](const std::string_view& key, const std::string_view& val) {
		PQSTRING(path)
	});
	QREQPARAM(path);
	std::ifstream file(path, std::ios::in | std::ios::binary);
	if (!file.fail()) {
		std::stringstream ss;
		ss << file.rdbuf();
		std::string str = ss.str();
		file.close();
		return new Browser::ResourceHandler(str, 200, "application/json");
	} else {
		QSENDNOTFOUND();
	}
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::StartPlugin(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	CefString id;
	bool has_id = false;
	CefString path;
	bool has_path  = false;
	CefString main;
	bool has_main  = false;
	uint64_t client;
	bool has_client  = false;
	bool client_valid = false;
	ParseQuery(query, [&](const std::string_view& key, const std::string_view& val) {
		PQCEFSTRING(id)
		PQCEFSTRING(path)
		PQCEFSTRING(main)
		PQINT(client)
	});
	QREQPARAM(id);
	QREQPARAM(main);
	QREQPARAMINT(client);
	std::string sid = id;
	std::string spath;
	if (has_path) {
		spath = path;
	} else {
		std::filesystem::path p = this->plugins_data_dir;
		p.append(sid);
		spath = p.string() + '/';
	}
	this->client->StartPlugin(client, sid, spath, std::string(main));
	QSENDOK();
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::StopPlugin(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	uint64_t client;
	bool has_client = false;
	bool client_valid = false;
	uint64_t uid;
	bool has_uid = false;
	bool uid_valid = false;
	ParseQuery(query, [&](const std::string_view& key, const std::string_view& val) {
		PQINT(client)
		PQINT(uid)
	});
	QREQPARAMINT(client);
	QREQPARAMINT(uid);
	this->client->StopPlugin(client, uid);
	QSENDOK();
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::InstallPlugin(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
#if defined(HAS_LIBARCHIVE)
	CefRefPtr<CefPostData> post_data = request->GetPostData();
	QSENDBADREQUESTIF(!post_data || post_data->GetElementCount() != 1);

	QSTRING id;
	bool has_id = false;
	ParseQuery(query, [&](const std::string_view& key, const std::string_view& val) {
		PQSTRING(id)
	});
	QREQPARAM(id);
	QSENDBADREQUESTIF(id.size() == 0);
	std::filesystem::path plugin_dir = this->plugins_data_dir;
	plugin_dir.append(id);
	std::error_code err;
	QSENDSYSTEMERRORIF(std::filesystem::remove_all(plugin_dir, err) == -1);
	QSENDSYSTEMERRORIF(!std::filesystem::create_directories(plugin_dir, err));

	CefPostData::ElementVector elements;
	post_data->GetElements(elements);
	size_t file_length = elements[0]->GetBytesCount();
	char* archive_file = new char[file_length];
	elements[0]->GetBytes(file_length, archive_file);

	struct archive* archive = archive_read_new();
	archive_read_support_format_all(archive);
	archive_read_support_filter_all(archive);
	archive_read_open_memory(archive, archive_file, file_length);

	char buf[65536];
	struct archive_entry* entry;
	while (true) {
		const int r = archive_read_next_header(archive, &entry);
		if (r == ARCHIVE_EOF) break;
		if (r != ARCHIVE_OK) {
			archive_read_close(archive);
			archive_read_free(archive);
			delete[] archive_file;
			QSENDSTR("file is malformed", 400);
		}
		std::filesystem::path p = plugin_dir;
		const char* pathname = archive_entry_pathname(entry);
		if (pathname[0] == '\0' || pathname[strlen(pathname) - 1] == '/') continue;
		p.append(pathname);
		std::filesystem::create_directories(p.parent_path(), err);
		std::ofstream ofs(p, std::ofstream::binary);
		if (ofs.fail()) {
			archive_read_close(archive);
			archive_read_free(archive);
			delete[] archive_file;
			QSENDSTR("failed to create files", 500);
		}

		const la_int64_t entry_size = archive_entry_size(entry);
		la_int64_t written = 0;
		while (written < entry_size) {
			const la_ssize_t w = archive_read_data(archive, buf, sizeof(buf));
			ofs.write(buf, w);
			written += w;
		}
		ofs.close();
	}

	archive_read_close(archive);
	archive_read_free(archive);
	delete[] archive_file;
	QSENDOK();
#else
	QSENDNOTSUPPORTED();
#endif
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::UninstallPlugin(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	QSTRING id;
	bool has_id = false;
	bool delete_data_dir = false;
	ParseQuery(query, [&](const std::string_view& key, const std::string_view& val) {
		PQSTRING(id)
		PQBOOL(delete_data_dir)
	});
	QREQPARAM(id);
	QSENDBADREQUESTIF(id.size() == 0);

	if (delete_data_dir) {
		std::filesystem::path plugin_dir = this->plugins_data_dir;
		plugin_dir.append(id);
		std::filesystem::remove_all(plugin_dir);
	}
	std::filesystem::path config_dir = this->plugins_config_dir;
	config_dir.append(id);
	std::filesystem::remove_all(config_dir);
	QSENDOK();
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::BrowsePluginData(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	QSTRING id;
	bool has_id = false;
	ParseQuery(query, [&](const std::string_view& key, const std::string_view& val) {
		PQSTRING(id)
	});
	QREQPARAM(id);
	QSENDBADREQUESTIF(id.size() == 0);
	std::filesystem::path plugin_dir = this->plugins_data_dir;
	plugin_dir.append(id);
	QSENDSYSTEMERRORIF(BrowseFile(plugin_dir));
	QSENDOK();
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::BrowsePluginConfig(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	QSTRING id;
	bool has_id = false;
	ParseQuery(query, [&](const std::string_view& key, const std::string_view& val) {
		PQSTRING(id)
	});
	QREQPARAM(id);
	QSENDBADREQUESTIF(id.size() == 0);
	std::filesystem::path plugin_dir = this->plugins_config_dir;
	plugin_dir.append(id);
	QSENDSYSTEMERRORIF(BrowseFile(plugin_dir));
	QSENDOK();
}

CefRefPtr<CefResourceRequestHandler> Browser::Launcher::GetPluginDirJson(CefRefPtr<CefRequest> request, CefRefPtr<CefBrowser> browser, std::string_view query) {
	QSTRING id;
	bool has_id = false;
	ParseQuery(query, [&](const std::string_view& key, const std::string_view& val) {
		PQSTRING(id)
	});
	QREQPARAM(id);
	QSENDBADREQUESTIF(id.size() == 0);
	std::filesystem::path plugin_dir = this->plugins_data_dir;
	plugin_dir.append(id);
	plugin_dir.append("bolt.json");
	std::ifstream file(plugin_dir, std::ios::in | std::ios::binary);
	if (!file.fail()) {
		std::stringstream ss;
		ss << file.rdbuf();
		std::string str = ss.str();
		file.close();
		return new Browser::ResourceHandler(str, 200, "application/json");
	} else {
		QSENDNOTFOUND();
	}
}

void Browser::Launcher::UpdateClientList(bool need_lock_mutex) const {
	CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("__bolt_clientlist");
	CefRefPtr<CefListValue> list = message->GetArgumentList();
	this->client->ListGameClients(list, need_lock_mutex);
	this->browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
}
#endif

void Browser::Launcher::UpdateSessionList(CefRefPtr<CefListValue> session_list) const {
	CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("__bolt_sessionlist");
	CefRefPtr<CefListValue> list = message->GetArgumentList();
	list->SetSize(1);
	list->SetList(0, session_list);
	this->browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
}

void Browser::Launcher::Refresh() const {
	// we can't just call ReloadIgnoreCache() (a.k.a. ctrl+f5) here, we need to rebuild the URL in case
	// it has changed - especially due to the credentials file changing, which happens every 30 minutes
	this->browser->GetMainFrame()->LoadURL(this->BuildURL());
}

void Browser::Launcher::NotifyClosed() {
	this->client->OnLauncherClosed();
}

void Browser::Launcher::OnWindowBoundsChanged(CefRefPtr<CefWindow> window, const CefRect& new_bounds) {
	this->last_x = new_bounds.x;
	this->last_y = new_bounds.y;
}

bool Browser::Launcher::OnProcessMessageReceived(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefProcessId, CefRefPtr<CefProcessMessage> message) {
	const CefString name = message->GetName();
	
	if (name == "__bolt_exception") {
		constexpr int max_dist = 80;
		const CefRefPtr<CefListValue> args = message->GetArgumentList();
		
		const std::string source_line = args->GetString(2);
		const size_t first_non_whitespace = source_line.find_first_not_of(" \t");
		const size_t last_non_whitespace = source_line.find_last_not_of(" \t");
		const std::string_view source_view_trimmed(source_line.begin() + first_non_whitespace, source_line.begin() + last_non_whitespace + 1);
		const int exc_start_column = args->GetInt(3) - first_non_whitespace;
		const int exc_end_column = args->GetInt(4) - first_non_whitespace;
		const bool do_trim_start = exc_start_column > max_dist;
		const bool do_trim_end = source_view_trimmed.size() - exc_end_column > max_dist;
		const std::string_view code_trimmed(
			source_view_trimmed.data() + (do_trim_start ? exc_start_column - max_dist : 0),
			source_view_trimmed.data() + (do_trim_end ? exc_end_column + max_dist : source_view_trimmed.size())
		);

		fmt::print("[B] unhandled exception in {}: {}\n", args->GetString(0).ToString(), args->GetString(1).ToString());
		for (int i = 5; i < args->GetSize(); i += 1) {
			const CefRefPtr<CefListValue> frame_details = args->GetList(i);
			fmt::print("[B] in {}:{}:{}\n", frame_details->GetString(0).ToString(), frame_details->GetInt(1), frame_details->GetInt(2));
		}
		fmt::print("[B] {}{}{}\n[B] {}", do_trim_start ? "..." : "", code_trimmed, do_trim_end ? "..." : "", do_trim_start ? "---" : "");
		int i = 0;
		while (i < exc_start_column && i < max_dist) {
			fmt::print("-");
			i += 1;
		}
		i = 0;
		while (i < exc_end_column - exc_start_column) {
			fmt::print("^");
			i += 1;
		}
		fmt::print("\n");
		return true;
	}
	
	return false;
}

void Browser::Launcher::OnAuthError(CefString& error) {
	CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("__bolt_auth_error");
	CefRefPtr<CefListValue> list = message->GetArgumentList();
	list->SetString(0, error);
	this->browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
}

void Browser::Launcher::GetWindowPos(int* x, int* y) const {
	*x = this->last_x;
	*y = this->last_y;
}

CefRefPtr<CefResourceRequestHandler> SaveFileFromPost(CefRefPtr<CefRequest> request, const std::filesystem::path::value_type* path) {
	CefRefPtr<CefPostData> post_data = request->GetPostData();
	QSENDBADREQUESTIF(!post_data || post_data->GetElementCount() != 1);

	CefPostData::ElementVector elements;
	post_data->GetElements(elements);
	size_t byte_count = elements[0]->GetBytesCount();
	
	std::ofstream file(path, std::ios::out | std::ios::binary);
	QSENDSYSTEMERRORIF(file.fail());
	char* buf = new char[byte_count];
	elements[0]->GetBytes(byte_count, buf);
	file.write(buf, byte_count);
	delete[] buf;
	QSENDOK();
}
