#
# Try to find GLEW library and include path.
# Once done this will define
#
# GLEW_FOUND
# GLEW_INCLUDE_PATH
# GLEW_LIBRARY
# 

FIND_PATH( GLEW_INCLUDE_PATH GL/glew.h
	${GLEW_ROOT}/include
	/usr/include
	/usr/local/include
	/sw/include
	/opt/local/include
	DOC "The directory where GL/glew.h resides")
FIND_LIBRARY( GLEW_LIBRARY
	NAMES GLEW glew glew32 glew32s
	PATHS
	${GLEW_ROOT}/lib
	${GLEW_ROOT}/lib/Release/Win32
	${GLEW_ROOT}/lib/Release/x64
	/usr/lib64
	/usr/lib
	/usr/local/lib64
	/usr/local/lib
	/sw/lib
	/opt/local/lib
	DOC "The GLEW library")

IF(GLEW_INCLUDE_PATH AND GLEW_LIBRARY)
	SET( GLEW_FOUND TRUE )
ENDIF()

MARK_AS_ADVANCED( GLEW_FOUND )
MARK_AS_ADVANCED( GLEW_INCLUDE_PATH )
MARK_AS_ADVANCED( GLEW_LIBRARY )
