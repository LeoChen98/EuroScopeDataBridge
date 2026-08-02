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

    void OnControllerPositionUpdate(EuroScopePlugIn::CController Controller) override;
    void OnControllerDisconnect(EuroScopePlugIn::CController Controller) override;

    void OnRadarTargetPositionUpdate(EuroScopePlugIn::CRadarTarget RadarTarget) override;

    void OnFlightPlanDisconnect(EuroScopePlugIn::CFlightPlan FlightPlan) override;
    void OnFlightPlanFlightPlanDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan) override;
    void OnFlightPlanControllerAssignedDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan, int DataType) override;
    void OnFlightPlanFlightStripPushed(EuroScopePlugIn::CFlightPlan FlightPlan,
                                       const char* sSenderController,
                                       const char* sTargetController) override;

    void OnPlaneInformationUpdate(const char* sCallsign,
                                  const char* sLivery,
                                  const char* sPlaneType) override;

    void OnCompilePrivateChat(const char* sSenderCallsign,
                              const char* sReceiverCallsign,
                              const char* sChatMessage) override;

    void OnCompileFrequencyChat(const char* sSenderCallsign,
                                double Frequency,
                                const char* sChatMessage) override;

    void OnNewMetarReceived(const char* sStation, const char* sFullMetar) override;
    bool OnCompileCommand(const char* sCommandLine) override;
    void OnAirportRunwayActivityChanged(void) override;

    void OnTimer(int Counter) override;

    // Accessors
    edb::ThreadSafeQueue& GetIncomingQueue() { return m_incomingQueue; }
    edb::ThreadSafeQueue& GetOutgoingQueue() { return m_outgoingQueue; }

private:
    // Helper: push a JSON string to the outgoing queue, wrapped with a type
    void PushEvent(const char* type, const std::string& dataJson);
    void PushEvent(const char* type);

    // Send a full snapshot of all flight plans, radar targets, and controllers
    void SendFullSnapshot();

    edb::ThreadSafeQueue m_incomingQueue;
    edb::ThreadSafeQueue m_outgoingQueue;
    std::unique_ptr<edb::WebSocketServer> m_wsServer;
};
