set( MOC_HEADERS
	AboutWidget.h
	CheckConnection.h
	CertificateWidget.h
	ComboBox.h
	Common.h
	IKValidator.h
	PinDialog.h
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
	SslCertificate.h
	TokenData.h
	)
QT4_WRAP_CPP( MOC_SOURCES ${MOC_HEADERS} )
set( SOURCES
	${HEADERS}
	${MOC_SOURCES}
	AboutWidget.cpp
	CheckConnection.cpp
	CertificateWidget.cpp
	ComboBox.cpp
	Common.cpp
	IKValidator.cpp
	PinDialog.cpp
	SslCertificate.cpp
	sslConnect.cpp
	TokenData.cpp
	)

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${OPENSSL_INCLUDE_DIR}
	${LIBP11_INCLUDE_DIR}
	)

if(APPLE)
    # xCode hacks
    include_directories(/usr/local/include/../include)
endif(APPLE)

ADD_LIBRARY(
	qdigidoccommon
	STATIC
	${SOURCES}
	)
