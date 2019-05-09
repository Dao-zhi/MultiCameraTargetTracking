"# MultiCameraTargetTracking" 
## 开发环境
 - 操作系统：Ubuntu 16.04
 - OpenCV版本：OpenCV-3.4.5
## 一、安装OpenCV
&nbsp;&nbsp;&nbsp;&nbsp;由于只有OpenCV 3.0以后的版本才有目标跟踪的功能，自从进入3.X时代以后，OpenCV将代码库分成了两部分，分别是稳定的核心功能库和试验性质的contrib库。下面是OpenCV + Contrib的编译与安装过程。
### 1.下载OpenCV和Contrib：
&nbsp;&nbsp;&nbsp;&nbsp;Contrib没有在OpenCV的官网发布，而是发布在了OpenCV的github上，需要注意的是OpenCV和Contrib的版本要对应，不然无法编译安装。OpenCV和Contrib的源代码都可以在OpenCV的github上下载到，下载连接：

- [OpenCV](https://github.com/opencv/opencv/archive/3.4.5.zip)
- [Contrib](https://github.com/opencv/opencv_contrib/archive/3.4.5.zip)
### 2.编译安装：

准备，先安装以下依赖包：
``` bash
sudo apt-get install build-essential  
  
sudo apt-get install cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev  
  
sudo apt-get install python-dev python-numpy libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libjasper-dev libdc1394-22-dev  

sudo apt-get install build-essential qt5-default ccache libv4l-dev libavresample-dev  libgphoto2-dev libopenblas-base libopenblas-dev doxygen  openjdk-8-jdk pylint libvtk6-dev

sudo apt-get install pkg-config
```
编译：

（1）解压下载好的源代码：
``` bash
sudo unzip opencv-3.3.1.zip
sudo unzip opencv_contrib-3.3.1.zip
```
（2）解压完后需要将opencv_contrib.zip提取到opencv目录下，同时在该目录下新建一个文件夹build：
``` bash
sudo cp -r opencv_contrib-3.3.1 opencv-3.3.1  #复制opencv_contrib到opencv目录下

cd opencv-3.3.1

sudo mkdir build                              #新建文件夹build
```
（3）进入build目录，并且执行cmake生成makefile文件：
``` bash
sudo cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=/usr/local -D OPENCV_EXTRA_MODULES_PATH=/home/user_name/opencv-3.4.3/opencv_contrib-3.4.3/modules/ ..
```
注意：OPENCV_EXTRA_MODULES_PATH就是 opencv_contrib-3.3.1下面的modules目录，请按照自己的实际目录修改地址。接下来就是漫长的等待了……

（4）在cmake成功之后，就可以在build文件下make了：
``` bash
make
```
接下来就是更漫长的等待了……make成功后再执行`make install`就ok了！

（5）链接库共享：

编译安装完毕之后，为了让你的链接库被系统共享，让编译器发现，需要执行管理命令ldconfig：
``` bash
sudo ldconfig -v
```
Ubuntu16.04会报错：
```
Scanning dependencies of target opencv_test_superres
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFReadDirectory@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFGetFieldDefaulted@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFNumberOfDirectories@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFIsTiled@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFOpen@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFDefaultStripSize@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFReadRGBAImage@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFReadTile@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFSetField@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFWriteScanline@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `_TIFFfree@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFGetField@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFScanlineSize@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFSetWarningHandler@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFTileSize@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `_TIFFmalloc@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFSetErrorHandler@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFSetDirectory@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFReadScanline@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFClose@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFNumberOfTiles@LIBTIFF_4.0'
//usr/lib/x86_64-linux-gnu/libvtkIOImage-6.2.so.6.2: undefined reference to `TIFFClientOpen@LIBTIFF_4.0'
[ 74%] Building CXX object modules/imgcodecs/CMakeFiles/opencv_perf_imgcodecs.dir/perf/perf_main.cpp.o
[ 74%] Building CXX object modules/shape/CMakeFiles/opencv_test_shape.dir/test/test_main.cpp.o
[ 74%] Building CXX object modules/highgui/CMakeFiles/opencv_test_highgui.dir/test/test_main.cpp.o
[ 75%] Building CXX object modules/superres/CMakeFiles/opencv_test_superres.dir/test/test_main.cpp.o
[ 75%] Building CXX object modules/videoio/CMakeFiles/opencv_perf_videoio.dir/perf/perf_main.cpp.o
[ 75%] Building CXX object modules/videoio/CMakeFiles/opencv_test_videoio.dir/test/test_main.cpp.o
collect2: error: ld returned 1 exit status
modules/viz/CMakeFiles/opencv_test_viz.dir/build.make:233: recipe for target 'bin/opencv_test_viz' failed
make[2]: *** [bin/opencv_test_viz] Error 1
CMakeFiles/Makefile2:5627: recipe for target 'modules/viz/CMakeFiles/opencv_test_viz.dir/all' failed
make[1]: *** [modules/viz/CMakeFiles/opencv_test_viz.dir/all] Error 2
make[1]: *** Waiting for unfinished jobs...
```
解决办法：
``` bash
sudo apt-get autoremove libtiff5-dev
sudo apt-get install libtiff5-dev
```
### 3.测试
``` bash
# 进入opencv-2.4.13/samples/c/example_cmake
cd opencv-2.4.13/samples/c/example_cmake
# 编译
cmake
make
# 编译之后会生成可执行文件opencv_example，运行
./opencv_example
# 运行结果为摄像头拍摄并在画布上显示hello opencv
```
## 二、编译源代码
&nbsp;&nbsp;&nbsp;&nbsp;首先下载源代码，并解压，进入解压后的目录。新建build目录并进入build：
``` bash
mkdir build
cd build
```
&nbsp;&nbsp;&nbsp;&nbsp;然后用cmake进行编译：
``` bash
cmake ../src/
make
```
&nbsp;&nbsp;&nbsp;&nbsp;make命令执行成功后会在build目录下生成bin文件夹，进入bin文件夹就可以运行可执行文件了：
``` bash
./run_tld -p ../parameters.yml -s ../datasets/06_car/car.mpg    # -p用来指定tld算法的参数文件 -s选项用来选择传入的视频文件，可以连续输入多个视频做跨摄像头目标跟踪
```
### 三、联系方式
 - 邮箱：1725805106@qq.com
 - QQ：1725805106
