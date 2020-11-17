#pragma once
#include <QObject>
#include <QJsonObject>
#include <QVariant>

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

	Q_INVOKABLE QString configPath() const;
	Q_INVOKABLE QJsonObject& params();
	Q_INVOKABLE QVariant get(const QString&, const QVariant& = QVariant{});
	Q_INVOKABLE void set(const QString&, const QVariant&);

	static Settings* setup(SingletonPolicy = Get, const QString& = "qt_chooser.json");
signals:
	void configUpdated();
protected slots:
	void configChanged(QString const&);

protected:
	Settings(const QString&);
	void configRead();
	void configWrite();

private:
	QFileSystemWatcher* m_watcher = nullptr;
	const QString m_confName;
	QJsonObject m_config;
};

////////////////////////////////////////////////////////////////////////////////
