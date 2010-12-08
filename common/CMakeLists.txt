if( WIN32 )
	execute_process( COMMAND "date" "/T" OUTPUT_VARIABLE BUILD_DATE )
	string( REGEX REPLACE "(..)/(..)/(....).*" "\\1.\\2.\\3" BUILD_DATE ${BUILD_DATE} )
elseif( UNIX )
	execute_process( COMMAND "date" "+%d/%m/%Y" OUTPUT_VARIABLE BUILD_DATE )
	string( REGEX REPLACE "(..)/(..)/(....).*" "\\1.\\2.\\3" BUILD_DATE ${BUILD_DATE} )
else( WIN32 )
	message( SEND_ERROR "date not implemented")
	set( BUILD_DATE "00.00.0000" )
endif( WIN32 )

add_definitions( -DBUILD_DATE=\"${BUILD_DATE}\" )

set( MOC_HEADERS
	AboutWidget.h
	CheckConnection.h
	CertificateWidget.h
	ComboBox.h
	Common.h
	IKValidator.h
	PinDialog.h
	QPKCS11.h
	QPKCS11_p.h
	Settings.h
	sslConnect.h
	sslConnect_p.h
	)
QT4_WRAP_UI( UI_HEADERS
	ui/AboutWidget.ui
	ui/CertificateWidget.ui
	)
set( HEADERS
	${MOC_HEADERS}
	${UI_HEADERS}
	pkcs11.h
	SslCertificate.h
	TokenData.h
	)

if( APPLE )
	# xCode hacks
	include_directories(/usr/local/include/../include)
	set( MOC_HEADERS ${MOC_HEADERS} Application_mac.h )
	set( SOURCES Application_mac.mm )
endif( APPLE )

QT4_WRAP_CPP( MOC_SOURCES ${MOC_HEADERS} )
set( SOURCES
	${SOURCES}
	${HEADERS}
	${MOC_SOURCES}
	AboutWidget.cpp
	CheckConnection.cpp
	CertificateWidget.cpp
	ComboBox.cpp
	Common.cpp
	IKValidator.cpp
	PinDialog.cpp
	QPKCS11.cpp
	SslCertificate.cpp
	sslConnect.cpp
	TokenData.cpp
	)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${OPENSSL_INCLUDE_DIR}
	${LIBP11_INCLUDE_DIR}
	)

if( LIBP11_TOKEN_FLAGS )
	add_definitions( -DLIBP11_TOKEN_FLAGS )
endif( LIBP11_TOKEN_FLAGS )
include_directories( ${LIBP11_INCLUDE_DIR} )

add_library( qdigidoccommon STATIC ${SOURCES} )
