cmake_minimum_required(VERSION 2.8)
project( qesteidutil )

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
set(CMAKE_FIND_ROOT_PATH ${CMAKE_OSX_SYSROOT})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

include( GNUInstallDirs )
include( VersionInfo )

set_app_name( PROGNAME qesteidutil )

find_package( Qt5Widgets QUIET HINTS ${QTSDK} )
if( Qt5Widgets_FOUND )
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
set_target_properties( ${PROGNAME} PROPERTIES AUTOMOC TRUE )
if( APPLE )
	set_source_files_properties( src/QSmartCard.cpp src/CertUpdate.cpp PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations" )
endif()

if( Qt5Widgets_FOUND )
	qt5_use_modules( ${PROGNAME} Widgets Network )
endif()

target_link_libraries( ${PROGNAME}
	${Qt5Core_QTMAIN_LIBRARIES}
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
