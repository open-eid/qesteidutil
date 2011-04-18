set( MOC_HEADERS
	AboutWidget.h
	CheckConnection.h
	CertificateWidget.h
	ComboBox.h
	Common.h
	IKValidator.h
	MessageBox.h
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
	SOAPDocument.h
	SslCertificate.h
	TokenData.h
	)

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
	MessageBox.cpp
	PinDialog.cpp
	QPKCS11.cpp
	SOAPDocument.cpp
	SslCertificate.cpp
	sslConnect.cpp
	TokenData.cpp
	)
if( APPLE )
	set( SOURCES ${SOURCES} Application_mac.mm )
endif( APPLE )

include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${OPENSSL_INCLUDE_DIR}
	${LIBP11_INCLUDE_DIR}
	)

if( LIBP11_TOKEN_FLAGS )
	add_definitions( -DLIBP11_TOKEN_FLAGS )
endif( LIBP11_TOKEN_FLAGS )
if( MSVC )
	add_definitions( -DWIN32_LEAN_AND_MEAN )
endif( MSVC )

add_library( qdigidoccommon STATIC ${SOURCES} )
