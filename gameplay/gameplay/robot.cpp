// Provides an interface to the Wunderboard
// that is controlling the robot

#include "robot.h"
#include <stdio.h>

Robot::Robot() : QAsyncSerial()
{
	cur_lat_pos = 0;
	cur_deg = 0;

	tar_lat_pos = 0;
	tar_deg = 0;
}

void Robot::set_cur_pos(int l) {
	if (l > MAX_LAT_POS) {
		cur_lat_pos = MAX_LAT_POS;
	} else if (l < MIN_LAT_POS) {
		cur_lat_pos = MIN_LAT_POS;
	} else {
		cur_lat_pos = l;
	}
}

void Robot::set_cur_deg(int d) {
	if (d > MAX_DEG) {
		cur_deg = MAX_DEG;
	} else if (d < MIN_DEG) {
		cur_deg = MIN_DEG;
	} else {
		cur_deg= d;
	}
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


