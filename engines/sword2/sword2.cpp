/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * Additional copyright for this file:
 * Copyright (C) 1994-1998 Revolution Software Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/sword2/sword2.cpp $
 * $Id: sword2.cpp 48101 2010-02-21 04:04:13Z bluddy $
 */



#include "base/plugins.h"

#include "common/config-manager.h"
#include "common/file.h"
#include "common/fs.h"
#include "common/events.h"
#include "common/EventRecorder.h"
#include "common/savefile.h"
#include "common/system.h"

#include "engines/metaengine.h"

#include "sword2/sword2.h"
#include "sword2/defs.h"
#include "sword2/header.h"
#include "sword2/console.h"
#include "sword2/controls.h"
#include "sword2/logic.h"
#include "sword2/maketext.h"
#include "sword2/memory.h"
#include "sword2/mouse.h"
#include "sword2/resman.h"
#include "sword2/router.h"
#include "sword2/screen.h"
#include "sword2/sound.h"
#include "sword2/saveload.h"

namespace Sword2 {

Common::Platform Sword2Engine::_platform;

struct GameSettings {
	const char *gameid;
	const char *description;
	uint32 features;
	const char *detectname;
};

static const GameSettings sword2_settings[] = {
	/* Broken Sword II */
	{"sword2", "Broken Sword II: The Smoking Mirror", 0, "players.clu" },
	{"sword2alt", "Broken Sword II: The Smoking Mirror (alt)", 0, "r2ctlns.ocx" },
	{"sword2psx", "Broken Sword II: The Smoking Mirror (PlayStation)", 0, "screens.clu"},
	{"sword2psxdemo", "Broken Sword II: The Smoking Mirror (PlayStation/Demo)", Sword2::GF_DEMO, "screens.clu"},
	{"sword2demo", "Broken Sword II: The Smoking Mirror (Demo)", Sword2::GF_DEMO, "players.clu" },
	{NULL, NULL, 0, NULL}
};

} // End of namespace Sword2

class Sword2MetaEngine : public MetaEngine {
public:
	virtual const char *getName() const {
		return "Broken Sword II";
	}
	virtual const char *getOriginalCopyright() const {
		return "Broken Sword Games (C) Revolution";
	}

	virtual bool hasFeature(MetaEngineFeature f) const;
	virtual GameList getSupportedGames() const;
	virtual GameDescriptor findGame(const char *gameid) const;
	virtual GameList detectGames(const Common::FSList &fslist) const;
	virtual SaveStateList listSaves(const char *target) const;
	virtual int getMaximumSaveSlot() const;
	virtual void removeSaveState(const char *target, int slot) const;

	virtual Common::Error createInstance(OSystem *syst, Engine **engine) const;
};

bool Sword2MetaEngine::hasFeature(MetaEngineFeature f) const {
	return
		(f == kSupportsListSaves) ||
		(f == kSupportsLoadingDuringStartup) ||
		(f == kSupportsDeleteSave);
}

bool Sword2::Sword2Engine::hasFeature(EngineFeature f) const {
	return
		(f == kSupportsRTL) ||
		(f == kSupportsSubtitleOptions) ||
		(f == kSupportsSavingDuringRuntime) ||
		(f == kSupportsLoadingDuringRuntime);
}

GameList Sword2MetaEngine::getSupportedGames() const {
	const Sword2::GameSettings *g = Sword2::sword2_settings;
	GameList games;
	while (g->gameid) {
		games.push_back(GameDescriptor(g->gameid, g->description));
		g++;
	}
	return games;
}

GameDescriptor Sword2MetaEngine::findGame(const char *gameid) const {
	const Sword2::GameSettings *g = Sword2::sword2_settings;
	while (g->gameid) {
		if (0 == scumm_stricmp(gameid, g->gameid))
			break;
		g++;
	}
	return GameDescriptor(g->gameid, g->description);
}

GameList Sword2MetaEngine::detectGames(const Common::FSList &fslist) const {
	GameList detectedGames;
	const Sword2::GameSettings *g;
	Common::FSList::const_iterator file;

	// TODO: It would be nice if we had code here which distinguishes
	// between the 'sword2' and 'sword2demo' targets. The current code
	// can't do that since they use the same detectname.

	for (g = Sword2::sword2_settings; g->gameid; ++g) {
		// Iterate over all files in the given directory
		for (file = fslist.begin(); file != fslist.end(); ++file) {
			if (!file->isDirectory()) {
				const char *fileName = file->getName().c_str();

				if (0 == scumm_stricmp(g->detectname, fileName)) {
					// Match found, add to list of candidates, then abort inner loop.
					detectedGames.push_back(GameDescriptor(g->gameid, g->description, Common::UNK_LANG, Common::kPlatformUnknown, Common::GUIO_NOMIDI));
					break;
				}
			}
		}
	}


	if (detectedGames.empty()) {
		// Nothing found -- try to recurse into the 'clusters' subdirectory,
		// present e.g. if the user copied the data straight from CD.
		for (file = fslist.begin(); file != fslist.end(); ++file) {
			if (file->isDirectory()) {
				const char *fileName = file->getName().c_str();

				if (0 == scumm_stricmp("clusters", fileName)) {
					Common::FSList recList;
					if (file->getChildren(recList, Common::FSNode::kListAll)) {
						GameList recGames(detectGames(recList));
						if (!recGames.empty()) {
							detectedGames.push_back(recGames);
							break;
						}
					}
				}
			}
		}
	}


	return detectedGames;
}

SaveStateList Sword2MetaEngine::listSaves(const char *target) const {
	Common::SaveFileManager *saveFileMan = g_system->getSavefileManager();
	Common::StringList filenames;
	char saveDesc[SAVE_DESCRIPTION_LEN];
	Common::String pattern = target;
	pattern += ".???";

	filenames = saveFileMan->listSavefiles(pattern);
	sort(filenames.begin(), filenames.end());	// Sort (hopefully ensuring we are sorted numerically..)

	SaveStateList saveList;
	for (Common::StringList::const_iterator file = filenames.begin(); file != filenames.end(); ++file) {
		// Obtain the last 3 digits of the filename, since they correspond to the save slot
		int slotNum = atoi(file->c_str() + file->size() - 3);

		if (slotNum >= 0 && slotNum <= 999) {
			Common::InSaveFile *in = saveFileMan->openForLoading(*file);
			if (in) {
				in->readUint32LE();
				in->read(saveDesc, SAVE_DESCRIPTION_LEN);
				saveList.push_back(SaveStateDescriptor(slotNum, saveDesc));
				delete in;
			}
		}
	}

	return saveList;
}

int Sword2MetaEngine::getMaximumSaveSlot() const { return 999; }

void Sword2MetaEngine::removeSaveState(const char *target, int slot) const {
	char extension[6];
	snprintf(extension, sizeof(extension), ".%03d", slot);

	Common::String filename = target;
	filename += extension;

	g_system->getSavefileManager()->removeSavefile(filename);
}

Common::Error Sword2MetaEngine::createInstance(OSystem *syst, Engine **engine) const {
	assert(syst);
	assert(engine);

	Common::FSList fslist;
	Common::FSNode dir(ConfMan.get("path"));
	if (!dir.getChildren(fslist, Common::FSNode::kListAll)) {
		return Common::kNoGameDataFoundError;
	}

	// Invoke the detector
	Common::String gameid = ConfMan.get("gameid");
	GameList detectedGames = detectGames(fslist);

	for (uint i = 0; i < detectedGames.size(); i++) {
		if (detectedGames[i].gameid() == gameid) {
			*engine = new Sword2::Sword2Engine(syst);
			return Common::kNoError;
		}
	}

	return Common::kNoGameDataFoundError;
}

#if PLUGIN_ENABLED_DYNAMIC(SWORD2)
	REGISTER_PLUGIN_DYNAMIC(SWORD2, PLUGIN_TYPE_ENGINE, Sword2MetaEngine);
#else
	REGISTER_PLUGIN_STATIC(SWORD2, PLUGIN_TYPE_ENGINE, Sword2MetaEngine);
#endif

namespace Sword2 {

Sword2Engine::Sword2Engine(OSystem *syst) : Engine(syst) {
	// Add default file directories
	SearchMan.addSubDirectoryMatching(_gameDataDir, "clusters");
	SearchMan.addSubDirectoryMatching(_gameDataDir, "sword2");
	SearchMan.addSubDirectoryMatching(_gameDataDir, "video");
	SearchMan.addSubDirectoryMatching(_gameDataDir, "smacks");

	if (!scumm_stricmp(ConfMan.get("gameid").c_str(), "sword2demo") || !scumm_stricmp(ConfMan.get("gameid").c_str(), "sword2psxdemo"))
		_features = GF_DEMO;
	else
		_features = 0;

	// Check if we are running PC or PSX version.
	if (!scumm_stricmp(ConfMan.get("gameid").c_str(), "sword2psx") || !scumm_stricmp(ConfMan.get("gameid").c_str(), "sword2psxdemo"))
		Sword2Engine::_platform = Common::kPlatformPSX;
	else
		Sword2Engine::_platform = Common::kPlatformPC;

	_bootParam = ConfMan.getInt("boot_param");
	_saveSlot = ConfMan.getInt("save_slot");

	_memory = NULL;
	_resman = NULL;
	_sound = NULL;
	_screen = NULL;
	_mouse = NULL;
	_logic = NULL;
	_fontRenderer = NULL;
	_debugger = NULL;

	_keyboardEvent.pending = false;
	_mouseEvent.pending = false;

	_wantSfxDebug = false;

#ifdef SWORD2_DEBUG
	_stepOneCycle = false;
	_renderSkip = false;
#endif

	_gamePaused = false;

	_gameCycle = 0;
	_gameSpeed = 1;

	_gmmLoadSlot = -1; // Used to manage GMM Loading

	g_eventRec.registerRandomSource(_rnd, "sword2");
}

Sword2Engine::~Sword2Engine() {
	delete _debugger;
	delete _sound;
	delete _fontRenderer;
	delete _screen;
	delete _mouse;
	delete _logic;
	delete _resman;
	delete _memory;
}

GUI::Debugger *Sword2Engine::getDebugger() {
	return _debugger;
}

void Sword2Engine::registerDefaultSettings() {
	ConfMan.registerDefault("gfx_details", 2);
	ConfMan.registerDefault("reverse_stereo", false);
}

void Sword2Engine::syncSoundSettings() {
	// Sound settings. At the time of writing, not all of these can be set
	// by the global options dialog, but it seems silly to split them up.
	_mixer->setVolumeForSoundType(Audio::Mixer::kMusicSoundType, ConfMan.getInt("music_volume"));
	_mixer->setVolumeForSoundType(Audio::Mixer::kSpeechSoundType, ConfMan.getInt("speech_volume"));
	_mixer->setVolumeForSoundType(Audio::Mixer::kSFXSoundType, ConfMan.getInt("sfx_volume"));
	setSubtitles(ConfMan.getBool("subtitles"));
	_sound->muteMusic(ConfMan.getBool("music_mute"));
	_sound->muteSpeech(ConfMan.getBool("speech_mute"));
	_sound->muteFx(ConfMan.getBool("sfx_mute"));
	_sound->setReverseStereo(ConfMan.getBool("reverse_stereo"));
}

void Sword2Engine::readSettings() {
	syncSoundSettings();
	_mouse->setObjectLabels(ConfMan.getBool("object_labels"));
	_screen->setRenderLevel(ConfMan.getInt("gfx_details"));
}

void Sword2Engine::writeSettings() {
	ConfMan.setInt("music_volume", _mixer->getVolumeForSoundType(Audio::Mixer::kMusicSoundType));
	ConfMan.setInt("speech_volume", _mixer->getVolumeForSoundType(Audio::Mixer::kSpeechSoundType));
	ConfMan.setInt("sfx_volume", _mixer->getVolumeForSoundType(Audio::Mixer::kSFXSoundType));
	ConfMan.setBool("music_mute", _sound->isMusicMute());
	ConfMan.setBool("speech_mute", _sound->isSpeechMute());
	ConfMan.setBool("sfx_mute", _sound->isFxMute());
	ConfMan.setInt("gfx_details", _screen->getRenderLevel());
	ConfMan.setBool("subtitles", getSubtitles());
	ConfMan.setBool("object_labels", _mouse->getObjectLabels());
	ConfMan.setInt("reverse_stereo", _sound->isReverseStereo());

	ConfMan.flushToDisk();
}

int Sword2Engine::getFramesPerSecond() {
	return _gameSpeed * FRAMES_PER_SECOND;
}

/**
 * The global script variables and player object should be kept open throughout
 * the game, so that they are never expelled by the resource manager.
 */

void Sword2Engine::setupPersistentResources() {
	_logic->_scriptVars = _resman->openResource(1) + ResHeader::size();
	_resman->openResource(CUR_PLAYER_ID);
}

Common::Error Sword2Engine::run() {
	// Get some falling RAM and put it in your pocket, never let it slip
	// away

	_debugger = NULL;
	_sound = NULL;
	_fontRenderer = NULL;
	_screen = NULL;
	_mouse = NULL;
	_logic = NULL;
	_resman = NULL;
	_memory = NULL;

	initGraphics(640, 480, true);
	_screen = new Screen(this, 640, 480);

	// Create the debugger as early as possible (but not before the
	// screen object!) so that errors can be displayed in it. In
	// particular, we want errors about missing files to be clearly
	// visible to the user.

	_debugger = new Debugger(this);

	_memory = new MemoryManager(this);
	_resman = new ResourceManager(this);

	if (!_resman->init())
		return Common::kUnknownError;

	_logic = new Logic(this);
	_fontRenderer = new FontRenderer(this);
	_sound = new Sound(this);
	_mouse = new Mouse(this);

	registerDefaultSettings();
	readSettings();

	initStartMenu();

	// During normal gameplay, we care neither about mouse button releases
	// nor the scroll wheel.
	setInputEventFilter(RD_LEFTBUTTONUP | RD_RIGHTBUTTONUP | RD_WHEELUP | RD_WHEELDOWN);

	setupPersistentResources();
	initialiseFontResourceFlags();

	if (_features & GF_DEMO)
		_logic->writeVar(DEMO, 1);
	else
		_logic->writeVar(DEMO, 0);

	if (_saveSlot != -1) {
		if (saveExists(_saveSlot))
			restoreGame(_saveSlot);
		else {
			RestoreDialog dialog(this);
			if (!dialog.runModal())
				startGame();
		}
	} else if (!_bootParam && saveExists() && !isPsx()) { // Initial load/restart panel disabled in PSX
		int32 pars[2] = { 221, FX_LOOP };                 // version because of missing panel resources
		bool result;

		_mouse->setMouse(NORMAL_MOUSE_ID);
		_logic->fnPlayMusic(pars);

		StartDialog dialog(this);

		result = (dialog.runModal() != 0);

		// If the game is started from the beginning, the cutscene
		// player will kill the music for us. Otherwise, the restore
		// will either have killed the music, or done a crossfade.

		if (shouldQuit())
			return Common::kNoError;

		if (result)
			startGame();
	} else
		startGame();

	_screen->initialiseRenderCycle();

	while (1) {
		if (_debugger->isAttached())
			_debugger->onFrame();

#ifdef SWORD2_DEBUG
		if (_stepOneCycle) {
			pauseEngine(true);
			_stepOneCycle = false;
		}
#endif

		// Handle GMM Loading
		if (_gmmLoadSlot != -1) {

			// Hide mouse cursor and fade screen
			_mouse->hideMouse();
			_screen->fadeDown();

			// Clean up and load game
			_logic->_router->freeAllRouteMem();

			// TODO: manage error handling
			restoreGame(_gmmLoadSlot);

			// Reset load slot
			_gmmLoadSlot = -1;

			// Show mouse
			_mouse->addHuman();
		}

		KeyboardEvent *ke = keyboardEvent();

		if (ke) {
			if ((ke->kbd.hasFlags(Common::KBD_CTRL) && ke->kbd.keycode == Common::KEYCODE_d) || ke->kbd.ascii == '#' || ke->kbd.ascii == '~') {
				_debugger->attach();
			} else if (ke->kbd.hasFlags(0) || ke->kbd.hasFlags(Common::KBD_SHIFT)) {
				switch (ke->kbd.keycode) {
				case Common::KEYCODE_p:
					if (_gamePaused)
						pauseEngine(false);
					else
						pauseEngine(true);
					break;
#if 0
				// Disabled because of strange rumors about the
				// credits running spontaneously every few
				// minutes.
				case Common::KEYCODE_c:
					if (!_logic->readVar(DEMO) && !_mouse->isChoosing()) {
						ScreenInfo *screenInfo = _screen->getScreenInfo();
						_logic->fnPlayCredits(NULL);
						screenInfo->new_palette = 99;
					}
					break;
#endif
#ifdef SWORD2_DEBUG
				case Common::KEYCODE_SPACE:
					if (_gamePaused) {
						_stepOneCycle = true;
						pauseEngine(false);
					}
					break;
				case Common::KEYCODE_s:
					_renderSkip = !_renderSkip;
					break;
#endif
				default:
					break;
				}
			}
		}

		// skip GameCycle if we're paused
		if (!_gamePaused) {
			_gameCycle++;
			gameCycle();
		}

		// We can't use this as termination condition for the loop,
		// because we want the break to happen before updating the
		// screen again.

		if (shouldQuit())
			break;

		// creates the debug text blocks
		_debugger->buildDebugText();

#ifdef SWORD2_DEBUG
		// if not in console & '_renderSkip' is set, only render
		// display once every 4 game-cycles

		if (!_renderSkip || (_gameCycle % 4) == 0)
			_screen->buildDisplay();
#else
		_screen->buildDisplay();
#endif
	}

	return Common::kNoError;
}

void Sword2Engine::restartGame() {
	ScreenInfo *screenInfo = _screen->getScreenInfo();
	uint32 temp_demo_flag;

	_mouse->closeMenuImmediately();

	// Restart the game. To do this, we must...

	// Stop music instantly!
	_sound->stopMusic(true);

	// In case we were dead - well we're not anymore!
	_logic->writeVar(DEAD, 0);

	// Restart the game. Clear all memory and reset the globals
	temp_demo_flag = _logic->readVar(DEMO);

	// Remove all resources from memory, including player object and
	// global variables
	_resman->removeAll();

	// Reopen global variables resource and player object
	setupPersistentResources();

	_logic->writeVar(DEMO, temp_demo_flag);

	// Free all the route memory blocks from previous game
	_logic->_router->freeAllRouteMem();

	// Call the same function that first started us up
	startGame();

	// Prime system with a game cycle

	// Reset the graphic 'BuildUnit' list before a new logic list
	// (see fnRegisterFrame)
	_screen->resetRenderLists();

	// Reset the mouse hot-spot list (see fnRegisterMouse and
	// fnRegisterFrame)
	_mouse->resetMouseList();

	_mouse->closeMenuImmediately();

	// FOR THE DEMO - FORCE THE SCROLLING TO BE RESET!
	// - this is taken from fnInitBackground
	// switch on scrolling (2 means first time on screen)
	screenInfo->scroll_flag = 2;

	if (_logic->processSession())
		error("restart 1st cycle failed??");

	// So palette not restored immediately after control panel - we want
	// to fade up instead!
	screenInfo->new_palette = 99;
}

bool Sword2Engine::checkForMouseEvents() {
	return _mouseEvent.pending;
}

MouseEvent *Sword2Engine::mouseEvent() {
	if (!_mouseEvent.pending)
		return NULL;

	_mouseEvent.pending = false;
	return &_mouseEvent;
}

KeyboardEvent *Sword2Engine::keyboardEvent() {
	if (!_keyboardEvent.pending)
		return NULL;

	_keyboardEvent.pending = false;
	return &_keyboardEvent;
}

uint32 Sword2Engine::setInputEventFilter(uint32 filter) {
	uint32 oldFilter = _inputEventFilter;

	_inputEventFilter = filter;
	return oldFilter;
}

/**
 * OSystem Event Handler. Full of cross platform goodness and 99% fat free!
 */

void Sword2Engine::parseInputEvents() {
	Common::Event event;

	while (_eventMan->pollEvent(event)) {
		switch (event.type) {
		case Common::EVENT_KEYDOWN:
			if (event.kbd.hasFlags(Common::KBD_CTRL)) {
				if (event.kbd.keycode == Common::KEYCODE_f) {
					if (_gameSpeed == 1)
						_gameSpeed = 2;
					else
						_gameSpeed = 1;
				}
			}
			if (!(_inputEventFilter & RD_KEYDOWN)) {
				_keyboardEvent.pending = true;
				_keyboardEvent.kbd = event.kbd;
			}
			break;
		case Common::EVENT_LBUTTONDOWN:
			if (!(_inputEventFilter & RD_LEFTBUTTONDOWN)) {
				_mouseEvent.pending = true;
				_mouseEvent.buttons = RD_LEFTBUTTONDOWN;
			}
			break;
		case Common::EVENT_RBUTTONDOWN:
			if (!(_inputEventFilter & RD_RIGHTBUTTONDOWN)) {
				_mouseEvent.pending = true;
				_mouseEvent.buttons = RD_RIGHTBUTTONDOWN;
			}
			break;
		case Common::EVENT_LBUTTONUP:
			if (!(_inputEventFilter & RD_LEFTBUTTONUP)) {
				_mouseEvent.pending = true;
				_mouseEvent.buttons = RD_LEFTBUTTONUP;
			}
			break;
		case Common::EVENT_RBUTTONUP:
			if (!(_inputEventFilter & RD_RIGHTBUTTONUP)) {
				_mouseEvent.pending = true;
				_mouseEvent.buttons = RD_RIGHTBUTTONUP;
			}
			break;
		case Common::EVENT_WHEELUP:
			if (!(_inputEventFilter & RD_WHEELUP)) {
				_mouseEvent.pending = true;
				_mouseEvent.buttons = RD_WHEELUP;
			}
			break;
		case Common::EVENT_WHEELDOWN:
			if (!(_inputEventFilter & RD_WHEELDOWN)) {
				_mouseEvent.pending = true;
				_mouseEvent.buttons = RD_WHEELDOWN;
			}
			break;
		default:
			break;
		}
	}
}

void Sword2Engine::gameCycle() {
	// Do one game cycle, that is run the logic session until a full loop
	// has been performed.

	if (_logic->getRunList()) {
		do {
			// Reset the 'BuildUnit' and mouse hot-spot lists
			// before each new logic list. The service scripts
			// will fill thrm through fnRegisterFrame() and
			// fnRegisterMouse().

			_screen->resetRenderLists();
			_mouse->resetMouseList();

			// Keep going as long as new lists keep getting put in
			// - i.e. screen changes.
		} while (_logic->processSession());
	} else {
		// Start the console and print the start options perhaps?
		_debugger->attach("AWAITING START COMMAND: (Enter 's 1' then 'q' to start from beginning)");
	}

	// If this screen is wide, recompute the scroll offsets every cycle
	ScreenInfo *screenInfo = _screen->getScreenInfo();

	if (screenInfo->scroll_flag)
		_screen->setScrolling();

	_mouse->mouseEngine();
	_sound->processFxQueue();
}

void Sword2Engine::startGame() {
	// Boot the game straight into a start script. It's always George's
	// script #1, but with different ScreenManager objects depending on
	// if it's the demo or the full game, or if we're using a boot param.

	int screen_manager_id = 0;

	debug(5, "startGame() STARTING:");

	if (!_bootParam) {
		if (_logic->readVar(DEMO))
			screen_manager_id = 19;		// DOCKS SECTION START
		else
			screen_manager_id = 949;	// INTRO & PARIS START
	} else {
		// FIXME this could be validated against startup.inf for valid
		// numbers to stop people shooting themselves in the foot

		if (_bootParam != 0)
			screen_manager_id = _bootParam;
	}

	_logic->runResObjScript(screen_manager_id, CUR_PLAYER_ID, 1);
}

// FIXME: Move this to some better place?

void Sword2Engine::sleepUntil(uint32 time) {
	while (getMillis() < time) {
		// Make sure menu animations and fades don't suffer, but don't
		// redraw the entire scene.
		_mouse->processMenu();
		_screen->updateDisplay(false);
		_system->delayMillis(10);
	}
}

void Sword2Engine::pauseEngine(bool pause) {
	if (pause == _gamePaused)
		return;

	// We don't need to hide the cursor for outside pausing. Not as long
	// as it replaces the cursor with the GUI cursor, at least.

	_mouse->pauseEngine(pause);
	pauseEngineIntern(pause);

	if (pause) {
#ifdef SWORD2_DEBUG
		// Don't dim it if we're single-stepping through frames
		// dim the palette during the pause

		if (!_stepOneCycle)
			_screen->dimPalette(true);
#else
		_screen->dimPalette(true);
#endif
	} else {
		_screen->dimPalette(false);

		// If mouse is about or we're in a chooser menu
		if (!_mouse->getMouseStatus() || _mouse->isChoosing())
			_mouse->setMouse(NORMAL_MOUSE_ID);
	}
}

void Sword2Engine::pauseEngineIntern(bool pause) {
	if (pause == _gamePaused)
		return;

	if (pause) {
		_sound->pauseAllSound();
		_logic->pauseMovie(true);
		_screen->pauseScreen(true);
		_gamePaused = true;
	} else {
		_logic->pauseMovie(false);
		_screen->pauseScreen(false);
		_sound->unpauseAllSound();
		_gamePaused = false;
	}
}

uint32 Sword2Engine::getMillis() {
	return _system->getMillis();
}

Common::Error Sword2Engine::saveGameState(int slot, const char *desc) {
	uint32 saveVal = saveGame(slot, (const byte *)desc);

	if (saveVal == SR_OK)
		return Common::kNoError;
	else if (saveVal == SR_ERR_WRITEFAIL || saveVal == SR_ERR_FILEOPEN)
		return Common::kWritingFailed;
	else
		return Common::kUnknownError;
}

bool Sword2Engine::canSaveGameStateCurrently() {
	bool canSave = true;

	// No save if dead
	if (_logic->readVar(DEAD))
		canSave = false;

	// No save if mouse not shown
	else if (_mouse->getMouseStatus())
		canSave = false;
	// No save if inside a menu
	else if (_mouse->getMouseMode() == MOUSE_system_menu)
		canSave = false;

	// No save if fading
	else if (_screen->getFadeStatus())
		canSave = false;

	return canSave;
}

Common::Error Sword2Engine::loadGameState(int slot) {

	// Prepare the game to load through GMM
	_gmmLoadSlot = slot;

	// TODO: error handling.
	return Common::kNoError;
}

bool Sword2Engine::canLoadGameStateCurrently() {
	bool canLoad = true;

	// No load if mouse is disabled
	if (_mouse->getMouseStatus())
		canLoad = false;
	// No load if mouse is in system menu
	else if (_mouse->getMouseMode() == MOUSE_system_menu)
		canLoad = false;
	// No load if we are fading
	else if (_screen->getFadeStatus())
		canLoad = false;

	// But if we are dead, ignore previous conditions
	if (_logic->readVar(DEAD))
		canLoad = true;

	return canLoad;
}

} // End of namespace Sword2
