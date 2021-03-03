![](https://github.com/vurdalaque/qt_chooser/blob/master/preview/sample.png)
## qt_chooser

Simple tool that provides fast switch between Qt's versions. Designed for Windows 10.
Also, added process monitor (configured in config file), and PostgreSQL manager.
// todo: add more specific documentation

### Compiling and running
Build tested on MSVC2017, and Qt 5.11.3

`%project_dir%> qmake && nmake`

After first run, will be created configuration file
`%LOCALAPPDATA%/qt_chooser/qt_chooser.json`

#### configuration parameters
* extsdk : is directory where will be searched installed Qt's versions
* mountPoint : is where will be created symlink
* services : (optional) list of system service names, wich will be monitored
* enableProcManager : true/false. Not fully implemented process manager
* process : list of processes that will be monitored

#### notes
* "set QT_QPA_PLATFORM_PLUGIN_PATH=C:/qt/qtbase/plugins/platforms"

### Known issues
+ extsdk directory path must be started with capital disk letter
	(odd Qt's bug: QML plugin loader fails with path like "d:/qt/qml")
+ service and process monitoring performed through WMI, so the are quite slow
* Qt's install directory must have subdirectory called qtbase:
	- %extsdk%
		- android
			- qtbase
				- bin
				- doc
				- lib
				- include
				... etc ...
		- 5.14.0
			- qtbase
				- bin
				- doc
				- lib
				- include
				... etc ...
	(sorry, but that exactly what I needed)

