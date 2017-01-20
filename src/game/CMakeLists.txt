cmake_minimum_required(VERSION 3.6.1)

SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

IF(NOT BUILD_HEADLESS)
  SET(GAME_SRCS
    "CJoyInput.cpp"
    "GenericBuffer.cpp"      
    "Poly.cpp"
    "Render.cpp"
    "SimpleStatus.cpp"        
    "Sound.cpp"
    "TankGUI.cpp"
  )
ELSE()
  SET(GAME_SRCS
    "CJoyInputHeadless.cpp"
    "GenericBufferHeadless.cpp"
    "PolyHeadless.cpp"
    "RenderHeadless.cpp"
    "SimpleStatusHeadless.cpp"
    "SoundHeadless.cpp"
    "TankGUIHeadless.cpp"
    "GL/TerrainStub.cpp"
  )
ENDIF(NOT BUILD_HEADLESS)

SET(GAME_SRCS 
${GAME_SRCS}
"BanList.cpp"
"BanList.h"           "Basis.cpp"            "Basis.h"
"splash.png"          "BitPacking.cpp"       "BitPacking.h"
"CamRbd.cpp"          "CamRbd.h"             "macros.h"
"CfgParse.cpp"        "CfgParse.h"
"CJoyInput.h"	  
"Compression.cpp"
"Compression.h"       "Crypt.cpp"            "Crypt.h"
"CStr.cpp"            "CStr.h"               
"Directories.h"       "EntityBase.cpp"       "EntityBase.h"
"EntityFlag.cpp"      "EntityFlag.h"         "EntityGUI.cpp"
"EntityGUI.h"         "EntityPFire.cpp"      "EntityPFire.h"
"EntityRacetank.cpp"  "EntityTank.cpp"       "EntityTank.h"
"FileCRCList.cpp"     "FileCRCList.h"        "FileManager.cpp"
"FileManager.h"       "Find.cpp"             "Find.h"
"GenericBuffer.h"   
"Heartbeat.cpp"
"Heartbeat.h"         "HelpText.cpp"         "HelpText.h"
"IFF.cpp"             "IFF.h"                "Image.cpp"
"Image.h"             "LadderManager.cpp"    "LadderManager.h"
"Mesh.cpp"
"Networking.cpp"      "Networking.h"         "neuquant.c"
"neuquant.h"          "PacketProcessors.cpp" "PacketProcessors.h"
"Physics.cpp"         "Physics.h"            "pi.cpp"
"pi.h"                "Poly.h"
"Quantizer.cpp"       "Quantizer.h"          "Reg.cpp"       
"Reg.h"               "Render.h"
"ResourceManager.cpp" "ResourceManager.h"    
"SimpleStatus.h"      "Sound.h"              "StatsPackage.cpp"
"StatsPackage.h"      "TankAI.cpp"           "TankAI.h"
"TankGame.cpp"        "TankGame.h"        
"TankGUI.h"           "TankRacing.cpp"       "TankRacing.h"
"Terrain.cpp"         "Terrain.h"            "TextLine.cpp"
"TextLine.hpp"        "Timer.cpp"            "Timer.h"
"TMMaster.h"          "Trees.h"
"TxtBlock.cpp"        "TxtBlock.hpp"         "vecmath.cpp"
"vecmath.h"           "Vector.cpp"           "Vector.h"
"version.h"
"VertexArray.h"       "VoxelAfx.h"           "VoxelWorld.cpp"
"VoxelWorld.h"        "Web.h"
)

SET(GAME_RESOURCES "resources.qrc")
SET(GAME_SRCS ${GAME_SRCS} ${GAME_RESOURCES})

SOURCE_GROUP("base" FILES ${GAME_SRCS})

IF(WIN32)
  SET(WIN32_SRCS
    "win32/Directories.cpp"
    "win32/Web.cpp"            "win32/Treadmarks.rc"
    "win32/TreadMarks.gdf.xml" )
  SET(GAME_SRCS ${GAME_SRCS} ${WIN32_SRCS})
  SOURCE_GROUP("win32" FILES ${WIN32_SRCS})
ELSE()
  SET(POSIX_SRCS
    "Posix/Directories.cpp" )

  SET(GAME_SRCS ${GAME_SRCS} ${POSIX_SRCS})
  SOURCE_GROUP("posix" FILES ${POSIX})
ENDIF()

IF(NOT BUILD_HEADLESS)
  SET(BAD_GUI_REQS "")

  FIND_PACKAGE(OpenGL)
  IF(OPENGL_FOUND)
    SET(GL_SRCS
      "GL/GLPolyRender.cpp"
      "GL/GLPolyRender.h"      "GL/GLTerrain.cpp"        "GL/GLTerrain.h"
      "GL/GLTerrainRender.cpp" "GL/GLTerrainRender.h"    "GL/GLTerrainRender2.cpp"
      "GL/GLTerrainRender2.h"  "GL/GLTerrainRender3.cpp" "GL/GLTerrainRender3.h")
    SET(GAME_SRCS ${GAME_SRCS} ${GL_SRCS})
    SOURCE_GROUP("gl" FILES ${GL_SRCS})
  ELSE()
    SET(BAD_GUI_REQS "${BAD_GUI_REQS} OpenGL")
  ENDIF()

  SET(CMAKE_INCLUDE_CURRENT_DIR ON)
  SET(CMAKE_AUTOMOC ON)

  FIND_PACKAGE(Qt5 COMPONENTS Core Gui Widgets)
  IF(${Qt5_FOUND})
    SET(QT_SRCS	"Qt/Dedicated.h"	"Qt/Dedicated.cpp")
    SET(GAME_SRCS ${GAME_SRCS} ${QT_SRCS})
    SOURCE_GROUP("qt" FILES ${QT_SRCS})
    qt5_add_resources(RCCS_SRCS ${GAME_RESOURCES})
    SET(GAME_SRCS ${GAME_SRCS} ${RCCS_SRCS})
    SOURCE_GROUP("gen" FILES ${RCCS_SRCS})
  ENDIF()
ENDIF(NOT BUILD_HEADLESS)

FIND_PACKAGE(Boost COMPONENTS system filesystem)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

IF(WIN32 AND NOT BUILD_HEADLESS)
  ADD_EXECUTABLE(TM WIN32 ${GAME_SRCS})
ELSE()
  ADD_EXECUTABLE(TM ${GAME_SRCS})
ENDIF()

IF(WIN32)
  TARGET_LINK_LIBRARIES(TM winmm.lib ws2_32.lib version.lib)
ENDIF()

IF(NOT BUILD_HEADLESS)
  IF(${Qt5_FOUND})
    INCLUDE_DIRECTORIES(${Qt5_INCLUDE_DIR})
    qt5_use_modules(TM Core Gui Widgets)
  ENDIF()

  IF(OPENGL_FOUND)
    INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
    TARGET_LINK_LIBRARIES(TM ${OPENGL_LIBRARIES})
  ENDIF()

  TARGET_LINK_LIBRARIES(TM ${QT_LIBRARIES})
ENDIF(NOT BUILD_HEADLESS)

FIND_PACKAGE(ZLIB REQUIRED)
INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(TM ${ZLIB_LIBRARIES})

IF(NOT BUILD_HEADLESS)
  IF(WIN32)
    FIND_PACKAGE(SFML 2 COMPONENTS system window graphics audio main)
  ELSE()
    FIND_PACKAGE(SFML 2 COMPONENTS system window graphics audio)
  ENDIF()
  IF(${SFML_FOUND})
    INCLUDE_DIRECTORIES(${SFML_INCLUDE_DIR})
    TARGET_LINK_LIBRARIES(TM ${SFML_LIBRARIES})
  ELSE()
    SET(BAD_GUI_REQS "${BAD_GUI_REQS} SFML")
  ENDIF()

  FIND_PACKAGE(GLEW)
  IF(${GLEW_FOUND})
    INCLUDE_DIRECTORIES(${GLEW_INCLUDE_PATH})
    TARGET_LINK_LIBRARIES(TM ${GLEW_LIBRARY})
  ELSE()
    SET(BAD_GUI_REQS "${BAD_GUI_REQS} GLEW")
  ENDIF()
ENDIF(NOT BUILD_HEADLESS)

TARGET_LINK_LIBRARIES(TM ${Boost_LIBRARIES})

MARK_AS_ADVANCED(SFML_INCLUDE_DIR)
MARK_AS_ADVANCED(QT_QMAKE_EXECUTABLE)

IF(NOT "${BAD_GUI_REQS}" STREQUAL "")
  MESSAGE(FATAL_ERROR "Libraries missing for GUI-enabled build. Remember to set the PATH_ params if using non-standard locations.  The following libraries were not found:${BAD_GUI_REQS}")
ENDIF()

SET(SEARCH_DIRS "${Qt5Core_DIR}/../../../bin/" "${SFML_INCLUDE_DIR}/../bin" "${GLEW_INCLUDE_PATH}/../bin/Release/Win32" "${ZLIB_INCLUDE_DIR}/../bin" "${Boost_LIBRARY_DIRS}")

INSTALL(TARGETS TM RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/" CONFIGURATIONS Release)

INSTALL(DIRECTORY "${CMAKE_SOURCE_DIR}/Resources/" DESTINATION "${CMAKE_INSTALL_PREFIX}/" CONFIGURATIONS Release)

INSTALL(CODE "
	include(BundleUtilities)
	fixup_bundle(\"${CMAKE_CURRENT_BINARY_DIR}/\${BUILD_TYPE}/TM${EXESUFFIX}\" \"\" \"${SEARCH_DIRS}\")
	if( \${CMAKE_INSTALL_CONFIG_NAME} STREQUAL \"Release\" )
	  fixup_bundle(\"${CMAKE_INSTALL_PREFIX}/TM${EXESUFFIX}\" \"\" \"${SEARCH_DIRS}\")
	endif()
	" COMPONENT Runtime)
