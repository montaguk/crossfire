#ifndef ROBOT_H
#define ROBOT_H

#include "QAsyncSerial.h"
#include <QPoint>

#define MAX_LAT_POS 245
#define MIN_LAT_POS 7
#define MAX_DEG 140
#define MIN_DEG 40

class Robot : public QAsyncSerial
{
	Q_OBJECT

public:
	Robot();
	void move();

	void set_cur_pos(int l);
	void set_cur_deg(int d);
	void set_tar_pos(int l);
	void set_tar_deg(int d);

	int get_cur_pos();
	int get_cur_deg();
	int get_tar_pos();
	int get_tar_deg();

	int get_lat_min();
	int get_lat_max();
	int get_deg_min();
	int get_deg_max();

	char get_firing();

	QPoint get_cur_barrel();

private:
	int cur_lat_pos;
	int cur_deg;

	int tar_lat_pos;
	int tar_deg;

	char firing;

public slots:
	void cease_fire();
	void fire_one();
	void fire();
	void toggle_fire();
};

#endif // ROBOT_H
