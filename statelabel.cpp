#include "statelabel.h"

#include <QStyle>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QMouseEvent>

#include <QDebug>

////////////////////////////////////////////////////////////////////////////////

StateLabel::StateLabel(QWidget* parent)
	: QLabel(parent)
	, m_ani(new QSequentialAnimationGroup)
	, m_pacity(new QGraphicsOpacityEffect(this))
{
	m_pacity->setOpacity(1.0);
	m_pacity->setEnabled(true);
	this->setGraphicsEffect(m_pacity);

	QPropertyAnimation
		*fadeIn = new QPropertyAnimation(this),
		*fadeOut = new QPropertyAnimation(this);
	fadeIn->setPropertyName("opacity");
	fadeIn->setTargetObject(m_pacity);
	fadeIn->setDuration(500);
	fadeIn->setStartValue(0.5);
	fadeIn->setEndValue(1.0);

	fadeOut->setPropertyName("opacity");
	fadeOut->setTargetObject(m_pacity);
	fadeOut->setDuration(500);
	fadeOut->setStartValue(1.0);
	fadeOut->setEndValue(0.5);

	m_ani->addAnimation(fadeIn);
	m_ani->addAnimation(fadeOut);
	m_ani->setLoopCount(-1);
}

StateLabel::~StateLabel()
{
	if (m_ani != nullptr)
		m_ani->deleteLater();
}

void StateLabel::setState(int s)
{
	if (m_state == s)
		return;
	if (s > 3 /* Paused */)
	{
		m_state = (s - 3);
		animationStart();
	}
	else
	{
		animationStop();
		m_state = s;
	}
	style()->unpolish(this);
	style()->polish(this);
}

int StateLabel::getState() const
{
	return m_state;
}

void StateLabel::animationStart()
{
	m_ani->start();
}

bool StateLabel::animation() const
{
	return (m_ani != nullptr && m_ani->state() == QAbstractAnimation::Running);
}

void StateLabel::animationStop()
{
	if (animation())
	{
		m_ani->stop();
		m_pacity->setOpacity(1.);
	}
}

void StateLabel::mousePressEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton && !animation())
		emit clicked();
}

////////////////////////////////////////////////////////////////////////////////

MemoryWatchLabel::MemoryWatchLabel(QWidget* w)
	: QLabel(w)
{}

MemoryWatchLabel::~MemoryWatchLabel()
{}

void MemoryWatchLabel::setState(int s)
{
	if (m_state == s)
		return;
	m_state = s;
	style()->unpolish(this);
	style()->polish(this);
}

int MemoryWatchLabel::getState() const
{
	return m_state;
}

void MemoryWatchLabel::setWorkSet(int workset)
{
	int
		mbytes = workset / (1024 * 1024),
		kbytes = (workset % (1024 * 1024)) % 1000;
	this->setText(QString{ "%1.%2 Mb " }.arg(mbytes).arg(kbytes));

	if (m_workMem == 0)
	{
		m_workMem = workset;
		return;
	}
	int diff = (workset - m_workMem);
	if (std::abs(diff) > 1024 * 5)
		this->setProperty("state", diff > 0 ? -1 : 1);
	else
	{
		if (this->property("state") != 0)
			this->setProperty("state", 0);
	}
	if (std::abs(diff) > (1024 * 50))
		m_workMem = workset;
}

////////////////////////////////////////////////////////////////////////////////
