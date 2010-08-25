# $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/tools/create_kyradat/module.mk $
# $Id: module.mk 44143 2009-09-17 12:02:17Z lordhoto $

MODULE := tools/create_kyradat

MODULE_OBJS := \
	create_kyradat.o \
	extract.o \
	games.o \
	md5.o \
	pak.o \
	search.o \
	tables.o \
	util.o

# Set the name of the executable
TOOL_EXECUTABLE := create_kyradat

# Include common rules
include $(srcdir)/rules.mk
