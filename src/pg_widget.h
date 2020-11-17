#pragma once
#include "pg_version.h"
#include "wmiservice.h"

#include <QFrame>
#include <QRunnable>

#include <mutex>
#include <deque>

class Settings;
class QTimer;
class PostgresManager;
namespace Ui
{
	class PostgresWidget;
	class PostgresFrame;
	class ClusterWidget;
	;
	class PostgresNotFoundWidget;
} /* namespace Ui */

////////////////////////////////////////////////////////////////////////////////

class DirectoriesEnumerator : public QObject, public QRunnable
{
	Q_OBJECT
public:
	DirectoriesEnumerator();
	void awake();
	void run() override;
	void findPostgresServices();

signals:
	void versionsEnumerated(const pg::PGVersion::List&);
	void directoriesEnumerated(const pg::PGVersion&, const pg::PGCluster::List&);
	void serviceDiscovered(const pg::PGVersion&, const pg::PGCluster&, pWmiService);

private:
	std::mutex m_ux;
	bool m_continue{ false };
	std::condition_variable m_waiter;
	pg::PGVersion::List m_versions;
};

////////////////////////////////////////////////////////////////////////////////

class PostgresNotFound : public QFrame
{
	Q_OBJECT
public:
	PostgresNotFound(QWidget* = nullptr);

protected:
	Ui::PostgresNotFoundWidget* m_ui{ nullptr };
};

////////////////////////////////////////////////////////////////////////////////

class ClusterWidget : public QFrame
{
	Q_OBJECT
	Q_PROPERTY(int state READ getState)
public:
	ClusterWidget(const pg::PGVersion&, const pg::PGCluster&, QWidget* = nullptr);

	int getState() const;
public slots:
	void updateState();
	void serviceDiscovered(const pg::PGVersion&, const pg::PGCluster&, pWmiService);

protected:
	Ui::ClusterWidget* m_ui{ nullptr };
	pg::PGVersion m_version;
	pg::PGCluster m_cluster;
	pWmiService m_service;
};

////////////////////////////////////////////////////////////////////////////////

class PostgresWidget : public QFrame
{
	Q_OBJECT
public:
	PostgresWidget(const pg::PGVersion&, PostgresManager*);
	pg::PGVersion& version();

public slots:
	void addClusters(const pg::PGVersion&, const pg::PGCluster::List&);

protected:
	PostgresManager* m_parent{ nullptr };
	Ui::PostgresWidget* m_ui{ nullptr };
	pg::PGVersion m_version;
	std::deque<ClusterWidget*> m_clusters;
};

////////////////////////////////////////////////////////////////////////////////

class PostgresManager : public QFrame
{
	Q_OBJECT
public:
	PostgresManager(QWidget* = nullptr);
	~PostgresManager();

	DirectoriesEnumerator* directoriesEnumerator() const;

protected slots:
	void versionsEnumerated(const pg::PGVersion::List&);

protected:
	Ui::PostgresFrame* m_ui{ nullptr };
	QTimer* m_updateTimer{ nullptr };
	std::deque<PostgresWidget*> m_versions;
	DirectoriesEnumerator* m_enumerator{ nullptr };
};

////////////////////////////////////////////////////////////////////////////////
