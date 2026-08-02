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
// Serializer — Convert EuroScope plugin objects to JSON strings
//
// ALL functions serialize synchronously. ES temporary-object lifetime rules
// require that callers serialize inside the callback block and never store
// CFlightPlan/CRadarTarget/etc. references.
// ============================================================================

// --- Top-level objects ---
std::string SerializeFlightPlanToJson(EuroScopePlugIn::CFlightPlan& fp);
std::string SerializeRadarTargetToJson(EuroScopePlugIn::CRadarTarget& rt);
std::string SerializeControllerToJson(EuroScopePlugIn::CController& ctr);
std::string SerializeSectorElementToJson(EuroScopePlugIn::CSectorElement& se);
std::string SerializeVoiceChannelToJson(EuroScopePlugIn::CGrountToAirChannel& ch);

// --- Sub-object serializers (public for use by Handlers) ---
std::string SerializeFlightPlanDataToJson(EuroScopePlugIn::CFlightPlanData& fpd);
std::string SerializeControllerAssignedDataToJson(EuroScopePlugIn::CFlightPlanControllerAssignedData& cad);
std::string SerializeExtractedRouteToJson(EuroScopePlugIn::CFlightPlanExtractedRoute& route);
std::string SerializePositionPredictionsToJson(EuroScopePlugIn::CFlightPlanPositionPredictions& pred);
std::string SerializeRadarTargetPositionDataToJson(EuroScopePlugIn::CRadarTargetPositionData& pos);

// --- Utility ---
inline const char* SafeStr(const char* s) { return s ? s : ""; }

} // namespace edb
