cmake_minimum_required(VERSION 2.8)
project(qesteidutil)
set( PROGNAME qesteidutil )
set( MAJOR_VER 3 )
set( MINOR_VER 5 )
set( RELEASE_VER 0 )

# Custom cmake modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

include( InstallSettings )
include( VersionInfo )

find_package(Qt4 4.6.0 COMPONENTS QtCore QtGui QtNetwork QtWebkit QtXml REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PKCS11)
find_package(SmartCardpp REQUIRED)
if(UNIX)
	# Hack to get around smartcardpp header inclusion scheme
	find_package(PCSCLite REQUIRED)
endif(UNIX)

add_definitions(-DPKCS11_MODULE="${PKCS11_MODULE}")

if(MSVC)
    # Needed for smartcardpp
    add_definitions(-D_SECURE_SCL=0)
endif(MSVC)

include(${QT_USE_FILE})
add_subdirectory( qtsingleapplication )
set( USE_SECURITY YES )
add_subdirectory( common )
include( common/resources.cmake )

set(qesteidutil_MOC_HDRS
	src/CertUpdate.h
	src/DiagnosticsDialog.h
	src/jscardmanager.h
	src/jscertdata.h
	src/jsesteidcard.h
	src/jsextender.h
	src/mainwindow.h
)
if( APPLE OR WIN32 )
	set( qesteidutil_MOC_HDRS ${qesteidutil_MOC_HDRS} src/SettingsDialog.h )
endif( APPLE OR WIN32 )

set(qesteidutil_HDRS
	${qesteidutil_MOC_HDRS}
	src/version.h
)

QT4_WRAP_CPP(qesteidutil_MOC_SRCS ${qesteidutil_MOC_HDRS})
set(qesteidutil_SRCS
	${qesteidutil_HDRS}
	${qesteidutil_MOC_SRCS}
	src/CertUpdate.cpp
	src/DiagnosticsDialog.cpp
	src/jscardmanager.cpp
	src/jscertdata.cpp
	src/jsesteidcard.cpp
	src/jsextender.cpp
	src/main.cpp
	src/mainwindow.cpp
)
if( APPLE OR WIN32 )
	set( qesteidutil_SRCS ${qesteidutil_SRCS} src/SettingsDialog.cpp )
endif( APPLE OR WIN32 )
if( WIN32 )
	set( qesteidutil_SRCS ${qesteidutil_SRCS} src/CertStore.cpp src/DiagnosticsDialog_win.cpp )
else( WIN32 )
	set( qesteidutil_SRCS ${qesteidutil_SRCS} src/DiagnosticsDialog_unix.cpp )
endif( WIN32 )

QT4_WRAP_UI( UI_HEADERS
	src/ui/DiagnosticsDialog.ui
	src/ui/SettingsDialog.ui
)

set(QM_DIR ${CMAKE_BINARY_DIR})
configure_file(${COMMON}/translations/common_tr.qrc.cmake ${QM_DIR}/common_tr.qrc)
configure_file(src/translations/tr.qrc.cmake ${QM_DIR}/tr.qrc)

QT4_ADD_RESOURCES( qesteidutil_RCC_SRCS
	${COMMON_RCS}
	${QM_DIR}/tr.qrc
	${QM_DIR}/common_tr.qrc
	src/qesteidutil.qrc
)

QT4_ADD_TRANSLATION( QM_FILES
	${COMMON_TS}
	src/translations/en.ts
	src/translations/et.ts
	src/translations/ru.ts
)

include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_BINARY_DIR}
	${OPENSSL_INCLUDE_DIR}
	${SMARTCARDPP_INCLUDE_DIR}
	${PCSCLITE_INCLUDE_DIR}
	qtsingleapplication/src
)

if( APPLE )
	include( VersionInfoMac )

	find_library( CORESERVICES_LIB CoreServices )
	find_library( SECURITY_LIB Security )
	find_library( COCOA_LIBRARY Cocoa )
	set( ADDITIONAL_LIBRARIES ${CORESERVICES_LIB} ${SECURITY_LIB} ${COCOA_LIBRARY} )

	# Qt jpeg plugin
	if( NOT DEFINED QT_PLUGINS_DIR )
		set( QT_PLUGINS_DIR "/Developer/Applications/Qt/" )
	endif( NOT DEFINED QT_PLUGINS_DIR )
	set( LIBQJPEG_FILE "${QT_PLUGINS_DIR}/imageformats/libqjpeg.dylib" )
	set( RESOURCE_FILES ${RESOURCE_FILES} ${LIBQJPEG_FILE} )
	set_source_files_properties( ${LIBQJPEG_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "MacOS/imageformats" )
endif( APPLE )

IF(WIN32)
	SET(qesteidutil_SRCS ${qesteidutil_SRCS} src/qesteidutil.rc)
	SET(ADDITIONAL_LIBRARIES ws2_32 Crypt32)
ENDIF(WIN32)
    
add_executable( ${PROGNAME} WIN32 MACOSX_BUNDLE
    ${qesteidutil_SRCS}
    ${QM_FILES}
    ${qesteidutil_RCC_SRCS}
    ${UI_HEADERS}
    ${RESOURCE_FILES}
)

if( APPLE )
	set_target_properties( ${PROGNAME}
		PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/mac/Info.plist )
endif( APPLE )
if( WIN32 )
	set_target_properties( ${PROGNAME}
		PROPERTIES
		LINK_FLAGS "/MANIFESTDEPENDENCY:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' publicKeyToken='6595b64144ccf1df' language='*' processorArchitecture='*'\"")
endif( WIN32 )

target_link_libraries( ${PROGNAME}
	${QT_QTMAIN_LIBRARY}
	${QT_LIBRARIES}
	${OPENSSL_LIBRARIES}
	${SMARTCARDPP_LIBRARIES}
	${ADDITIONAL_LIBRARIES}
	${CMAKE_DL_LIBS}
	qdigidoccommon
	qtsingleapplication
)

if(UNIX AND NOT APPLE)
    INSTALL(FILES qesteidutil.desktop DESTINATION ${SHARE_INSTALL_PREFIX}/applications)

    # Install icons
    foreach(RES 16x16 32x32 48x48)
        install(
            FILES src/html/images/id_icon_${RES}.png
            DESTINATION ${SHARE_INSTALL_PREFIX}/icons/hicolor/${RES}/apps/
            RENAME qesteidutil.png
        )
    endforeach(RES)
endif(UNIX AND NOT APPLE)

install(TARGETS ${PROGNAME} DESTINATION ${BIN_INSTALL_DIR})
