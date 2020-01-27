#pragma once
#include <QObject>
#include <QJsonObject>

class QFileSystemWatcher;

enum SingletonPolicy
{
	Get,
	Release
};

////////////////////////////////////////////////////////////////////////////////

class Settings : public QObject
{
	Q_OBJECT

public:
	~Settings();

	QString configPath() const;
	QJsonObject& params();

	static Settings* setup(SingletonPolicy = Get);
signals:
	void configUpdated();
protected slots:
	void configChanged(QString const&);

protected:
	Settings();
	void configRead();
	void configWrite();

private:
	QFileSystemWatcher* m_watcher = nullptr;
	QJsonObject m_config;
};

////////////////////////////////////////////////////////////////////////////////
