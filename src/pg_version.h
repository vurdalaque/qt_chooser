#pragma once
#include <QObject>
#include <deque>
#include <memory>

namespace pg
{
	class Configuration;
	class PGVersion;
	using pConfiguration = std::shared_ptr<Configuration>;

	////////////////////////////////////////////////////////////////////////////////

	/** @brief object represents existing cluster directory */
	class PGCluster : public QObject
	{
		Q_OBJECT
	public:
		using List = std::deque<PGCluster>;
		PGCluster() {}
		PGCluster(const QString&);
		PGCluster(const PGCluster&);
		PGCluster& operator=(const PGCluster&);

		/** найдена связанная БД */
		bool valid() const;
		/** Запущенн ли кластер */
		bool running() const;
		/** версия постгреса, с которым был развернут кластер */
		QString version() const;
		/** путь, по которому развернут кластер */
		QString path() const;
		/** postgresql.conf */
		pConfiguration configuration() const;
		/** check if this cluster is compatible with requested binaries */
		bool compatible(const PGVersion&) const;
		/** рекурсивный поиск базы в директории */
		static PGCluster::List enumerate(const QString&);
		/** get real directory mountpoint */
		QString pathJunction() const;

		bool operator==(const PGCluster&) const;
		bool operator!=(const PGCluster&) const;
	signals:
		void databaseStateUpdated();
	public slots:
		void updateDatabaseState();

	protected:
		/** выделить версию initdb */
		void getInfo();
		/** прочесть как запускалась БД раньше */
		void readPostmasterOpts();
		/** расковырять pid файл (при наличии) */
		void readPostgresPid();

	protected:
		QString
			m_path,
			m_version;
		bool m_dbIsRunning = false;
		pConfiguration m_conf;
	};

	////////////////////////////////////////////////////////////////////////////////

	/** @brief object represents postgres installed binaries */
	class PGVersion : public QObject
	{
		Q_OBJECT
	public:
		using List = std::deque<PGVersion>;
		PGVersion();
		PGVersion(const QString&);
		PGVersion(const PGVersion&);
		PGVersion& operator=(const PGVersion&);

		bool valid() const;
		/** версия бинарников */
		QString version() const;
		QString path() const;
		/** путь до бинарников */
		QString binaryPath() const;
		bool operator==(const PGVersion&) const;
		bool operator!=(const PGVersion&) const;

		/** try to locate all available pg dirs (non recursive) */
		static List enumerate(const QString&);

	protected:
		/** версия найденных бинарников */
		void getVersion();

	protected:
		QString
			m_path,
			m_binaryPath,
			m_binaryVersion;
	};

	////////////////////////////////////////////////////////////////////////////////
} /* namespace pg */

Q_DECLARE_METATYPE(pg::PGVersion);
Q_DECLARE_METATYPE(pg::PGCluster);
Q_DECLARE_METATYPE(pg::PGVersion::List);
Q_DECLARE_METATYPE(pg::PGCluster::List);
