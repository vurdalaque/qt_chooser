#pragma once
#include "wmi.h"

////////////////////////////////////////////////////////////////////////////////

struct WmiProcessStartupInfo
{
	enum CreateFlags
	{
		Debug_Process = 1,
		Debug_Only_This_Process = 2,
		Create_Suspended = 4,
		Detached_Process = 8,
		Create_New_Console = 16,
		Create_New_Process_Group = 512,
		Create_Unicode_Environment = 1024,
		Create_Default_Error_Mode = 67108864,
		CREATE_BREAKAWAY_FROM_JOB = 16777216,
	};
	enum ShowWindow
	{
		SW_HIDE = 0,
		SW_NORMAL = 1,
		SW_SHOWMINIMIZED = 2,
		SW_SHOWMAXIMIZED = 3,
		SW_SHOWNOACTIVATE = 4,
		SW_SHOW = 5,
		SW_MINIMIZE = 6,
		SW_SHOWMINNOACTIVE = 7,
		SW_SHOWNA = 8,
		SW_RESTORE = 9,
		SW_SHOWDEFAULT = 10,
		SW_FORCEMINIMIZE = 11,
	};
	int createFlags = -1;
	int showWindow = -1;
	QString title;
};

////////////////////////////////////////////////////////////////////////////////

class WmiProcess : public tool::WmiObject
{
public:
	enum State
	{
		Unknown,
		Other,
		Ready,
		Running,
		Blocked,
		SuspendedBlocked,
		SuspendedReady,
		Terminated,
		Stopped,
		Growing,
	};
	WmiProcess();
	WmiProcess(const tool::WmiObject&);
	~WmiProcess();

	/** Short description of an object—a one-line string. */
	QString caption() const;
	/** Command line used to start a specific process, if applicable. */
	QString commandLine() const;
	/** Numeric identifier used to distinguish one process from another. */
	int processID() const;
	/** Name of the executable file responsible for the process, equivalent to the Image Name property in Task Manager */
	QString processName() const;
	/** Path to the executable file of the process. */
	QString executablePath() const;
	/** Current operating condition of the process. */
	State executionState() const;
	/** Check if state is Terminated or Stopped */
	bool isTerminated() const;
	/** method: Terminates a process and all of its threads. */
	int terminate();
	/** method: Launches the currently registered debugger for a process. */
	int attachDebugger();

	/** The Create WMI class method creates a new process. */
	static int create(const QString& commandLine, const QString& commandDirectory = QString{}, WmiProcessStartupInfo* = nullptr);
	/** Get process object by ProcessId */
	static WmiProcess process(int);
};

////////////////////////////////////////////////////////////////////////////////
Q_DECLARE_METATYPE(WmiProcess);
