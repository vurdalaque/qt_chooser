#pragma once
#include <QFrame>

class Settings;
namespace tool
{
	class WmiObject;
	struct CoInitialize;
}

namespace Ui
{
	class SymLinkFrame;
	class MainServiceFrame;
	class MainMoveFrame;
}

////////////////////////////////////////////////////////////////////////////////

class MoveArea : public QFrame
{
	Q_OBJECT
public:
	MoveArea(QWidget*);
	~MoveArea();

signals:
	void resizePressed(bool);
protected slots:
	void onClose();

protected:
	void mousePressEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;
	void mouseMoveEvent(QMouseEvent*) override;
	bool eventFilter(QObject*, QEvent*) override;

protected:
	Ui::MainMoveFrame* m_ui = nullptr;
	QPoint m_ouse;
	bool m_resize{ false };
};

////////////////////////////////////////////////////////////////////////////////

class SymlinkinQtWidget : public QFrame
{
	Q_OBJECT
public:
	SymlinkinQtWidget(Settings* setup, QWidget*);
	~SymlinkinQtWidget();

protected slots:
	void changeVersion(int);

protected:
	void initDirs();
	QString extsdkQtPath() const;

protected:
	Ui::SymLinkFrame* m_ui = nullptr;
	QString
		m_mountPoint,
		m_qtdir,
		m_extsdk;
};

////////////////////////////////////////////////////////////////////////////////

class DesktopWidget : public QFrame
{
	Q_OBJECT
public:
	DesktopWidget(QWidget* = 0);
	~DesktopWidget();

protected slots:
	void configUpdated();
	void init();
	void cleanup();

protected:
	void keyReleaseEvent(QKeyEvent*) override;

protected:
	Settings* m_setup = nullptr;
};

////////////////////////////////////////////////////////////////////////////////
