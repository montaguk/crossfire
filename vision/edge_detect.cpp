#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <iostream>
#include <fstream>

#define DEBUG
//#define DEBUG2

#define NUM_PUCKS 2
#define MIN_PUCK_AREA 20
#define CAM_NUM 1

//#define FIELD_W 720
//#define FIELD_H 480
#define FIELD_W 265 // In encoder ticks
#define FIELD_H 484 // In encoder ticks

#define FIFO_PATH "../gameplay/gameplay/fifo"

#ifdef DEBUG
#warning Building in DEBUG mode
#endif

using namespace cv;

// Holds a CV moments and a pointer to the associated contour
struct puck_list_element {
	Moments moments;
	vector<Point> *contour;
	const char *type;
	Point2f center;
};
	

// Global variables
// function for sorting area array

bool area_sorter_decending(struct puck_list_element i, struct puck_list_element j) {
	return i.moments.m00 > j.moments.m00;
}

/// Global variables
Mat src, src_hsv;
Mat dst, detected_edges, tmp, warp_bin;
Mat warp_out, cal_mat;

std::ofstream fifo;

enum _modes {still_image, live_capture} mode;

pthread_mutex_t src_mutex = PTHREAD_MUTEX_INITIALIZER;


cv::Point2f cal_points[4];

// Each horizontal pixel represents 1/200 of an inch, given the demensions of the playing field
static const cv::Point2f cal_transform[4] = {cv::Point(0, 0), cv::Point(FIELD_W, 0), cv::Point(0, FIELD_H), cv::Point(FIELD_W, FIELD_H)};

//vector<cv::Point2f> cal_trans_vec (cal_transform, cal_transform + sizeof(cal_transform) / sizeof(cal_transform[0]));
//vector<cv::Point2f> cal_points_vec(cal_points, cal_points + sizeof(cal_points) / sizeof(cal_points[0]));

int cal_target = -1;

const char *cal_strings[4] = {"upper left", "upper right", "lower right", "lower left"};


int edgeThresh = 1;
int lowThreshold;
int const max_lowThreshold = 100;
int ratio = 3;
int kernel_size = 3;
const char *window_name = "Raw Input";
const char *warped_window = "Adjusted Playing Field";
const char *bin_window = "Binary Image";
const char *gray_window = "Grayscale Image";


// Mouse callback function
void mouseEvent(int evt, int x, int y, int flags, void* param){

    if(evt==CV_EVENT_LBUTTONDOWN) {


        if (cal_target >= 0) {

            cal_points[cal_target].x = x;
            cal_points[cal_target].y = y;

            printf("(%f,%f) Done\n", cal_points[cal_target].x, cal_points[cal_target].y);


            // Set up for next click
            if (cal_target == 3) {
                printf("Calibration complete.  Press spacebar to continue\n");
                cal_target = -1;
            } else {
                cal_target++;
                printf("Click %s corner of playing field...", cal_strings[cal_target]);
            }
        }
    }
}

void mouseEventHSV(int evt, int x, int y, int flags, void *param) {
	if (evt == CV_EVENT_LBUTTONDOWN) {
		Vec3b pixel =  warp_out.at<Vec3b>(x, y);
		printf("HSV @ (%d, %d): %d, %d, %d\n", x, y, pixel[0], pixel[1], pixel[2]);
	}

}

void calibrate_field()
{
    printf("Begin calibration...\n");
    cal_target = 0;
    printf("Click %s corner of playing field...", cal_strings[cal_target]);

    // Block until user finishes calibration
    //while(cal_target != -1);
}


//// Run this a seperate thread.  Displays a window with the field's
//// adjusted perspective, and calculate the physical location
//// of the puck
//void *find_puck(void *ptr)
//{
//    // Put window back here if using this function
////	 Mat cal_mat = cv::getPerspectiveTransform(cal_points, cal_transform);

//	 while (1) {
//		 pthread_mutex_lock(&src_mutex);
//		 cv::warpPerspective(src, warp_out, cal_mat, Size(720,480));
//		 pthread_mutex_unlock(&src_mutex);
//		 imshow(warped_window, warp_out);

//         if (waitKey(10) > 0)
//			 break;
//	 }

//}

/**
 * @function CannyThreshold
 * @brief Trackbar callback - Canny thresholds input with a ratio 1:3
 */
void CannyThreshold(int, void*)
{
  /// Reduce noise with a kernel 3x3
  blur( src_hsv, detected_edges, Size(3,3) );

  /// Canny detector
  Canny( detected_edges, detected_edges, lowThreshold, lowThreshold*ratio, kernel_size );

  /// Using Canny's output as a mask, we display our result
  dst = Scalar::all(0);

  src.copyTo( dst, detected_edges);
  imshow( window_name, dst );
 }

// Draw lines between calibration points
void draw_cal_lines()
{
    cv::line(src, cal_points[0], cal_points[1], Scalar(255,255,255));  // top bar
    cv::line(src, cal_points[1], cal_points[3], Scalar(255,255,255));  // right bar
    cv::line(src, cal_points[2], cal_points[3], Scalar(255,255,255));  // bottom bar
    cv::line(src, cal_points[2], cal_points[0], Scalar(255,255,255));  // left bar
}

void find_puck (Mat &src, Mat &dst, vector<struct puck_list_element> &pl) {
	//printf("Building binary image...");
	Mat gray;
	vector<vector<Point> > contours;
	vector<Point> centers;
	int i;

	// Convert to grayscale
	cvtColor(src, gray, CV_RGB2GRAY);

	// maybe blur here? -montaguk
	 blur( gray, gray, Size(3,3) );
	
	// Find the average color of the playing field
	Scalar mean, std_dev, tmp;
	cv::meanStdDev(gray, mean, std_dev);

#ifdef DEBUG
	printf("thresh_min: %f, ", mean[0] + (2 * std_dev[0]));
#endif

	// Create binary image from grayscale
	cv::multiply(std_dev, Scalar(1.5), tmp);	
	cv::inRange(gray, mean + tmp, Scalar(255), dst);
	//printf("Done\n");

#ifdef DEBUG
	dst.copyTo(warp_bin);
	imshow(bin_window, warp_bin);
	imshow(gray_window, gray);
#endif
	
	
	//printf("Searching for contours...");
	// Find the pucks on the field using contours
	cv::findContours(dst, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
	cv::drawContours(src, contours, -1, Scalar(0, 0, 255));

	// Find moments of all contours
	vector<struct puck_list_element> mu(contours.size());
	for (i = 0; i < contours.size(); i++) {
		mu[i].moments = cv::moments(contours[i], true);
		mu[i].contour = &contours[i];
	}

	// Sort the list
	std::sort(mu.begin(), mu.end(), area_sorter_decending);

#ifdef DEBUG2
	printf("Contour areas: [");
	for (i = 0; i < mu.size(); i++) {
		printf(" %f,", mu[i].moments.m00);
	}
	printf("]");
#endif
	
	// Find the mass centers for all moments, and detect the shape
	// of the contour
	vector<Point> vertecies;	// Holds vertecies of contour

	//vector<Point2f> mc(contours.size());
	for (i = 0; (i < mu.size()) && (i < NUM_PUCKS); i++) {
		mu[i].center = Point2f(mu[i].moments.m10/mu[i].moments.m00, mu[i].moments.m01/mu[i].moments.m00);
		approxPolyDP(*mu[i].contour, vertecies, arcLength(Mat(*mu[i].contour), true)*0.05, true);

		// Set puck type
		if (vertecies.size() == 3) {
			mu[i].type = "T";
		} else {
			mu[i].type = "S";
		}
		
		// Draw circles around the centers
		cv::circle(src, mu[i].center, 4, Scalar(255, 0, 0), -1, 8, 0);
	}

	for (i = 0; (i < mu.size()) && (i < NUM_PUCKS); i++) {
		if (mu[i].moments.m00 > MIN_PUCK_AREA) {
			pl.push_back(mu[i]);
			
#ifdef DEBUG
			printf(" %s at (%.2f, %.2f)", mu[i].type, mu[i].center.x, mu[i].center.y);
#endif
			
		}
	}
}

// Write puck locations in pl to the file named fifo
// located in the current directory
void write_fifo(std::vector<struct puck_list_element> pl) {
	int i;
	char buf[25] = {0};
	char str[50] = {0};

	if (pl.size() > 0) {
	
		for (i = 0; i < pl.size(); i++) {
			sprintf(buf, "%s,%d,%d", pl[i].type, (int)pl[i].center.x, (int)pl[i].center.y);
			strcat(str, buf);

			// If this is not the last element, insert a semi-colon
			if (i != pl.size() - 1) {
				strcat(str, ";");
			}
		}

		strcat(str, "\n");
	} else {
		strcpy(str, "no pucks\n");
	}

	fifo << str;
	fifo.flush();			// Write everything
	//sleep(0.25);
}

/** @function main */
int main( int argc, char** argv )
{
    //pthread_t warp_thread;
    //CvCapture *capture = 0;
	VideoCapture *cap;

    if (argc > 1) {
	    /// Load an image
	    printf("Loading image...");
	    tmp = imread( argv[1] );
	    cv::resize(tmp, src, Size(round(0.25*tmp.cols), round(0.25*tmp.rows)), 0, 0);
	    printf("Done\n");
	    mode = still_image;
    } else {
	    mode = live_capture;
	    
	    printf("Opening fifo for writing...");
	    fifo.open(FIFO_PATH);
	    if (!fifo.is_open()) {
		    printf("Failed.\n");
		    goto _cleanup;
	    }
	    printf("Done\n");
	    
	    printf("Opening camera...");
	    cap = new VideoCapture(CAM_NUM);
	    if (!cap->isOpened()) {
		    printf("Failed. Terminating\n");
		    return -1;
	    }
	    printf("Done\n");

	    printf("Capturing from cam 0...");
	    *cap >> src;
    }

    
    if( !src.data ) {
	    printf("Failed\n");
	    goto _cleanup;
    }
    printf("Done\n");

    /// Create windows
    namedWindow( window_name, CV_WINDOW_AUTOSIZE );
    printf("Window created\n");
   
    /// Register mouse callback function
    cvSetMouseCallback(window_name, mouseEvent, 0);
    
    
    /// Show image
    calibrate_field();

    if (mode == live_capture) {
	    while(1) {
		    *cap >> src; 			// Keep the sceen refreshed
		    imshow(window_name, src);
		    if (waitKey(10) > 0) {
			    break;
		    }
	    }
    } else {
	    imshow(window_name, src);
	    waitKey(0);
    }
    cal_mat = cv::getPerspectiveTransform(cal_points, cal_transform);
    
    namedWindow(warped_window);

#ifdef DEBUG
    namedWindow(bin_window);
    namedWindow(gray_window);
#endif

    // Set mouse callback function for warped_window
    cvSetMouseCallback(warped_window, mouseEventHSV, 0);
	    
    if (mode == live_capture) {
	    
	    while (1) {
		    vector<struct puck_list_element> pl;	// Where are the pucks?
	    
		    *cap >> src;
	    
		    cv::warpPerspective(src, warp_out, cal_mat, Size(FIELD_W, FIELD_H));

		    draw_cal_lines();
		    imshow(window_name, src);
        
		    find_puck(warp_out, warp_bin, pl);
		    imshow(warped_window, warp_out);
		    write_fifo(pl);
		    //10ms delay, exit if user presses any key
		    if (waitKey(10) > 0)
			    break;

#ifdef DEBUG
		    printf("\n");
#endif
		    
	    } // while(1)
	    
    } else {
	    vector<struct puck_list_element> pl;	// Where are the pucks?
	    cv::warpPerspective(src, warp_out, cal_mat, Size(FIELD_W, FIELD_H));
	    draw_cal_lines();
	    imshow(window_name, src);
	    find_puck(warp_out, warp_bin, pl);
	    imshow(warped_window, warp_out);

	    waitKey(0);				// Wait forever for user input
    }

 _cleanup:
    
    // Free up the video capture memory
    if (mode == live_capture) {
	    delete(cap);
    }

    printf("Exiting\n");
    return 0;
}
