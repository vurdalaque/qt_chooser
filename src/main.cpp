#include "mainwidget.h"
#include "src/wmi.h"
#include "wmiservice.h"
#include "pg_version.h"

#include <QApplication>
#include <QMessageBox>
#include <QTextCodec>
#include <QDebug>

#include <exception>
#include <functional>
#include <windows.h>

class MyApplication : public QApplication
{
public:
	MyApplication(int argc, char** argv)
		: QApplication{ argc, argv } {}

	virtual bool notify(QObject* receiver, QEvent* e) override
	{
		try
		{
			return QApplication::notify(receiver, e);
		}
		catch (const std::exception& e)
		{
			qDebug() << "exception: " << e.what();
		}
		catch (...)
		{
			qDebug() << "unhandled";
		}
		return false;
	}
};

int main(int argc, char** argv)
{
	qRegisterMetaType<tool::WmiObject>();
	qRegisterMetaType<pWmiService>();
	qRegisterMetaType<pg::PGVersion>();
	qRegisterMetaType<pg::PGCluster>();
	qRegisterMetaType<pg::PGVersion::List>();
	qRegisterMetaType<pg::PGCluster::List>();
	QTextStream out{ stdout };
	MyApplication app(argc, argv);
	out << "starting " << qApp->applicationName() << endl;
	try
	{
		QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF8"));

		// disabling qt-s com initialization
		::OleUninitialize();
		tool::CoInitialize com;

		DesktopWidget w;
		w.show();
		return app.exec();
	}
	catch (const std::exception& e)
	{
		out << e.what() << endl;
	}
	catch (...)
	{
		out << "unhandled exception" << endl;
	}
	qDebug() << "ok";
	return -1;
}

