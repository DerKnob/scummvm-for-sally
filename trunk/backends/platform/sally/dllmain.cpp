////////////////////////////////////////////////////////////////////////////////////////////////////
/// \file	sally\dllmain.cpp
///
/// \brief	Implements the dllmain function. 
///
/// \author	Christian Knobloch
/// \date	13.09.2010
///
/// This file is part of the Sally Project
/// 
/// Copyright(c) 2008-2010 Sally Project
/// http://www.sally-project.de/
///
/// This program is free software; you can redistribute it and/or
/// modify it under the terms of the GNU General Public License
/// as published by the Free Software Foundation; either version 3
/// of the License, or (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GPL along with this
/// program. If not, go to http://www.gnu.org/licenses/gpl.html
/// or write to the Free Software Foundation, Inc.,
/// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
////////////////////////////////////////////////////////////////////////////////////////////////////

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