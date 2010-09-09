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