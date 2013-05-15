#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QFuture>
#include "robot.h"
#include "puck.h"

#define FIELD_H 430
#define FIELD_W 240

#define NUM_PUCKS 2

#define REFRESH_RATE 30  // in ms

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
	
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();



	
private:
	void update_curPos_label();
	void update_tarPos_label();
	void update_slider();
	void update_curDial();
	void update_tarDial();
	void update_gui();
	void update_field();
	void update_line();
	void update_rect();
	void draw_pucks();

	Ui::MainWindow *ui;
	QGraphicsScene scene;
	QGraphicsRectItem *rect;
	QGraphicsLineItem *line;
	Robot robot;

	Puck *pucks[NUM_PUCKS];
	QGraphicsEllipseItem *puck_img[NUM_PUCKS];
	QGraphicsLineItem *fv[NUM_PUCKS];

	// Keeps track of the puck updating thread
	QFuture<void> puck_updater;
	void update_pucks();
	bool continue_update;

	QTimer *screen_refresh_timer;

private slots:
	void on_fireButton_clicked();
	void on_refreshButton_clicked();
	void on_openCloseButton_clicked();
	void onLineReceived(QString data);
	void on_slider_valueChanged(int value);
	void on_tarDial_sliderMoved(int value);
	void refresh_field();
};

#endif // MAINWINDOW_H
