#pragma once
#include "wmi.h"
#include <mutex>
#include <thread>

#include <QFrame>

class Settings;
namespace Ui
{
	class ProcessMainFrame;
	class ProcessWidget;

} /* namespace Ui */

////////////////////////////////////////////////////////////////////////////////

class WmiProcess : public tool::WmiObject
{
public:
	WmiProcess(const tool::WmiObject&);
	~WmiProcess();

	int processID() const;
	QString processName() const;
	QString executablePath() const;
	bool isTerminated() const;
	int kill();
	int attachDebugger();
};

////////////////////////////////////////////////////////////////////////////////

class ProcessWidget
	: public QWidget
{
	Q_OBJECT
public:
	ProcessWidget(QWidget* = nullptr);
	~ProcessWidget();

	void setProcessID(int);
	void setProcessName(const QString&);
	void setExecutablePath(const QString&);

public slots:
	void onMemoryChanged(int, int);

signals:
	void killProcess(int);
	void attachDebugger(int);
protected slots:
	void onKillProcess();
	void onAttachDebugger();

protected:
	Ui::ProcessWidget* m_ui = nullptr;
	int m_pid = 0;
};

////////////////////////////////////////////////////////////////////////////////

class ProcessManager
	: public QFrame
{
	Q_OBJECT
public:
	ProcessManager(Settings*, QWidget* = nullptr);
	~ProcessManager();

signals:
	void watchableProcess(int, const QString&, const QString&);
	void diedProcess(int);
	void memoryChanged(int, int);

protected slots:
	void onWatchableProcess(int, const QString&, const QString&);
	void onProcessDied(int);
	void onKillProcess(int);
	void onAttachDebugger(int);

protected:
	void notifierThread();
	void monitorWatchableCreate();

protected:
	Ui::ProcessMainFrame* m_ui = nullptr;
	std::mutex m_lock;
	std::thread m_thread;
	bool m_stopThread = false;
	QStringList m_watchableProcess;

	std::map<int, WmiProcess> m_watchable;
};

////////////////////////////////////////////////////////////////////////////////
