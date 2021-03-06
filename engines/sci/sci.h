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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/sci/sci.h $
 * $Id: sci.h 48059 2010-02-14 12:23:22Z thebluegr $
 *
 */

#ifndef SCI_H
#define SCI_H

#include "engines/engine.h"
//#include "engines/advancedDetector.h"

struct ADGameDescription;

/**
 * This is the namespace of the SCI engine.
 *
 * Status of this engine: ???
 *
 * Supported games:
 * - ???
 */
namespace Sci {

// Uncomment this to use old music functions
//#define USE_OLD_MUSIC_FUNCTIONS

struct EngineState;
class Vocabulary;
class ResourceManager;
class Kernel;
class GameFeatures;
class Console;
class AudioPlayer;

class GfxAnimate;
class GfxCache;
class GfxCompare;
class GfxControls;
class GfxCoordAdjuster;
class GfxCursor;
class GfxMenu;
class GfxPaint;
class GfxPaint16;
class GfxPalette;
class GfxPorts;
class GfxScreen;
class SciGui;


#ifdef ENABLE_SCI32
class SciGui32;
class GfxFrameout;
#endif

// our engine debug levels
enum kDebugLevels {
	kDebugLevelError      = 1 << 0,
	kDebugLevelNodes      = 1 << 1,
	kDebugLevelGraphics   = 1 << 2,
	kDebugLevelStrings    = 1 << 3,
	kDebugLevelMemory     = 1 << 4,
	kDebugLevelFuncCheck  = 1 << 5,
	kDebugLevelBresen     = 1 << 6,
	kDebugLevelSound      = 1 << 7,
	kDebugLevelGfxDriver  = 1 << 8,
	kDebugLevelBaseSetter = 1 << 9,
	kDebugLevelParser     = 1 << 10,
	kDebugLevelMenu       = 1 << 11,
	kDebugLevelSaid       = 1 << 12,
	kDebugLevelFile       = 1 << 13,
	kDebugLevelTime       = 1 << 14,
	kDebugLevelRoom       = 1 << 15,
	kDebugLevelAvoidPath  = 1 << 16,
	kDebugLevelDclInflate = 1 << 17,
	kDebugLevelVM         = 1 << 18,
	kDebugLevelScripts    = 1 << 19,
	kDebugLevelGC         = 1 << 20,
	kDebugLevelSci0Pic    = 1 << 21,
	kDebugLevelResMan     = 1 << 22,
	kDebugLevelOnStartup  = 1 << 23
};

/** SCI versions */
enum SciVersion {
	SCI_VERSION_NONE,
	SCI_VERSION_0_EARLY, // Early KQ4, 1988 xmas card
	SCI_VERSION_0_LATE, // KQ4, LSL2, LSL3, SQ3 etc
	SCI_VERSION_01, // KQ1 and multilingual games (S.old.*)
	SCI_VERSION_1_EGA, // EGA with parser, QFG2
	SCI_VERSION_1_EARLY, // KQ5. (EGA/VGA)
	SCI_VERSION_1_MIDDLE, // LSL1, JONESCD. (EGA?/VGA)
	SCI_VERSION_1_LATE, // ECO1, LSL5. (EGA/VGA)
	SCI_VERSION_1_1, // KQ6, ECO2
	SCI_VERSION_2, // GK1, PQ4 (Floppy), QFG4 (Floppy)
	SCI_VERSION_2_1, // GK2, KQ7, SQ6, Torin
	SCI_VERSION_3 // LSL7, RAMA, Lighthouse
};

enum MoveCountType {
	kMoveCountUninitialized,
	kIgnoreMoveCount,
	kIncrementMoveCount
};

/** Supported languages */
enum kLanguage {
	K_LANG_NONE = 0,
	K_LANG_ENGLISH = 1,
	K_LANG_FRENCH = 33,
	K_LANG_SPANISH = 34,
	K_LANG_ITALIAN = 39,
	K_LANG_GERMAN = 49,
	K_LANG_JAPANESE = 81,
	K_LANG_PORTUGUESE = 351
};


class SciEngine : public Engine {
	friend class Console;
public:
	SciEngine(OSystem *syst, const ADGameDescription *desc);
	~SciEngine();

	// Engine APIs
	virtual Common::Error run();
	bool hasFeature(EngineFeature f) const;
	void pauseEngineIntern(bool pause);
	virtual GUI::Debugger *getDebugger();
	Console *getSciDebugger();
	Common::Error loadGameState(int slot);
	Common::Error saveGameState(int slot, const char *desc);
	bool canLoadGameStateCurrently();
	bool canSaveGameStateCurrently();
	void syncSoundSettings();

	const char* getGameID() const;
	int getResourceVersion() const;
	Common::Language getLanguage() const;
	Common::Platform getPlatform() const;
	uint32 getFlags() const;
	bool isDemo() const;

	inline ResourceManager *getResMan() const { return _resMan; }
	inline Kernel *getKernel() const { return _kernel; }
	inline EngineState *getEngineState() const { return _gamestate; }
	inline Vocabulary *getVocabulary() const { return _vocabulary; }

	Common::String getSavegameName(int nr) const;
	Common::String getSavegamePattern() const;

	Common::String getFilePrefix() const;

	/** Prepend 'TARGET-' to the given filename. */
	Common::String wrapFilename(const Common::String &name) const;

	/** Remove the 'TARGET-' prefix of the given filename, if present. */
	Common::String unwrapFilename(const Common::String &name) const;

public:

	/**
	 * Processes a multilanguage string based on the current language settings and
	 * returns a string that is ready to be displayed.
	 * @param str		the multilanguage string
	 * @param sep		optional seperator between main language and subtitle language,
	 *					if NULL is passed no subtitle will be added to the returned string
	 * @return processed string
	 */
	Common::String strSplit(const char *str, const char *sep = "\r----------\r");

	kLanguage getSciLanguage();

	Common::String getSciLanguageString(const char *str, kLanguage lang) const;

public:
	GfxAnimate *_gfxAnimate; // Animate for 16-bit gfx
	GfxCache *_gfxCache;
	GfxCompare *_gfxCompare;
	GfxControls *_gfxControls; // Controls for 16-bit gfx
	GfxCoordAdjuster *_gfxCoordAdjuster;
	GfxCursor *_gfxCursor;
	GfxMenu *_gfxMenu; // Menu for 16-bit gfx
	GfxPalette *_gfxPalette;
	GfxPaint *_gfxPaint;
	GfxPaint16 *_gfxPaint16; // Painting in 16-bit gfx
	GfxPorts *_gfxPorts; // Port managment for 16-bit gfx
	GfxScreen *_gfxScreen;
	SciGui *_gui; /* Currently active Gui */

#ifdef ENABLE_SCI32
	SciGui32 *_gui32; // GUI for SCI32 games
	GfxFrameout *_gfxFrameout; // kFrameout and the like for 32-bit gfx
#endif

	AudioPlayer *_audio;
	GameFeatures *_features;

private:
	const ADGameDescription *_gameDescription;
	ResourceManager *_resMan; /**< The resource manager */
	EngineState *_gamestate;
	Kernel *_kernel;
	Vocabulary *_vocabulary;
	Console *_console;
	OSystem *_system;
};


/**
 * Global instance of the SciEngine class, similar to g_engine.
 * This is a hackish way to make all central components available
 * everywhere. Ideally, we would get rid of this again in the future,
 * but for now it's a pragmatic and simple way to achieve the goal.
 */
extern SciEngine *g_sci;

/**
 * Convenience function to obtain the active SCI version.
 */
SciVersion getSciVersion();

/**
 * Convenience function converting an SCI version into a human-readable string.
 */
const char *getSciVersionDesc(SciVersion version);

} // End of namespace Sci

#endif // SCI_H
