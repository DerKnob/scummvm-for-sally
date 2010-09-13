////////////////////////////////////////////////////////////////////////////////////////////////////
/// \file	sally\AppScummVM.cpp
///
/// \brief	Implements the application scummvm class. 
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

#include "AppScummVM.h"

#define pixelSpace int

static int mapKey(SDLKey key, SDLMod mod, Uint16 unicode)
{
	if (key >= SDLK_F1 && key <= SDLK_F9) {
		return key - SDLK_F1 + Common::ASCII_F1;
	} else if (key >= SDLK_KP0 && key <= SDLK_KP9) {
		return key - SDLK_KP0 + '0';
	} else if (key >= SDLK_UP && key <= SDLK_PAGEDOWN) {
		return key;
	} else if (unicode) {
		return unicode;
	} else if (key >= 'a' && key <= 'z' && (mod & KMOD_SHIFT)) {
		return key & ~0x20;
	} else if (key >= SDLK_NUMLOCK && key <= SDLK_EURO) {
		return 0;
	}
	return key;
}

CAppScummVM::CAppScummVM(SallyAPI::GUI::CGUIBaseObject *parent, int graphicID, const std::string& pluginPath)
	:SallyAPI::GUI::CGameWindow(parent, graphicID, pluginPath),
	m_iSurfaceWidth(0), m_iSurfaceHeight(0), m_pScummVMPicture(NULL), m_bPause(false),
	m_fLastRender(0)
{
	SDL_SetModuleHandle(GetModuleHandle(NULL));

	m_pKeyboard = new SallyAPI::GUI::CEdit(this, 0, 0, 0);
	m_pKeyboard->Visible(false);
	m_pKeyboard->Enable(false);
	this->AddChild(m_pKeyboard);

	m_pBackBlackground = new SallyAPI::GUI::CImageBox(m_pGameForm, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	m_pBackBlackground->SetImageId(GUI_THEME_SALLY_BLACK_BACKGROUND);
	m_pGameForm->AddChild(m_pBackBlackground);

	m_pScummVMImageBox = new SallyAPI::GUI::CImageBox(m_pGameForm, 0, 0, 0, 0);
	m_pScummVMImageBox->Visible(false);
	m_pScummVMImageBox->SetAlphaBlending(0);
	m_pGameForm->AddChild(m_pScummVMImageBox);

	m_pResume = new SallyAPI::GUI::CButton(m_pGameForm, (WINDOW_WIDTH - 128) / 2, (WINDOW_HEIGHT - 60) / 2 + 30, 128, CONTROL_HEIGHT, GUI_APP_RESUME);
	m_pResume->SetText("Resume");
	m_pResume->SetAlign(DT_CENTER | DT_VCENTER);
	m_pResume->Visible(false);
	m_pResume->SetAlphaBlending(0);
	m_pGameForm->AddChild(m_pResume);

	// side menu border
	m_pSideMenu = new SallyAPI::GUI::CSideMenu(m_pGameForm);
	m_pSideMenu->Visible(false);
	m_pSideMenu->SetAlphaBlending(0);
	m_pGameForm->AddChild(m_pSideMenu);

	m_pMenuShowMenu = new SallyAPI::GUI::CSideMenuButton(m_pSideMenu, SallyAPI::GUI::SIDE_MENUE_BUTTON_TYPE_NORMAL, GUI_APP_SHOW_MENU);
	m_pMenuShowMenu->SetText("Menu");
	m_pMenuShowMenu->SetImageId(GUI_THEME_SALLY_ICON_PROPERTIES);
	m_pSideMenu->AddChild(m_pMenuShowMenu);

	m_pMenuPause = new SallyAPI::GUI::CSideMenuButton(m_pSideMenu, SallyAPI::GUI::SIDE_MENUE_BUTTON_TYPE_NORMAL, GUI_APP_PAUSE);
	m_pMenuPause->SetText("Pause");
	m_pMenuPause->SetImageId(GUI_THEME_SALLY_ICON_MEDIA_PAUSE);
	m_pSideMenu->AddChild(m_pMenuPause);

	m_pMenuFullscreen = new SallyAPI::GUI::CSideMenuButton(m_pSideMenu, SallyAPI::GUI::SIDE_MENUE_BUTTON_TYPE_NORMAL, GUI_APP_FULLSCREEN);
	m_pMenuFullscreen->SetText("Fullscreen");
	m_pMenuFullscreen->SetImageId(GUI_THEME_SALLY_ICON_FULLSCREEN);
	m_pSideMenu->AddChild(m_pMenuFullscreen);

	m_pMenuKeyboard = new SallyAPI::GUI::CSideMenuButton(m_pSideMenu, SallyAPI::GUI::SIDE_MENUE_BUTTON_TYPE_NORMAL, GUI_APP_KEYBOARD);
	m_pMenuKeyboard->SetText("Keyboard");
	m_pMenuKeyboard->SetImageId(GUI_THEME_SALLY_ICON_KEYBOARD);
	m_pSideMenu->AddChild(m_pMenuKeyboard);

	m_pMenuEsc = new SallyAPI::GUI::CSideMenuButton(m_pSideMenu, SallyAPI::GUI::SIDE_MENUE_BUTTON_TYPE_NORMAL, GUI_APP_ESC);
	m_pMenuEsc->SetText("Skip");
	m_pMenuEsc->SetImageId(GUI_THEME_SALLY_ICON_MEDIA_SKIP_FORWARD);
	m_pSideMenu->AddChild(m_pMenuEsc);

	m_pMenuMouse = new SallyAPI::GUI::CSideMenuButton(m_pSideMenu, SallyAPI::GUI::SIDE_MENUE_BUTTON_TYPE_SEPERATOR);
	m_pSideMenu->AddChild(m_pMenuMouse);

	m_pMenuMouseLeft = new SallyAPI::GUI::CSideMenuButton(m_pSideMenu, SallyAPI::GUI::SIDE_MENUE_BUTTON_TYPE_NORMAL, GUI_APP_MOUSE_LEFT);
	m_pMenuMouseLeft->SetText("Left");
	m_pMenuMouseLeft->SetImageId(GUI_THEME_SALLY_ICON_MOUSE);
	m_pMenuMouseLeft->SetCheckStatus(true);
	m_pSideMenu->AddChild(m_pMenuMouseLeft);

	m_pMenuMouseRight = new SallyAPI::GUI::CSideMenuButton(m_pSideMenu, SallyAPI::GUI::SIDE_MENUE_BUTTON_TYPE_NORMAL, GUI_APP_MOUSE_RIGHT);
	m_pMenuMouseRight->SetText("Right");
	m_pMenuMouseRight->SetImageId(GUI_THEME_SALLY_ICON_MOUSE);
	m_pSideMenu->AddChild(m_pMenuMouseRight);

	// correct the save path
	SallyAPI::System::CLogger* logger = SallyAPI::Core::CGame::GetLogger();
	char	dir[MAX_PATH];

	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, dir);
	std::string iniFile = dir;
	iniFile.append("\\ScummVM\\scummvm.ini");

	//////////////////////////////////////////////////////////////////////////
	std::string scummvmInfo = "ScummVM ini file: ";
	scummvmInfo.append(iniFile);
	logger->Info(scummvmInfo);
	//////////////////////////////////////////////////////////////////////////

	SallyAPI::System::COption option(iniFile);

	std::string savePath = option.GetPropertyString("scummvm", "savepath", "");
	if (savePath.length() == 0)
	{
		std::string mediaDir = dir;
		mediaDir.append("\\ScummVM");

		option.SetPropertyString("scummvm", "savepath", mediaDir);

		savePath = mediaDir;
	}

	//////////////////////////////////////////////////////////////////////////
	scummvmInfo = "ScummVM save folder: ";
	scummvmInfo.append(savePath);
	logger->Info(scummvmInfo);
	//////////////////////////////////////////////////////////////////////////

	m_pScummThread.SetStaticValues(this);
}

CAppScummVM::~CAppScummVM()
{
	m_pScummThread.WaitForStop();

	SafeDelete(m_pScummVMPicture);
}

void CAppScummVM::LoadConfig()
{
	if (GetPropertyBool("fullscreen", true))
		m_pMenuFullscreen->SetCheckStatus(true);
	else
		m_pMenuFullscreen->SetCheckStatus(false);
}

void CAppScummVM::SendMessageToParent(SallyAPI::GUI::CGUIBaseObject* reporter, int reporterId, int messageId, SallyAPI::GUI::SendMessage::CParameterBase* messageParameter)
{
	switch (messageId)
	{
	case MS_SALLY_KEYBOARD_CLOSED:
		OnCommandKeyboardClosed();
		return;
	case GUI_CONTROL_BLENDED:
		if (reporter->GetAlphaBlending() == 0)
		{
			SallyAPI::GUI::CControl* control = static_cast<SallyAPI::GUI::CControl*> (reporter);
			if (control == NULL)
				return;

			control->Visible(false);

			// blend out is done
			if (reporter == m_pStartForm)
				OnCommandStartPaint();

			if (reporter == m_pResume)
			{
				// Pause off
				m_pSideMenu->Visible(true);
				m_pScummVMImageBox->Visible(true);
				m_pSideMenu->BlendAnimated(255, 500);
				m_pScummVMImageBox->BlendAnimated(255, 500);
			}
			else if (reporter == m_pScummVMImageBox)
			{
				// Pause on
				m_pResume->Enable(true);
				m_pResume->Visible(true);
				m_pResume->BlendAnimated(255, 500);
			}
		}
		break;
	case GUI_APP_THREAD_ENDED:
		OnCommandThreadEnded();
		return;
	case GUI_BUTTON_CLICKED:
		switch (reporterId)
		{
		case GUI_APP_MOUSE_LEFT:
			m_pMenuMouseLeft->SetCheckStatus(true);
			m_pMenuMouseRight->SetCheckStatus(false);
			return;
		case GUI_APP_MOUSE_RIGHT:
			m_pMenuMouseLeft->SetCheckStatus(false);
			m_pMenuMouseRight->SetCheckStatus(true);
			return;
		case GUI_APP_SHOW_MENU:
			OnCommandShowMenu();
			return;
		case GUI_APP_FULLSCREEN:
			OnCommandFullscreen();
			return;
		case GUI_APP_PAUSE:
			OnCommandPause();
			return;
		case GUI_APP_ESC:
			OnCommandEsc();
			return;
		case GUI_APP_RESUME:
			OnCommandResume();
			return;
		case GUI_APP_KEYBOARD:
			OnCommandShowKeyboard();
			return;
		}
		break;
	}
	SallyAPI::GUI::CGameWindow::SendMessageToParent(reporter, reporterId, messageId, messageParameter);
}

void CAppScummVM::OnCommandStartPaint()
{
	// blend in
	m_pScummVMImageBox->Visible(true);
	m_pSideMenu->Visible(true);
	m_pScummVMImageBox->BlendAnimated(255, 500);
	m_pSideMenu->BlendAnimated(255, 500);
}

void CAppScummVM::OnCommandThreadEnded()
{
	SendMessageToParent(this, MS_SALLY_GAME_STOP, GUI_BUTTON_CLICKED);
}

void CAppScummVM::OnCommandPause()
{
	m_bPause = true;

	m_pSideMenu->BlendAnimated(0, 500);
	m_pScummVMImageBox->BlendAnimated(0, 500);

	if (g_system == NULL)
		return;

	Common::EventManager* eventManager = g_system->getEventManager();

	Common::Event event;
	event.type = Common::EVENT_KEYDOWN;
	event.kbd.keycode = Common::KEYCODE_SPACE;
	event.kbd.ascii = mapKey(SDLK_SPACE, KMOD_NONE, 0);
	eventManager->pushEvent(event);

	event.type = Common::EVENT_KEYUP;
	eventManager->pushEvent(event);
}

void CAppScummVM::OnCommandEsc()
{
	if (g_system == NULL)
		return;

	Common::EventManager* eventManager = g_system->getEventManager();

	Common::Event event;
	event.type = Common::EVENT_KEYDOWN;
	event.kbd.keycode = Common::KEYCODE_ESCAPE;
	event.kbd.ascii = mapKey(SDLK_ESCAPE, KMOD_NONE, 0);
	eventManager->pushEvent(event);

	event.type = Common::EVENT_KEYUP;
	eventManager->pushEvent(event);
}

void CAppScummVM::OnCommandResume()
{
	m_pResume->Enable(false);
	m_pResume->BlendAnimated(0, 500);

	if (g_system == NULL)
		return;

	Common::EventManager* eventManager = g_system->getEventManager();

	Common::Event event;
	event.type = Common::EVENT_KEYDOWN;
	event.kbd.keycode = Common::KEYCODE_SPACE;
	event.kbd.ascii = mapKey(SDLK_SPACE, KMOD_NONE, 0);
	eventManager->pushEvent(event);

	event.type = Common::EVENT_KEYUP;
	eventManager->pushEvent(event);

	m_bPause = false;
}

void CAppScummVM::GameLoadEx()
{
	// start the scummvm thread
	m_pScummThread.Start();
}

void CAppScummVM::OnCommandFullscreen()
{
	if (g_system == NULL)
		return;

	if (m_pMenuFullscreen->GetCheckStatus())
	{
		m_pMenuFullscreen->SetCheckStatus(false);
		SetPropertyBool("fullscreen", false);
	}
	else
	{
		m_pMenuFullscreen->SetCheckStatus(true);
		SetPropertyBool("fullscreen", true);
	}
	UpdateScreenResolution();
}

void CAppScummVM::OnCommandShowMenu()
{
	if (g_system == NULL)
		return;

	Common::EventManager* eventManager = g_system->getEventManager();

	Common::Event event;
	event.type = Common::EVENT_MAINMENU;
	eventManager->pushEvent(event);
}

void CAppScummVM::OnCommandShowKeyboard()
{
	m_pKeyboard->SetText("");

	SallyAPI::GUI::SendMessage::CParameterInteger messageInteger(0);
	this->SendMessageToParent(m_pKeyboard, 0, MS_SALLY_SHOW_KEYBOARD, &messageInteger);

	m_bPause = true;
}

void CAppScummVM::OnCommandKeyboardClosed()
{
	if (g_system == NULL)
		return;

	std::string text = m_pKeyboard->GetText();

	for (int i = 0; i < (int) text.length(); ++i)
	{
		char c = text[i];

		Common::EventManager* eventManager = g_system->getEventManager();

		Common::Event event;
		event.type = Common::EVENT_KEYDOWN;
		event.kbd.ascii = c;
		eventManager->pushEvent(event);

		event.type = Common::EVENT_KEYUP;
		eventManager->pushEvent(event);

		Sleep(100);
	}
	m_bPause = false;
}

void CAppScummVM::RenderControl()
{
	// time for the next frame?
	if (m_fLastRender + 0.05f > m_fTimeDelta)
	{
		SallyAPI::GUI::CGameWindow::RenderControl();
		return;
	}
	m_fLastRender = m_fTimeDelta;

	// is summv active?
	if (!ScummIsActive(-1, -1))
	{
		SallyAPI::GUI::CGameWindow::RenderControl();
		return;
	}

	OSystem_SDL* sdl_system = static_cast<OSystem_SDL*> (g_system);
	if (sdl_system == NULL)
		return;

	SDL_Surface* surf = sdl_system->lockHardwareScreen();

	RenderScummVMWindow(surf);

	sdl_system->unlockHardwareScreen();

	SallyAPI::GUI::CGameWindow::RenderControl();
}

void CAppScummVM::RenderScummVMWindow(SDL_Surface* surf)
{
	if (surf == NULL)
		return;

	if (surf->format == NULL)
		return;

	char* sdlTextureData = (char*) surf->pixels;
	if (sdlTextureData == NULL)
		return;

	// do we already have a image created?
	if ((m_iSurfaceWidth != surf->w) || (m_iSurfaceHeight != surf->h))
	{
		m_pScummVMImageBox->SetPicture(NULL);
		SafeDelete(m_pScummVMPicture);
	}
	if (m_pScummVMPicture == NULL)
	{
		m_iSurfaceWidth = surf->w;
		m_iSurfaceHeight = surf->h;

		m_pScummVMPicture = new SallyAPI::GUI::CPicture();
		m_pScummVMPicture->CreateEmptyD3DFormat(m_iSurfaceWidth, m_iSurfaceHeight, D3DFMT_R8G8B8);//D3DFMT_X8B8G8R8);
		m_pScummVMImageBox->SetPicture(m_pScummVMPicture);

		UpdateScreenResolution();
	}
	SallyAPI::Core::CTexture* texture = m_pScummVMPicture->GetTexture();
	if (texture == NULL)
		return;

	LPDIRECT3DTEXTURE9 d3dTexture = texture->GetTexture();
	if (d3dTexture == NULL)
		return;

	IDirect3DSurface9* d3d_surf = NULL;
	d3dTexture->GetSurfaceLevel(0, &d3d_surf);

	if (d3d_surf == NULL)
		return;

	// lock the surface
	D3DLOCKED_RECT locked_rect;
	d3d_surf->LockRect(&locked_rect, NULL, 0);

	pixelSpace* directXTextutureData = (pixelSpace*)(locked_rect.pBits);

	int pitchDirectX = locked_rect.Pitch / sizeof(pixelSpace);

	char* pSource = sdlTextureData;
	pixelSpace* pDestination = directXTextutureData;

	for(int i = 0; i < m_iSurfaceHeight; ++i)
	{
		// fist pixel of this line
		pDestination = directXTextutureData;
		pSource = sdlTextureData;
		for (int k = 0; k < m_iSurfaceWidth; ++k)
		{
			SDL_Color color;
			Uint32 col = 0;

			//copy pixel data
	
			memcpy(&col, pSource, surf->format->BytesPerPixel);

			//convert color
			SDL_GetRGB(col, surf->format, &color.r, &color.g, &color.b);

			int r = color.r;
			int g = color.g;
			int b = color.b;
			pixelSpace colorDX =  D3DCOLOR_ARGB(255, r, g, b);

			*pDestination = colorDX;

			// go on...
			++pDestination;
			pSource += surf->format->BytesPerPixel;
		}
		// next lines
		sdlTextureData += surf->pitch;
		directXTextutureData += pitchDirectX;
	}

	// we are done, unlock the surface
	d3d_surf->UnlockRect();
}

void CAppScummVM::UpdateScreenResolution()
{
	m_iXScreen = 0;
	m_iYScreen = 0;
	m_iWidthScreen = 0;
	m_iHeightScreen = 0;

	if ((m_iSurfaceWidth > WINDOW_WIDTH - MENU_WIDTH) || (m_iSurfaceHeight > WINDOW_HEIGHT))
	{
		// the source is bigger than the target... so resize
		SallyAPI::GUI::GUIHelper::CalculateImageSize(m_iSurfaceWidth, m_iSurfaceHeight,
			WINDOW_WIDTH - MENU_WIDTH, WINDOW_HEIGHT, m_iXScreen, m_iYScreen, m_iWidthScreen, m_iHeightScreen);
	}
	else if (m_pMenuFullscreen->GetCheckStatus())
	{
		// resize is requested
		SallyAPI::GUI::GUIHelper::CalculateImageSize(m_iSurfaceWidth, m_iSurfaceHeight,
			WINDOW_WIDTH - MENU_WIDTH, WINDOW_HEIGHT, m_iXScreen, m_iYScreen, m_iWidthScreen, m_iHeightScreen);
	}
	else
	{
		m_iXScreen = (WINDOW_WIDTH - MENU_WIDTH - m_iSurfaceWidth) / 2;
		m_iYScreen = (WINDOW_HEIGHT - m_iSurfaceHeight) / 2;

		m_iWidthScreen = m_iSurfaceWidth;
		m_iHeightScreen = m_iSurfaceHeight;
	}

	//m_iXScreen -= MENU_WIDTH;
	

	m_pScummVMImageBox->Move(m_iXScreen, m_iYScreen);
	m_pScummVMImageBox->Resize(m_iWidthScreen, m_iHeightScreen);

	m_fCorrection = (float) m_iWidthScreen / m_iSurfaceWidth;
}

float CAppScummVM::GetScalar()
{
	OSystem_SDL* sdl_system = static_cast<OSystem_SDL*> (g_system);
	if (sdl_system == NULL)
		return 1;

	return sdl_system->getVideoScaleVactor();
}

bool CAppScummVM::ProcessMouseDoubleClick(int x, int y)
{
	SallyAPI::GUI::CGameWindow::ProcessMouseDoubleClick(x, y);

	if (!ScummIsActive(x, y))
		return true;

	float scalar = GetScalar();

	Common::EventManager* eventManager = g_system->getEventManager();

	// mouse down
	Common::Event event;
	if (m_pMenuMouseLeft->GetCheckStatus())
		event.type = Common::EVENT_LBUTTONDOWN;
	else
		event.type = Common::EVENT_RBUTTONDOWN;
	event.mouse.x = (x - m_iXScreen) / m_fCorrection / scalar;
	event.mouse.y = (y - m_iYScreen) / m_fCorrection / scalar;

	eventManager->pushEvent(event);

	// mouse up
	if (m_pMenuMouseLeft->GetCheckStatus())
		event.type = Common::EVENT_LBUTTONUP;
	else
		event.type = Common::EVENT_RBUTTONUP;
	eventManager->pushEvent(event);

	return true;
}

bool CAppScummVM::ProcessMouseDown(int x, int y)
{
	SallyAPI::GUI::CGameWindow::ProcessMouseDown(x, y);

	if (!ScummIsActive(x, y))
		return true;

	float scalar = GetScalar();

	Common::EventManager* eventManager = g_system->getEventManager();

	Common::Event event;
	if (m_pMenuMouseLeft->GetCheckStatus())
		event.type = Common::EVENT_LBUTTONDOWN;
	else
		event.type = Common::EVENT_RBUTTONDOWN;
	event.mouse.x = (x - m_iXScreen) / m_fCorrection / scalar;
	event.mouse.y = (y - m_iYScreen) / m_fCorrection / scalar;

	eventManager->pushEvent(event);

	return true;
}

bool CAppScummVM::ProcessMouseUp(int x, int y)
{
	SallyAPI::GUI::CGameWindow::ProcessMouseUp(x, y);

	if (!ScummIsActive(x, y))
		return true;

	float scalar = GetScalar();

	Common::EventManager* eventManager = g_system->getEventManager();

	Common::Event event;
	if (m_pMenuMouseLeft->GetCheckStatus())
		event.type = Common::EVENT_LBUTTONUP;
	else
		event.type = Common::EVENT_RBUTTONUP;
	event.mouse.x = (x - m_iXScreen) / m_fCorrection / scalar;
	event.mouse.y = (y - m_iYScreen) / m_fCorrection / scalar;
	eventManager->pushEvent(event);

	return true;
}

bool CAppScummVM::ProcessMouseMove(int x, int y)
{
	SallyAPI::GUI::CGameWindow::ProcessMouseMove(x, y);

	if (!ScummIsActive(x, y))
		return true;

	float scalar = GetScalar();
	
	Common::EventManager* eventManager = g_system->getEventManager();

	Common::Event event;
	event.type = Common::EVENT_MOUSEMOVE;
	event.mouse.x = (x - m_iXScreen) / m_fCorrection / scalar;
	event.mouse.y = (y - m_iYScreen) / m_fCorrection / scalar;
	eventManager->pushEvent(event);
	g_system->warpMouse((x - m_iXScreen) / m_fCorrection / scalar, (y - m_iYScreen) / m_fCorrection / scalar);

	return true;
}

bool CAppScummVM::CheckIfScummIsHit(int x, int y)
{
	// sind wir auf der scummvm schaltfläche?
	if ((x < m_iXScreen) || (y < m_iYScreen))
		return false;
	if ((x > m_iXScreen + m_iWidthScreen) || (y > m_iYScreen + m_iHeightScreen))
		return false;

	return true;
}

bool CAppScummVM::ScummIsActive(int x, int y)
{
	if (!m_pScummVMImageBox->IsVisible())
		return false;

	if (g_system == NULL)
		return false;

	if ((x > -1) && (y > -1))
	{
		// only on mouse commands
		if (m_bPause)
			return false;

		if (CheckIfScummIsHit(x, y) == false)
			return false;
	}
	return true;
}