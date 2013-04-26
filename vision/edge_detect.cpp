#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define DEBUG

#ifdef DEBUG
#warning Building in DEBUG mode
#endif

using namespace cv;

struct puck_location {
	int x;
	int y;
};

struct area_map {
	double area;
	int index;
};


// Global variables
// function for sorting area array
bool area_sorter(struct area_map i, struct area_map j) {
	return i.area < j.area;
}

/// Global variables
Mat src, src_hsv;
Mat dst, detected_edges, tmp, warp_bin;
Mat warp_out, cal_mat;

enum _modes {still_image, live_capture} mode;

pthread_mutex_t src_mutex = PTHREAD_MUTEX_INITIALIZER;


cv::Point2f cal_points[4];

// Each horizontal pixel represents 1/200 of an inch, given the demensions of the playing field
static const cv::Point2f cal_transform[4] = {cv::Point(0, 0), cv::Point(720, 0), cv::Point(0, 480), cv::Point(720, 480)};

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

void find_puck (Mat &src, Mat &dst, struct puck_location pl) {
	//printf("Building binary image...");
	Mat gray;
	vector<vector<Point> > contours;
	vector<Point> centers;
	vector<struct puck_location> pucks;
	int i;

	// Convert to grayscale
	cvtColor(src, gray, CV_RGB2GRAY);

	// maybe blur here? -montaguk
	
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
	

	// Needs some more work here -montaguk
	// Create a lookup table that maps areas to vectors
	/*for (i = 0; i < contours.size(); i++) {
		
		contourArea(contours[i]); // might be able to get orientation -montaguk


		}
	*/
	cv::drawContours(src, contours, -1, Scalar(0, 0, 255));

	// Find moments of all contours
	vector<Moments> mu(contours.size());
	for (i = 0; i < contours.size(); i++) {
		mu[i] = cv::moments(contours[i], true);
	}

	// Find the mass centers for all moments
	vector<Point2f> mc(contours.size());
	for (i = 0; i < contours.size(); i++) {
		mc[i] = Point2f(mu[i].m10/mu[i].m00, mu[i].m01/mu[i].m00);
		
		// Draw circles around the centers
		cv::circle(src, mc[i], 4, Scalar(255, 0, 0), -1, 8, 0);
	}

}

/** @function main */
int main( int argc, char** argv )
{
    //pthread_t warp_thread;
    //CvCapture *capture = 0;
	VideoCapture *cap;
	struct puck_location pl;	// Where is the puck?

    if (argc > 1) {
	    /// Load an image
	    printf("Loading image...");
	    tmp = imread( argv[1] );
	    cv::resize(tmp, src, Size(round(0.25*tmp.cols), round(0.25*tmp.rows)), 0, 0);
	    printf("Done\n");
	    mode = still_image;
    } else {
	    mode = live_capture;
	    printf("Opening camera...");
	    cap = new VideoCapture(0);
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
    while(1) {
	    *cap >> src; 			// Keep the sceen refreshed
	    imshow(window_name, src);
	    if (waitKey(10) > 0) {
		    break;
	    }
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
	    
		    *cap >> src;
	    
		    cv::warpPerspective(src, warp_out, cal_mat, Size(720,480));

		    draw_cal_lines();
		    imshow(window_name, src);
        
		    find_puck(warp_out, warp_bin, pl);
		    imshow(warped_window, warp_out);

		    //10ms delay, exit if user presses any key
		    if (waitKey(10) > 0)
			    break;

#ifdef DEBUG
		    printf("\n");
#endif
		    
	    } // while(1)
	    
    } else {
	    cv::warpPerspective(src, warp_out, cal_mat, Size(720, 480));
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
