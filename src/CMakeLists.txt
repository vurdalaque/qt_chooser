target_include_directories(
	qt_chooser
	PRIVATE ${CMAKE_SOURCE_DIR})

target_sources(
	qt_chooser
	PRIVATE main.cpp
	PRIVATE mainwidget.cpp
	PRIVATE mainwidget.h
	PRIVATE pg_conf.cpp
	PRIVATE pg_conf.h
	PRIVATE pg_version.cpp
	PRIVATE pg_version.h
	PRIVATE pg_widget.cpp
	PRIVATE pg_widget.h
	PRIVATE process.cpp
	PRIVATE process.h
	PRIVATE service.cpp
	PRIVATE service.h
	PRIVATE settings.cpp
	PRIVATE settings.h
	PRIVATE statelabel.cpp
	PRIVATE statelabel.h
	PRIVATE tooling.cpp
	PRIVATE tooling.h
	PRIVATE wmi.cpp
	PRIVATE wmi.h
	PRIVATE wmi_process.cpp
	PRIVATE wmi_process.h
	PRIVATE wmi_service.cpp
	PRIVATE wmi_service.h
	)
