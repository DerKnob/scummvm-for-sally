# $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/tools/qtable/module.mk $
# $Id: module.mk 48148 2010-02-27 17:02:58Z jvprat $

MODULE := tools/qtable

MODULE_OBJS := \
	qtable.o

# Set the name of the executable
TOOL_EXECUTABLE := qtable

# Include common rules
include $(srcdir)/rules.mk
