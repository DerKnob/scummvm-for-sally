# $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/tools/create_teenagent/module.mk $
# $Id: module.mk 43952 2009-09-05 07:42:49Z megath $

MODULE := tools/create_teenagent

MODULE_OBJS := \
	create_teenagent.o \
	md5.o

# Set the name of the executable
TOOL_EXECUTABLE := create_teenagent

# Include common rules
include $(srcdir)/rules.mk
