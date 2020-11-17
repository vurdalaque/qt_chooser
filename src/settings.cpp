#include "settings.h"

#include <QStandardPaths>
#include <QFileSystemWatcher>
#include <QFileInfo>
#include <QJsonDocument>
#include <QDir>
#include <QDebug>

#include <iostream>

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
	QFileInfo conf(configPath());
	conf.exists() ? configRead() : configWrite();
	m_watcher->addPath(configPath());

	QObject::connect(m_watcher, SIGNAL(fileChanged(const QString&)), this, SLOT(configChanged(const QString&)));
}

Settings::~Settings()
{
	configWrite();
}

QString Settings::configPath() const
{
	auto loc_list = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
	if (loc_list.isEmpty())
		throw std::runtime_error("cant locate configuration file");
	QDir loc(loc_list[0]);
	if (!loc.exists())
	{
		// qDebug() << "create settings directory" << loc.absolutePath();
		loc.mkdir(".");
	}
	return QString("%1/%2").arg(loc_list[0]).arg(m_confName);
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
	// qDebug() << "configuration reloaded";
}

void Settings::configRead()
{
	QFile file(configPath());
	if (!file.open(QIODevice::ReadOnly))
	{
		// qDebug() << "cant read defaults to: " << configPath();
		throw std::runtime_error("failed to open config file");
	}
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	params() = doc.object();
	file.close();
	// qDebug() << "loaded configuration" << configPath();
}

void Settings::configWrite()
{
	QFile file(configPath());
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		throw std::runtime_error("failed to open config file (cant write defaults)");
	file.write(QJsonDocument(m_config).toJson(QJsonDocument::Indented));
	file.close();
	// qDebug() << "configuration saved" << configPath();
}

