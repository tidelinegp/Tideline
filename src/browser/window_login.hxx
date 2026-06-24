#ifndef _BOLT_WINDOW_LOGIN_HXX_
#define _BOLT_WINDOW_LOGIN_HXX_

#include "../browser.hxx"
#include <string_view>

#define STATE_LENGTH 12
#define VERIFIER_LENGTH 43

namespace Browser {
	struct LoginWindow: public Window, CefRequestHandler {
		LoginWindow(CefRefPtr<Browser::Client> client, int, int, bool show_devtools);

		CefRefPtr<CefRequestHandler> GetRequestHandler() override;
		CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(
			CefRefPtr<CefBrowser>,
			CefRefPtr<CefFrame>,
			CefRefPtr<CefRequest>,
			bool,
			bool,
			const CefString&,
			bool&
		) override;

		void NotifyClosed() override;

		void SetSub(CefString sub);
		void SetSessionID(CefString session_id);

		void AuthError(CefString error);

        private:
			CefRefPtr<CefResourceRequestHandler> RequestOAuthToken(const std::string_view& code);
			CefRefPtr<CefResourceRequestHandler> RequestSession(const std::string_view& token);

            char state1[STATE_LENGTH];
			char state2[STATE_LENGTH];
            char verifier[VERIFIER_LENGTH];
			char nonce[STATE_LENGTH];
			std::string redirect_url_encoded;
			int auth_time;

			bool has_tokens;
			CefString display_name;
			CefString discriminator;
        	CefString sub;
	};
}

#endif
