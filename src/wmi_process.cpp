#include "wmi_process.h"
#include "src/wmi.h"

#include <QDebug>
#include <algorithm>
#include <stdexcept>

////////////////////////////////////////////////////////////////////////////////

WmiProcess::WmiProcess()
{}

WmiProcess::WmiProcess(const tool::WmiObject& o)
	: tool::WmiObject(o)
{}

WmiProcess::~WmiProcess()
{}

QString WmiProcess::caption() const
{
	return property("Caption").toString();
}

QString WmiProcess::commandLine() const
{
	return property("CommandLine").toString();
}

int WmiProcess::processID() const
{
	return property("ProcessId").toInt();
}

QString WmiProcess::processName() const
{
	return property("Name").toString();
}

QString WmiProcess::executablePath() const
{
	// can be used to extract icon
	return property("ExecutablePath").toString();
}

WmiProcess::State WmiProcess::executionState() const
{
	auto state = property("ExecutionState").toUInt();
	if (state > Growing)
		throw std::runtime_error{ "unknown state value" };
	return static_cast<State>(state);
}

bool WmiProcess::isTerminated() const
{
	int state = property("ExecutionState").toInt();
	qDebug() << processName() << state;
	return (state == 7 || state == 8);
}

int WmiProcess::terminate()
{
	return method("Terminate(uint)", { 0 });
}

int WmiProcess::attachDebugger()
{
	return method("AttachDebugger()");
}

////////////////////////////////////////////////////////////////////////////////

int WmiProcess::create(const QString& cmd, const QString& dir, WmiProcessStartupInfo* si)
{
	try
	{
		tool::WmiObject objProcess = tool::WmiObject::object("Process");
		int retcode = -1, handle = -1;
		if (si != nullptr)
		{
			tool::WmiObject objConfig = tool::WmiObject::object("ProcessStartup");
			if (si->createFlags > 0)
				objConfig.setProperty("CreateFlags", static_cast<unsigned int>(si->createFlags));
			if (!si->title.isEmpty())
				objConfig.setProperty("Title", si->title);
			if (si->showWindow >= 0)
				objConfig.setProperty("ShowWindow", si->showWindow);
			retcode = objProcess.method("Create(QString,QString,QObject*,uint&)",
				{ cmd, dir, objConfig, tool::ref(handle) });
		}
		else
			retcode = objProcess.method("Create(QString,QString,QObject*,uint&)",
				{ cmd, dir, tool::nullWmiObject, tool::ref(handle) });
		if (retcode != 0)
		{
			qDebug() << "retcode: " << retcode;
			throw std::runtime_error{ "cannot start process" };
		}
		return handle;
	}
	catch (const std::runtime_error& e)
	{
		qDebug() << "WmiProcess::create: " << e.what();
	}
	return -1;
}

WmiProcess WmiProcess::process(int handle)
{
	auto lst = tool::WmiObject::objects("Process");
	auto it = std::find_if(lst.begin(), lst.end(),
		[handle](const WmiObject& o) {
			return (o.property("ProcessId").toInt() == handle);
		});
	if (it == lst.end())
		throw std::runtime_error{ "cannot find specified process" };
	return *it;
}

////////////////////////////////////////////////////////////////////////////////
