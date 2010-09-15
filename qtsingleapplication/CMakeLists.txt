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
add_definitions( -DUNICODE )
add_library( qtsingleapplication STATIC ${SOURCES} )
