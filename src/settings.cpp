#include "settings.h"

#include <QApplication>
#include <QStandardPaths>
#include <QFileSystemWatcher>
#include <QFileInfo>
#include <QJsonDocument>
#include <QDateTime>
#include <QDir>
#include <QDebug>

#include <iostream>
#include <memory>

////////////////////////////////////////////////////////////////////////////////

#include <QMessageBox>
struct Log
{
	QFile m_file;
	QString m_dateTimeFormat{ "hh:mm:ss.zzz" };
	QStringList m_logLevels{ "debug", "info", "warn", "crit", "fatal" };
	int m_logLevel{ 0 };

	static std::unique_ptr<Log> g_log;

	Log(const QString& mpath, const QString& hello)
		: m_file{ mpath }
	{
		if (!m_file.open(QIODevice::Truncate | QIODevice::WriteOnly))
			throw std::runtime_error{ "cannot open log file" };
		if (!hello.isEmpty())
			writeLogMessage(hello);
		qInstallMessageHandler(&Log::qtMessageOutput);
	}

	void writeLogMessage(const QString& msg, int logLevel = 0)
	{
		if (logLevel < m_logLevel)
			return;
		QTextStream str{ &m_file };
		auto dt = QDateTime::currentDateTime();
		str
			<< dt.toString(m_dateTimeFormat)
			<< QString{ 1, QChar{ ' ' } } << "[" << QString{ "%1" }.arg(m_logLevels[static_cast<size_t>(logLevel)], 5, QChar(' ')) << "]"
			<< QString{ 1, QChar{ ' ' } } << msg << endl;
		m_file.flush();
	}

	static void qtMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
	{
		QString qtLogString;
		QTextStream out{ &qtLogString };
		int logLevel = 0;
		switch (type)
		{
		case QtMsgType::QtDebugMsg:
			logLevel = 0;
			break;
		case QtMsgType::QtInfoMsg:
			logLevel = 1;
			break;
		case QtMsgType::QtWarningMsg:
			logLevel = 2;
			break;
		case QtMsgType::QtCriticalMsg:
			logLevel = 3;
			break;
		case QtMsgType::QtFatalMsg:
			logLevel = 4;
			break;
		}
		out << msg;
#ifdef QT_MESSAGELOGCONTEXT
		out << QString{ 15, QChar{ ' ' } } << "{" << context.file << ":" << context.line << " " << context.function << "}";
#endif
		if (g_log)
			g_log->writeLogMessage(qtLogString, logLevel);
		else
		{
			std::cout << "\n\n\n\n\n\nFUCK!!! EVERYTHING IS FUCKED UP! SHIT! DAMNT!" << std::endl;
			__asm { int 3 }
			throw std::runtime_error{ "fuckdup!" };
		}
	}
};

std::unique_ptr<Log> Log::g_log;

////////////////////////////////////////////////////////////////////////////////

Settings* Settings::setup(SingletonPolicy policy, const QString& confname)
{
	static Settings* instace{ nullptr };
	if (policy == Get && instace == nullptr)
		instace = new Settings(confname);
	if (policy == Release && instace != nullptr)
		delete instace, instace = nullptr;
	return instace;
}

////////////////////////////////////////////////////////////////////////////////

Settings::Settings(const QString& cname)
	: m_watcher(new QFileSystemWatcher(this))
	, m_confName(cname)
{
	qWarning() << "hello";
	Log::g_log = std::make_unique<Log>(logPath(), QString{});
	QFileInfo conf(configPath());
	conf.exists() ? configRead() : configWrite();
	m_watcher->addPath(configPath());

	QObject::connect(m_watcher, SIGNAL(fileChanged(const QString&)), this, SLOT(configChanged(const QString&)));
}

Settings::~Settings()
{
	configWrite();
	Log::g_log.reset();
}

QString Settings::configPath() const
{
	auto loc_list = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
	if (loc_list.isEmpty())
		throw std::runtime_error("cant locate configuration file");
	QDir loc(loc_list[0]);
	if (!loc.exists())
	{
		qInfo() << "create settings directory" << loc.absolutePath();
		loc.mkdir(".");
	}
	return QString("%1/%2").arg(loc_list[0]).arg(m_confName);
}

QString Settings::logPath() const
{
	auto loc_list = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
	if (loc_list.isEmpty())
		throw std::runtime_error("cant locate configuration file");
	QDir loc(loc_list[0]);
	if (!loc.exists())
	{
		qInfo() << "create settings directory" << loc.absolutePath();
		loc.mkdir(".");
	}
	return QString("%1/%2.log").arg(loc_list[0]).arg(QApplication::applicationName());
}

QJsonObject& Settings::params()
{
	return m_config;
}

QVariant Settings::get(const QString& name, const QVariant& def)
{
	if (!m_config.contains(name))
		m_config.insert(name, def.toJsonValue());
	return m_config.value(name);
}

void Settings::set(const QString& name, const QVariant& p)
{
	m_config[name] = p.toJsonValue();
}

void Settings::configChanged(QString const&)
{
	configRead();
	emit configUpdated();
	qInfo() << "configuration reloaded";
}

void Settings::configRead()
{
	QFile file(configPath());
	if (!file.open(QIODevice::ReadOnly))
	{
		qInfo() << "cant read defaults to: " << configPath();
		throw std::runtime_error("failed to open config file");
	}
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	params() = doc.object();
	file.close();
	qInfo() << "loaded configuration" << configPath();
}

void Settings::configWrite()
{
	QFile file(configPath());
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		throw std::runtime_error("failed to open config file (cant write defaults)");
	file.write(QJsonDocument(m_config).toJson(QJsonDocument::Indented));
	file.close();
	qInfo() << "configuration saved" << configPath();
}

