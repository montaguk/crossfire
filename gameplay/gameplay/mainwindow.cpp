#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <stdio.h>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QFile>
#include <QtCore>
#include <QGraphicsSceneMouseEvent>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	int i;
	ui->setupUi(this);

	// Start the connection to the robot
	ui->portName->addItem("/dev/ttyUSB0", 0);
	ui->portName->addItem("COM3", 0);
	ui->portName->addItem("COM4", 0);

	// Configure the default state of the gui indicators
	ui->slider->setMinimum(robot.get_lat_min());
	ui->slider->setMaximum(robot.get_lat_max());

	// Add the robot views to the field
	scene.setSceneRect(0, 0, FIELD_W, FIELD_H);
	rect = scene.addRect(0,FIELD_H - 10,10,10);

	// Add the line of fire to the scene
	line = scene.addLine(0,0,10,10);

	// Init array of pucks
	for (i = 0; i < NUM_PUCKS; i++) {
		puck_img[i] = 0;
		fv[i] = 0;
	}

	// Init current firing info
	fire = false;
	current_tar = 0;
	cur_fv = 0;
	current_fv = 0;
	//cur_tar = 0;
	shooting_at = NONE;
	targeting_mode = MAN;

	// Keep track of the pucks
	printf("Launching puck update thread...");
	continue_update = true;
	memset(pucks, 0, NUM_PUCKS * sizeof(QPoint *));
	puck_updater = QtConcurrent::run(this, &MainWindow::update_pucks);
	printf("Done\n");

	// Set initial states for all gui components
	update_gui();

	// Make the timer
	screen_refresh_timer = new QTimer(this);

	// Connect various slots
	connect(&robot,SIGNAL(lineReceived(QString)),
			this,SLOT(onLineReceived(QString)));

	connect(screen_refresh_timer, SIGNAL(timeout()),
			this, SLOT(refresh_field()));

	// Add event handler for grabbing mouse clicks
	scene.installEventFilter(this);

	// Start the timer
	screen_refresh_timer->start(REFRESH_RATE);
}

MainWindow::~MainWindow()
{
	printf("Kill the update thread...");
	continue_update = false;
	puck_updater.waitForFinished();
	printf("Done\n");

	printf("Stop the timer...");
	screen_refresh_timer->stop();
	delete screen_refresh_timer;
	printf("Done\n");

	for (int i = 0; i < NUM_PUCKS; i++) {
		if (pucks[i] != NULL) {
			delete pucks[i];
		}

		if (puck_img[i] != NULL) {
			delete puck_img[i];
		}

		if (fv[i] != NULL) {
			delete fv[i];
		}
	}

	if (current_tar != 0) {
		delete current_tar;
	}

	if (cur_fv != 0) {
		delete cur_fv;
	}

	if (current_fv != 0) {
		delete current_fv;
	}
	delete ui;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *ev) {
	if (obj == &scene) {
		//printf("Handling event %d\n", ev->type());
		if (ev->type() == QEvent::GraphicsSceneMousePress) {
			scene_clicked(ev);
			return true;
		} else {
			return false;
		}
	} else {
		// Pass event to parent class
		return QMainWindow::eventFilter(obj, ev);
	}
}

// Function to continuously update the current position of
// the pucks on the playing field
// This funciton is run in a separate thread using the
// QFuture class.  Continuously reads from a fifo in
// the local directory called fifo.  This should be
// created before starting the program.
void MainWindow::update_pucks() {
	int i;
	static int num_pucks = 0;

	// Open the fifo
	printf("Opening Fifo...");
	QFile fifo("./fifo");
	char buf[10] = {0};

	if(!fifo.open(QIODevice::ReadOnly)) {
		printf("Could not open fifo.\n");
		return;
	}

	printf("Done\n");
	fflush(stdout);

	// Start reading from the fifo
	// first byte on a line is puck number
	// next two bytes are x coord, followed by two bytes of y coord
	// [puck num, x1, x0, y1, y0, \n]
	while (continue_update) {		
		if (fifo.readLine(buf, 10) > 0) {
			qint8 puck_num = buf[0];

			if (puck_num == -1) {
				printf("No pucks found\n");
				continue;
			}

			printf("0x%02x 0x%02x 0x%02x 0x%02x\n", buf[1], buf[2], buf[3], buf[4]);

			quint16 x = ((buf[1] & 0x00FF) << 8) | ((buf[2]) & 0x00FF);
			quint16 y = ((buf[3] & 0x00FF) << 8) | ((buf[4]) & 0x00FF);

			printf("updating puck %d at position (%d, %d)\n", puck_num, x, y);

			if (puck_num < NUM_PUCKS) {
				if (pucks[puck_num] == 0) {
					pucks[puck_num] = new Puck(x, y);
					num_pucks++;
				} else {
					pucks[puck_num]->pos->setX(x);
					pucks[puck_num]->pos->setY(y);
				}

				// Find firing vector for this puck
				pucks[puck_num]->firing_vector =
						new QLineF(QPoint(robot.get_cur_pos(), FIELD_H - 10),
								  pucks[puck_num]->center());

				// If we are shooting at this puck, also
				// update the current target info
				if (shooting_at == puck_num) {
					cur_tar = pucks[puck_num]->pos;

					if (ui->lmove_cbox->isChecked()) {
						robot.set_tar_pos(pucks[puck_num]->center().x());
					}

					cur_fv = pucks[puck_num]->firing_vector;
				}

			}

		} else {
			printf("FIFO has been closed\n");
			break;
		}
	}

	fifo.close();
}

// Works with no more than 2 pucks
int MainWindow::find_best_target() {
	// If there are no pucks, return error
	if (!pucks[0] && !pucks[1]) {
		return -1;
	}

	// If there is only one puck, shoot at that one
	if (pucks[0] && !pucks[1]) {
		return 0;
	} else if (pucks[1] && !pucks[0]) {
		return 1;
	}

	// If there is a puck that is too close, always chose that
	// one.  If they are both too close, shoot the closest one
	int y0 = pucks[0]->center().y();
	int y1 = pucks[1]->center().y();

	if (y0 > DANGER_THRESH || y1 > DANGER_THRESH) {
		if (y0 > y1 ) {
			return 0;
		} else {
			return 1;
		}
	}

	// If there are no pucks in the danger zone, select
	// the puck that is closest to the opponents goal
	if (y0 > y1) {
		return 1;
	} else {
		return 0;
	}

	return 0;
}

// Tries to select the optimal puck to shoot at.
// Only works with a max of 2 pucks
void MainWindow::select_target() {
	int best_tar = find_best_target();

	// Set the target to the best one
	if (best_tar == 0) {
		shooting_at = PUCK1;
	} else if (best_tar == 1){
		shooting_at = PUCK2;
	} else {
		shooting_at = NONE;
	}

}

// Send string on serial line to robot
void MainWindow::on_fireButton_clicked() {
	printf("FIRE! X: %d, Deg: %d)\n", robot.get_cur_pos(), robot.get_cur_deg());
}

// Update the screen
void MainWindow::on_refreshButton_clicked() {
	update_gui();
}

// Select puck 1 as target
void MainWindow::on_puck1Button_clicked() {
	shooting_at = PUCK1;
	targeting_mode = MAN;
	fire = true;
}

// Select puck 2 as target
void MainWindow::on_puck2Button_clicked() {
	shooting_at = PUCK2;
	targeting_mode = MAN;
	fire = true;
}

// Parse input data, and update current position
// of robot
void MainWindow::onLineReceived(QString data)
{
	// TODO: Parse this data from serial packet -montaguk
	//int cur_lat_pos = 10;//robot.get_cur_pos();
	//int cur_deg = 90;//robot.get_cur_deg();

	//quint8 cur_lat_pos = (quint8)data[0].digitValue();
	//quint8 cur_deg = (quint8)data[1].digitValue();
	QStringList l = data.split(",");

	if (l.length() >= 3) {
		quint8 cur_lat_pos = l.at(0).toInt();
		quint8 cur_deg = l.at(1).toInt();
		quint8 tar_pos = l.at(2).toInt();


		printf("X: %d at %d deg, Tar: %d\n", cur_lat_pos, cur_deg, tar_pos);

		robot.set_cur_pos(cur_lat_pos);
		robot.set_cur_deg(cur_deg);

		// We need to update the firing vector if we
		// are in manual fire mode
		if (shooting_at == MANUAL) {
			if (cur_fv != 0) {
				delete cur_fv;
			}

			cur_fv = new QLineF(QPointF(robot.get_cur_pos(), FIELD_H - 10),
								*cur_tar);
		}
	}
	update_gui();
}


void MainWindow::on_openCloseButton_clicked()
{
	if(robot.isOpen())
	{
		robot.close();
		ui->openCloseButton->setText("Open");
	} else {
		robot.open(ui->portName->currentText(),9600);
		if(!robot.isOpen() || robot.errorStatus()) return;
		ui->openCloseButton->setText("Close");
	}
}

void MainWindow::on_autoSelectButton_clicked() {
	targeting_mode = AUTO;
}

// Handle signals emmitted by the slider moving
// Value is the new value of the slider
void MainWindow::on_slider_valueChanged(int value) {
	robot.set_tar_pos(value);
	//robot.set_cur_pos(value); // remove this -montaguk
	update_gui();
	//robot.move();
}

// Handle signals emmitted by the dial
// Value is the new value of the dial
// Subtract 90 deg to adjust for GUI
void MainWindow::on_tarDial_sliderMoved(int value) {
	robot.set_tar_deg(value - 90);
	//robot.set_cur_deg(value - 90); // remove this -montaguk
	update_gui();
	//robot.move();
}

// Handles clicks on the playing field
// Shoots at the location clicked
void MainWindow::scene_clicked(QEvent *ev){
	QGraphicsSceneMouseEvent *e = static_cast<QGraphicsSceneMouseEvent*>(ev);
	cur_tar = new QPoint(e->scenePos().x(), e->scenePos().y());

	printf("Mouse click at (%d, %d)\n", cur_tar->x(), cur_tar->y());

	// Set the target location for the robot
	if (ui->lmove_cbox->isChecked()) {
		robot.set_tar_pos(cur_tar->x());
	}

	cur_fv = new QLineF(QPointF(robot.get_cur_pos(), FIELD_H - 10),
						*cur_tar);

	shooting_at = MANUAL;
	targeting_mode = MAN;

	fire = true;  // start shooting
	//robot.move(); // start moving the robot
}

// Draws the lines for the current target, and
// set the angle of the turret to point at the
// target from the current location of the robot
void MainWindow::update_cur_target() {
	if (current_fv != 0) {
		scene.removeItem(current_fv);
		current_fv = 0;
	}

	if (current_tar != 0) {
		scene.removeItem(current_tar);
	}

	if (cur_fv != 0) {
		current_fv = scene.addLine(*cur_fv);
		current_tar = scene.addEllipse(cur_tar->x() - 5, cur_tar->y() - 5, 10, 10);

		int angle = 180 - cur_fv->angle();
		robot.set_tar_deg(angle);
		//robot.move();
	}
}

void MainWindow::update_curPos_label() {
	char buf[1024] = {0};
	sprintf(buf, "Cur X: %03d, Deg: %03d", robot.get_cur_pos(), robot.get_cur_deg());
	ui->curPosLabel->setText(buf);
}

void MainWindow::update_tarPos_label() {
	char buf[1024] = {0};
	sprintf(buf, "Tar X: %03d, Deg: %03d", robot.get_tar_pos(), robot.get_tar_deg());
	ui->tarPosLabel->setText(buf);
}

void MainWindow::update_shootingAt_label() {
	char buf[1024] = {0};
	sprintf(buf, "Shooting at: (%03d, %03d)", cur_tar->x(), cur_tar->y());
	ui->shootingAtLabel->setText(buf);
}

void MainWindow::update_currentTarget_label() {
	char buf[1024] = {0};
	const char *tar_str;

	if (shooting_at == PUCK1) {
		tar_str = "Puck 1";
	} else if (shooting_at == PUCK2) {
		tar_str = "Puck 2";
	} else if (shooting_at == MANUAL) {
		tar_str = "Manual";
	} else if (shooting_at == NONE) {
		tar_str = "None";
	} else {
		tar_str = "Unknown";
	}

	sprintf(buf, "Current Target: %s", tar_str);
	ui->targetLabel->setText(buf);

}

void MainWindow::update_slider() {
	ui->slider->setValue(robot.get_tar_pos());
}

// Update the dial, adjusting for the GUI
void MainWindow::update_tarDial() {
	int deg = robot.get_tar_deg();
	ui->tarDial->setValue(deg + 90);
}

// Update the dial, adjusting for the GUI
void MainWindow::update_curDial() {
	int deg = robot.get_cur_deg();
	ui->curDial->setValue(deg + 90);
}

void MainWindow::update_rect() {
	// Update robot
	rect->setX(robot.get_cur_pos() - 5);
}

void MainWindow::update_line() {
	QLineF l = line->line();

	// Update line of fire
	l.setP1(QPointF(robot.get_cur_pos(), FIELD_H - 10));
	l.setAngle(MAX_DEG - robot.get_cur_deg());
	l.setLength(10);

	scene.removeItem(line);
	line = scene.addLine(l);
}


// Update puck locations
void MainWindow::draw_pucks() {
	int i;

	// Remove existing pucks
	for (i = 0; i < NUM_PUCKS; i++) {
		// Remove pucks
		if (puck_img[i] != 0) {
			scene.removeItem(puck_img[i]);
			puck_img[i] = 0;

		}

		// Remove firing vectors
		if (fv[i] != 0) {
			scene.removeItem(fv[i]);
			fv[i] = 0;
		}
	}

	// Draw newly found pucks
	for (i = 0; i < NUM_PUCKS; i++) {
		if (pucks[i] != 0) {
			puck_img[i] =
					scene.addEllipse(pucks[i]->pos->x(),
									 pucks[i]->pos->y(), 10, 10);
			//fv[i] = scene.addLine(*pucks[i]->firing_vector);
		}
	}
}

// Update the field view
void MainWindow::update_field() {
	update_rect();
	update_line();
	draw_pucks();

	scene.update();
	ui->field->setScene(&scene);
}

void MainWindow::update_gui() {
	update_curPos_label();
	update_tarPos_label();
	update_shootingAt_label();
	update_currentTarget_label();
	update_slider();
	update_tarDial();
	update_curDial();
	update_field();
	update_cur_target();
}

void MainWindow::refresh_field() {
	robot.move();
	if (targeting_mode == AUTO) {
		select_target();
	} else {
		// Probably shouldn't go here, but works -montaguk
		ui->autoSelectButton->setChecked(false);
	}
	update_gui();
}
