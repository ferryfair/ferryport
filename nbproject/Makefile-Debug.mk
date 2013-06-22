#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/ClientSocket.o \
	${OBJECTDIR}/Multimedia.o \
	${OBJECTDIR}/ServerSocket.o \
	${OBJECTDIR}/Socket.o \
	${OBJECTDIR}/debug.o \
	${OBJECTDIR}/main.o \
	${OBJECTDIR}/myconverters.o \
	${OBJECTDIR}/mycurl.o \
	${OBJECTDIR}/mystdlib.o \
	${OBJECTDIR}/myxml.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-lxml2 -lpthread -ldbus-1 -ldbus-glib-1 -Wall -Wextra

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/remotedevicecontroller

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/remotedevicecontroller: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/remotedevicecontroller ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/ClientSocket.o: ClientSocket.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/include/libxml2 -I. -I/usr/include/dbus-1.0 -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/lib/i386-linux-gnu/dbus-1.0/include -MMD -MP -MF $@.d -o ${OBJECTDIR}/ClientSocket.o ClientSocket.cpp

${OBJECTDIR}/Multimedia.o: Multimedia.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/include/libxml2 -I. -I/usr/include/dbus-1.0 -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/lib/i386-linux-gnu/dbus-1.0/include -MMD -MP -MF $@.d -o ${OBJECTDIR}/Multimedia.o Multimedia.cpp

${OBJECTDIR}/ServerSocket.o: ServerSocket.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/include/libxml2 -I. -I/usr/include/dbus-1.0 -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/lib/i386-linux-gnu/dbus-1.0/include -MMD -MP -MF $@.d -o ${OBJECTDIR}/ServerSocket.o ServerSocket.cpp

${OBJECTDIR}/Socket.o: Socket.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/include/libxml2 -I. -I/usr/include/dbus-1.0 -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/lib/i386-linux-gnu/dbus-1.0/include -MMD -MP -MF $@.d -o ${OBJECTDIR}/Socket.o Socket.cpp

${OBJECTDIR}/debug.o: debug.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/include/libxml2 -I. -I/usr/include/dbus-1.0 -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/lib/i386-linux-gnu/dbus-1.0/include -MMD -MP -MF $@.d -o ${OBJECTDIR}/debug.o debug.cpp

${OBJECTDIR}/main.o: main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/include/libxml2 -I. -I/usr/include/dbus-1.0 -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/lib/i386-linux-gnu/dbus-1.0/include -MMD -MP -MF $@.d -o ${OBJECTDIR}/main.o main.cpp

${OBJECTDIR}/myconverters.o: myconverters.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/include/libxml2 -I. -I/usr/include/dbus-1.0 -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/lib/i386-linux-gnu/dbus-1.0/include -MMD -MP -MF $@.d -o ${OBJECTDIR}/myconverters.o myconverters.cpp

${OBJECTDIR}/mycurl.o: mycurl.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/include/libxml2 -I. -I/usr/include/dbus-1.0 -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/lib/i386-linux-gnu/dbus-1.0/include -MMD -MP -MF $@.d -o ${OBJECTDIR}/mycurl.o mycurl.cpp

${OBJECTDIR}/mystdlib.o: mystdlib.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/include/libxml2 -I. -I/usr/include/dbus-1.0 -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/lib/i386-linux-gnu/dbus-1.0/include -MMD -MP -MF $@.d -o ${OBJECTDIR}/mystdlib.o mystdlib.cpp

${OBJECTDIR}/myxml.o: myxml.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -I/usr/include/libxml2 -I. -I/usr/include/dbus-1.0 -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/lib/i386-linux-gnu/dbus-1.0/include -MMD -MP -MF $@.d -o ${OBJECTDIR}/myxml.o myxml.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/remotedevicecontroller

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
