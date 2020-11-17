#pragma once
#include "wmi.h"
#include <memory>

////////////////////////////////////////////////////////////////////////////////

class WmiService : public tool::WmiObject
{
public:
	enum State
	{
		Unknown,
		Stopped,
		Running,
		Paused,
		StartPending,
		StopPending,
		ContinuePending,
		PausePending,
	};
	enum StartMode
	{
		Boot,
		System,
		Auto,
		Manual,
		Disabled,
	};

	WmiService(const tool::WmiObject&);
	const QString executableKeyData(const QString&);
	const QString fullPath() const;
	const QString path() const;
	const QString executable() const;
	const QString executableDirectory() const;
	const QString name() const;
	State state();
	StartMode startMode() const;

public slots:

	int start();
	int stop();
	int resume();
};

using pWmiService = std::shared_ptr<WmiService>;

////////////////////////////////////////////////////////////////////////////////
Q_DECLARE_METATYPE(pWmiService);
