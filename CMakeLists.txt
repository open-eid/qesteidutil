cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0020 NEW)
project( qesteidutil )

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
include( VersionInfo )
set_app_name( PROGNAME qesteidutil )

find_package( Threads )
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
	src/main.cpp
	src/MainWindow.cpp
	src/QSmartCard.cpp
	src/sslConnect.cpp
	src/XmlReader.cpp
	src/Updater.cpp
	${SOURCES}
	${RESOURCE_FILES}
)
add_manifest( ${PROGNAME} )
target_link_libraries( ${PROGNAME}
	${QT_LIBRARIES}
	${ADDITIONAL_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	qdigidoccommon
	smartcardpp
)

if(APPLE)
	set_source_files_properties( src/main.cpp src/sslConnect.cpp src/QSmartCard.cpp src/Updater.cpp
		PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations" )
	add_custom_target( macdeployqt DEPENDS ${PROGNAME}
		COMMAND ${_qt5Core_install_prefix}/bin/macdeployqt ${CMAKE_CURRENT_BINARY_DIR}/${PROGNAME}.app
		COMMAND find $<TARGET_FILE_DIR:${PROGNAME}>/../PlugIns/imageformats -type f -a ! -name libqjpeg.dylib -delete
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
	add_custom_target( productbuild DEPENDS ${PROGNAME}
		COMMAND productbuild --component ${CMAKE_CURRENT_BINARY_DIR}/${PROGNAME}.app /Applications
			--sign \"$$INSTCERT\" ${PROGNAME}-store_${VERSION}$ENV{VER_SUFFIX}.pkg
	)
	add_custom_target( zip DEPENDS ${PROGNAME}
		COMMAND zip --symlinks -r -D ${PROGNAME}_${VERSION}$ENV{VER_SUFFIX}.zip ${PROGNAME}.app
	)
	add_custom_target( zipdebug DEPENDS ${PROGNAME}
		COMMAND dsymutil -o ${PROGNAME}.dSYM $<TARGET_FILE:${PROGNAME}>
		COMMAND zip -r ${PROGNAME}-dbg_${VERSION}$ENV{VER_SUFFIX}.zip ${PROGNAME}.dSYM
	)
elseif(WIN32)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(PLATFORM x64)
		list(APPEND CANDLE_EXTRA "-dlibs_path=C:/Openssl-Win64/bin")
	else()
		set(PLATFORM x86)
		list(APPEND CANDLE_EXTRA "-dlibs_path=C:/Openssl-Win32/bin")
	endif()
	set(MSI_FILE "ID-card_utility-${VERSION}$ENV{VER_SUFFIX}.${PLATFORM}")
	list(APPEND CANDLE_CMD "$ENV{WIX}bin\\candle.exe" -nologo -arch ${PLATFORM}
		-dMSI_VERSION=${VERSION} -dPlatform=${PLATFORM} ${CANDLE_EXTRA}
		-dutility_path=$<TARGET_FILE:${PROGNAME}> ${CMAKE_SOURCE_DIR}/qesteidutil.wxs)
	list(APPEND LIGHT_CMD "$ENV{WIX}bin\\light.exe" -nologo qesteidutil.wixobj -ext WixUIExtension
		-dWixUILicenseRtf=${CMAKE_SOURCE_DIR}/cmake/modules/LICENSE.LGPL.rtf
		-dWixUIDialogBmp=${CMAKE_SOURCE_DIR}/cmake/modules/dlgbmp.bmp
		-dWixUIBannerBmp=${CMAKE_SOURCE_DIR}/cmake/modules/banner.bmp)
	add_custom_target(msi DEPENDS ${PROGNAME}
		COMMAND ${CANDLE_CMD}
		COMMAND ${LIGHT_CMD} -loc ${CMAKE_SOURCE_DIR}/qesteidutil.en-US.wxl -cultures:en-US -o "${MSI_FILE}.en-US.msi"
		COMMAND ${LIGHT_CMD} -loc ${CMAKE_SOURCE_DIR}/qesteidutil.et-EE.wxl -cultures:et-EE -o "${MSI_FILE}.et-EE.msi"
		#Build MSI with QT
		COMMAND ${CANDLE_CMD} -dqt_path=${_qt5Core_install_prefix}
		COMMAND ${LIGHT_CMD} -loc ${CMAKE_SOURCE_DIR}/qesteidutil.en-US.wxl -cultures:en-US -o "${MSI_FILE}.en-US.qt.msi"
		COMMAND ${LIGHT_CMD} -loc ${CMAKE_SOURCE_DIR}/qesteidutil.et-EE.wxl -cultures:et-EE -o "${MSI_FILE}.et-EE.qt.msi"
	)
	if(SIGNCERT)
		list(APPEND SIGNCMD signtool.exe sign /a /v /s MY /n "${SIGNCERT}" /fd SHA256
			/du http://installer.id.ee /t http://timestamp.verisign.com/scripts/timstamp.dll)
		add_custom_command(TARGET ${PROGNAME} POST_BUILD
			COMMAND ${SIGNCMD} $<TARGET_FILE:${PROGNAME}>
		)
		add_custom_command(TARGET msi POST_BUILD
			COMMAND ${SIGNCMD} "${MSI_FILE}.en-US.msi"
			COMMAND ${SIGNCMD} "${MSI_FILE}.et-EE.msi"
			COMMAND ${SIGNCMD} "${MSI_FILE}.en-US.qt.msi"
			COMMAND ${SIGNCMD} "${MSI_FILE}.et-EE.qt.msi"
		)
	endif()
else()
	include( GNUInstallDirs )
	configure_file( src/qesteidutil.1.cmake qesteidutil.1 )
	install( TARGETS ${PROGNAME} DESTINATION ${CMAKE_INSTALL_BINDIR} )
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
