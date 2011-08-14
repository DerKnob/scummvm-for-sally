# $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/tools/create_msvc/module.mk $
# $Id: module.mk 45447 2009-10-27 19:11:23Z lordhoto $

MODULE := tools/create_msvc

MODULE_OBJS := \
	create_msvc.o \

# Set the name of the executable
TOOL_EXECUTABLE := create_msvc

# Include common rules
include $(srcdir)/rules.mk
