#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include "EuroScopePlugIn.h"
#include "Plugin.h"

// Global plugin instance pointer for cleanup
static DataBridgePlugin* g_pPlugin = nullptr;

void __declspec(dllexport) EuroScopePlugInInit(EuroScopePlugIn::CPlugIn** ppPluginInstance)
{
    if (!ppPluginInstance)
        return;

    g_pPlugin = new DataBridgePlugin();
    *ppPluginInstance = g_pPlugin;
}

void __declspec(dllexport) EuroScopePlugInExit(void)
{
    // The plugin instance is deleted by EuroScope via the base class destructor.
    // Our cleanup happens in DataBridgePlugin's destructor.
    g_pPlugin = nullptr;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
