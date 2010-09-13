////////////////////////////////////////////////////////////////////////////////////////////////////
/// \file	sally\AppScummVM.h
///
/// \brief	Declares the application scummvm class. 
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
#include <limits>
#include "Define.h"
#include "ScummVMRunThread.h"

class CAppScummVM :
	public SallyAPI::GUI::CGameWindow
{
private:
	SallyAPI::GUI::CButton*				m_pResume;
	SallyAPI::GUI::CEdit*				m_pKeyboard;

	SallyAPI::GUI::CImageBox*			m_pBackBlackground;

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
	void OnCommandPause();
	void OnCommandResume();
	void OnCommandThreadEnded();
	void OnCommandStartPaint();
	void OnCommandShowKeyboard();
	void OnCommandKeyboardClosed();
	void OnCommandEsc();
	
	void RenderScummVMWindow(SDL_Surface* surf);

	virtual void RenderControl();
	virtual bool ProcessMouseDoubleClick(int x, int y);
	virtual bool ProcessMouseDown(int x, int y);
	virtual bool ProcessMouseUp(int x, int y);
	virtual bool ProcessMouseMove(int x, int y);

	virtual void	GameLoadEx();
public:
	CAppScummVM(SallyAPI::GUI::CGUIBaseObject *parent, int graphicID, const std::string& pluginPath);
	virtual ~CAppScummVM();

	virtual void SendMessageToParent(SallyAPI::GUI::CGUIBaseObject* reporter, int reporterId, int messageId, SallyAPI::GUI::SendMessage::CParameterBase* messageParameter = NULL);
	virtual void LoadConfig();
};
