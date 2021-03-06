/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/backends/platform/gp2x/gp2x-common.h $
 * $Id: gp2x-common.h 45796 2009-11-09 23:58:12Z fingolfin $
 *
 */

#ifndef GP2X_COMMON_H
#define GP2X_COMMON_H

#include <SDL.h>
#include <SDL_gp2x.h>

#include "backends/base-backend.h"
#include "graphics/scaler.h"

#define __GP2X__
#define USE_OSD
/* #define DISABLE_SCALERS */
#define MIXER_DOUBLE_BUFFERING 1

namespace Audio {
	class MixerImpl;
}

enum {
	GFX_NORMAL = 0
};


class OSystem_GP2X : public BaseBackend {
public:
	OSystem_GP2X();
	virtual ~OSystem_GP2X();

	virtual void initBackend();

	void beginGFXTransaction(void);
	TransactionError endGFXTransaction(void);

	// Set the size of the video bitmap.
	// Typically, 320x200
	void initSize(uint w, uint h, const Graphics::PixelFormat *format);

	int getScreenChangeID() const { return _screenChangeCount; }

	// Set colors of the palette
	void setPalette(const byte *colors, uint start, uint num);

	// Get colors of the palette
	void grabPalette(byte *colors, uint start, uint num);

	// Draw a bitmap to screen.
	// The screen will not be updated to reflect the new bitmap
	void copyRectToScreen(const byte *src, int pitch, int x, int y, int w, int h);

	virtual Graphics::Surface *lockScreen();
	virtual void unlockScreen();

	// Update the dirty areas of the screen
	void updateScreen();

	// Either show or hide the mouse cursor
	bool showMouse(bool visible);

	// Warp the mouse cursor. Where set_mouse_pos() only informs the
	// backend of the mouse cursor's current position, this function
	// actually moves the cursor to the specified position.
	void warpMouse(int x, int y);

	// Set the bitmap that's used when drawing the cursor.
	void setMouseCursor(const byte *buf, uint w, uint h, int hotspot_x, int hotspot_y, uint32 keycolor, int cursorTargetScale, const Graphics::PixelFormat *format);

	// Set colors of cursor palette
	void setCursorPalette(const byte *colors, uint start, uint num);

	// Disables or enables cursor palette
	void disableCursorPalette(bool disable) {
		_cursorPaletteDisabled = disable;
		blitCursor();
	}

	// Shaking is used in SCUMM. Set current shake position.
	void setShakePos(int shake_pos);

	// Get the number of milliseconds since the program was started.
	uint32 getMillis();

	// Delay for a specified amount of milliseconds
	void delayMillis(uint msecs);

	// Get the next event.
	// Returns true if an event was retrieved.
	virtual bool pollEvent(Common::Event &event); // overloaded by CE backend

	// Sets up the keymapper with the backends hardware key set
	void setupKeymapper();

	// Set function that generates samples
	void setupMixer();
	static void mixCallback(void *s, byte *samples, int len);

	void closeMixer();

	virtual Audio::Mixer *getMixer();

	// Poll CD status
	// Returns true if cd audio is playing
	bool pollCD();

	// Play CD audio track
	void playCD(int track, int num_loops, int start_frame, int duration);

	// Stop CD audio track
	void stopCD();

	// Update CD audio status
	void updateCD();

	// Quit
	void quit();

	void getTimeAndDate(TimeDate &t) const;
	virtual Common::TimerManager *getTimerManager();

	// Mutex handling
	MutexRef createMutex();
	void lockMutex(MutexRef mutex);
	void unlockMutex(MutexRef mutex);
	void deleteMutex(MutexRef mutex);

	// Overlay
	Graphics::PixelFormat getOverlayFormat() const { return _overlayFormat; }
	void showOverlay();
	void hideOverlay();
	void clearOverlay();
	void grabOverlay(OverlayColor *buf, int pitch);
	void copyRectToOverlay(const OverlayColor *buf, int pitch, int x, int y, int w, int h);
	int16 getHeight();
	int16 getWidth();
	int16 getOverlayHeight()  { return _videoMode.overlayHeight; }
	int16 getOverlayWidth()   { return _videoMode.overlayWidth; }

	const GraphicsMode *getSupportedGraphicsModes() const;
	int getDefaultGraphicsMode() const;
	bool setGraphicsMode(int mode);
	int getGraphicsMode() const;

	bool openCD(int drive);

	bool hasFeature(Feature f);
	void setFeatureState(Feature f, bool enable);
	bool getFeatureState(Feature f);

	void displayMessageOnOSD(const char *msg);

	virtual Common::SaveFileManager *getSavefileManager();
	virtual FilesystemFactory *getFilesystemFactory();
	virtual void addSysArchivesToSearchSet(Common::SearchSet &s, int priority = 0);

	virtual Common::SeekableReadStream *createConfigReadStream();
	virtual Common::WriteStream *createConfigWriteStream();

protected:
	bool _inited;

	SDL_Surface *_osdSurface;
	Uint8 _osdAlpha;			// Transparency level of the OSD
	uint32 _osdFadeStartTime;	// When to start the fade out
	enum {
		kOSDFadeOutDelay = 2 * 1000,	// Delay before the OSD is faded out (in milliseconds)
		kOSDFadeOutDuration = 500,		// Duration of the OSD fade out (in milliseconds)
		kOSDColorKey = 1,
		kOSDInitialAlpha = 80			// Initial alpha level, in percent
	};

	// hardware screen
	SDL_Surface *_hwscreen;

	// unseen game screen
	SDL_Surface *_screen;

	// temporary screen (for scalers)
	SDL_Surface *_tmpscreen;
	SDL_Surface *_tmpscreen2;

	// overlay
	SDL_Surface *_overlayscreen;
	bool _overlayVisible;
	Graphics::PixelFormat _overlayFormat;

	// Audio
	int _samplesPerSec;

	// CD Audio
	SDL_CD *_cdrom;
	int _cdTrack, _cdNumLoops, _cdStartFrame, _cdDuration;
	uint32 _cdEndTime, _cdStopTime;

	enum {
		DF_WANT_RECT_OPTIM			= 1 << 0
	};

	enum {
		kTransactionNone = 0,
		kTransactionActive = 1,
		kTransactionRollback = 2
	};

	struct TransactionDetails {
		bool sizeChanged;
		bool needHotswap;
		bool needUpdatescreen;
		bool normal1xScaler;
	};
	TransactionDetails _transactionDetails;

	struct VideoState {
		bool setup;

		bool fullscreen;
		bool aspectRatioCorrection;

		int mode;
		int scaleFactor;

		int screenWidth, screenHeight;
		int overlayWidth, overlayHeight;
	};
	VideoState _videoMode, _oldVideoMode;

	virtual void setGraphicsModeIntern(); // overloaded by CE backend

	/** Force full redraw on next updateScreen */
	bool _forceFull;
	ScalerProc *_scalerProc;
	int _scalerType;
	int _transactionMode;

	bool _screenIsLocked;
	Graphics::Surface _framebuffer;

	/** Current video mode flags (see DF_* constants) */
	uint32 _modeFlags;
	bool _modeChanged;
	int _screenChangeCount;

	/* True if zoom on mouse is enabled. (only set by > 240 high games) */
	bool _adjustZoomOnMouse;

	enum {
		NUM_DIRTY_RECT = 100,
		MAX_MOUSE_W = 80,
		MAX_MOUSE_H = 80,
		MAX_SCALING = 3
	};

	// Dirty rect management
	SDL_Rect _dirtyRectList[NUM_DIRTY_RECT];
	int _numDirtyRects;
	uint32 *_dirtyChecksums;
	bool _cksumValid;
	int _cksumNum;

	// Keyboard mouse emulation.  Disabled by fingolfin 2004-12-18.
	// I am keeping the rest of the code in for now, since the joystick
	// code (or rather, "hack") uses it, too.
	struct KbdMouse {
		int16 x, y, x_vel, y_vel, x_max, y_max, x_down_count, y_down_count;
		uint32 last_time, delay_time, x_down_time, y_down_time;
	};

	struct MousePos {
		// The mouse position, using either virtual (game) or real
		// (overlay) coordinates.
		int16 x, y;

		// The size and hotspot of the original cursor image.
		int16 w, h;
		int16 hotX, hotY;

		// The size and hotspot of the pre-scaled cursor image, in real
		// coordinates.
		int16 rW, rH;
		int16 rHotX, rHotY;

		// The size and hotspot of the pre-scaled cursor image, in game
		// coordinates.
		int16 vW, vH;
		int16 vHotX, vHotY;

		MousePos() : x(0), y(0), w(0), h(0), hotX(0), hotY(0),
		             rW(0), rH(0), rHotX(0), rHotY(0), vW(0), vH(0),
		             vHotX(0), vHotY(0)
			{ }
	};

	// mouse
	KbdMouse _km;
	bool _mouseVisible;
	bool _mouseNeedsRedraw;
	byte *_mouseData;
	SDL_Rect _mouseBackup;
	MousePos _mouseCurState;
	byte _mouseKeyColor;
	int _cursorTargetScale;
	bool _cursorPaletteDisabled;
	SDL_Surface *_mouseOrigSurface;
	SDL_Surface *_mouseSurface;
	enum {
		kMouseColorKey = 1
	};

	// joystick
	SDL_Joystick *_joystick;
	bool _stickBtn[32];

	// Shake mode
	int _currentShakePos;
	int _newShakePos;

	// Palette data
	SDL_Color *_currentPalette;
	uint _paletteDirtyStart, _paletteDirtyEnd;

	// Cursor palette data
	SDL_Color *_cursorPalette;

	/**
	 * Mutex which prevents multiple threads from interfering with each other
	 * when accessing the screen.
	 */
	MutexRef _graphicsMutex;

#ifdef MIXER_DOUBLE_BUFFERING
	SDL_mutex *_soundMutex;
	SDL_cond *_soundCond;
	SDL_Thread *_soundThread;
	bool _soundThreadIsRunning;
	bool _soundThreadShouldQuit;

	byte _activeSoundBuf;
	uint _soundBufSize;
	byte *_soundBuffers[2];

	void mixerProducerThread();
	static int SDLCALL mixerProducerThreadEntry(void *arg);
	void initThreadedMixer(Audio::MixerImpl *mixer, uint bufSize);
	void deinitThreadedMixer();
#endif

	FilesystemFactory *_fsFactory;
	Common::SaveFileManager *_savefile;
	Audio::MixerImpl *_mixer;

	SDL_TimerID _timerID;
	Common::TimerManager *_timer;

protected:
	void addDirtyRgnAuto(const byte *buf);
	void makeChecksums(const byte *buf);

	virtual void addDirtyRect(int x, int y, int w, int h, bool realCoordinates = false);

	void drawMouse();
	void undrawMouse();
	void blitCursor();

	/** Set the position of the virtual mouse cursor. */
	void setMousePos(int x, int y);
	void fillMouseEvent(Common::Event &event, int x, int y);
	void toggleMouseGrab();

	void internUpdateScreen();

	bool loadGFXMode();
	void unloadGFXMode();
	bool hotswapGFXMode();

	void setFullscreenMode(bool enable);
	void setAspectRatioCorrection(bool enable);

	void setZoomOnMouse(); // GP2X: On > 240 high games zooms on the mouse + radius.

	bool saveScreenshot(const char *filename);

	int effectiveScreenHeight() const {
		return (_videoMode.aspectRatioCorrection ? real2Aspect(_videoMode.screenHeight) : _videoMode.screenHeight)
			* _videoMode.scaleFactor;
	}

	void setupIcon();
	void handleKbdMouse();

	virtual bool remapKey(SDL_Event &ev, Common::Event &event);

	void handleScalerHotkeys(const SDL_KeyboardEvent &key);

	void moveStick();
	int _gp2xInputType;
};

#endif // GP2X_COMMON_H
