// hrTradeMark.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <iostream>
#include "opencv.hpp"
#include "nonfree/nonfree.hpp"
#include "nonfree/features2d.hpp"
#include <tchar.h>
#include <stdint.h>
#include "templateM.h"
#include "BmpOperation.h"

using namespace std;

TemplateTrade tt;
BmpOperation bo;

const int FrameDifference = 5;//当背景差值大于这个值时，当作前景
const int minFeaturePoint = 4;//对一张图/一个区域寻找特征点时，结果不能小于这个值


const uint8_t 处理成功 = 0; //PROCESSOK
const uint8_t 加载模板错误 = 1; //LOADTEMPLATEERROR
const uint8_t 加载图片失败 = 2; //LOADIMAGEERROR
const uint8_t 特征不够 = 3;//INSUFFICIENTFEATURE
const uint8_t 特征不匹配 = 4; //FEATUREDISMATCH
const uint8_t 标记区域错误 = 5;//INVAILDCOORDINATE
const uint8_t 没有提取到前景 = 6; //NOFOREGROUND
const uint8_t 透视变换失败 = 7; //HOMOFAILED

const uint8_t 上位机调用错误 = 98;


 uint8_t __stdcall Initialization()
{
	 return 0;
}
 //LoadTemplate(要加载图片的路径，图片所属的类别)
 //参数说明：type: 1代表加载背景， 2代表商标区域
 //返回值说明：0-成功
 //            1-type输入的类型不是1或2
 //            2-图片加载时为空
 //            3-处理商标区域时，特征点小于4个
 uint8_t __stdcall LoadTemplate(int type, char* fileName, int x_tl, int y_tl, int x_br, int y_br)
{
	uint8_t result;
	if (type == 0)
	{
		cv::Mat tempImage = cv::imread(fileName, CV_LOAD_IMAGE_GRAYSCALE);
		if (tempImage.empty())
			return  加载图片失败;
		result = tt.LoadImageWithPos(tempImage, cv::Rect(cv::Point(x_tl, y_tl), cv::Point(x_br, y_br)));
		//cv::imwrite("D:\\2017.08商标自动检测\\商标自动检测项目\\潍柴新动力原集\\tradeImage.jpg", tt.tradeImage);
		return result;
	}
	else if (type == 1)
	{
		result = tt.SetRegion(cv::Rect(cv::Point(x_tl, y_tl), cv::Point(x_br, y_br)), type);
		if (result != 处理成功)
			return result;
		result = tt.GetCheckShootRegion();
		//cout << "检查区域：" << tt.defaultShooltRegion.x << " " << tt.defaultShooltRegion.y << " " << tt.defaultShooltRegion.width + tt.defaultShooltRegion.x << " " << tt.defaultShooltRegion.height + tt.defaultShooltRegion.y << endl;

		return result;
	}
	else if (type == 2)
	{
		result = tt.SetRegion(cv::Rect(cv::Point(x_tl, y_tl), cv::Point(x_br, y_br)), type);
		return result;
	}
	else
	{
		return 上位机调用错误;
	}
}

 //confirm the status of template load;
 //the mismatch of the size of kepPoint and descriptor: return 1
 //the size of hepPoints less than 4: return 3;
 //background/trade template/mark region is empty: return 2
 //the loadCount should  also be checked outside the DLL
 uint8_t __stdcall StatusConfirm()
{
	 cout << "已加载的背景图片尺寸：" << tt.background.cols << " X " << tt.background.rows << endl;
	 cout << "已加载的商标模板尺寸：" << tt.tradeImage.cols << " X " << tt.tradeImage.rows << endl;
	 cout << "检测拍照特征区域尺寸：" << tt.shootingImage.cols << " X " << tt.shootingImage.rows << endl;
	 cout << "商标整体特征点数量：" << tt.tradeKeypoints.size() << endl;
	 cout << "标记区域数量：" << tt.markImage.size() << endl;
	 return 处理成功;
}


 uint8_t __stdcall CheckIfShoot(char* fileName,double& value, float& incline)
 {
	 cv::Mat checkImage = cv::imread(fileName, CV_LOAD_IMAGE_GRAYSCALE);
	 if (checkImage.empty() || tt.background.cols != checkImage.cols)
	 {
		 return 加载图片失败;
	 }
	 bool isShoot;
	 float matchRate;
	 uint8_t result = tt.CheckShootingAndEva(checkImage, isShoot, matchRate, incline);
	 if (result != 处理成功)
		 return result;
	 if (isShoot)
	 {
		 value = matchRate;
	 }
	 else
	 {
		 value = -1;
	 }
	return 处理成功;
 }

 uint8_t __stdcall CheckIfShoot_Stream(BYTE* ImageStream,int streamLength, int width, int height, double& value, float& incline)
 {
	 cv::Mat checkImage;
	 int rst = bo.RawByteToMat(ImageStream, streamLength, height, width, checkImage);
	 if (checkImage.empty() || tt.background.cols != checkImage.cols)
	 {
		 return 加载图片失败;
	 }

	 bool isShoot;
	 float matchRate;
	 uint8_t result = tt.CheckShootingAndEva(checkImage, isShoot, matchRate, incline);
	 if (result != 处理成功)
		 return result;
	 if (isShoot)
	 {
		 value = matchRate;
	 }
	 else
	 {
		 value = -1;
	 }
	 return 处理成功;
 }

 uint8_t __stdcall CheckTradeStatus(char* fileName, double& value, float& incline)
 {
	 uint8_t result;
	 //传入待检原图（含背景）
	 cv::Mat detected = cv::imread(fileName, CV_LOAD_IMAGE_GRAYSCALE);
	 if (detected.empty() || tt.background.cols != detected.cols)
	 {
		 return 加载图片失败;
	 }
	 float matchRate;
	 result = tt.EvaAnImage(detected, matchRate, incline);
	 if (result != 处理成功)
		 return result;
	 value = matchRate;
	 return 处理成功;
 }

 uint8_t __stdcall CheckTradeStatus_Stream(BYTE* ImageStream, int streamLength, int width, int height, double& value, float& incline)
 {
	 uint8_t result;
	 //传入待检原图（含背景）
	 cv::Mat detected;
	 result = bo.RawByteToMat(ImageStream, streamLength, height, width, detected);
	 if (detected.empty() || tt.background.cols != detected.cols)
	 {
		 return 加载图片失败;
	 }
	 float matchRate;
	 result = tt.EvaAnImage(detected, matchRate, incline);
	 if (result != 处理成功)
		 return result;
	 value = matchRate;

	 return 处理成功;
 }

 uint8_t __stdcall SaveTheStream(BYTE* ImageStream, int streamLength, int width, int height, char* filePath)
 {
	 cv::Mat checkImage;
	 int rst = bo.RawByteToMat(ImageStream, streamLength, height, width, checkImage);
	 if (checkImage.empty() || tt.background.cols != checkImage.cols)
	 {
		 return 加载图片失败;
	 }
	 cv::imwrite(filePath, checkImage);
	 return 处理成功;
 }

 uint8_t __stdcall ResetTrade()
{
	 tt.markRegion.clear();
	 tt.markImage.clear();
	 return 处理成功;
}






