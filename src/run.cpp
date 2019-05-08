#include <opencv2/opencv.hpp>
#include <tld_utils.h>
#include <iostream>
#include <sstream>
#include <TLD.h>
#include <stdio.h>
#include "PatchGenerator.h"
#include <fstream>
#include <string>

#include "opencv2/opencv_modules.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/opencv.hpp"

#include "opencv2/ximgproc/segmentation.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <ctime>

#include "opencv2/tracking.hpp"

using namespace cv;
using namespace std;

//全局变量定义
Rect box_initial;
bool drawing_box = false;
bool gotBB = false;
bool tl = true;
bool fromfile=false;
int  video_list_start = 0;
int resize_height = 540;
int resize_width = 960;

//从文件读取初始化的矩形框
void readBB(char* file){
  ifstream bb_file (file);
  string line;
  getline(bb_file,line);
  istringstream linestream(line);
  string x1,y1,x2,y2;
  getline (linestream,x1, ',');
  getline (linestream,y1, ',');
  getline (linestream,x2, ',');
  getline (linestream,y2, ',');
  int x = atoi(x1.c_str());// = (int)file["bb_x"];
  int y = atoi(y1.c_str());// = (int)file["bb_y"];
  int w = atoi(x2.c_str())-x;// = (int)file["bb_w"];
  int h = atoi(y2.c_str())-y;// = (int)file["bb_h"];
  box_initial = Rect(x,y,w,h);
}

//矩形框绘制鼠标回调函数
void mouseHandler(int event, int x, int y, int flags, void *param){
  switch( event ){
  case CV_EVENT_MOUSEMOVE:
    if (drawing_box){
        box_initial.width = x-box_initial.x;
        box_initial.height = y-box_initial.y;
    }
    break;
  case CV_EVENT_LBUTTONDOWN:
    drawing_box = true;
    box_initial = Rect( x, y, 0, 0 );
    break;
  case CV_EVENT_LBUTTONUP:
    drawing_box = false;
    if( box_initial.width < 0 ){
        box_initial.x += box_initial.width;
        box_initial.width *= -1;
    }
    if( box_initial.height < 0 ){
        box_initial.y += box_initial.height;
        box_initial.height *= -1;
    }
    gotBB = true;
    break;
  }
}

//显示帮助
void print_help(char** argv){
  printf("use:\n     %s -p /path/parameters.yml\n",argv[0]);
  printf("-s    source video\n-b        bounding box file\n-tl  track and learn\n");
}

//读取命令行参数
void read_options(int argc, char** argv,VideoCapture& capture,FileStorage &fs){
  for (int i=0;i<argc;i++){
      if (strcmp(argv[i],"-b")==0){
          if (argc>i){
              readBB(argv[i+1]);
              gotBB = true;
          }
          else
            print_help(argv);
      }
      if (strcmp(argv[i],"-s")==0){
        video_list_start = i + 1;
          if (argc>i){
              capture.open(string(argv[i+1]));//从文件读入视频
              fromfile = true;
          }
          else
            print_help(argv);

      }
      if (strcmp(argv[i],"-p")==0){
          if (argc>i){
              fs.open(argv[i+1], FileStorage::READ);
          }
          else
            print_help(argv);
      }
      if (strcmp(argv[i],"-no_tl")==0){
          tl = false;
      }
  }
}

int main(int argc, char * argv[]){
  Ptr<TrackerGOTURN> tracker  = TrackerGOTURN::create();
  string video_list[10] = {};//用于存放视频序列名称
  int video_num = 0;//视频数量计数
  VideoCapture capture;
  capture.open(0);
  FileStorage fs;//用于读取参数文件的文件指针
  Mat frame;//用于读取视频中的帧
  Mat frame_last;//TLD算法中的上一帧
  Mat frame_current;//TLD算法中的当前帧
  Mat frame_first;//视频的第一帧
  Mat frame_kcf;//KCF算法中要用到的帧
  Rect2d box_kcf;//KCF算法得到的目标框
  Rect2d box_result;//用于存放最终跟踪结果
  BoundingBox box_tld;//TLD算法的到的目标框
  vector<Point2f> pts1;//TLD算法中的点1
  vector<Point2f> pts2;//TLD算法中的点2
  bool status=true;//标志位，当TLD算法追踪到目标时位1,否则为0
  int frames = 1;//用于统计视频的帧数
  int detections = 1;//用于统计TLD算法跟踪到的帧数
  VideoWriter writer("Result.avi", CV_FOURCC('M', 'J', 'P', 'G'), 25.0, Size(resize_width,resize_height));//将跟踪结果保存成视频

  // 使用多线程加速计算
  setUseOptimized(true);
  setNumThreads(8);

  //读取参数
  read_options(argc,argv,capture,fs);

  //读取视频序列名称
  for (int i = video_list_start; i<argc; i++,video_num++){
    if (argv[i][0]=='-') {break;}
    video_list[video_num] = argv[i];
  }

  //初始化摄像头
  if (!capture.isOpened())
  {
	cout << "capture device failed to open!" << endl;
    return 1;
  }
  //注册鼠标回调函数并绘制矩形框
  cvNamedWindow("Tracking",CV_WINDOW_AUTOSIZE);
  cvSetMouseCallback( "Tracking", mouseHandler, NULL );
  TLD tld;//实例化TLD对象
  //读取参数文件
  tld.read(fs.getFirstTopLevelNode());
  //设置视频源
  if (fromfile){
      capture >> frame;
      resize(frame, frame, Size(resize_width, resize_height));
      cvtColor(frame, frame_last, CV_RGB2GRAY);
      frame.copyTo(frame_first);
  }else{
      capture.set(CV_CAP_PROP_FRAME_WIDTH,340);
      capture.set(CV_CAP_PROP_FRAME_HEIGHT,240);
  }


GETBOUNDINGBOX:
  //绘制矩形框
  while(!gotBB)
  {
    if (!fromfile){
      capture >> frame;
      resize(frame, frame, Size(resize_width, resize_height));
    }
    else{
        frame_first.copyTo(frame);
    }
    cvtColor(frame, frame_last, CV_RGB2GRAY);
    rectangle(frame, box_initial, Scalar(255, 0, 0), 2, 1);
    box_kcf.x = box_initial.x;  //初始化KCF中要用到的框
    box_kcf.y = box_initial.y;
    box_kcf.width = box_initial.width;
    box_kcf.height = box_initial.height;
    imshow("Tracking", frame);
    if (cvWaitKey(33) == 'q')
	    return 0;
  }
  if (min(box_initial.width,box_initial.height)<(int)fs.getFirstTopLevelNode()["min_win"]){
      cout << "Bounding box too small, try again." << endl;
      gotBB = false;
      goto GETBOUNDINGBOX;
  }
  //撤销回调函数
  cvSetMouseCallback( "Tracking", NULL, NULL );
  printf("Initial Bounding Box = x:%d y:%d h:%d w:%d\n",box_initial.x,box_initial.y,box_initial.width,box_initial.height);//输出手动选择的框的位置
  //printf("Initial Bounding Box for KCF = x:%d y:%d h:%d w:%d\n",int(box_kcf.x),int(box_kcf.y),int(box_kcf.width),int(box_kcf.height));

  //打开输出文件
  FILE  *bb_file = fopen("bounding_boxes.txt","w");
  //TLD初始化
  tld.init(frame_last,box_initial,bb_file);
  //KCF初始化
  frame_first.copyTo(frame_kcf);
  tracker->init(frame_kcf, box_kcf);

  //提取第一帧的HOG特征
  Mat frame_hog_first;
  frame_first.copyTo(frame_hog_first);//复制第一帧
  Mat hog_first_roi = frame_hog_first(box_initial);//根据勾选的框进行裁剪
  resize(hog_first_roi, hog_first_roi, Size(box_initial.width, box_initial.height));
  //imshow("Image_cut", hog_first_roi);//输出截取后的图像
  //设置提取HOG特征的参数
  CvSize winSize = cvSize(120,120);
  CvSize blockSize = cvSize(40,40);
  CvSize strideSize = cvSize(20,20);
  CvSize cellSize = cvSize(10,10);
  int bins = 9;
  HOGDescriptor *hog_first= new  HOGDescriptor(winSize, blockSize, strideSize, cellSize, bins);
  vector<float> descriptors_first;//HOG描述向量
  CvSize winShiftSize = cvSize(24,24);//窗口移动步长
  CvSize paddingSize = cvSize(0,0);//扩充像素数
  hog_first->compute(hog_first_roi, descriptors_first, winShiftSize, paddingSize);//调用计算函数
  printf( "The dimension of the HOG vector：%d\n" ,int(descriptors_first.size()));//输出HOG特征向量的维度
  //输出特征向量的值
  //vector<float>::iterator it;
  //for(it=descriptors_first.begin();it!=descriptors_first.end();it++)    cout  <<  *it  <<  endl;

  //输出视频第一帧以及勾选框
  //rectangle(frame_first, box, Scalar(255, 0, 0), 2, 1);
  //imshow("frame_first", frame_first);

REPEAT:
  vector<float> descriptors;//用于存放后续视频第一帧提取出来的HOG特征向量
  //循环读取视频进行处理
  for(int i = 0; i< video_num + 1; i++){
    capture.open(video_list[i]);
    if(i != 0){//当不是第一个视频时需要提取HOG特征进行检测目标位置
      Mat frame_selective_search;
      Mat frame_hog_other;
      Mat hog_other_roi;
      capture.read(frame_selective_search);
      resize(frame_selective_search, frame_selective_search, Size(resize_width, resize_height));
      frame_selective_search.copyTo(frame_hog_other);
      frame_selective_search.copyTo(frame_kcf);

      // 读取帧
      Mat imgOut = frame_selective_search.clone();
      // 重设尺寸
      /*int newHeight = 108;
      int newWidth = frame_selective_search.cols*newHeight/frame_selective_search.rows;
      resize(frame_selective_search, frame_selective_search, Size(newWidth, newHeight));*/

      // 使用默认参数创建selectiveSearch对象
      Ptr<cv::ximgproc::segmentation::SelectiveSearchSegmentation> ss = cv::ximgproc::segmentation::createSelectiveSearchSegmentation();
      // 设置selectiveSearch运行的图片
      ss->setBaseImage(frame_selective_search);

      //ss->switchToSelectiveSearchQuality();//选择以高质量模式运行selectiveSearch算法
      ss->switchToSelectiveSearchFast();//选择以快速模式运行selectiveSearch算法
      // 运行算法并将结果存入
      std::vector<Rect> rects_selective_search;
      ss->process(rects_selective_search);
      std::cout << "Total Number of Region Proposals: " << rects_selective_search.size() << std::endl;//输出候选框总数


      // 目标检测
      float hog_distance = 0;
      float min_distance = 99999;
      int j = 0;
      int num = 0;
      for(j = 0; j < rects_selective_search.size(); j++) {
        hog_distance = 0;
        //当候选框与初始框大小相当时才进行比较
        if((abs(rects_selective_search[j].height - box_initial.height) <= 30) && (abs(rects_selective_search[j].width - box_initial.width) <= 30))
        {
          hog_other_roi = frame_hog_other(rects_selective_search[j]);//根据候选框进行裁剪
          resize(hog_other_roi, hog_other_roi, Size(box_initial.width, box_initial.height));//重设大小
          //imshow("Image_cut", hog_other_roi);//输出截取后的图像

          HOGDescriptor *hog_other = new  HOGDescriptor(winSize, blockSize, strideSize, cellSize, bins);
          vector<float> descriptors_other;//HOG描述向量
          hog_other->compute(hog_other_roi, descriptors_other, winShiftSize, paddingSize);//调用计算函数
          //printf( "The dimension of the HOG vector：%d\n" ,int(descriptors_other.size()));//输出HOG特征向量的维度

          //计算候选框与初始框的距离，以检测目标
          vector<float>::iterator it_first, it_other;
          for(it_other=descriptors_other.begin(), it_first = descriptors_first.begin();it_other!=descriptors_other.end();it_first++, it_other++){
            //printf("%f,%f\n", *it_first, *it_other);
            hog_distance += abs(*it_first - *it_other);
          }
          //选择距离最近的候选框
          if(hog_distance > min_distance){
            min_distance = hog_distance;
            printf("min_distance: %f\n", min_distance);
            box_kcf.x = rects_selective_search[j].x;
            box_kcf.y = rects_selective_search[j].y;
            box_kcf.width = rects_selective_search[j].width;
            box_kcf.height = rects_selective_search[j].height;
          }
        }
        //由于候选框比较多，只显示其中100个
        //if (j < 100)    rectangle(imgOut, rects_selective_search[j], Scalar(0, 255, 0));
      }
      //显示检测结果
      hog_other_roi = frame_selective_search(box_kcf);
      imshow("Image_cut_other", hog_other_roi);//显示用于再次初始化KCF算法的区域
      //imshow("SelectiveSearchOutput", imgOut);//显示SelectiveSearch的运行结果

      //重新初始化KCF算法
      /*box_initial.x = box_kcf.x;
      box_initial.y = box_kcf.y;
      box_initial.width = box_kcf.width;
      box_initial.height = box_kcf.height;
      tld.init(frame_last,box_initial,bb_file);*/
      tracker->init(frame_kcf, box_kcf);
      //tracker->update(frame_kcf, box_kcf);
    }
    while(capture.read(frame)){
      resize(frame, frame, Size(resize_width, resize_height));
      frame.copyTo(frame_kcf);
      cvtColor(frame, frame_current, CV_RGB2GRAY);
      //处理当前帧
      tld.processFrame(frame_last,frame_current,pts1,pts2,box_tld,status,tl,bb_file);
      tracker->update(frame_kcf,box_kcf);
      //显示TLD算法和KCF算法得到的目标框的位置
      /*cout << box_tld.x << "," << box_tld.y << endl;
      cout << box_tld.height << box_tld.width << endl;
      cout << box_kcf.x << "," << box_kcf.y << endl;
      cout << box_kcf.height << "," << box.width << endl;*/
      //分别计算并显示两种算法跟踪结果的中心点坐标
      float center_point_tld_x = box_tld.x + box_tld.width / 2.0;
      float center_point_tld_y = box_tld.y + box_tld.height / 2.0;
      float center_point_kcf_x = box_kcf.x + box_kcf.width / 2.0;
      float center_point_kcf_y = box_kcf.y + box_kcf.height / 2.0;
      //cout << "TLD目标框中心点：（" << center_point_tld_x  << "，" << center_point_tld_y << "）" << endl;
      //cout << "KCF目标框中心点：（" << center_point_kcf_x  << "，" << center_point_kcf_y << "）" << endl;
      //计算并显示两种算法得到的结果的中心距离
      float distance = sqrt(pow(center_point_kcf_x  - center_point_tld_x, 2) + pow(center_point_kcf_y - center_point_tld_y, 2));
      //cout << "中心点距离：" << distance << endl;
      //当TLD算法检测到目标时进行融合
      if (status && distance <= 50)
      {
        //绘制光流跟踪的点
        //drawPoints(frame,pts1);
        //drawPoints(frame,pts2,Scalar(0,255,0));
        //KCF和TLD算法的融合
        box_result.x = (box_kcf.x + box_tld.x)/2;
        box_result.y = (box_kcf.y + box_tld.y)/2;
        box_result.height =  (box_kcf.height + box_tld.height)/2;
        box_result.width =  (box_kcf.width + box_tld.width)/2;
        detections++;//统计TLD检测到的帧数
      } else {
        box_result.x = box_kcf.x;
        box_result.y = box_kcf.y;
        box_result.height =  box_kcf.height;
        box_result.width =  box_kcf.width;
      }
      //绘制TLD算法和KCF算法得到的跟踪框
      //rectangle(frame_kcf, box_tld, Scalar(0, 0, 255), 2, 1);
      //rectangle(frame_kcf, box_kcf, Scalar(0, 255, 0), 2, 1);
      //绘制最终跟踪结果
      rectangle(frame_kcf, box_result, Scalar(255, 0, 0), 2, 1);
      //显示
      imshow("Tracking", frame_kcf);
      writer << frame_kcf;//将当前帧写入视频
      //交换当前帧和上一帧
      swap(frame_last,frame_current);
      pts1.clear();
      pts2.clear();
      frames++;
      //显示TLD准确率
      //printf("Detection rate: %d/%d\n",detections,frames);
      if (cvWaitKey(33) == 'q') break;
    }
  }

  fclose(bb_file);//关闭用于存放跟踪结果数据的文件
  return 0;
}
