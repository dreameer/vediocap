/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                          License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <cstring>
#include <sstream>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

using namespace std;
using namespace cv;

namespace patch
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}
std::string get_tegra_pipeline(int width, int height, int fps) {
    return "nvcamerasrc sensor-id= 0 ! video/x-raw(memory:NVMM), width=(int)" + patch::to_string(width) + ", height=(int)" +
           patch::to_string(height) + ", format=(string)I420, framerate=(fraction)" + patch::to_string(fps) +
           "/1 ! nvvidconv flip-method=2 ! video/x-raw, format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! appsink";
           //"/1 ! nvvidconv flip-method=2 !  appsink";
}
void focal_length(unsigned short length){
	unsigned short _length = length;
	if(_length>993||length<0){
		printf("error:focal_length:%d over flow\n",_length);
	}else{
		int file;
		int adapter_nr = 2; //probably dynamically determined
		char filename[20];
		snprintf(filename,19,"/dev/i2c-%d",adapter_nr);
		file = open(filename,O_RDWR);
		if(file<0){
			printf("open file failed\n");
			exit(1);
		}
		int addr = 0x0c; //the i2c address
		if(ioctl(file,I2C_SLAVE,addr)<0){
			printf("ioctl error\n");
			exit(1);
		}
		__u8 reg = 0x04;
		__s32 res;
		char buff[10];
		buff[0] = reg;
		memcpy(&buff[1],((unsigned char*)&_length)+1,1);
		memcpy(&buff[2],((unsigned char*)&_length),1);
		res = write(file,buff,3);
		if(res!=3){
			printf("write failed:%d\n",res);
		}else{
			}
		}
}
int main(){
	int FPS = 30;
	std::string pipeline = get_tegra_pipeline(1280, 720, FPS);
	VideoCapture light_cap(pipeline, cv::CAP_GSTREAMER);
	
	VideoCapture heat_cap(1);
	heat_cap.set(CAP_PROP_FRAME_WIDTH,320);
	heat_cap.set(CAP_PROP_FRAME_HEIGHT,240);
	if (!heat_cap.isOpened()||!light_cap.isOpened()) {
		printf("cant open camera!\n");
	} else {
		Mat heat_img,light_img;
		unsigned short length = 350;
		focal_length(length);
		char char_key = 'a';
		
		const char heat_window[] = "heatwindow";
		const char light_window[] = "lightwindow";
		namedWindow(heat_window,0);
		namedWindow(light_window,1);
		bool whileflag = true;
		while(whileflag)
		{

			heat_cap >> heat_img;
			light_cap >> light_img;
			Rect roi_rect = Rect(360+80,240,320,240);
			Mat roi_img = light_img(roi_rect);
			vector<Mat> rgb_img(3),rgb_binary_img(3);
			split(heat_img,rgb_img);
			//Canny( rgb_img[0], rgb_binary_img[0], 30, 30*3, 3 );
			//Canny( rgb_img[1], rgb_binary_img[1], 30, 30*3, 3 );
			//Canny( rgb_img[2], rgb_binary_img[2], 30, 30*3, 3 );
			threshold( rgb_img[0], rgb_binary_img[0], 200, 255,3 );
			threshold( rgb_img[1], rgb_binary_img[1], 200, 255,3 );
			threshold( rgb_img[2], rgb_binary_img[2], 200, 255,3 );
			merge(rgb_binary_img,heat_img);
			Mat mixed_img;
			addWeighted( roi_img, 1, heat_img, 1, 0.0, mixed_img);
			mixed_img.copyTo(light_img(roi_rect));
            
			imshow(light_window, light_img);
			

			char_key = waitKey(1);
			switch(char_key){
			case 'f':length = length+10;
			         if((length<993)&&(length>0)){
						 focal_length(length);
					 }else{
						length = 0;
					 }
					break;
			case 'g':length = length-10;
			         if((length<993)&&(length>0)){
						 focal_length(length);
					 }else{
						length = 0;
					 }
					break;
			case 'c':imwrite("heat_img.bmp",heat_img);break;
			case 'e':whileflag = false;break;
			default:break;
			}
		}
	}
	return 0;
}
