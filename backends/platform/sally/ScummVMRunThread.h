////////////////////////////////////////////////////////////////////////////////////////////////////
/// \file	sally\ScummVMRunThread.h
///
/// \brief	Declares the scummvm run thread class. 
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

#pragma once
#include <sallyAPI\sallyAPI.h>
#include "Define.h"

#include "common/scummsys.h"

#include "backends/platform/sally/sdl.h"
#include "backends/plugins/sdl/sdl-provider.h"
//#include "backends/plugins/win32/win32-provider.h"
#include "base/main.h"

class CScummVMRunThread
	: public SallyAPI::System::CThread
{
private:
	SallyAPI::GUI::CGUIBaseObject*	m_pMainWindow;

	virtual void RunEx();
public:
	CScummVMRunThread();
	virtual ~CScummVMRunThread();

	virtual void WaitForStop(bool force = false);

	void SetStaticValues(SallyAPI::GUI::CGUIBaseObject* mainWindow);
};
