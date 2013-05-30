#include "puck.h"

Puck::Puck(qint16 x, qint16 y)
{
	pos = new QPoint(x, y);
	firing_vector = 0;
	//type = t;
}

Puck::~Puck() {
	if (firing_vector != 0) {
		delete firing_vector;
	}

	if (pos != NULL) {
		delete pos;
	}
}

QPoint Puck::center() {
	return QPoint (pos->x(), pos->y());
}
