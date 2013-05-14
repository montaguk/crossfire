#ifndef ROBOT_H
#define ROBOT_H

#include "QAsyncSerial.h"
#include <QPoint>

#define MAX_LAT_POS 230
#define MIN_LAT_POS 0
#define MAX_DEG 180
#define MIN_DEG 0

class Robot : public QAsyncSerial
{
public:
	Robot();
	void fire();
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

	QPoint get_cur_barrel();

private:
	int cur_lat_pos;
	int cur_deg;

	int tar_lat_pos;
	int tar_deg;
};

#endif // ROBOT_H
