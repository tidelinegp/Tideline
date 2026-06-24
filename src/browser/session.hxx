#ifndef _BOLT_SESSION_HXX_
#define _BOLT_SESSION_HXX_

#include "include/cef_base.h"

/// A set of tokens associated with an active login session to an account.
/// Stored in the credentials file. All of this information is highly sensitive.
struct Session {
    CefString session_id; // JX_SESSION_ID value identifying this login session
    CefString display_name; // name of account, for example the displayName for the account "Adam#4321" would be "Adam"
	CefString suffix; // discriminator, for example the suffix for "Adam#4321" would be "4321"
    CefString sub; // unique id of this account
    bool is_new; // "true" only if this account has just been newly logged in to; used only to tell the launcher UI to print an info message
};

#endif
