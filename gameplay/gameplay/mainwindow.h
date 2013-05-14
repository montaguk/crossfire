#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QFuture>
#include "robot.h"

#define FIELD_H 490
#define FIELD_W 240

#define NUM_PUCKS 2

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

	Ui::MainWindow *ui;
	QGraphicsScene scene;
	QGraphicsRectItem *rect;
	QGraphicsLineItem *line;
	Robot robot;

	QPoint *pucks[NUM_PUCKS];

	// Keeps track of the puck updating thread
	QFuture<void> puck_updater;
	void update_pucks();
	bool continue_update;

private slots:
	void on_fireButton_clicked();
	void on_refreshButton_clicked();
	void on_openCloseButton_clicked();
	void onLineReceived(QString data);
	void on_slider_sliderMoved(int value);
	void on_tarDial_sliderMoved(int value);
};

#endif // MAINWINDOW_H
