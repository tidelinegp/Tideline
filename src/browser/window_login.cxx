#include "window_login.hxx"
#include "client.hxx"
#include "include/cef_request.h"
#include "include/cef_resource_request_handler.h"
#include "include/cef_values.h"
#include "include/internal/cef_types.h"
#include "request.hxx"
#include "resource_handler.hxx"
#include "include/cef_parser.h"

#include "../sha256/sha256.h"

#include <cstdlib>
#include <sstream>

#define WINDOW_WIDTH 480
#define WINDOW_HEIGHT 720

#define ENDPOINT_AUTH "/oauth2/auth"
#define ENDPOINT_TOKEN "/oauth2/token"
#define ENDPOINT_SESSION "/game-session/v1/sessions"

#define REDIRECT_SCHEME "https"
#define REDIRECT_PATH "/m=weblogin/launcher-redirect"
#define REDIRECT_URL REDIRECT_SCHEME "://" BOLT_DOMAIN_SECURE REDIRECT_PATH

#define LOCALHOST_DOMAIN "localhost"
#define LOCALHOST_SCHEME "http"

// technically, the characters that can be selected by the official launcher also includes '~', but
// we don't *have* to include that, and it's the only character that wouldn't be URL-safe if we did.
// so rather than calling CefURIEncode an extra time I'd rather just leave it out.
constexpr char VERIFIER_CHARS[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '.', '_'
};

// makes a random "state", as defined in the oauth2 flow,
// outputting it to an array of length STATE_LENGTH
void MakeRandomState(char out[]) {
    for (size_t i = 0; i < STATE_LENGTH; i += 1) {
        const int r = std::rand();
        const char base_char = (((r + i) & 1) == 0) ? 'A' : 'a';
        out[i] = base_char + (char)((r >> 1) % ('Z' + 1 - 'A'));
    }
}

// makes a random "verifier", as defined in the oauth2 flow,
// outputting it to an array of length VERIFIER_LENGTH
void MakeRandomVerifier(char out[]) {
    for (size_t i = 0; i < VERIFIER_LENGTH; i += 1) {
        const int r = std::rand();
        out[i] = VERIFIER_CHARS[r % (sizeof(VERIFIER_CHARS) / sizeof(VERIFIER_CHARS[0]))];
    }
}

// given an id-token, returns a JSON value representing the contents of the body
// or nullptr if the token is invalid in any way
CefRefPtr<CefValue> ParseTokenBodyJSON(const std::string_view& token) {
    // a valid id_token has three base64-encoded sections delimited by '.',
    // so start by finding two of those
    const std::string::size_type sep = token.find('.');
    if (sep == std::string_view::npos) return nullptr;
    const std::string::size_type data_start = sep + 1;
    const std::string::size_type data_end = token.find('.', data_start);
    if (data_end == std::string_view::npos) return nullptr;
    const std::string_view data_body(token.begin() + data_start, token.begin() + data_end);

    // per base64 standard, CefBase64Decode requires input that's a multiple of 4 bytes long,
    // padded with '=' if necessary
    const size_t data_length = data_end - data_start;
    const size_t padded_length = (data_length + 3) & ~3;    
    std::stringstream ss;
    ss << data_body;
    for (size_t i = data_length; i < padded_length; i += 1) ss << '=';

    // now we have the right section of the token, decode it from base64 to (hopefully) text
    const CefRefPtr<CefBinaryValue> body_decoded = CefBase64Decode(ss.str());
    if (!body_decoded) return nullptr;

    // parse the json and return it
    // this also returns nullptr if it fails to parse the JSON
    return CefParseJSON(CefString(reinterpret_cast<const char*>(body_decoded->GetRawData()), body_decoded->GetSize()), JSON_PARSER_RFC);
}

#define AUTHERROR(ERR) \
this->AuthError(AUTHERRORPREFIX ERR); \
return AUTHRET; \

#define AUTHERRORIF(COND, ERR) if (COND) { AUTHERROR(ERR) }
#define AUTHCHECK(V, ERR) AUTHERRORIF((V) == nullptr, ERR)
#define REQSTRING(D, NAME) AUTHERRORIF((D)->GetType(#NAME) != VTYPE_STRING, "missing required param '" #NAME "'") const CefString NAME = (D)->GetString(#NAME);
#define REQINT(D, NAME) AUTHERRORIF((D)->GetType(#NAME) != VTYPE_INT, "missing required param '" #NAME "'") const int NAME = (D)->GetInt(#NAME);

#define AUTHRET

#include <fmt/core.h>

struct AuthConsentHandler: public Browser::URLHandler {
    AuthConsentHandler(CefRefPtr<Browser::LoginWindow> login_window, const char* state, const char* nonce, CefRefPtr<CefRequest> request, CefRefPtr<CefRequestContext> context): Browser::URLHandler(request, context), login_window(login_window), state(state), nonce(nonce) { }
    
    void AuthError(CefString error) {
        this->login_window->AuthError(error);
    }
    
    #define AUTHERRORPREFIX "AuthConsentHandler::OnRequestComplete: "
    void OnRequestComplete(CefRefPtr<CefURLRequest> request) override {
        // handle the response from auth server, set ResourceHandler overrides so that
        // the webpage will get an appropriate response (ideally a redirect), and update
        // our records with the new oauth information.
        // first, parse the JSON:
        CefRefPtr<CefValue> json = CefParseJSON(CefString(reinterpret_cast<const char*>(this->data.data()), this->data.size()), JSON_PARSER_RFC);
        AUTHCHECK(json, "invalid json")
        CefRefPtr<CefDictionaryValue> dict = json->GetDictionary();
        AUTHCHECK(dict, "invalid json type")
        REQSTRING(dict, id_token)
        REQSTRING(dict, access_token)
        REQSTRING(dict, refresh_token)
        REQINT(dict, expires_in)

        // parse body of the id_token
        const std::string token = id_token.ToString();
        CefRefPtr<CefValue> body_json = ParseTokenBodyJSON(token);
        AUTHCHECK(body_json, "invalid json in token")
        CefRefPtr<CefDictionaryValue> body_dict = body_json->GetDictionary();
        AUTHCHECK(body_dict, "invalid json type in token")
        REQSTRING(body_dict, sub)

        // store all the credentials
        login_window->SetSub(sub);

        // build a URL to redirect the page to for the next part of the login flow
        std::stringstream ss;
        ss << BOLT_ACCOUNT_ORIGIN ENDPOINT_AUTH
            "?prompt=consent"
            "&redirect_uri=" LOCALHOST_SCHEME "%3A%2F%2F" LOCALHOST_DOMAIN
            "&response_type=id_token+code"
            "&client_id=1fddee4e-b100-4f4e-b2b0-097f9088f9d2" // hard-coded PRODUCTION value, there are other values for DEVELOPMENT and STAGING
            "&scope=openid+offline"
            "&id_token_hint=" << token
            << "&state=" << std::string_view(this->state, STATE_LENGTH)
            << "&nonce=" << std::string_view(this->nonce, STATE_LENGTH);
        this->location = ss.str();

        const char c = 0;
        this->return_data_ptr = &c; // can't be null, otherwise it won't override at all
        this->return_data_length = 0;
        this->status_override = 302;
        this->mime_type_override = "text/plain";
        this->location_override = this->location.c_str();
        Browser::URLHandler::OnRequestComplete(request);
    }
    #undef AUTHERRORPREFIX

    private:
        const char* state;
        const char* nonce;
        std::string location;
        CefRefPtr<Browser::LoginWindow> login_window;
};

struct AuthSessionHandler: public Browser::URLHandler {
    AuthSessionHandler(CefRefPtr<Browser::LoginWindow> login_window, CefRefPtr<CefRequest> request, CefRefPtr<CefRequestContext> context): Browser::URLHandler(request, context), login_window(login_window) { }

    void AuthError(CefString error) {
        this->login_window->AuthError(error);
    }

    #define AUTHERRORPREFIX "AuthSessionHandler::OnRequestComplete: "
    void OnRequestComplete(CefRefPtr<CefURLRequest> request) override {
        // parse the session response, a json object containing "sessionId"
        CefRefPtr<CefValue> json = CefParseJSON(CefString(reinterpret_cast<const char*>(this->data.data()), this->data.size()), JSON_PARSER_RFC);
        AUTHCHECK(json, "invalid json")
        CefRefPtr<CefDictionaryValue> dict = json->GetDictionary();
        AUTHCHECK(dict, "invalid json type")
        REQSTRING(dict, sessionId)
        this->login_window->SetSessionID(sessionId);
    }
    #undef AUTHERRORPREFIX

    private:
        CefRefPtr<Browser::LoginWindow> login_window;
};

Browser::LoginWindow::LoginWindow(CefRefPtr<Browser::Client> client, int startx, int starty, bool show_devtools):
    Window(client, Browser::Details {
        .preferred_width = WINDOW_WIDTH,
		.preferred_height = WINDOW_HEIGHT,
        .startx = startx,
        .starty = starty,
		.center_on_open = true,
		.resizeable = true,
		.frame = true,
		.is_devtools = false,
		.has_custom_js = false,
    }, show_devtools), has_tokens(false)
{
    std::srand(time(nullptr));

    // URL-encode the redirect URL
    // it would be nice to do this at compile time instead of using CefURIEncode
    this->redirect_url_encoded = CefURIEncode(REDIRECT_URL, true).ToString();

    // make random states, sent in various requests and present in subsequent
    // responses from the server, useful for us to verify that the server is
    // the same entity that we initiated the request with
    MakeRandomState(this->state1);
    MakeRandomState(this->state2);
    const std::string_view state_encoded(this->state1, STATE_LENGTH);

    // make random nonce, same as state but only sent part-way through the flow
    // and can be checked in a later response.
    // in the official launcher this is a UUID, but it seems like it can be anything
    MakeRandomState(this->nonce);

    // make random verifier, sent in hashed form in the initial request
    // and later we send the original, to verify that we're the same client who
    // initiated the request.
    MakeRandomVerifier(this->verifier);
    // hash it, then base64-encode the resulting 32 bytes, using
    // URL-safe characters '-' and '_', and omit the '=' at the end
    SHA256_CTX ctx;
    unsigned char verifier_hash[SHA256_BLOCK_SIZE];
    sha256_init(&ctx);
    sha256_update(&ctx, reinterpret_cast<const unsigned char*>(this->verifier), VERIFIER_LENGTH);
    sha256_final(&ctx, verifier_hash);
    std::string encoded_verifier = CefBase64Encode(verifier_hash, SHA256_BLOCK_SIZE).ToString();
    std::replace(encoded_verifier.begin(), encoded_verifier.end(), '+', '-');
    std::replace(encoded_verifier.begin(), encoded_verifier.end(), '/', '_');
    const std::string::size_type p = encoded_verifier.find('=');
    const std::string_view code_challenge(encoded_verifier.begin(), p == std::string::npos ? encoded_verifier.end() : (encoded_verifier.begin() + p));

    std::stringstream ss;
    ss << BOLT_ACCOUNT_ORIGIN ENDPOINT_AUTH
        "?auth_method="
        "&login_type="
        "&flow=launcher"
        "&response_type=code"
        "&client_id=" BOLT_CLIENT_ID
        "&code_challenge_method=S256"
        "&prompt=login"
        "&scope=openid+offline+gamesso.token.create+user.profile.read+user.entitlement.read+user.game.read+user.sku.read+user.voucher.redeem"
        "&redirect_uri=" << this->redirect_url_encoded
        << "&code_challenge=" << code_challenge
        << "&state=" << state_encoded;
    const std::string url = ss.str();
    this->Init(url, nullptr);
}

CefRefPtr<CefRequestHandler> Browser::LoginWindow::GetRequestHandler() {
    return this;
}

#define AUTHERRORPREFIX "GetResourceRequestHandler: "
#undef AUTHRET
#define AUTHRET nullptr
CefRefPtr<CefResourceRequestHandler> Browser::LoginWindow::GetResourceRequestHandler(
    CefRefPtr<CefBrowser>,
    CefRefPtr<CefFrame>,
    CefRefPtr<CefRequest> request,
    bool,
    bool,
    const CefString&,
    bool& disable_default_handling
) {
    disable_default_handling = false;
    
	// Parse URL
	const std::string request_url = request->GetURL().ToString();
	const std::string::size_type colon = request_url.find_first_of(':');
	if (colon == std::string::npos) {
		return nullptr;
	}
	const std::string_view schema(request_url.begin(), request_url.begin() + colon);

	if ((schema != "http" && schema != "https") || request_url.size() < colon + 4
		|| request_url.at(colon + 1) != '/' || request_url.at(colon + 2) != '/')
	{
		return nullptr;
	}
	const std::string::size_type next_hash = request_url.find_first_of('#', colon + 3);
	const auto url_end = next_hash == std::string::npos ? request_url.end() : request_url.begin() + next_hash;
	const std::string::size_type next_sep = request_url.find_first_of('/', colon + 3);
	const std::string::size_type next_question = request_url.find_first_of('?', colon + 3);
	const auto domain_end = next_sep == std::string::npos && next_question == std::string::npos ? url_end : request_url.begin() + (next_sep < next_question ? next_sep : next_question);
	const std::string_view domain(request_url.begin() + colon + 3, domain_end);
	const std::string_view path(domain_end, next_question == std::string::npos ? url_end : request_url.begin() + next_question);
	const std::string_view query(request_url.begin() + next_question + 1, next_question == std::string::npos ? request_url.begin() + next_question + 1 : url_end);
	const std::string_view comment(next_hash == std::string::npos ? request_url.end() : request_url.begin() + next_hash + 1, request_url.end());

    if (domain == BOLT_DOMAIN_SECURE && path == REDIRECT_PATH) {
		std::string_view code;
		QSTRING state;
		bool has_code = false;
		bool has_state = false;
		ParseQuery(query, [&](const std::string_view& key, const std::string_view& val) {
			PQSTRINGVIEW(code)
			PQSTRING(state)
		});
		if (has_code && has_state) {
			disable_default_handling = true;
            AUTHERRORIF(state != std::string_view(this->state1, STATE_LENGTH), "state mismatch in request");
			return this->RequestOAuthToken(code);
		}
	}

    if (domain == LOCALHOST_DOMAIN && path == "/") {
        std::string_view code;
		QSTRING state;
        std::string_view id_token;
		bool has_code = false;
		bool has_state = false;
        bool has_id_token = false;
		ParseQuery(comment, [&](const std::string_view& key, const std::string_view& val) {
			PQSTRINGVIEW(code)
			PQSTRING(state)
            PQSTRINGVIEW(id_token)
		});
        if (has_code && has_state && has_id_token) {
            disable_default_handling = true;
            AUTHERRORIF(state != std::string_view(this->state2, STATE_LENGTH), "state mismatch in request");
            return this->RequestSession(id_token);
        }
    }

    return nullptr;
}
#undef AUTHERRORPREFIX

void Browser::LoginWindow::NotifyClosed() {
    this->client->OnLoginWindowClosed();
}

void Browser::LoginWindow::SetSub(CefString sub) {
    this->sub = sub;
}

void Browser::LoginWindow::SetSessionID(CefString session_id) {
    const Session session {
        .session_id = session_id,
        .display_name = this->display_name,
        .suffix = this->discriminator,
        .sub = this->sub,
        .is_new = true,
    };
    this->client->AddSession(session);
    this->Close();
}

void Browser::LoginWindow::AuthError(CefString error) {
    this->client->OnAuthError(error);
    this->Close();
}

CefRefPtr<CefResourceRequestHandler> Browser::LoginWindow::RequestOAuthToken(const std::string_view& code) {
    // note: we don't need to call CefURIEncode here because all of these things
    // are already URI-encoded - most notably the `code` argument, which is a string_view
    // into a query string from another request, so we can assume it to be safe
    std::stringstream ss;
    ss <<
        "grant_type=authorization_code"
        "&client_id=" BOLT_CLIENT_ID
        "&code=" << code
        << "&code_verifier=" << std::string_view(this->verifier, VERIFIER_LENGTH)
        << "&redirect_uri=" << this->redirect_url_encoded;
    const std::string body = ss.str();
    CefRefPtr<CefPostData> post_data = CefPostData::Create();
    CefRefPtr<CefPostDataElement> element = CefPostDataElement::Create();
    element->SetToBytes(body.length(), body.c_str());
    post_data->AddElement(element);
    CefRefPtr<CefRequest> request = CefRequest::Create();
	request->Set(BOLT_ACCOUNT_ORIGIN ENDPOINT_TOKEN, "POST", post_data, {
        {L"Content-Type", L"application/x-www-form-urlencoded"},
        {L"Accept", L"application/json"},
    });
    return new AuthConsentHandler(this, this->state2, this->nonce, request, this->browser->GetHost()->GetRequestContext());
}

#define AUTHERRORPREFIX "RequestSession: "
#undef AUTHRET
#define AUTHRET nullptr
CefRefPtr<CefResourceRequestHandler> Browser::LoginWindow::RequestSession(const std::string_view& token) {
    CefRefPtr<CefValue> body_json = ParseTokenBodyJSON(token);
    AUTHCHECK(body_json, "invalid json in token")
    CefRefPtr<CefDictionaryValue> body_dict = body_json->GetDictionary();
    AUTHCHECK(body_json, "invalid json format in token")
    REQSTRING(body_dict, nonce)
    REQSTRING(body_dict, sub)
    REQSTRING(body_dict, nickname)
    REQINT(body_dict, auth_time)
    AUTHERRORIF(nonce.ToString() != std::string_view(this->nonce, STATE_LENGTH), "nonce mismatch in token");
    AUTHERRORIF(sub != this->sub, "sub mismatch in token");
    this->auth_time = auth_time;

    const std::string nickname_str = nickname.ToString();
    const std::string::size_type hash_pos = nickname_str.rfind('#');
    AUTHERRORIF(hash_pos == std::string::npos, "invalid nickname in token")
    this->display_name = std::string(std::string_view(nickname_str.begin(), nickname_str.begin() + hash_pos));
    this->discriminator = std::string(std::string_view(nickname_str.begin() + hash_pos + 1, nickname_str.end()));

    CefRefPtr<CefPostData> post_data = CefPostData::Create();
    CefRefPtr<CefPostDataElement> element = CefPostDataElement::Create(); 
    std::stringstream ss;
    ss << R"({"idToken":")" << token << R"("})";
    const std::string post_body = ss.str();
    element->SetToBytes(post_body.length(), post_body.c_str());
    post_data->AddElement(element);

    CefRefPtr<CefRequest> request = CefRequest::Create();
	request->Set(BOLT_AUTH_ORIGIN ENDPOINT_SESSION, "POST", post_data, {
        {L"Accept", L"application/json"},
        {L"Content-Type", L"application/json"},
    });
    return new AuthSessionHandler(this, request, this->browser->GetHost()->GetRequestContext());
}
#undef AUTHERRORPREFIX
