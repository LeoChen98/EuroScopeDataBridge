#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include "EuroScopePlugIn.h"
#include "Constants.h"
#include "ThreadSafeQueue.h"
#include "WebSocketServer.h"

#include <memory>
#include <string>

// Forward declarations for functions in Serializer.h and Handlers.h
namespace edb {

// Serializers (defined in Serializer.h / Serializer.cpp)
std::string SerializeFlightPlanToJson(EuroScopePlugIn::CFlightPlan& fp);
std::string SerializeRadarTargetToJson(EuroScopePlugIn::CRadarTarget& rt);
std::string SerializeControllerToJson(EuroScopePlugIn::CController& ctr);
std::string SerializeSectorElementToJson(EuroScopePlugIn::CSectorElement& se);
std::string SerializeVoiceChannelToJson(EuroScopePlugIn::CGrountToAirChannel& ch);

// Request handler (defined in Handlers.h / Handlers.cpp)
std::string HandleRequest(EuroScopePlugIn::CPlugIn& plugin, const std::string& requestJson);

} // namespace edb


// ============================================================================
// DataBridgePlugin — the EuroScope plugin class
// ============================================================================
class DataBridgePlugin : public EuroScopePlugIn::CPlugIn
{
public:
    DataBridgePlugin();
    virtual ~DataBridgePlugin();

    // --- EuroScope Callbacks (Push) ---

    void OnControllerPositionUpdate(EuroScopePlugIn::CController Controller);
    void OnControllerDisconnect(EuroScopePlugIn::CController Controller);

    void OnRadarTargetPositionUpdate(EuroScopePlugIn::CRadarTarget RadarTarget);

    void OnFlightPlanDisconnect(EuroScopePlugIn::CFlightPlan FlightPlan);
    void OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan);
    void OnFlightPlanControllerAssignedDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan, int DataType);
    void OnFlightPlanFlightStripPushed(EuroScopePlugIn::CFlightPlan FlightPlan,
                                       const char* sSenderController,
                                       const char* sTargetController);

    void OnPlaneInformationUpdate(const char* sCallsign,
                                  const char* sLivery,
                                  const char* sPlaneType);

    void OnCompilePrivateChat(const char* sSenderCallsign,
                              const char* sReceiverCallsign,
                              const char* sChatMessage);

    void OnCompileFrequencyChat(const char* sSenderCallsign,
                                double Frequency,
                                const char* sChatMessage);

    void OnNewMetarReceived(const char* sStation, const char* sFullMetar);
    bool OnCompileCommand(const char* sCommandLine);
    void OnAirportRunwayActivityChanged(void) ;

    void OnTimer(int Counter) ;

    // Accessors
    edb::ThreadSafeQueue& GetIncomingQueue() { return m_incomingQueue; }

private:
    // Helper: push a JSON string to the outgoing queue, wrapped with a type
    void PushEvent(const char* type, const std::string& dataJson);
    void PushEvent(const char* type);

    // Helper: log an exception escaping from an ES callback (catch(...) handler)
    void LogCallbackError(const char* callbackName);


    edb::ThreadSafeQueue m_incomingQueue;
    std::unique_ptr<edb::WebSocketServer> m_wsServer;
};
