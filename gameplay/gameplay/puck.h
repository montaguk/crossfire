#ifndef PUCK_H
#define PUCK_H


#include <QPoint>
#include <QLineF>
#include <QString>

class Puck
{


public:
	Puck(qint16 x, qint16 y, QString t);
	QPoint *pos;
	QLineF *firing_vector; // Vector from robot to puck
	QString type;
	~Puck();

	QPoint center();
};

#endif // PUCK_H
