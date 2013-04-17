#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

using namespace cv;

/// Global variables

Mat src, src_hsv;
Mat dst, detected_edges, tmp;



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


/** @function main */
int main( int argc, char** argv )
{
    //pthread_t warp_thread;
    //CvCapture *capture = 0;
    Mat warp_out, cal_mat;


    /// Load an image
    //tmp = imread( argv[1] );


    printf("Opening camera...");
    VideoCapture cap(0);
    if (!cap.isOpened()) {
         printf("Failed. Terminating\n");
         return -1;
     }
    printf("Done\n");

    /// Create windows
    namedWindow( window_name, CV_WINDOW_AUTOSIZE );

    printf("Window created\n");
   
    //waitKey(0);
    //exit(0);
    
    /// Register mouse callback function
    cvSetMouseCallback(window_name, mouseEvent, 0);

    printf("Capturing from cam 0...");

    cap >> src;

    if( !src.data ) {
	    printf("Failed\n");
	    goto _cleanup;
    }

    //cv::resize(tmp, src, Size(round(0.25*tmp.cols), round(0.25*tmp.rows)), 0, 0);

    printf("Done\n");

//    /// Create a matrix of the same type and size as src (for dst)
//    //dst.create( src.size(), src.type() );
//    warp_out.create(src.size(), src.type());

    /// Convert the image to HSV
    //cvtColor( src, src, CV_BGR2HSV );

    /// Show image
    imshow(window_name, src);

    //pthread_create(&cal_thread, NULL,  &calibrate_field, NULL);
    calibrate_field();
    waitKey(0);  // Block here until calibration is complete
    cal_mat = cv::getPerspectiveTransform(cal_points, cal_transform);

    namedWindow(warped_window);

    //pthread_join(cal_thread, NULL);

     //pthread_create(&warp_thread, NULL,  &find_puck, NULL);

    while (1) {
	    
        cap >> src;
	    
	    // update the global src image
        //pthread_mutex_lock(&src_mutex);
	    // Resize image so it fits on the screen
        //cv::resize(tmp, src, Size(round(0.7*tmp.cols), round(0.7*tmp.rows)), 0, 0);

        //pthread_mutex_unlock(&src_mutex);

        draw_cal_lines();

        imshow(window_name, src);

        cv::warpPerspective(src, warp_out, cal_mat, Size(720,480));
        
        cvtColor( warp_out, warp_out, CV_BGR2HSV ); // Convert to HSV
        imshow(warped_window, warp_out);

        //10ms delay, exit if user presses any key
        if (waitKey(10) > 0)
            break;
    }

 _cleanup:
    
    //cvReleaseCapture (&capture);
    //pthread_join(warp_thread, NULL);
    

    printf("Exiting\n");
    return 0;
}
