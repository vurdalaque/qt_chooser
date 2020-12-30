#pragma once
#include "wmi_service.h"

#include <QFrame>
#include <QTextStream>

#include <mutex>
#include <thread>

class Settings;
namespace Ui
{
	class MainServiceFrame;
}

////////////////////////////////////////////////////////////////////////////////

class ServiceManager
	: public QFrame
{
	Q_OBJECT

public:
	ServiceManager(const QString& serviceName, QWidget*);
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

QTextStream& operator<<(QTextStream&, WmiService::State);
QTextStream& operator<<(QTextStream&, WmiService::StartMode);

////////////////////////////////////////////////////////////////////////////////
