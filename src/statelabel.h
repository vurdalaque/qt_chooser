#pragma once
#include <QLabel>

class QGraphicsOpacityEffect;
class QSequentialAnimationGroup;
class QPropertyAnimation;
class QMouseEvent;

////////////////////////////////////////////////////////////////////////////////

class StateLabel : public QLabel
{
	Q_OBJECT
	Q_PROPERTY(int state READ getState WRITE setState)
public:
	StateLabel(QWidget*);
	~StateLabel();

	void setState(int s);
	int getState() const;

	void animationStart();
	void animationStop();
	bool animation() const;

signals:
	void clicked();

protected:
	void mousePressEvent(QMouseEvent*) override;

protected:
	int m_state = 0;
	QSequentialAnimationGroup* m_ani = nullptr;
	QGraphicsOpacityEffect* m_pacity = nullptr;
};

////////////////////////////////////////////////////////////////////////////////

class MemoryWatchLabel : public QLabel
{
	Q_OBJECT
	Q_PROPERTY(int state READ getState WRITE setState)
public:
	MemoryWatchLabel(QWidget*);
	~MemoryWatchLabel();

	void setState(int s);
	int getState() const;

	void setWorkSet(int);

protected:
	int m_state = 0, m_workMem = 0, m_dynamicDirection = 0;
};

////////////////////////////////////////////////////////////////////////////////
