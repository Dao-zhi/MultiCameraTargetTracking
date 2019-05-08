#include <iostream>
#include <fstream>
#include <string>

#include "opencv2/opencv_modules.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/opencv.hpp"

#include "opencv2/tracking.hpp"

using namespace std;
using namespace cv;


int main(int argc, char** argv)
{
	//可选BOOSTING, MIL, KCF, TLD, MEDIANFLOW, or GOTURN
	
    Ptr<TrackerBoosting> tracker = TrackerBoosting::create();

	VideoCapture video("car.mpg");
    //VideoCapture video(0);
	if (!video.isOpened()) {
		cout << "cannot read video!" << endl;
		return -1;
	}

	Mat frame;
	video.read(frame);
    imshow("Tracking", frame);
	Rect2d box(120,110,120,60);

        //第一帧初始化tracker
	tracker->init(frame, box);

	while (video.read(frame)) 
        {
                //跟踪目标并更新模型
		tracker->update(frame, box);

		rectangle(frame, box, Scalar(255, 0, 0), 2, 1);
		imshow("Tracking", frame);
		int k = waitKey(1);
		if (k == 27) 
                    break;
	}
}
