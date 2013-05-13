#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <stdio.h>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QFile>
#include <QtCore>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
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

	// Set initial states for all gui components
	update_gui();

	// Keep track of the pucks
	printf("Launching puck update thread...");
	continue_update = true;
	memset(pucks, 0, NUM_PUCKS * sizeof(QPoint *));
	puck_updater = QtConcurrent::run(this, &MainWindow::update_pucks);
	printf("Done\n");

	// Connect various slots
	connect(&robot,SIGNAL(lineReceived(QString)),
			this,SLOT(onLineReceived(QString)));

	// Setup listening socket for remote / automated control


}

MainWindow::~MainWindow()
{
	printf("Kill the update thread...");
	continue_update = false;
	puck_updater.waitForFinished();
	printf("Done\n");

	for (int i = 0; i < NUM_PUCKS; i++) {
		if (pucks[i] != NULL) {
			delete pucks[i];
		}
	}
	delete ui;
}

// Function to continuously update the current position of
// the pucks on the playing field
// This funciton is run in a separate thread using the
// QFuture class.  Continuously reads from a fifo in
// the local directory called fifo.  This should be
// created before starting the program.
void MainWindow::update_pucks() {
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
				printf("no pucks found\n");
				continue;
			}

			qint16 x = (buf[1] << 8) | (buf[2]);
			qint16 y = (buf[3] << 8) | (buf[4]);

			printf("updating puck %d at position (%d, %d)\n", puck_num, x, y);

			if (pucks[puck_num] == 0) {
				pucks[puck_num] = new QPoint(x, y);
			} else {
				pucks[puck_num]->setX(x);
				pucks[puck_num]->setY(y);
			}
		} else {
			printf("No bytes read\n");
			break;
		}
	}

	fifo.close();
}

// Send string on serial line to robot
void MainWindow::on_fireButton_clicked() {
	printf("FIRE! X: %d, Deg: %d)\n", robot.get_cur_pos(), robot.get_cur_deg());
}

// Update the screen
void MainWindow::on_refreshButton_clicked() {
	update_gui();
}

// Parse input data, and update current position
// of robot
void MainWindow::onLineReceived(QString data)
{
	// TODO: Parse this data from serial packet -montaguk
	int cur_lat_pos = 10;//robot.get_cur_pos();
	int cur_deg = 90;//robot.get_cur_deg();

	robot.set_cur_pos(cur_lat_pos);
	robot.set_cur_deg(cur_deg);

	update_gui();
}


void MainWindow::on_openCloseButton_clicked()
{
	if(robot.isOpen())
	{
		robot.close();
		ui->openCloseButton->setText("Open");
	} else {
		robot.open(ui->portName->currentText(),115200);
		if(!robot.isOpen() || robot.errorStatus()) return;
		ui->openCloseButton->setText("Close");
	}
}

// Handle signals emmitted by the slider moving
// Value is the new value of the slider
void MainWindow::on_slider_sliderMoved(int value) {
	robot.set_tar_pos(value);
	robot.set_cur_pos(value); // remove this -montaguk
	update_gui();
}

// Handle signals emmitted by the dial
// Value is the new value of the dial
// Subtract 90 deg to adjust for GUI
void MainWindow::on_tarDial_sliderMoved(int value) {
	robot.set_tar_deg(value - 90);
	robot.set_cur_deg(value - 90); // remove this -montaguk
	update_gui();
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
	rect->setX(robot.get_cur_pos());
}

void MainWindow::update_line() {
	QLineF l = line->line();

	// Update line of fire
	l.setP1(QPointF(robot.get_cur_pos() + 5, FIELD_H - 10));
	l.setAngle(MAX_DEG - robot.get_cur_deg());
	l.setLength(10);

	scene.removeItem(line);
	line = scene.addLine(l);
}

// Update the field view
void MainWindow::update_field() {
	update_rect();
	update_line();

	scene.update();
	ui->field->setScene(&scene);
}

void MainWindow::update_gui() {
	update_curPos_label();
	update_tarPos_label();
	update_slider();
	update_tarDial();
	update_curDial();
	update_field();
}
