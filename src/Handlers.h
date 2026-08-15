#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include "EuroScopePlugIn.h"
#include <string>

namespace edb {

// ============================================================================
// HandleRequest — route an incoming WebSocket JSON request to the appropriate
//                  ES query/setter function.
//
// Called on the EuroScope main thread (dispatched via the server's hidden
// message window, with OnTimer as a fallback). Thread-safe.
//
// Returns the JSON response string. Empty string means no response needed.
// ============================================================================
std::string HandleRequest(EuroScopePlugIn::CPlugIn& plugin, const std::string& requestJson);

} // namespace edb
