#pragma once
#include <sallyAPI\sallyAPI.h>
#include <limits>
#include "Define.h"
#include "ScummVMRunThread.h"

class CAppScummVM :
	public SallyAPI::GUI::CApplicationWindow
{
private:
	SallyAPI::GUI::CImageBox*			m_pStartLogo;
	SallyAPI::GUI::CButton*				m_pStart;
	SallyAPI::GUI::CButton*				m_pResume;
	SallyAPI::GUI::CEdit*				m_pKeyboard;

	SallyAPI::GUI::CImageBox*			m_pScummVMImageBox;
	SallyAPI::GUI::CPicture*			m_pScummVMPicture;

	SallyAPI::GUI::CSideMenu*			m_pSideMenu;
	SallyAPI::GUI::CSideMenuButton*		m_pMenuShowMenu;
	SallyAPI::GUI::CSideMenuButton*		m_pMenuPause;
	SallyAPI::GUI::CSideMenuButton*		m_pMenuEsc;
	SallyAPI::GUI::CSideMenuButton*		m_pMenuFullscreen;
	SallyAPI::GUI::CSideMenuButton*		m_pMenuKeyboard;
	SallyAPI::GUI::CSideMenuButton*		m_pMenuMouse;
	SallyAPI::GUI::CSideMenuButton*		m_pMenuMouseLeft;
	SallyAPI::GUI::CSideMenuButton*		m_pMenuMouseRight;

	float								m_fLastRender;
	CScummVMRunThread					m_pScummThread;
	bool								m_bPause;

	int		m_iSurfaceWidth;
	int		m_iSurfaceHeight;

	int		m_iXScreen;
	int		m_iYScreen;
	int		m_iWidthScreen;
	int		m_iHeightScreen;
	float	m_fCorrection;

	float GetScalar();
	bool ScummIsActive(int x, int y);
	bool CheckIfScummIsHit(int x, int y);
	void UpdateScreenResolution();
	void OnCommandShowMenu();
	void OnCommandFullscreen();
	void OnCommandStart();
	void OnCommandPause();
	void OnCommandResume();
	void OnCommandThreadEnded();
	void OnCommandStartPaint();
	void OnCommandEndPaint();
	void OnCommandShowKeyboard();
	void OnCommandKeyboardClosed();
	void OnCommandEsc();
	
	void RenderScummVMWindow(SDL_Surface* surf);

	virtual void RenderControl();
	virtual bool ProcessMouseDoubleClick(int x, int y);
	virtual bool ProcessMouseDown(int x, int y);
	virtual bool ProcessMouseUp(int x, int y);
	virtual bool ProcessMouseMove(int x, int y);
public:
	CAppScummVM(SallyAPI::GUI::CGUIBaseObject *parent, int graphicID, const std::string& pluginPath);
	virtual ~CAppScummVM();

	virtual void SendMessageToParent(SallyAPI::GUI::CGUIBaseObject* reporter, int reporterId, int messageId, SallyAPI::GUI::SendMessage::CParameterBase* messageParameter = NULL);
	virtual void LoadConfig();
};