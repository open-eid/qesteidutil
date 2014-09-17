find_package( OpenSSL REQUIRED )

if( UNIX AND NOT APPLE )
	find_package(PCSCLite REQUIRED)
	include_directories( ${PCSCLITE_INCLUDE_DIR} )
	set( LIBRARIES ${PCSCLITE_LIBRARY} )
endif()

include_directories(${OPENSSL_INCLUDE_DIR} ${PCSCLITE_INCLUDE_DIR})

set( KILLSWITCH $ENV{KILLSWITCH} CACHE STRING "Set KillSwitch verification URL" )
if( KILLSWITCH )
	add_definitions( -DKILLSWITCH="${KILLSWITCH}" )
endif()
if( WIN32 )
	set( SOURCES Diagnostics_win.cpp )
	set( LIBRARIES Version Ws2_32 SetupApi winscard )
	add_definitions( -DUNICODE -DWIN32_LEAN_AND_MEAN )
else()
	list( APPEND SOURCES Diagnostics_unix.cpp )
endif()

if( APPLE )
	list( APPEND SOURCES Application_mac.mm MacMenuBar.cpp )
	set_source_files_properties( Application_mac.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc" )
	set_source_files_properties( SslCertificate.cpp QPKCS11.cpp sslConnect.cpp
		PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations" )
	set( LIBRARIES "-framework Cocoa -framework PCSC -fobjc-arc" )
else()
	include_directories( ${CMAKE_SOURCE_DIR} )
	add_subdirectory( ../qtsingleapplication ${CMAKE_BINARY_DIR}/qtsingleapplication )
	list( APPEND LIBRARIES qtsingleapplication )
endif()

set( TS_FILES translations/common_en.ts translations/common_et.ts translations/common_ru.ts )
if( NOT Qt5Widgets_FOUND )
	list( APPEND TS_FILES translations/qt_et.ts translations/qt_ru.ts )
	configure_file( translations/common_tr.qrc common_tr.qrc COPYONLY )
else()
	list( APPEND TS_FILES translations/qtbase_et.ts translations/qtbase_ru.ts )
	configure_file( translations/common_tr5.qrc common_tr.qrc COPYONLY )
endif()
set( RC_FILES images/common_images.qrc ${CMAKE_CURRENT_BINARY_DIR}/common_tr.qrc )
set( UI_FILES ui/AboutDialog.ui ui/CertificateWidget.ui )

if( NOT Qt5Widgets_FOUND )
	qt4_add_translation( SOURCES ${TS_FILES} )
	qt4_add_resources( SOURCES ${RC_FILES} )
	qt4_wrap_ui( SOURCES ${UI_FILES} )
else()
	qt5_add_translation( SOURCES ${TS_FILES} )
	qt5_add_resources( SOURCES ${RC_FILES} )
	qt5_wrap_ui( SOURCES ${UI_FILES} )
endif()

add_library( qdigidoccommon STATIC
	${SOURCES}
	AboutDialog.cpp
	CertificateWidget.cpp
	ComboBox.cpp
	Common.cpp
	DateTime.cpp
	Diagnostics.cpp
	IKValidator.cpp
	PinDialog.cpp
	QPCSC.cpp
	SOAPDocument.cpp
	Settings.h
	SslCertificate.cpp
	TokenData.cpp
)

if( Qt5Widgets_FOUND )
	qt5_use_modules( qdigidoccommon Widgets )
endif()

target_link_libraries( qdigidoccommon ${OPENSSL_LIBRARIES} ${LIBRARIES} )
