if( UNIX AND NOT APPLE )
	find_package( Threads )
endif()

set( TS_FILES
	translations/breakpad_en.ts
	translations/breakpad_et.ts
	translations/breakpad_ru.ts
)
configure_file( translations/breakpad.qrc breakpad.qrc COPYONLY )
set( RC_FILES ${CMAKE_CURRENT_BINARY_DIR}/breakpad.qrc )

if( QT_VERSION_MAJOR EQUAL 4 )
	qt4_add_translation( QM_FILES ${TS_FILES} )
	qt4_add_resources( RC_SOURCES ${RC_FILES} )
else()
	qt5_add_translation( QM_FILES ${TS_FILES} )
	qt5_add_resources( RC_SOURCES ${RC_FILES} )
endif()

set( SOURCES QBreakPad.cpp )
set( BREAKPAD_PATH google-breakpad/src )
# http://google-breakpad.googlecode.com/svn/trunk@r1279
# svn merge -r 960:1177 http://google-breakpad.googlecode.com/svn/trunk/ .
# svn merge -r 1177:1279 http://google-breakpad.googlecode.com/svn/trunk/ .
include_directories( ${BREAKPAD_PATH} ${CMAKE_SOURCE_DIR} )

if( UNIX )
	set( SOURCES ${SOURCES}
		${BREAKPAD_PATH}/client/minidump_file_writer.cc
		${BREAKPAD_PATH}/common/string_conversion.cc
		${BREAKPAD_PATH}/common/convert_UTF.c
		${BREAKPAD_PATH}/common/md5.cc
	)
	add_definitions( -D_FILE_OFFSET_BITS=64 )
	set( COMMON_FLAGS "-fno-exceptions -Wno-unused-parameter -Wno-missing-field-initializers" )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}" )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -fno-rtti -fno-threadsafe-statics" )
if( APPLE )
	set( SOURCES ${SOURCES}
		${BREAKPAD_PATH}/client/mac/handler/exception_handler.cc
		${BREAKPAD_PATH}/client/mac/handler/minidump_generator.cc
		${BREAKPAD_PATH}/client/mac/handler/dynamic_images.cc
		${BREAKPAD_PATH}/client/mac/handler/breakpad_nlist_64.cc
		${BREAKPAD_PATH}/client/mac/crash_generation/crash_generation_client.cc
		${BREAKPAD_PATH}/common/mac/string_utilities.cc
		${BREAKPAD_PATH}/common/mac/file_id.cc
		${BREAKPAD_PATH}/common/mac/macho_id.cc
		${BREAKPAD_PATH}/common/mac/macho_utilities.cc
		${BREAKPAD_PATH}/common/mac/macho_walker.cc
		${BREAKPAD_PATH}/common/mac/bootstrap_compat.cc
		${BREAKPAD_PATH}/common/mac/MachIPC.mm
	)
else()
	set( SOURCES ${SOURCES}
		${BREAKPAD_PATH}/client/linux/handler/exception_handler.cc
		${BREAKPAD_PATH}/client/linux/handler/minidump_descriptor.cc
		${BREAKPAD_PATH}/client/linux/log/log.cc
		${BREAKPAD_PATH}/client/linux/crash_generation/crash_generation_client.cc
		${BREAKPAD_PATH}/client/linux/minidump_writer/minidump_writer.cc
		${BREAKPAD_PATH}/client/linux/minidump_writer/linux_ptrace_dumper.cc
		${BREAKPAD_PATH}/client/linux/minidump_writer/linux_dumper.cc
		${BREAKPAD_PATH}/common/linux/elfutils.cc
		${BREAKPAD_PATH}/common/linux/linux_libc_support.cc
		${BREAKPAD_PATH}/common/linux/memory_mapped_file.cc
		${BREAKPAD_PATH}/common/linux/safe_readlink.cc
		${BREAKPAD_PATH}/common/linux/guid_creator.cc
		${BREAKPAD_PATH}/common/linux/file_id.cc
	)
	string(REGEX REPLACE "-std=c\\+\\+0x|-std=c\\+\\+11" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endif()
endif()
if( WIN32 )
	set( SOURCES ${SOURCES}
		${BREAKPAD_PATH}/client/windows/handler/exception_handler.cc
		${BREAKPAD_PATH}/client/windows/crash_generation/crash_generation_client.cc
		${BREAKPAD_PATH}/common/windows/guid_string.cc
	)
	add_definitions( -DUNICODE
		-D_CRT_SECURE_NO_DEPRECATE
		-D_CRT_NONSTDC_NO_WARNINGS
		-D_CRT_NONSTDC_NO_DEPRECATE
		-DWIN32_LEAN_AND_MEAN
	)
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4800" )
endif()

set( SOURCES ${SOURCES}
	${BREAKPAD_PATH}/processor/basic_code_modules.cc
	${BREAKPAD_PATH}/processor/basic_source_line_resolver.cc
	${BREAKPAD_PATH}/processor/call_stack.cc
	${BREAKPAD_PATH}/processor/cfi_frame_info.cc
	${BREAKPAD_PATH}/processor/disassembler_x86.cc
	${BREAKPAD_PATH}/processor/exploitability.cc
	${BREAKPAD_PATH}/processor/exploitability_linux.cc
	${BREAKPAD_PATH}/processor/exploitability_win.cc
	${BREAKPAD_PATH}/processor/logging.cc
	${BREAKPAD_PATH}/processor/minidump.cc
	${BREAKPAD_PATH}/processor/minidump_processor.cc
	${BREAKPAD_PATH}/processor/pathname_stripper.cc
	${BREAKPAD_PATH}/processor/process_state.cc
	${BREAKPAD_PATH}/processor/source_line_resolver_base.cc
	${BREAKPAD_PATH}/processor/stack_frame_symbolizer.cc
	${BREAKPAD_PATH}/processor/stackwalker.cc
	${BREAKPAD_PATH}/processor/stackwalker_amd64.cc
	${BREAKPAD_PATH}/processor/stackwalker_arm.cc
	${BREAKPAD_PATH}/processor/stackwalker_arm64.cc
	${BREAKPAD_PATH}/processor/stackwalker_mips.cc
	${BREAKPAD_PATH}/processor/stackwalker_ppc.cc
	${BREAKPAD_PATH}/processor/stackwalker_ppc64.cc
	${BREAKPAD_PATH}/processor/stackwalker_sparc.cc
	${BREAKPAD_PATH}/processor/stackwalker_x86.cc
	${BREAKPAD_PATH}/processor/tokenize.cc
	${BREAKPAD_PATH}/third_party/libdisasm/ia32_implicit.c
	${BREAKPAD_PATH}/third_party/libdisasm/ia32_insn.c
	${BREAKPAD_PATH}/third_party/libdisasm/ia32_invariant.c
	${BREAKPAD_PATH}/third_party/libdisasm/ia32_modrm.c
	${BREAKPAD_PATH}/third_party/libdisasm/ia32_opcode_tables.c
	${BREAKPAD_PATH}/third_party/libdisasm/ia32_operand.c
	${BREAKPAD_PATH}/third_party/libdisasm/ia32_reg.c
	${BREAKPAD_PATH}/third_party/libdisasm/ia32_settings.c
	${BREAKPAD_PATH}/third_party/libdisasm/x86_disasm.c
	${BREAKPAD_PATH}/third_party/libdisasm/x86_imm.c
	${BREAKPAD_PATH}/third_party/libdisasm/x86_insn.c
	${BREAKPAD_PATH}/third_party/libdisasm/x86_misc.c
	${BREAKPAD_PATH}/third_party/libdisasm/x86_operand_list.c
)

add_library( qbreakpad STATIC ${SOURCES} ${QM_FILES} ${RC_SOURCES} )
if( NOT QT_VERSION_MAJOR EQUAL 4 )
	qt5_use_modules( qbreakpad Widgets )
endif()
target_link_libraries( qbreakpad ${CMAKE_THREAD_LIBS_INIT} )
