cmake_minimum_required(VERSION 2.8)
project(qesteidutil)

# Custom cmake modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

include(InstallSettings)

find_package(Qt4 4.6.0 COMPONENTS QtCore QtGui QtNetwork QtWebkit QtXml REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PKCS11)
find_package(LibP11 REQUIRED)
find_package(SmartCardpp REQUIRED)
find_package(LibDL REQUIRED)
find_package(LibLtdl REQUIRED)

add_definitions(-DPKCS11_MODULE="${PKCS11_MODULE}")

# Hack to get around smartcardpp header inclusion scheme
if(UNIX)
    find_package(PCSCLite REQUIRED)
endif(UNIX)

if(MSVC)
    # Needed for smartcardpp
    add_definitions(-D_SECURE_SCL=0)
endif(MSVC)

set(qesteidutil_MOC_HDRS
    common/CertificateWidget.h
    common/PinDialog.h
    common/Settings.h
    common/sslConnect.h
    common/sslConnect_p.h
    src/CertUpdate.h
    src/DiagnosticsDialog.h
    src/SettingsDialog.h
    src/jscardmanager.h
    src/jscertdata.h
    src/jsesteidcard.h
    src/jsextender.h
    src/mainwindow.h
)

set(qesteidutil_HDRS
    ${qesteidutil_MOC_HDRS}
    common/SslCertificate.h
    common/TokenData.h
    src/version.h
)

QT4_WRAP_CPP(qesteidutil_MOC_SRCS ${qesteidutil_MOC_HDRS})
set(qesteidutil_SRCS
    ${qesteidutil_HDRS}
    ${qesteidutil_MOC_SRCS}
    common/CertificateWidget.cpp
    common/PinDialog.cpp
    common/SslCertificate.cpp
    common/TokenData.cpp
    common/sslConnect.cpp
    src/CertUpdate.cpp
    src/DiagnosticsDialog.cpp
    src/SettingsDialog.cpp
    src/jscardmanager.cpp
    src/jscertdata.cpp
    src/jsesteidcard.cpp
    src/jsextender.cpp
    src/main.cpp
    src/mainwindow.cpp
)

QT4_WRAP_UI( UI_HEADERS
    common/ui/CertificateWidget.ui
    src/ui/DiagnosticsDialog.ui
    src/ui/SettingsDialog.ui
)

set(QM_DIR ${CMAKE_BINARY_DIR})
configure_file(common/translations/common_tr.qrc.cmake ${QM_DIR}/common_tr.qrc)
configure_file(src/translations/tr.qrc.cmake ${QM_DIR}/tr.qrc)

QT4_ADD_RESOURCES( qesteidutil_RCC_SRCS
    ${QM_DIR}/common_tr.qrc
    ${QM_DIR}/tr.qrc
    src/qesteidutil.qrc
)

QT4_ADD_TRANSLATION( QM_FILES
    common/translations/common_en.ts
    common/translations/common_et.ts
    common/translations/common_ru.ts
    common/translations/qt_et.ts
    common/translations/qt_ru.ts
    src/translations/en.ts
    src/translations/et.ts
    src/translations/ru.ts
)

include(${QT_USE_FILE})

add_subdirectory( qtsingleapplication )

find_package(Subversion)
if (Subversion_FOUND AND EXISTS ${PROJECT_SOURCE_DIR}/.svn)
  Subversion_WC_INFO(${PROJECT_SOURCE_DIR} PROJECT)
  message(STATUS "Current SVN revision is ${PROJECT_WC_REVISION}")
  add_definitions(-DBUILD_VER=${PROJECT_WC_REVISION})
else (Subversion_FOUND AND EXISTS ${PROJECT_SOURCE_DIR}/.svn)
  add_definitions(-DBUILD_VER=0)
endif (Subversion_FOUND AND EXISTS ${PROJECT_SOURCE_DIR}/.svn)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${OPENSSL_INCLUDE_DIR}
    ${LIBP11_INCLUDE_DIR}
    ${SMARTCARDPP_INCLUDE_DIR}
    ${PCSCLITE_INCLUDE_DIR}
    qtsingleapplication/src
)

if(APPLE)
    set(APP_CONTENTS_DIR "${CMAKE_SOURCE_DIR}/mac")
    if(NOT DEFINED QT_PLUGINS_DIR)
	SET(QT_PLUGINS_DIR "/Developer/Applications/Qt/")
    endif(NOT DEFINED QT_PLUGINS_DIR)

    set(LIBQJPEG_FILE "${QT_PLUGINS_DIR}/imageformats/libqjpeg.dylib")
    
    # xCode hacks
    include_directories(/usr/local/include/../include)
    
    # Bundle resources
    file(GLOB_RECURSE RESOURCE_FILES ${APP_CONTENTS_DIR}/Resources/*.icns ${APP_CONTENTS_DIR}/Resources/**/*.strings)
    
    foreach(_file ${RESOURCE_FILES})
        get_filename_component(_file_dir ${_file} PATH)
        file(RELATIVE_PATH _file_dir ${APP_CONTENTS_DIR}/Resources ${_file_dir})
        set_source_files_properties(${_file} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/${_file_dir}" )
    endforeach(_file)
    
    # Qt jpeg plugin
    set(RESOURCE_FILES ${RESOURCE_FILES} ${LIBQJPEG_FILE})
    set_source_files_properties(${LIBQJPEG_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "MacOS/imageformats" )
endif(APPLE)

IF(WIN32)
    SET(qesteidutil_SRCS ${qesteidutil_SRCS} src/qesteidutil.rc)
	SET(ADDITIONAL_LIBRARIES ws2_32)
ENDIF(WIN32)
    
add_executable(qesteidutil WIN32 MACOSX_BUNDLE
    ${qesteidutil_SRCS}
    ${QM_FILES}
    ${qesteidutil_RCC_SRCS}
    ${UI_HEADERS}
    ${RESOURCE_FILES}
)

if(APPLE)
	configure_file(
		${APP_CONTENTS_DIR}/Info.plist.cmake
		${APP_CONTENTS_DIR}/Info.plist
		@ONLY
	)

    set_target_properties(qesteidutil
		PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST ${APP_CONTENTS_DIR}/Info.plist
	)
	find_library( ADDITIONAL_LIBRARIES CoreServices )
endif(APPLE)

if(WIN32)
	SET_TARGET_PROPERTIES(qesteidutil
		PROPERTIES
		LINK_FLAGS "/MANIFESTDEPENDENCY:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' publicKeyToken='6595b64144ccf1df' language='*' processorArchitecture='*'\"")
endif(WIN32)

target_link_libraries(qesteidutil
	${QT_QTMAIN_LIBRARY}
	${QT_LIBRARIES}
	${OPENSSL_LIBRARIES}
	${OPENSSLCRYPTO_LIBRARIES}
	${LIBP11_LIBRARIES}
	${SMARTCARDPP_LIBRARIES}
	${ADDITIONAL_LIBRARIES}
	${LIBDL_LIBRARIES}
	${LIBLTDL_LIBRARIES}
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

install(TARGETS qesteidutil DESTINATION ${BIN_INSTALL_DIR})
