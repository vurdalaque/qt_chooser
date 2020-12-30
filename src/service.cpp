#include "ui_serviceframe.h"
#include "service.h"

#include <QMetaObject>
#include <QMetaMethod>
#include <QFileIconProvider>
#include <QRegularExpression>
#include <QTimer>
#include <chrono>
#include <thread>

#include <QDebug>

////////////////////////////////////////////////////////////////////////////////

ServiceManager::ServiceManager(const QString& sn, QWidget* parent)
	: QFrame(parent)
	, m_serviceName{ sn }
	, m_ui(new Ui::MainServiceFrame)
{
	m_ui->setupUi(this);
	// m_ui->label->setText(sh);
	QObject::connect(this, SIGNAL(stateChanged(int)),
		SLOT(onStateChanged(int)), Qt::QueuedConnection);
	QObject::connect(this, SIGNAL(tooltipChange(const QString&)),
		SLOT(onTooltipChange(const QString&)), Qt::QueuedConnection);
	QObject::connect(this, SIGNAL(executablePath(const QString&)),
		SLOT(onExecutablePath(const QString&)), Qt::QueuedConnection);
	QObject::connect(m_ui->label, SIGNAL(clicked()),
		this, SLOT(toggleService()));

	QTimer::singleShot(std::chrono::milliseconds(10),
		[&]() { m_thread = std::thread(&ServiceManager::notifierThread, this); });
}

ServiceManager::~ServiceManager()
{
	m_lock.lock();
	m_stopThread = true;
	m_lock.unlock();
	if (m_thread.joinable())
		m_thread.join();
}

void ServiceManager::onStateChanged(int s)
{
	if (m_ui->label->getState() == s)
		return;
	m_ui->label->setProperty("state", s);
}

void ServiceManager::onTooltipChange(const QString& tool)
{
	m_ui->label->setToolTip(tool);
}

void ServiceManager::onExecutablePath(const QString& e)
{
	QFileIconProvider pro;
	QIcon ico = pro.icon(QFileInfo{ e });
	m_ui->label->setPixmap(ico.pixmap(QSize{ 16, 16 }));
}

void ServiceManager::toggleService()
{
	auto lock = std::lock_guard{ m_lock };
	m_serviceToggle = true;
}

void ServiceManager::enumerateServices()
{
	auto lock = std::lock_guard{ m_lock };
	try
	{
		auto services = tool::WmiObject::objects("service");
		for (const auto& service : services)
		{
			const QRegularExpression re{ m_serviceName };
			const QString serviceName{ service.property("Name").toString() };
			if (serviceName == m_serviceName || re.match(serviceName).hasMatch())
			{
				m_service = std::make_unique<WmiService>(service);

				int state = m_service->property("State").toInt();
				const QString
					regPath = m_service->property("PathName").toString().replace("\"", "").replace('\\', '/'),
					displayName = m_service->property("DisplayName").toString(),
					description = m_service->property("Description").toString();

				emit tooltipChange(QString{ "%1\n%2\n%3\n%4" }
									   .arg(displayName.isEmpty() ? QString{ "no display name available" } : displayName)
									   .arg(description.isEmpty() ? QString{ "no description available" } : description)
									   .arg(m_serviceName)
									   .arg(regPath));
				emit stateChanged(state);
				emit executablePath(m_service->path());
				break;
			}
		}
		if (m_service.get() == nullptr)
			emit stateChanged(0);
	}
	catch (const std::exception& e)
	{
		qDebug() << e.what();
	}
}

void ServiceManager::onToggleService(WmiService::State state)
{
	try
	{
		m_serviceToggle = false;

		switch (state)
		{
		case WmiService::Stopped:
			m_service->start();
			break;
		case WmiService::Running:
			m_service->stop();
			break;
		case WmiService::Paused:
			m_service->resume();
			break;
		case WmiService::Unknown:
		case WmiService::StartPending:
		case WmiService::StopPending:
		case WmiService::ContinuePending:
		case WmiService::PausePending:
			throw std::runtime_error("invalid service toggling state");
		}
	}
	catch (const std::exception& e)
	{
		qDebug() << "toggle " << e.what();
	}
	catch (...)
	{
		qDebug() << "unknown exception!";
	}
}

void ServiceManager::notifierThread()
{
	WmiService::State lastState = static_cast<WmiService::State>(-1); // -V1016
	enumerateServices();
	while (true)
	{
		{
			auto lock = std::lock_guard{ m_lock };
			if (m_stopThread || !m_service)
				break;
			try
			{
				auto state = m_service->state();

				if (m_serviceToggle)
					onToggleService(state);
				else if (state != lastState)
				{
					emit stateChanged(static_cast<int>(state));
					lastState = state;
				}
			}
			catch (const std::exception& e)
			{
				QTextStream out{ stdout };
				out << "notifierThread: " << e.what() << endl;
				m_stopThread = true;
				continue;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

////////////////////////////////////////////////////////////////////////////////

QTextStream& operator<<(QTextStream& d, WmiService::State m)
{
	switch (m)
	{
	case WmiService::Unknown:
		d << "Unknown";
		break;
	case WmiService::Stopped:
		d << "Stopped";
		break;
	case WmiService::Running:
		d << "Running";
		break;
	case WmiService::Paused:
		d << "Paused";
		break;
	case WmiService::StartPending:
		d << "StartPending";
		break;
	case WmiService::StopPending:
		d << "StopPending";
		break;
	case WmiService::ContinuePending:
		d << "ContinuePending";
		break;
	case WmiService::PausePending:
		d << "PausePending";
		break;
	}
	return d;
}
QTextStream& operator<<(QTextStream& d, WmiService::StartMode m)
{
	switch (m)
	{
	case WmiService::Boot:
		d << "Boot";
		break;
	case WmiService::System:
		d << "System";
		break;
	case WmiService::Auto:
		d << "Auto";
		break;
	case WmiService::Manual:
		d << "Manual";
		break;
	case WmiService::Disabled:
		d << "Disabled";
		break;
	}
	return d;
}

////////////////////////////////////////////////////////////////////////////////
