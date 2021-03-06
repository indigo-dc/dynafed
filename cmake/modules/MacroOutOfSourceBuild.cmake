
SET(OUT_OF_SOURCE_CHECK TRUE
	CACHE STRING "Enable or disable the out of source check")
	
	
# Ensures that we do an out of source build
MACRO(MACRO_ENSURE_OUT_OF_SOURCE_BUILD MSG)
	IF(OUT_OF_SOURCE_CHECK)
		STRING(COMPARE EQUAL "${CMAKE_SOURCE_DIR}"
	"${CMAKE_BINARY_DIR}" insource)
		GET_FILENAME_COMPONENT(PARENTDIR ${CMAKE_SOURCE_DIR} PATH)
		STRING(COMPARE EQUAL "${CMAKE_SOURCE_DIR}"
	"${PARENTDIR}" insourcesubdir)
		IF(insource OR insourcesubdir)
			MESSAGE(FATAL_ERROR "${MSG}")
		ENDIF(insource OR insourcesubdir)
	ENDIF(OUT_OF_SOURCE_CHECK)
ENDMACRO(MACRO_ENSURE_OUT_OF_SOURCE_BUILD)
