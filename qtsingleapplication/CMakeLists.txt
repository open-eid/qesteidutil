set( MOC_HEADERS
	src/qtlocalpeer.h
	src/qtsingleapplication.h
	)
set( HEADERS
	${MOC_HEADERS}
	)
QT4_WRAP_CPP( MOC_SOURCES ${MOC_HEADERS} )
set( SOURCES
	${HEADERS}
	${MOC_SOURCES}
	src/qtlocalpeer.cpp
	src/qtsingleapplication.cpp
	)

ADD_LIBRARY( qtsingleapplication STATIC ${SOURCES} )
