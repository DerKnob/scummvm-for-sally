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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/backends/plugins/psp/psp-provider.cpp $
 * $Id: psp-provider.cpp 44822 2009-10-09 12:10:08Z joostp $
 *
 */

#if defined(DYNAMIC_MODULES) && defined(__PSP__)

#include "backends/plugins/psp/psp-provider.h"
#include "backends/plugins/dynamic-plugin.h"
#include "common/fs.h"

#include "backends/platform/psp/psploader.h"


class PSPPlugin : public DynamicPlugin {
protected:
	void *_dlHandle;
	Common::String _filename;

	virtual VoidFunc findSymbol(const char *symbol) {
		void *func = dlsym(_dlHandle, symbol);
		if (!func)
			warning("Failed loading symbol '%s' from plugin '%s' (%s)", symbol, _filename.c_str(), dlerror());

		// FIXME HACK: This is a HACK to circumvent a clash between the ISO C++
		// standard and POSIX: ISO C++ disallows casting between function pointers
		// and data pointers, but dlsym always returns a void pointer. For details,
		// see e.g. <http://www.trilithium.com/johan/2004/12/problem-with-dlsym/>.
		assert(sizeof(VoidFunc) == sizeof(func));
		VoidFunc tmp;
		memcpy(&tmp, &func, sizeof(VoidFunc));
		return tmp;
	}

public:
	PSPPlugin(const Common::String &filename)
		: _dlHandle(0), _filename(filename) {}

	~PSPPlugin() {
		if (_dlHandle) unloadPlugin();
	}

	bool loadPlugin() {
		assert(!_dlHandle);
		_dlHandle = dlopen(_filename.c_str(), RTLD_LAZY);

		if (!_dlHandle) {
			warning("Failed loading plugin '%s' (%s)", _filename.c_str(), dlerror());
			return false;
		}

		bool ret = DynamicPlugin::loadPlugin();

		if (ret)
			dlforgetsyms(_dlHandle);

		return ret;
	}

	void unloadPlugin() {
		DynamicPlugin::unloadPlugin();
		if (_dlHandle) {
			if (dlclose(_dlHandle) != 0)
				warning("Failed unloading plugin '%s' (%s)", _filename.c_str(), dlerror());
			_dlHandle = 0;
		}
	}
};


Plugin* PSPPluginProvider::createPlugin(const Common::FSNode &node) const {
	return new PSPPlugin(node.getPath());
}

bool PSPPluginProvider::isPluginFilename(const Common::FSNode &node) const {
	// Check the plugin suffix
	Common::String filename = node.getName();
	fprintf(stderr, "Testing name %s", filename.c_str());
	if (!filename.hasSuffix(".PLG") && !filename.hasSuffix(".plg")) {
		fprintf(stderr," fail.\n");
		return false;
	}

	fprintf(stderr," success!\n");
	return true;
}


#endif // defined(DYNAMIC_MODULES) && defined(__PSP__)
