#include "mainwidget.h"
#include "settings.h"
#include "tooling.h"
#include "service.h"
#include "process.h"
#include "pg_widget.h"

#include "ui_symlinkframe.h"
#include "ui_moveframe.h"

#include <QJsonArray>
#include <QFileInfo>
#include <QLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QDir>

#include <QDebug>
#include <stdexcept>

////////////////////////////////////////////////////////////////////////////////

MoveArea::MoveArea(QWidget* parent)
	: QFrame(parent)
	, m_ui(new Ui::MainMoveFrame)
{
	m_ui->setupUi(this);
	QObject::connect(m_ui->closeButton, SIGNAL(clicked()), this, SLOT(onClose()));
	setMouseTracking(true);
	m_ui->resizeButton->installEventFilter(this);
}

MoveArea::~MoveArea()
{
	delete m_ui, m_ui = nullptr;
}

void MoveArea::mousePressEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
	{
		m_ouse = e->globalPos();
	}
}

void MoveArea::mouseReleaseEvent(QMouseEvent*)
{
	m_ouse = QPoint();
}

void MoveArea::mouseMoveEvent(QMouseEvent* e)
{
	if (m_ouse != QPoint())
	{
		QWidget* top = parentWidget();
		QPoint global = e->globalPos();
		if (!m_resize)
		{
			QPoint pos(
				top->pos().x() + (global.x() - m_ouse.x()),
				top->pos().y() + (global.y() - m_ouse.y()));
			top->move(pos);
		}
		else
		{
			QPoint diff = global - m_ouse;
			auto rc = top->geometry();
			rc.setTopRight(rc.topRight() + diff);
			top->setGeometry(rc);
		}
		m_ouse = global;
	}
}

void MoveArea::onClose()
{
	parentWidget()->close();
}

bool MoveArea::eventFilter(QObject* obj, QEvent* ev)
{
	if (obj == qobject_cast<QObject*>(m_ui->resizeButton))
	{
		// catching events from resizeButton
		auto me = dynamic_cast<QMouseEvent*>(ev);
		if (ev->type() == QEvent::MouseButtonPress)
		{
			m_resize = true;
			emit resizePressed(m_resize);
			mousePressEvent(me);
		}
		if (ev->type() == QEvent::MouseButtonRelease)
		{
			m_resize = false;
			emit resizePressed(m_resize);
			mouseReleaseEvent(nullptr);
		}
		if (ev->type() == QEvent::MouseMove && m_resize)
			mouseMoveEvent(me);
	}
	return QFrame::eventFilter(obj, ev);
}

////////////////////////////////////////////////////////////////////////////////

SymlinkinQtWidget::SymlinkinQtWidget(Settings* setup, QWidget* parent)
	: QFrame(parent)
	, m_ui(new Ui::SymLinkFrame)
	, m_qtdir(QString{ qgetenv("QTDIR") }.replace('\\', '/'))
{
	m_ui->setupUi(this);

	QJsonObject& param = setup->params();
	if (!param.contains("mountPoint"))
		param.insert("mountPoint", "c:/qt");
	m_mountPoint = param.value("mountPoint").toString();
	if (m_qtdir.isEmpty())
	{
		if (!param.contains("qtdir"))
			param.insert("qtdir", m_mountPoint + "/qtbase");
		m_qtdir = param.value("qtdir").toString();
	}

	if (!param.contains("extsdk"))
		param.insert("extsdk", "D:/projects/extsdk/trunk/qt");
	m_extsdk = param.value("extsdk").toString();

	QString errorText;
	if (!QFileInfo(m_mountPoint).exists())
	{
		QStringList lst = m_mountPoint.split("/"),
					pathCreate{ lst.front() }; // disk letter
		for (int idx = 1; idx != lst.size(); ++idx)
		{
			if (lst.at(idx).isEmpty())
				continue;
			pathCreate.append(lst.at(idx));
			const QString path{ pathCreate.join("/") };
			const QDir dir{ path };
			if (!dir.mkdir("."))
			{
				errorText += QString{ "cannot create %1" }.arg(pathCreate.join("/"));
				break;
			}
		}
	}

	if (!QFileInfo(m_extsdk).exists())
		errorText += (QString{ errorText.isEmpty() ? "" : "\n" } + "no qt installation directory " + m_extsdk);

	if (!errorText.isEmpty())
	{
		m_ui->qtPath->setText(errorText);
		this->setToolTip(errorText);
		this->setEnabled(false);
	}
	else
		initDirs();

	QObject::connect(m_ui->currentVersion, SIGNAL(currentIndexChanged(int)),
		this, SLOT(changeVersion(int)));
	if (!errorText.isEmpty())
		qWarning() << "error text: " << errorText;
}

SymlinkinQtWidget::~SymlinkinQtWidget()
{
	delete m_ui, m_ui = nullptr;
}

void SymlinkinQtWidget::initDirs()
{
	m_ui->qtPath->setText(m_qtdir);

	int idx = 0;
	qDebug() << "initDirs";
	for (const auto& dir : QDir{ extsdkQtPath() }.entryList())
	{
		if (dir != QString{ "." } && dir != QString{ ".." })
		{
			qDebug() << "add version: " << dir;
			m_ui->currentVersion->insertItem(idx, dir);
			++idx;
		}
	}
	if (!QFileInfo{ m_qtdir }.exists() && m_ui->currentVersion->count() > 0)
		changeVersion(0);
	try
	{
		qInfo() << "mount point: " << m_mountPoint;
		tool::symlink qtdir{ m_mountPoint };
		const QString currentVersion = qtdir.mountPoint().split('/').back();
		for (int i = 0; i != m_ui->currentVersion->count(); ++i)
			if (m_ui->currentVersion->itemText(i) == currentVersion)
				m_ui->currentVersion->setCurrentIndex(i);
		m_ui->qtPath->setVisible(false);
	}
	catch (const std::exception& e)
	{
		const QString errorMessage = QString{ "Error occured: %1" }.arg(e.what());
		this->setToolTip(errorMessage);
		m_ui->qtPath->setText(errorMessage);
		this->setEnabled(false);
		m_ui->currentVersion->clear();
		m_ui->qtPath->setVisible(true);
	}
}

QString SymlinkinQtWidget::extsdkQtPath() const
{
	return m_extsdk;
}

void SymlinkinQtWidget::changeVersion(int idx)
{
	QString nupath = QString{ "%1/%2" }
						 .arg(m_extsdk)
						 .arg(m_ui->currentVersion->itemText(idx));

	qInfo() << "switching versions: " << nupath << " to " << m_mountPoint;
	tool::symlink l{ m_mountPoint };
	l.unlink();
	l.link(nupath);
}

////////////////////////////////////////////////////////////////////////////////

DesktopWidget::DesktopWidget(QWidget* parent)
	: QFrame(parent)
	, m_setup(Settings::setup(Get))
{
	setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);

	if (!QObject::connect(m_setup, &Settings::configUpdated, this, &DesktopWidget::configUpdated))
		throw std::runtime_error("connect failed");
	setLayout(new QHBoxLayout);
	layout()->setContentsMargins(0, 0, 0, 0);
	layout()->setSpacing(0);
	init();
}

DesktopWidget::~DesktopWidget()
{
	QObject::disconnect(m_setup, &Settings::configUpdated, this, &DesktopWidget::configUpdated);

	QJsonObject& param = m_setup->params();
	QRect g = this->geometry();
	param["geometry"] = QJsonArray{ g.x(), g.y(), g.width(), g.height() };
	m_setup = nullptr;
	Settings::setup(Release);
}

void DesktopWidget::configUpdated()
{
}

void DesktopWidget::init()
{
	QJsonObject& param = m_setup->params();
	if (param.contains("geometry") && param.value("geometry").isArray())
	{
		QJsonArray ga = param.value("geometry").toArray();
		if (ga.size() != 4)
			throw std::runtime_error("invalid geometry param");
		this->setGeometry(
			ga[0].toInt(),
			ga[1].toInt(),
			ga[2].toInt(),
			ga[3].toInt());
	}

	QFile stylesheet{ ":/stylesheet.css" };
	if (!stylesheet.exists() || !stylesheet.open(QIODevice::ReadOnly))
		throw std::runtime_error{ "no default stylesheet exists" };
	this->setStyleSheet(param.contains("css") ? param.value("css").toString() : stylesheet.readAll());
	stylesheet.close();

	if (!param.contains("services"))
		param.insert("services", QJsonArray{});
	if (!param.value("services").isArray())
		throw std::runtime_error("service parameter must be array");

	QHBoxLayout* h = new QHBoxLayout;
	QVBoxLayout* v = new QVBoxLayout;

	h->setContentsMargins(0, 0, 0, 0);
	h->setSpacing(0);
	v->setContentsMargins(0, 0, 0, 0);
	v->setSpacing(0);

	for (const auto& value : param.value("services").toArray())
	{
		if (!value.isString())
			throw std::runtime_error("service must be object");
		const QString serviceName = value.toString();
		h->addWidget(new ServiceManager{ serviceName, this });
	}
	h->addSpacerItem(new QSpacerItem{ 0, 0, QSizePolicy::Expanding, QSizePolicy::Maximum });
	h->addWidget(new SymlinkinQtWidget{ m_setup, this });

	if (!param.contains("pg_dir"))
		param.insert("pg_dir", QJsonArray{});
	v->addWidget(new PostgresManager{ this });

	if (!param.contains("enableProcManager"))
		param.insert("enableProcManager", false);
	if (param.value("enableProcManager").toBool())
		v->addWidget(new ProcessManager{ m_setup, this });

	v->addItem(h);

	this->layout()->addItem(v);
	this->layout()->addWidget(new MoveArea{ this });
}

void DesktopWidget::cleanup()
{
	if (this->layout() == nullptr)
		return;

	QLayoutItem* item = nullptr;
	while ((item = this->layout()->takeAt(0)) != nullptr)
		delete item;
}

void DesktopWidget::keyReleaseEvent(QKeyEvent* e)
{
	if (e->key() == Qt::Key_Escape && !m_setup->get("ignore_esc", false).toBool())
		this->close();
}

////////////////////////////////////////////////////////////////////////////////
