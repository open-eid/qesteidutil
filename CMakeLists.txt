cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0020 NEW)
project( qesteidutil )

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

include( GNUInstallDirs )
include( VersionInfo )

set_app_name( PROGNAME qesteidutil )

find_package( Qt5Widgets QUIET HINTS ${QTSDK} )
if( Qt5Widgets_FOUND )
	find_package( Qt5Network HINTS ${QTSDK} )
	find_package( Qt5LinguistTools HINTS ${QTSDK} )
	add_definitions( -DQT_DISABLE_DEPRECATED_BEFORE=0x040800 )
else()
	find_package( Qt4 4.8.0 COMPONENTS QtCore QtGui QtNetwork REQUIRED )
	include( ${QT_USE_FILE} )
endif()

add_subdirectory( smartcardpp )
include( common/BreakPad.cmake )
add_subdirectory( common )

configure_file( src/translations/tr.qrc tr.qrc COPYONLY )
set( RC_FILES ${CMAKE_BINARY_DIR}/tr.qrc src/qesteidutil.qrc )
set( TS_FILES src/translations/en.ts src/translations/et.ts src/translations/ru.ts )
set( UI_FILES src/ui/MainWindow.ui )
if( WIN32 )
	list( APPEND UI_FILES src/ui/SettingsDialog.ui )
	list( APPEND SOURCES src/SettingsDialog.cpp src/CertStore.cpp )
	list( APPEND ADDITIONAL_LIBRARIES Crypt32 )
endif()

if( NOT Qt5Widgets_FOUND )
	qt4_add_translation( SOURCES ${TS_FILES} )
	qt4_add_resources( SOURCES ${RC_FILES} )
	qt4_wrap_ui( SOURCES ${UI_FILES} )
else()
	qt5_add_translation( SOURCES ${TS_FILES} )
	qt5_add_resources( SOURCES ${RC_FILES} )
	qt5_wrap_ui( SOURCES ${UI_FILES} )
	list( APPEND QT_LIBRARIES Qt5::Widgets Qt5::Network )
endif()

include_directories(
	${OPENSSL_INCLUDE_DIR}
	${PCSCLITE_INCLUDE_DIR}
	qtsingleapplication/src
)

add_executable( ${PROGNAME} WIN32 MACOSX_BUNDLE
	src/qesteidutil.rc
	src/CertUpdate.cpp
	src/main.cpp
	src/MainWindow.cpp
	src/QSmartCard.cpp
	src/sslConnect.cpp
	src/XmlReader.cpp
	${SOURCES}
	${RESOURCE_FILES}
)
if( APPLE )
	set_source_files_properties( src/main.cpp src/sslConnect.cpp src/QSmartCard.cpp src/CertUpdate.cpp
		PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations" )
	add_custom_target( macdeployqt DEPENDS ${PROGNAME}
		COMMAND ${_qt5Core_install_prefix}/bin/macdeployqt ${CMAKE_CURRENT_BINARY_DIR}/${PROGNAME}.app
		COMMAND find $<TARGET_FILE_DIR:${PROGNAME}>/../PlugIns/imageformats -type f -a ! -name libqjpeg.dylib -delete
		COMMAND for f in $<TARGET_FILE_DIR:${PROGNAME}>/../Frameworks/*\; do
				name=$$\(basename $$f .framework\)\;
				mv $$f/Resources $$f/Versions/5\;
				ln -s Versions/Current/Resources $$f/Resources\;
				ln -s 5 $$f/Versions/Current\;
				ln -s Versions/Current/$$name $$f/$$name\;
				cp ${_qt5Core_install_prefix}/lib/$$name.framework/Contents/Info.plist $$f/Resources\;
				defaults write $$f/Resources/Info.plist CFBundleExecutable $$name\;
			done
	)
	add_custom_target( codesign DEPENDS ${PROGNAME}
		COMMAND for f in
			$<TARGET_FILE_DIR:${PROGNAME}>/../Frameworks/*
			$<TARGET_FILE_DIR:${PROGNAME}>/../PlugIns/*/*\; do
				codesign -f -s \"$$SIGNCERT\" $$f\;
			done
		COMMAND codesign -f -s \"$$SIGNCERT\" $<TARGET_FILE_DIR:${PROGNAME}>/../..
			--entitlements ${CMAKE_SOURCE_DIR}/src/${PROGNAME}.entitlements
	)
	add_custom_target( productbuild DEPENDS codesign
		COMMAND productbuild --component ${CMAKE_CURRENT_BINARY_DIR}/${PROGNAME}.app /Applications
			--sign \"$$INSTCERT\" ${PROGNAME}-store_${VERSION}$ENV{VER_SUFFIX}.pkg
	)
	add_custom_target( zip DEPENDS codesign
		COMMAND zip --symlinks -r -D ${PROGNAME}_${VERSION}$ENV{VER_SUFFIX}.zip ${PROGNAME}.app
	)
	add_custom_target( zipdebug DEPENDS ${PROGNAME}
		COMMAND dsymutil -o ${PROGNAME}.dSYM $<TARGET_FILE:${PROGNAME}>
		COMMAND zip -r ${PROGNAME}-dbg_${VERSION}$ENV{VER_SUFFIX}.zip ${PROGNAME}.dSYM
	)
endif()

target_link_libraries( ${PROGNAME}
	${QT_LIBRARIES}
	${ADDITIONAL_LIBRARIES}
	qdigidoccommon
	smartcardpp
)
add_manifest( ${PROGNAME} )
dump_syms( ${PROGNAME} )

if(UNIX AND NOT APPLE)
	configure_file( src/qesteidutil.1.cmake qesteidutil.1 )
	install( FILES ${CMAKE_CURRENT_BINARY_DIR}/qesteidutil.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1 )
	install( FILES qesteidutil.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/appdata )
	install( FILES qesteidutil.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications )
	foreach( RES 16x16 32x32 48x48 128x128 )
		install(
			FILES src/images/id_icon_${RES}.png
			DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${RES}/apps/
			RENAME qesteidutil.png
		)
	endforeach(RES)
endif()

install(TARGETS ${PROGNAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
