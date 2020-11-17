#include "pg_widget.h"
#include "settings.h"
#include "pg_conf.h"
#include "pg_version.h"
#include "wmiservice.h"
#include "ui_postgresframe.h"
#include "ui_postgreswidget.h"
#include "ui_postgresnotfound.h"
#include "ui_clusterwidget.h"

#include <QDir>
#include <QVBoxLayout>
#include <QJsonArray>
#include <QTimer>
#include <QRegularExpression>
#include <QValidator>
#include <QThreadPool>

#include <chrono>
#include <mutex>
#include <stdexcept>

#include <QDebug>
#include <thread>

////////////////////////////////////////////////////////////////////////////////

DirectoriesEnumerator::DirectoriesEnumerator()
{
}

void DirectoriesEnumerator::awake()
{
	std::unique_lock<std::mutex> lk(m_ux);
	m_continue = true;
	m_waiter.notify_one();
}

void DirectoriesEnumerator::run()
{
	const auto pathList = Settings::setup()->get("pg_dir").toJsonArray();
	for (const auto& pathObj : pathList)
	{
		const QString path = pathObj.toString().replace("\\", "/");
		const auto versions = pg::PGVersion::enumerate(path);
		for (const auto& version : versions)
			m_versions.push_back(version);
	}
	emit versionsEnumerated(m_versions);

	std::unique_lock<std::mutex> lk(m_ux);
	while (!m_continue)
		m_waiter.wait_for(lk, std::chrono::milliseconds(1000));

	for (const auto& ver : m_versions)
	{
		auto lst = pg::PGCluster::enumerate(ver.path());
		if (!lst.empty())
			emit directoriesEnumerated(ver, lst);
	}
	// now, here, we can enumerate system services to find pg services
	findPostgresServices();
}

void DirectoriesEnumerator::findPostgresServices()
{
	QTextStream out{ stdout };
	auto services = tool::WmiObject::objects("service");
	for (const auto& wmiservice : services)
	{
		auto service = std::make_shared<WmiService>(wmiservice);
		if (service->executable() != "pg_ctl.exe")
			continue;

		pg::PGVersion ver{ service->executableDirectory() };
		pg::PGCluster cluster{ service->executableKeyData("-D") };
		if (!ver.valid() || !cluster.valid())
			continue;
		emit serviceDiscovered(ver, cluster, service);
	}
}

////////////////////////////////////////////////////////////////////////////////

PostgresNotFound::PostgresNotFound(QWidget* parent)
	: QFrame{ parent }
	, m_ui{ new Ui::PostgresNotFoundWidget }
{
	m_ui->setupUi(this);
}

////////////////////////////////////////////////////////////////////////////////

ClusterWidget::ClusterWidget(const pg::PGVersion& v, const pg::PGCluster& c, QWidget* parent)
	: QFrame{ parent }
	, m_ui{ new Ui::ClusterWidget }
	, m_version{ v }
	, m_cluster{ c }
{
	m_ui->setupUi(this);
	m_ui->port->setValidator(new QRegularExpressionValidator{
		QRegularExpression{ "^([0-9]{1,4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$" } });

	updateState();
}

int ClusterWidget::getState() const
{
	if (m_service.get() != nullptr)
	{
		switch (m_service->state())
		{
		case WmiService::Running:
			return 3;
		case WmiService::Unknown:
		case WmiService::Stopped:
		case WmiService::Paused:
		case WmiService::StartPending:
		case WmiService::StopPending:
		case WmiService::ContinuePending:
		case WmiService::PausePending:
			return 2;
		};
	}
	return (m_cluster.running() ? 1 : 0);
}

void ClusterWidget::updateState()
{
	m_cluster.updateDatabaseState();
	m_ui->path->setText(m_cluster.pathJunction());

	// todo: known issue, sometimes values comes with spaces on beginning of value
	m_ui->port->setText(m_cluster.configuration()->get("port").value.trimmed());
}

void ClusterWidget::serviceDiscovered(const pg::PGVersion& v, const pg::PGCluster& c, pWmiService s)
{
	if (v != m_version || c != m_cluster)
		return;
	m_service = s;
	this->setStyleSheet(QString{});
	qDebug() << "service discovered";
}

////////////////////////////////////////////////////////////////////////////////

PostgresWidget::PostgresWidget(const pg::PGVersion& ver, PostgresManager* parent)
	: QFrame{ parent }
	, m_parent{ parent }
	, m_ui{ new Ui::PostgresWidget }
	, m_version{ ver }
{
	m_ui->setupUi(this);
	m_ui->version->setText(ver.version());
	m_ui->path->setText(ver.path());
	m_ui->versionFrame->setToolTip(ver.path());

	QVBoxLayout* layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(1);
	m_ui->dbControl->setLayout(layout);
}

pg::PGVersion& PostgresWidget::version()
{
	return m_version;
}

void PostgresWidget::addClusters(const pg::PGVersion& ver, const pg::PGCluster::List& clusters)
{
	if (ver != m_version)
		return;
	auto layout = m_ui->dbControl->layout();
	for (const auto& cluster : clusters)
	{
		auto cw = new ClusterWidget{ ver, cluster, this };
		QObject::connect(
			m_parent->directoriesEnumerator(), &DirectoriesEnumerator::serviceDiscovered,
			cw, &ClusterWidget::serviceDiscovered);
		m_clusters.push_back(cw);
		layout->addWidget(cw);
	}
}

////////////////////////////////////////////////////////////////////////////////

PostgresManager::PostgresManager(QWidget* parent)
	: QFrame{ parent }
	, m_ui{ new Ui::PostgresFrame }
	, m_enumerator{ new DirectoriesEnumerator }
{
	m_ui->setupUi(this);

	QVBoxLayout* l = new QVBoxLayout;
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing(-1);
	m_ui->list->setLayout(l);

	if (!Settings::setup()->get("pg_dir").isValid())
		throw std::runtime_error{ "no postgresql directory specified" };

	m_enumerator->setAutoDelete(true);
	QObject::connect(m_enumerator, &DirectoriesEnumerator::versionsEnumerated,
		this, &PostgresManager::versionsEnumerated);
	QThreadPool::globalInstance()->start(m_enumerator);
}

PostgresManager::~PostgresManager()
{
}

DirectoriesEnumerator* PostgresManager::directoriesEnumerator() const
{
	return m_enumerator;
}

void PostgresManager::versionsEnumerated(const pg::PGVersion::List& versions)
{
	auto layout = m_ui->list->layout();
	if (!layout)
		throw std::runtime_error{ "must be existing layout" };
	for (const auto& version : versions)
	{
		auto pw = new PostgresWidget{ version, this };
		QObject::connect(m_enumerator, &DirectoriesEnumerator::directoriesEnumerated,
			pw, &PostgresWidget::addClusters);
		m_versions.emplace_back(pw);
		layout->addWidget(pw);
	}
	if (m_versions.empty())
		layout->addWidget(new PostgresNotFound{ this });
	m_enumerator->awake();
}

////////////////////////////////////////////////////////////////////////////////
