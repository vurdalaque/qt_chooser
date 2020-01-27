#pragma once
#include "wmi.h"

#include <QFrame>

#include <mutex>
#include <thread>

class Settings;
namespace Ui
{
	class MainServiceFrame;
}

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
	WmiService(const tool::WmiObject&);

	const QString path() const;
	State state();
	int start();
	int stop();
	int resume();

protected:
	int runMethod(const QByteArray&);
};

////////////////////////////////////////////////////////////////////////////////

class ServiceManager
	: public QFrame
{
	Q_OBJECT

public:
	ServiceManager(const QString& serviceName, const QString& shorthand, Settings* setup, QWidget*);
	~ServiceManager();

signals:
	void stateChanged(int);
	void tooltipChange(const QString&);
	void executablePath(const QString&);
public slots:
	void onStateChanged(int);
	void onTooltipChange(const QString&);
	void onExecutablePath(const QString&);

	void toggleService();

protected:
	void enumerateServices();
	void notifierThread();
	void onToggleService(WmiService::State);

protected:
	const QString m_serviceName;
	Ui::MainServiceFrame* m_ui = nullptr;
	std::mutex m_lock;
	std::thread m_thread;
	bool m_stopThread = false, m_serviceToggle = false;
	std::unique_ptr<WmiService> m_service{ nullptr };
};

////////////////////////////////////////////////////////////////////////////////
