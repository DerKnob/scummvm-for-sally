#include <windows.h>
#include <string>
#include <sallyAPI\sallyAPI.h>

#undef ARRAYSIZE

#pragma comment(lib, "sallyAPI.lib")
#define EXPORT __declspec (dllexport) 
#define DLL_IMPORT_SALLY_API 1

BOOL APIENTRY DllMain( HMODULE hModule,
					  DWORD  ul_reason_for_call,
					  LPVOID lpReserved
					  )
{
	return TRUE;
}

#include "Define.h"
#include "AppScummVM.h"

extern "C" EXPORT SallyAPI::GUI::CApplicationWindow* 
	CreateApplication(SallyAPI::GUI::CGUIBaseObject *parent, int graphicID,
				  const std::string& pluginPath)
{
	CAppScummVM* app;

	app = new CAppScummVM(parent, graphicID, pluginPath);
	return app;
}