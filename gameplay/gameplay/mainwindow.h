#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QFuture>
#include "robot.h"
#include "puck.h"

// 4.7 ticks per cm
#define FIELD_H 484 // In encoder ticks
#define FIELD_W 265 // In encoder ticks
#define DEADZONE_H 103 // In encoder ticks
//#define DEADZONE_H 120

#define NUM_PUCKS 2

#define REFRESH_RATE 30  // in ms

#define DANGER_THRESH 330

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
	void update_currentTarget_label();
	void update_slider();
	void update_curDial();
	void update_tarDial();
	void update_gui();
	void update_field();
	void update_line();
	void update_rect();
	void draw_pucks();
	void update_cur_target();
	void scene_clicked(QEvent *ev);
	void update_shootingAt_label();
	void select_target();
	int find_best_target();

	Ui::MainWindow *ui;
	QGraphicsScene scene;
	QGraphicsRectItem *rect;
	QGraphicsLineItem *line;
	Robot robot;

	Puck *pucks[NUM_PUCKS];
	QGraphicsEllipseItem *puck_img[NUM_PUCKS];
	QGraphicsLineItem *fv[NUM_PUCKS];
	QPoint *cur_tar;
	QLineF *cur_fv;
	QGraphicsLineItem *current_fv;
	QGraphicsEllipseItem *current_tar;

	bool fire;  // Should we be shooting?

	// Keeps track of the puck updating thread
	QFuture<void> puck_updater;
	void update_pucks();
	bool continue_update;

	QTimer *screen_refresh_timer;

	enum _targets{PUCK1, PUCK2, MANUAL, NONE} shooting_at;
	enum _modes{MAN, AUTO} targeting_mode;

private slots:
	void on_fireButton_clicked();
	void on_refreshButton_clicked();
	void on_openCloseButton_clicked();
	void onLineReceived(QString data);
	void on_slider_valueChanged(int value);
	void on_tarDial_sliderMoved(int value);
	void on_puck1Button_clicked();
	void on_puck2Button_clicked();
	void on_autoSelectButton_clicked();
	void refresh_field();

protected:
	bool eventFilter(QObject *obj, QEvent *ev);

};

#endif // MAINWINDOW_H
