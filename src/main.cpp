#include "mainwidget.h"
#include "settings.h"
#include "wmi_process.h"
#include "wmi_service.h"
#include "pg_version.h"

#include <QApplication>
#include <QMessageBox>
#include <QTextCodec>
#include <QDebug>

#include <chrono>
#include <exception>
#include <functional>
#include <thread>
#include <windows.h>

int main(int argc, char** argv)
{
	qRegisterMetaType<tool::WmiObject>();
	qRegisterMetaType<WmiProcess>();
	qRegisterMetaType<pWmiService>();
	qRegisterMetaType<pg::PGVersion>();
	qRegisterMetaType<pg::PGCluster>();
	qRegisterMetaType<pg::PGVersion::List>();
	qRegisterMetaType<pg::PGCluster::List>();
	QTextStream out{ stdout };
	QApplication app(argc, argv);
	Settings::setup();
	try
	{
		QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF8"));

		// disabling qt-s com initialization
		::OleUninitialize();
		tool::CoInitialize com;

		qDebug() << "hello world!";
		DesktopWidget w;
		w.show();
		return app.exec();
	}
	catch (const std::exception& e)
	{
		out << "exception: " << e.what() << endl;
	}
	catch (...)
	{
		out << "unhandled exception" << endl;
	}
	return -1;
}

