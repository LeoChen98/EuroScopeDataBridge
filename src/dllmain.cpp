#include "dllmain.h"
#include "Plugin.h"


// Global plugin instance pointer for cleanup
DataBridgePlugin* gpMyPlugin = NULL;

void    __declspec (dllexport)    EuroScopePlugInInit(EuroScopePlugIn::CPlugIn** ppPlugInInstance)
{
	// create the instance
	*ppPlugInInstance = gpMyPlugin = new DataBridgePlugin();
}

//---EuroScopePlugInExit-----------------------------------------------

void    __declspec (dllexport)    EuroScopePlugInExit(void)
{
	// delete the instance
	delete gpMyPlugin;
}