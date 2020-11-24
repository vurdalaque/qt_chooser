#include "pg_version.h"
#include "pg_conf.h"
#include "tooling.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QRegularExpression>
#include <QCommandLineParser>
#include <QProcess>
#include <QSettings>
#include <QDirIterator>

#include <exception>
#include <iostream>
#include <QDebug>
#include <stdexcept>

#ifdef Q_OS_WIN
#	define VC_EXTRALEAN
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	include <processthreadsapi.h>
#	include <psapi.h>
#	include <tlhelp32.h>
#endif

namespace helper
{
	bool pidValid(const QString& pidFilename)
	{
		QFile file{ pidFilename };
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return false;
#ifdef Q_OS_WIN
		QTextStream in(&file);
		DWORD pid = static_cast<DWORD>(in.readLine().toLong());
		file.close();

		HANDLE hProcessSnap = nullptr;
		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);
		try
		{
			hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (hProcessSnap == INVALID_HANDLE_VALUE)
				throw std::runtime_error{ "CreateToolhelp32Snapshot (of processes)" };

			if (!Process32First(hProcessSnap, &pe32))
				throw std::runtime_error{ "Process32First" };

			do
			{
				const ushort* exeFile = (ushort*)pe32.szExeFile;
				if (pe32.th32ProcessID == pid) // && QString::fromUtf16(exeFile) == "postgres.exe")
				{
					CloseHandle(hProcessSnap);
					return true;
				}
			} while (Process32Next(hProcessSnap, &pe32));

			CloseHandle(hProcessSnap);
			// return true;
		}
		catch (const std::exception&)
		{
			if (hProcessSnap != nullptr)
				::CloseHandle(hProcessSnap);
		}
#endif
		return false;
	}
} // namespace helper

namespace pg
{
	PGCluster::List PGCluster::enumerate(const QString& path)
	{
		static const QStringList excludeDirs{
			".", "..", "bin", "doc", "include",
			"pgAdmin", "pgAdmin III", "pgAdmin 4",
			"symbols", "share", "lib", "StackBuilder"
		}; // speedup database searching
		PGCluster::List result;
		QStringList dirs;
		for (const auto& entry : QDir{ path }.entryList())
		{
			QFileInfo info{ QString{ "%1/%2" }.arg(path).arg(entry) };
			if (info.isFile() && entry == "PG_VERSION")
			{
				PGCluster cluster{ path }; // may throw if
				if (cluster.valid())
					return PGCluster::List{ cluster };
			}
			if (info.isDir() && !excludeDirs.contains(entry))
				dirs.push_back(entry);
		}
		for (const auto& entry : dirs)
		{
			QFileInfo info{ QString{ "%1/%2" }.arg(path).arg(entry) };
			if (info.absoluteFilePath() == path)
				throw std::runtime_error("recursion");
			PGCluster::List list = enumerate(info.absoluteFilePath());
			for (const auto& c : list)
				result.push_back(c);
		}
		return result;
	}

	PGVersion::List PGVersion::enumerate(const QString& path)
	{
		PGVersion::List result;
		QDir loc(path);
		if (!loc.exists())
		{
			qDebug() << path;
			throw std::runtime_error{ "postgresql location isnt exists" };
		}

		if (QFileInfo{ QString{ "%1/bin/postgres.exe" }.arg(path) }.exists())
			return PGVersion::List{ PGVersion{ path } };

		for (const auto& dir : loc.entryList())
		{
			QFileInfo current{ QString{ "%1/%2" }.arg(path).arg(dir) };
			if (dir != QString{ "." }
				&& dir != QString{ ".." }
				&& current.isDir())
			{
				pg::PGVersion ver{ current.absoluteFilePath() };
				if (ver.valid())
					result.emplace_back(ver);
			}
		}
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////

	PGCluster::PGCluster(const QString& path)
		: m_path{ path }
	{
		getInfo();
		try
		{
			if (!m_version.isEmpty())
			{
				m_conf = std::make_shared<Configuration>(QString{ "%1/postgresql.conf" }.arg(m_path));
				readPostmasterOpts();
				readPostgresPid();
			}
		}
		catch (const std::exception& e)
		{
			QTextStream out{ stdout };
			out << "Failed to build PGCluster at [" << path << "] " << e.what() << endl;
			m_conf.reset();
			m_path = m_version = QString{};
			m_dbIsRunning = false;
		}
	}

	PGCluster::PGCluster(const PGCluster& c)
		: m_path{ c.m_path }
		, m_version{ c.m_version }
		, m_dbIsRunning{ c.m_dbIsRunning }
		, m_conf{ c.m_conf }
	{
	}

	PGCluster& PGCluster::operator=(const PGCluster& c)
	{
		m_path = c.m_path;
		m_version = c.m_version;
		m_dbIsRunning = c.m_dbIsRunning;
		m_conf = c.m_conf;
		return *this;
	}

	bool PGCluster::valid() const
	{
		return (m_conf != nullptr);
	}

	bool PGCluster::running() const
	{
		return m_dbIsRunning;
	}

	QString PGCluster::version() const
	{
		return m_version;
	}

	QString PGCluster::path() const
	{
		return m_path;
	}

	pConfiguration PGCluster::configuration() const
	{
		return m_conf;
	}

	bool PGCluster::compatible(const PGVersion& bins) const
	{
		if (!valid() || !bins.valid())
			throw std::runtime_error{ "cannot check cluster compability" };
		return bins.version().startsWith(m_version);
	}

	QString PGCluster::pathJunction() const
	{
		try
		{
			tool::symlink lnk{ m_path };
			return lnk.mountPoint().replace("\\", "/");
		}
		catch (const std::runtime_error&)
		{}
		return m_path;
	}

	bool PGCluster::operator==(const PGCluster& c) const
	{
		return (m_path == c.m_path);
	}

	bool PGCluster::operator!=(const PGCluster& c) const
	{
		return !(*this == c);
	}

	void PGCluster::updateDatabaseState()
	{
		if (m_path.isEmpty())
			return;

		// получаем параметры кластера
		readPostmasterOpts();
		getInfo();
		readPostgresPid();

		emit databaseStateUpdated();
	}

	void PGCluster::getInfo()
	{
		QFile file{ QString{ "%1/PG_VERSION" }.arg(m_path) };
		// выделяем версию базы данных
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return;
		QString text = QString::fromUtf8(file.readAll());
		static const QRegularExpression ver{ "^((\\d+)\\.?(\\d+)?\\.?(\\d*))" };
		auto m = ver.match(text);
		if (!m.hasMatch())
			return;
		m_version = m.captured(1);
		file.close();
	}

	void PGCluster::readPostmasterOpts()
	{
		QFile file{ QString{ "%1/postmaster.opts" }.arg(m_path) };
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return;
		QString commandLine(file.readAll());
		file.close();

		QStringList args;
		static const QRegularExpression opts{ "\"([^\"]+)\"" };
		auto m = opts.globalMatch(commandLine);
		while (m.hasNext())
		{
			auto match = m.next();
			if (args.isEmpty())
				args.append(commandLine.mid(0, match.capturedRef().position() - 1));
			args.append(match.captured(1));
		}

		QCommandLineParser parser;
		// эти параметры перегрузят значения в m_pgConf
		static const QList<QCommandLineOption> optList = {
			{ "o", "listen_address", "localhost" },
			{ "D", "data_directory", "c:/pgsql" },
			{ "N", "max_connections", "100" },
			{ "B", "shared_buffers", "128Mb" },
			{ "p", "port", "5432" },
		};
		parser.addOptions(optList);
		parser.parse(args);
		for (const auto& opt : optList)
		{
			const QString optName = opt.names().front();
			if (parser.isSet(optName) && !opt.description().isEmpty())
				m_conf->set(opt.description(), parser.value(optName));
		}
	}

	void PGCluster::readPostgresPid()
	{
		QString pidFilename = QString{ "%1/postmaster.pid" }.arg(m_path);
		auto extPid = m_conf->get("external_pid_file");
		if (!extPid.commented)
			pidFilename = extPid.dequoteValue();

		m_dbIsRunning = helper::pidValid(pidFilename);
	}

	////////////////////////////////////////////////////////////////////////////////

	PGVersion::PGVersion()
	{}

	PGVersion::PGVersion(const QString& p)
		: m_path(p)
	{
		// if path was pointed directly to bin directory
		if (QFileInfo{ QString{ "%1/postgres.exe" }.arg(m_path) }.exists())
			m_path = QFileInfo{ QString{ "%1/../" }.arg(m_path) }.absoluteFilePath();

		m_binaryPath = QFileInfo{ QString{ "%1/bin/" }.arg(m_path) }.absoluteFilePath();
		const QFileInfo
			directory{ m_binaryPath },
			binary{ QString{ "%1/postgres.exe" }.arg(m_binaryPath) };

		if (directory.exists() && directory.isDir() && binary.exists())
			getVersion();
	}

	PGVersion::PGVersion(const PGVersion& v)
		: m_path(v.m_path)
		, m_binaryPath(v.m_binaryPath)
		, m_binaryVersion(v.m_binaryVersion)
	{}

	PGVersion& PGVersion::operator=(const PGVersion& v)
	{
		m_path = v.m_path;
		m_binaryPath = v.m_binaryPath;
		m_binaryVersion = v.m_binaryVersion;
		return *this;
	}

	bool PGVersion::valid() const
	{
		return !m_binaryVersion.isEmpty();
	}

	QString PGVersion::path() const
	{
		return m_path;
	}

	QString PGVersion::binaryPath() const
	{
		return m_binaryPath;
	}

	QString PGVersion::version() const
	{
		return m_binaryVersion;
	}

	bool PGVersion::operator==(const PGVersion& v) const
	{
		return (m_binaryVersion == v.m_binaryVersion
			&& m_binaryPath == v.m_binaryPath);
	}

	bool PGVersion::operator!=(const PGVersion& v) const
	{
		return !(*this == v);
	}

	void PGVersion::getVersion()
	{
		QProcessEnvironment pe = QProcessEnvironment::systemEnvironment();
		QProcess proc;
		proc.setProcessEnvironment(pe);
		QStringList arguments;
		arguments << "--version";
		proc.setProgram(QString{ "%1/postgres.exe" }.arg(m_binaryPath));
		proc.setArguments(arguments);
		proc.setWorkingDirectory(m_binaryPath);
		proc.start();
		proc.waitForFinished();
		QString version{ proc.readAllStandardOutput() };
		QRegularExpression extract("((\\d+)\\.?(\\d+)?\\.?(\\d*))");
		m_binaryVersion = extract.match(version).captured(1);
	}

	////////////////////////////////////////////////////////////////////////////////

} /* namespace pg */
