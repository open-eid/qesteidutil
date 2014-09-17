set( BREAKPAD "https://cr.eesti.ee/" CACHE STRING "URL for breakpad crash reporting, empty for disabled" )
if( BREAKPAD )
	set( TESTING NO CACHE BOOL "Enable -crash parameter" )
	if( TESTING OR "$ENV{ENABLE_CRASH}" )
		add_definitions( -DTESTING )
	endif()
	add_definitions( -DBREAKPAD="${BREAKPAD}" )
	add_subdirectory( breakpad )
	set( ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES} qbreakpad )
	find_program( DUMP_SYMS_EXECUTABLE dump_syms DOC "Path to dump_syms executable." )
endif()

macro( dump_syms PROGNAME )
	if( DUMP_SYMS_EXECUTABLE )
		add_custom_command(TARGET ${PROGNAME} POST_BUILD
			COMMAND ${DUMP_SYMS_EXECUTABLE} $<TARGET_FILE:${PROGNAME}> > ${PROGNAME}.sym
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		)
	endif()
endmacro()
