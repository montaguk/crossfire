// Provides an interface to the Wunderboard
// that is controlling the robot

#include "robot.h"
#include <stdio.h>
#include <QThread>
#include <QTimer>

Robot::Robot() : QAsyncSerial()
{
	cur_lat_pos = 0;
	cur_deg = 0;

	tar_lat_pos = 0;
	tar_deg = 0;

	firing = '0';
}

void delay(int x) {
	for (int i = 0; i <=x; i++){
		// Do nothing
	}
}


// Send command to Wunderboard to move robot
// Updates target position and target deg
void Robot::move() {

	// Serial must be open to update
	if (this->isOpen()) {

		//QString tmp;
		//tmp.append(QChar('@'));
		//tmp.append(QChar(tar_lat_pos));
		//tmp.append(QChar(tar_deg));
		//tmp.append(QChar(firing));

		// This timing is CRITICAL! Do not
		// modify unless you KNOW for sure
		// that it still works
		this->write(QString("@"));
		delay(0x2FFFFF);
		this->write(QString(tar_lat_pos));
		delay(0x2FFFFF);
		this->write(QString(tar_deg));
		delay(0x2FFFFF);
		this->write(QString(firing));
		//delay(0xFFFFF);

		//std::cout << tmp.toStdString() << std::endl;
	}

}

void Robot::set_cur_pos(int l) {
	//if (l > MAX_LAT_POS) {
		cur_lat_pos = MAX_LAT_POS;
	//} else if (l < MIN_LAT_POS) {
		cur_lat_pos = MIN_LAT_POS;
	//} else {
		cur_lat_pos = l;
	//}
}

void Robot::set_cur_deg(int d) {
	//if (d > MAX_DEG) {
		cur_deg = MAX_DEG;
	//} else if (d < MIN_DEG) {
		cur_deg = MIN_DEG;
	//} else {
		cur_deg= d;
	//}
}

void Robot::set_tar_pos(int l) {
	if (l > MAX_LAT_POS) {
		tar_lat_pos = MAX_LAT_POS;
	} else if (l < MIN_LAT_POS) {
		tar_lat_pos = MIN_LAT_POS;
	} else {
		tar_lat_pos = l;
	}
}

void Robot::set_tar_deg(int d) {
	if (d > MAX_DEG) {
		tar_deg = MAX_DEG;
	} else if (d < MIN_DEG) {
		tar_deg = MIN_DEG;
	} else {
		tar_deg= d;
	}
}

void Robot::fire() {
	firing = '1';
}

void Robot::cease_fire() {
	firing = '0';
}

void Robot::fire_one() {
	// Use timer to pulse dispenser for 15ms
	this->fire();
	QTimer::singleShot(50, this, SLOT(cease_fire()));
}

char Robot::get_firing() {
	return firing;
}

void Robot::toggle_fire() {
	if (firing == '1') {
		firing = '0';
	} else {
		firing = '1';
	}
}


int Robot::get_cur_pos() {
	return cur_lat_pos;
}

int Robot::get_cur_deg() {
	return cur_deg;
}

int Robot::get_tar_pos() {
	return tar_lat_pos;
}

int Robot::get_tar_deg() {
	return tar_deg;
}

int Robot::get_lat_min() {
	return MIN_LAT_POS;
}

int Robot::get_lat_max() {
	return MAX_LAT_POS;
}

int Robot::get_deg_max() {
	return MAX_DEG;
}

int Robot::get_deg_min() {
	return MIN_DEG;
}


