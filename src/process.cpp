#include "process.h"
#include "settings.h"
#include "ui_processframe.h"
#include "ui_processwidget.h"
#include "statelabel.h"

#include <QJsonArray>
#include <QPixmap>
#include <QFileIconProvider>
#include <QMetaMethod>

#include <QDebug>

////////////////////////////////////////////////////////////////////////////////

WmiProcess::WmiProcess(const tool::WmiObject& o)
	: tool::WmiObject(o)
{}

WmiProcess::~WmiProcess()
{}

int WmiProcess::processID() const
{
	return property("ProcessId").toInt();
}

QString WmiProcess::processName() const
{
	return property("Name").toString();
}

QString WmiProcess::executablePath() const
{
	// can be used to extract icon
	return property("ExecutablePath").toString();
}

bool WmiProcess::isTerminated() const
{
	int state = property("ExecutionState").toInt();
	qDebug() << processName() << state;
	return (state == 7 || state == 8);
}

int WmiProcess::kill()
{
	updateObject();
	int retval = -1, mi = 0, param = 0;
	const QMetaObject* o = metaObject();
	if (o == nullptr)
		throw std::runtime_error("no meta object!");
	mi = o->indexOfMethod("Terminate(uint)");
	if (mi < 0)
		throw std::runtime_error("service method not found!");
	QMetaMethod method = o->method(mi);
	if (!method.invoke(static_cast<tool::WmiObject*>(this),
			Qt::DirectConnection,
			Q_RETURN_ARG(int, retval), Q_ARG(int, param)))
		throw std::runtime_error("method call failed");
	return retval;
}

int WmiProcess::attachDebugger()
{
	updateObject();
	int retval = -1, mi = 0;
	const QMetaObject* o = metaObject();
	if (o == nullptr)
		throw std::runtime_error("no meta object!");
	mi = o->indexOfMethod("AttachDebugger()");
	if (mi < 0)
		throw std::runtime_error("service method not found!");
	QMetaMethod method = o->method(mi);
	if (!method.invoke(static_cast<tool::WmiObject*>(this),
			Qt::DirectConnection,
			Q_RETURN_ARG(int, retval)))
		throw std::runtime_error("method call failed");
	return retval;
}

////////////////////////////////////////////////////////////////////////////////

ProcessWidget::ProcessWidget(QWidget* parent)
	: QWidget(parent)
	, m_ui(new Ui::ProcessWidget)
{
	m_ui->setupUi(this);

	m_ui->closeProcess->setToolTip("Destroy Process");
	m_ui->attachDebugger->setToolTip("Attach Debugger");
	m_ui->mem->setToolTip("WorkingSetSize");
	m_ui->mem->setVisible(false); // disabled for time

	QObject::connect(m_ui->closeProcess,
		SIGNAL(clicked()),
		SLOT(onKillProcess()));
	QObject::connect(m_ui->attachDebugger,
		SIGNAL(clicked()),
		SLOT(onAttachDebugger()));
}

ProcessWidget::~ProcessWidget()
{
	delete m_ui, m_ui = nullptr;
}

void ProcessWidget::setProcessName(const QString& pn)
{
	m_ui->processName->setText(pn);
}

void ProcessWidget::setExecutablePath(const QString& e)
{
	QFileIconProvider pro;
	QIcon ico = pro.icon(QFileInfo{ e });
	m_ui->ico->setPixmap(ico.pixmap(QSize{ 16, 16 }));
}

void ProcessWidget::setProcessID(int id)
{
	m_pid = id;
}

void ProcessWidget::onKillProcess()
{
	emit killProcess(m_pid);
}

void ProcessWidget::onAttachDebugger()
{
	emit attachDebugger(m_pid);
}

void ProcessWidget::onMemoryChanged(int pid, int workset)
{
	if (m_pid != pid)
		return;
	m_ui->mem->setWorkSet(workset);
}

////////////////////////////////////////////////////////////////////////////////

ProcessManager::ProcessManager(Settings* s, QWidget* parent)
	: QFrame(parent)
	, m_ui(new Ui::ProcessMainFrame)
{
	m_ui->setupUi(this);
	QJsonObject& params = s->params();
	if (!params.contains("process") || !params.value("process").isArray())
		params.insert("process", QJsonArray{ "^qt_chooser.*$" });

	for (const auto& v : params.value("process").toArray())
		m_watchableProcess.push_back(v.toString());

	QObject::connect(this,
		SIGNAL(watchableProcess(int, const QString&, const QString&)),
		SLOT(onWatchableProcess(int, const QString&, const QString&)),
		Qt::QueuedConnection);

	QObject::connect(this,
		SIGNAL(diedProcess(int)),
		SLOT(onProcessDied(int)));

	m_ui->proclist->setSelectionMode(QAbstractItemView::NoSelection);
	m_thread = std::thread(&ProcessManager::notifierThread, this);
}

ProcessManager::~ProcessManager()
{
	m_lock.lock();
	m_stopThread = true;
	m_lock.unlock();
	if (m_thread.joinable())
		m_thread.join();
}

void ProcessManager::onWatchableProcess(int id, const QString& name, const QString& exe)
{
	auto* item = new QListWidgetItem(m_ui->proclist);
	auto* widget = new ProcessWidget(this);
	item->setData(Qt::UserRole, id);
	widget->setProcessName(name);
	widget->setExecutablePath(exe);
	widget->setProcessID(id);
	QObject::connect(widget, SIGNAL(killProcess(int)), this, SLOT(onKillProcess(int)));
	QObject::connect(widget, SIGNAL(attachDebugger(int)), this, SLOT(onAttachDebugger(int)));
	QObject::connect(this, SIGNAL(memoryChanged(int, int)),
		widget, SLOT(onMemoryChanged(int, int)));

	m_ui->proclist->setItemWidget(item, widget);
}

void ProcessManager::onProcessDied(int procID)
{
	for (int row = 0; row != m_ui->proclist->count(); ++row)
	{
		QListWidgetItem* item = m_ui->proclist->item(row);
		if (item->data(Qt::UserRole).toInt() == procID)
		{
			item = m_ui->proclist->takeItem(row);
			delete item;
			return;
		}
	}
}

void ProcessManager::onKillProcess(int id)
{
	auto lock = std::lock_guard{ m_lock };
	auto it = m_watchable.find(id);
	if (it == m_watchable.end())
		return;
	it->second.kill();
}

void ProcessManager::onAttachDebugger(int id)
{
	auto lock = std::lock_guard{ m_lock };
	auto it = m_watchable.find(id);
	if (it == m_watchable.end())
		return;
	it->second.attachDebugger();
}

void ProcessManager::monitorWatchableCreate()
{
	const auto list = tool::WmiObject::objects("process");
	std::set<int> repeatedIds;

	for (const auto& object : list)
	{
		WmiProcess proc{ object };
		auto procID = proc.processID();

		const auto it = m_watchable.find(procID);
		if (it != m_watchable.end())
		{
			repeatedIds.insert(procID);
			continue;
		}

		for (const auto& reString : m_watchableProcess)
		{
			QRegularExpression re{ reString };
			if (re.match(proc.processName()).hasMatch())
			{
				m_watchable.insert({ procID, proc });
				repeatedIds.insert(procID);
				emit watchableProcess(
					procID,
					proc.processName(),
					proc.executablePath());
			}
		}
	}
	for (auto it = m_watchable.begin(); it != m_watchable.end(); ++it)
	{
		if (repeatedIds.find(it->first) == repeatedIds.end())
		{
			emit diedProcess(it->first);
			it = m_watchable.erase(it);
		}
		// else
		// {
		// it->second.updateObject();
		// int
		// workset = it->second.property("WorkingSetSize").toInt(),
		// pid = it->second.property("ProcessId").toInt();
		// emit memoryChanged(pid, workset);
		// }
	}
}

void ProcessManager::notifierThread()
{
	while (true)
	{
		{
			auto lock = std::lock_guard{ m_lock };
			if (m_stopThread)
				break;
		}
		monitorWatchableCreate();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

////////////////////////////////////////////////////////////////////////////////
