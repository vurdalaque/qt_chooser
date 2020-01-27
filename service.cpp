#include "ui_serviceframe.h"
#include "service.h"

#include <QMetaObject>
#include <QMetaMethod>
#include <QFileIconProvider>
#include <QRegularExpression>

////////////////////////////////////////////////////////////////////////////////

WmiService::WmiService(const tool::WmiObject& o)
	: tool::WmiObject(o)
{
}

const QString WmiService::path() const
{
	static const QRegularExpression startWithQuotes{ "^\"([^\"]+).*$" },
		plainRun{ "^([^ ]+).*$" };
	const QString p = property("PathName").toString();
	return (p.startsWith('\"')
			? startWithQuotes.match(p).captured(1)
			: plainRun.match(p).captured(1))
		.replace('\\', '/');
}

WmiService::State WmiService::state()
{
	static const std::map<QString, State> serviceStatuses{
		{ "Stopped", Stopped },
		{ "Start Pending", StartPending },
		{ "Stop Pending", StopPending },
		{ "Running", Running },
		{ "Continue Pending", ContinuePending },
		{ "Pause Pending", PausePending },
		{ "Paused", Paused },
		{ "Unknown", Unknown },
	};
	updateObject();
	const auto stateString = this->property("State").toString();
	const auto s = serviceStatuses.find(stateString);
	if (s == serviceStatuses.end())
		throw std::runtime_error("no statuse found");
	return s->second;
}

int WmiService::start()
{
	return runMethod("StartService()");
}

int WmiService::stop()
{
	return runMethod("StopService()");
}

int WmiService::resume()
{
	return runMethod("ResumeService()");
}

int WmiService::runMethod(const QByteArray& signature)
{
	updateObject();
	int retval = -1, mi = 0;
	const QMetaObject* o = metaObject();
	mi = o->indexOfMethod(signature);
	if (mi < 0)
		throw std::runtime_error("service method not found!");
	QMetaMethod method = o->method(mi);
	if (!method.invoke(static_cast<tool::WmiObject*>(this),
			Qt::DirectConnection,
			Q_RETURN_ARG(int, retval)))
		throw std::runtime_error("method call failed");
	return retval;
}

////////////////////////////////////////////////////////////////////////////////

ServiceManager::ServiceManager(const QString& sn, const QString& sh, Settings* setup, QWidget* parent)
	: QFrame(parent)
	, m_serviceName(sn)
	, m_ui(new Ui::MainServiceFrame)
{
	m_ui->setupUi(this);
	m_ui->label->setText(sh);
	QObject::connect(this, SIGNAL(stateChanged(int)),
		SLOT(onStateChanged(int)), Qt::QueuedConnection);
	QObject::connect(this, SIGNAL(tooltipChange(const QString&)),
		SLOT(onTooltipChange(const QString&)), Qt::QueuedConnection);
	QObject::connect(this, SIGNAL(executablePath(const QString&)),
		SLOT(onExecutablePath(const QString&)), Qt::QueuedConnection);
	QObject::connect(m_ui->label, SIGNAL(clicked()),
		this, SLOT(toggleService()));

	m_thread = std::thread(&ServiceManager::notifierThread, this);
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
			if (service.property("Name") == m_serviceName)
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
			auto state = m_service->state();

			if (m_serviceToggle)
				onToggleService(state);
			else if (state != lastState)
			{
				emit stateChanged(static_cast<int>(state));
				lastState = state;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

////////////////////////////////////////////////////////////////////////////////
