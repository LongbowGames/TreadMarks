cmake_minimum_required(VERSION 3.6.1)

SET(SRCS
"Launcher.cpp"
"resource.h"		"Launcher.rc"
"../game/Reg.h"		"../game/Reg.cpp"
"../game/CStr.h"	"../game/CStr.cpp"
"../game/Directories.h"	"../game/win32/Directories.cpp"
)

ADD_EXECUTABLE(TreadMarks WIN32 ${SRCS})

IF(MSVC)
  SET_PROPERTY(TARGET TreadMarks APPEND PROPERTY LINK_FLAGS /DEBUG)
ENDIF()

INSTALL(TARGETS TreadMarks RUNTIME DESTINATION "/")
