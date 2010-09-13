////////////////////////////////////////////////////////////////////////////////////////////////////
/// \file	sally\ScummVMRunThread.cpp
///
/// \brief	Implements the scummvm run thread class. 
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

#include "ScummVMRunThread.h"

CScummVMRunThread::CScummVMRunThread()
{
}

CScummVMRunThread::~CScummVMRunThread()
{

}

void CScummVMRunThread::SetStaticValues(SallyAPI::GUI::CGUIBaseObject* mainWindow)
{
	m_pMainWindow = mainWindow;
}

void CScummVMRunThread::RunEx()
{
	g_system = new OSystem_SDL();

	char *argv[] = { "ScummVMSally" };

	scummvm_main(1, argv);

	g_system->quit();
	g_system = NULL;

	m_pMainWindow->SendMessageToParent(NULL, 0, GUI_APP_THREAD_ENDED);
}

void CScummVMRunThread::WaitForStop(bool force)
{
	if (g_system == NULL)
		return;

	// send the quit event
	Common::EventManager* eventManager = g_system->getEventManager();

	Common::Event event;
	event.type = Common::EVENT_QUIT;

	eventManager->pushEvent(event);

	// now wait for the stop
	int x = 0;
	while ((m_eStatus == SallyAPI::System::THREAD_RUNNING) && (x < 2000))
	{
		Sleep(10);
		x += 10;
	}

	if (m_eStatus != SallyAPI::System::THREAD_RUNNING)
		return;

	// ok, it will not end...
	g_system->quit();
	
	x = 0;
	while ((m_eStatus == SallyAPI::System::THREAD_RUNNING) && (x < 2000))
	{
		Sleep(10);
		x += 10;
	}

	if (m_eStatus != SallyAPI::System::THREAD_RUNNING)
		return;

	// now the really hard way
	SallyAPI::System::CThread::WaitForStop(true);

	// reset the g_system
	g_system = NULL;
	// send the thread ended messages
	m_pMainWindow->SendMessageToParent(NULL, 0, GUI_APP_THREAD_ENDED);

	SallyAPI::System::CLogger* logger = SallyAPI::Core::CGame::GetLogger();

	std::string scummvmInfo = "ScummVM thread was not ended correctly. Forced to stop!";
	logger->Warning(scummvmInfo);
}