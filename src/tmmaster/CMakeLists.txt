cmake_minimum_required(VERSION 3.6.1)

SET(SRCS
"BanList.h"	"BanList.cpp"
"BitPacking.h"	"BitPacking.cpp"
"CfgParse.h"	"CfgParse.cpp"
"Crypt.h"	"Crypt.cpp"
"CStr.h"	"CStr.cpp"
"Heartbeat.h"	"Heartbeat.cpp"
"Networking.h"	"Networking.cpp"
"Timer.h"	"Timer.cpp"
"TMMaster.h"	"TMMaster.cpp"
"Trees.h"
)

ADD_EXECUTABLE(TMMaster ${SRCS})

IF(WIN32)
  TARGET_LINK_LIBRARIES(TMMaster winmm.lib ws2_32.lib)
ENDIF()

INSTALL(TARGETS TMMaster RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/" CONFIGURATIONS Release)
