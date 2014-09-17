if( WIN32 )
	add_definitions( -DUNICODE )
endif()
add_library( qtsingleapplication STATIC src/qtlocalpeer.cpp src/qtsingleapplication.cpp )
set_target_properties( qtsingleapplication PROPERTIES AUTOMOC TRUE )

if( Qt5Widgets_FOUND )
	qt5_use_modules( qtsingleapplication Widgets Network )
endif()
