
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>
#include "test.h"

using namespace cv; 
using namespace std;
const int kTargetFps = 30;
int fixedDelayMs()
{
	return static_cast<int>(1000.0 / kTargetFps);
}
bool showFrame(const Mat& frame)
{
	if (frame.empty())
	{
		return false;
	}
	imshow("OpenCV Video Viewer", frame);
	int key = waitKey(fixedDelayMs()) & 0xFF;
	return key != 27;
}
void test()
{
	s_val0++;	s_val0++;	s_val0++;
    s_val1++;  s_val1++; s_val1++;
	s_val3++;  s_val3++; s_val3++;
	const int* s = &s_val2;
	printf("");
    //s_val2++;
	//std::string path = "e:/video/huajiaot.mp4";//askString("Video path");
	//VideoCapture cap(path);
	//if (!cap.isOpened())
	//{
	//	cerr << "Failed to open video: " << path << endl;
	//	return;
	//}

	//Mat frame;
	//while (true)
	//{
	//	if (!cap.read(frame))
	//	{
	//		break;
	//	}
	//	if (!showFrame(frame))
	//	{
	//		break;
	//	}
	//}
}