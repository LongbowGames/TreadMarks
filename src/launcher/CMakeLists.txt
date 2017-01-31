cmake_minimum_required(VERSION 3.6.1)

SET(SRCS
"Launcher.cpp"
"resource.h"		"Launcher.rc"
"Reg.h"			"Reg.cpp"
"../game/CStr.h"	"../game/CStr.cpp"
"../game/Directories.h"	"../game/win32/Directories.cpp"
)

FIND_PACKAGE(Boost COMPONENTS system filesystem)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

ADD_EXECUTABLE(TreadMarks WIN32 ${SRCS})

TARGET_LINK_LIBRARIES(TreadMarks ${Boost_LIBRARIES})

INSTALL(TARGETS TreadMarks RUNTIME DESTINATION "/")

SET(SEARCH_DIRS "${Boost_LIBRARY_DIRS}")
INSTALL(CODE "
	include(BundleUtilities)
	fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/TreadMarks${CMAKE_EXECUTABLE_SUFFIX}\" \"\" \"${SEARCH_DIRS}\")
	" COMPONENT Runtime)
