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
