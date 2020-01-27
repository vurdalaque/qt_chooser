#include "helper.h"
#include "wmi.h"

#include <QApplication>
#include <QTextCodec>

#include <windows.h>

int main(int argc, char** argv)
{
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF8"));
	QApplication app(argc, argv);

	// disabling qt-s com initialization
	::OleUninitialize();
	tool::CoInitialize com;

	DesktopWidget w;
	w.show();

	return app.exec();
}

