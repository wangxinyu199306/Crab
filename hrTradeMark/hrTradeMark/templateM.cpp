#include "stdafx.h"
#include <stdint.h>
#include <iostream>
#include <tchar.h>
#include <windows.h>
#include <algorithm>
#include <math.h>
#include "templateM.h"


using namespace std;

const uint8_t 处理成功 = 0; //PROCESSOK
const uint8_t 加载模板错误 = 1; //LOADTEMPLATEERROR      在商标上框选模板时，框面积为0或超出图片边界  
const uint8_t 加载图片失败 = 2; //LOADIMAGEERROR         各个阶段传进来的图片不可用，都会出现这种情况
const uint8_t 特征不够 = 3;//INSUFFICIENTFEATURE         传入的图片或框选的区域过黑，过亮，模糊等，导致算法计算不了特征
const uint8_t 特征不匹配 = 4; //FEATUREDISMATCH          传入图片与模板之间找不到匹配的点，通常情况下这种错误可以报不匹配
const uint8_t 标记区域错误 = 5;//INVAILDCOORDINATE       框选特征区域时，框面积为0或超出图片边界
const uint8_t 没有提取到前景 = 6; //NOFOREGROUND         根本没写这个逻辑，这是原来要传背景时留下来的
const uint8_t 透视变换失败 = 7; //HOMOFAILED             这个是数学问题，出这个的时候可以报不匹配


const uint8_t 上位机调用错误 = 98;


const int 标记区域最小像素面积 = 96;
const int 标记区域最小像素边长 = 15;


uint8_t TemplateTrade::LoadImageWithPos(const cv::Mat& backgroundImage, const cv::Rect& tradePos)
{
	if (backgroundImage.empty())
		return 加载图片失败;
	else
		background = backgroundImage.clone();
	if (tradePos.area() <= 0 || tradePos.x + tradePos.width >= background.cols || tradePos.y + tradePos.height >= background.rows)
		return 加载模板错误;
	else
	{
		tradeImage = backgroundImage(tradePos).clone();
		tradeOnBackground = tradePos;
		uint8_t result = GetSelfKeyPoints();
		return result;
	}
}

//标记模式一区域仅用来检测是否拍照，区域存在于背景内即可
//标记模式二区域用来精检，特征一定要存在于商标区域内
uint8_t TemplateTrade::SetRegion(cv::Rect region, int mode)
{
	if (mode == 1)
	{
		if (region.x + region.width >= background.cols || region.y + region.height >= background.rows)
		{
			return 标记区域错误;
		}
		else if (region.area() <= 标记区域最小像素面积 || min(region.width, region.height) < 标记区域最小像素边长)
		{
			return 特征不够;
		}
		else
		{
			shootingRegion = region;
			shootingImage = background(shootingRegion);
			return 处理成功;
		}
	}
	else if (mode == 2)
	{
		if (region.area() <= 0 || \
			region.x + region.width >= tradeOnBackground.x + tradeOnBackground.width || \
			region.y + region.height >= tradeOnBackground.y + tradeOnBackground.height || \
			region.x <= tradeOnBackground.x || \
			region.y <= tradeOnBackground.y)
		{
			return 标记区域错误;
		}
		else if (region.area() <= 标记区域最小像素面积 || min(region.width, region.height) < 标记区域最小像素边长)
		{
			region.x -= tradeOnBackground.x;
			region.y -= tradeOnBackground.y;
			markRegion.push_back(region);
			markImage.push_back(background(markRegion.back()));
			return 特征不够;
		}
		else
		{
			region.x -= tradeOnBackground.x;
			region.y -= tradeOnBackground.y;
			markRegion.push_back(region);
			markImage.push_back(background(markRegion.back()));
			return 处理成功;
		}
	}
	else
	{
		return 上位机调用错误;
	}
}


uint8_t TemplateTrade::ClearAll()
{
	tradeKeypoints.clear();
	markRegion.clear();
	markImage.clear();
	return 处理成功;
}

uint8_t TemplateTrade::CheckShootingAndEva(const cv::Mat& image, bool &isShoot, float& matchRate, float& incline)
{
	if (image.cols != background.cols || image.rows != background.rows)
		return 加载图片失败;
	float status;
	cv::Point matchedTLPoints;
	uint8_t result = CheckMode_2(image(defaultShooltRegion), shootingImage, status, matchedTLPoints);
	if (result != 处理成功)
		return  result;

	if (status > 0.98)
		isShoot = true;
	else
	{
		isShoot = false;
		return 处理成功;
	}

	int relatX = tradeOnBackground.x - shootingRegion.x;
	int relatY = tradeOnBackground.y - shootingRegion.y;


	//--,----------,--
	cv::Point tradeROIlt, tradeROIbr;
	tradeROIlt.x = matchedTLPoints.x + relatX - int(0.2*tradeImage.cols) < 0 ? 0 : matchedTLPoints.x + relatX - int(0.2*tradeImage.cols);
	tradeROIlt.y = matchedTLPoints.y + relatY - int(0.2*tradeImage.rows) < 0 ? 0 : matchedTLPoints.y + relatY - int(0.2*tradeImage.rows);
	tradeROIbr.x = tradeROIlt.x + int(1.4*tradeImage.cols) >= image.cols ? image.cols - 1 : tradeROIlt.x + int(1.4*tradeImage.cols);
	tradeROIbr.y = tradeROIlt.y + int(1.4*tradeImage.rows) >= image.rows ? image.rows - 1 : tradeROIlt.y + int(1.4*tradeImage.rows);
	cv::Rect tradeRegionInImage(tradeROIlt, tradeROIbr);
	//根据检测到的标记匹配点，推算商标区域所在的位置


	cv::Mat caliedImage;
	result = CaliImage(image(tradeRegionInImage), caliedImage, incline);
	if (result != 处理成功)
		return  result;
	//cv::imwrite("D:\\310test2\\image(tradeRegionInImage).jpg", image(tradeRegionInImage));
	//cv::imwrite("D:\\310test2\\caliedImage.jpg", caliedImage);
	//比对标记区域
	//cv::imwrite("D://310test2//123.png", tradeRegion);
	double minValue = DBL_MAX;
	for (int i = 0; i < markRegion.size(); i++)
	{

		cv::Mat standard(tradeImage, markRegion[i]);//商标模板的区域
		cv::Rect boostRegion = BoostRegion(markRegion[i], tradeImage);
		uint8_t result = CheckMode_1(caliedImage(boostRegion), standard, status);
		if (status < minValue)
		{
			minValue = status;
		}
	}

	if (minValue == -1)
	{
		matchRate = 1;
	}
	else if (minValue < 0.94)
	{
		matchRate = 2;
	}
	else
	{
		matchRate = 0;
	}
}

uint8_t TemplateTrade::EvaAnImage(const cv::Mat& image, float& matchRate, float& incline)
{
	uint8_t result;
	cv::Mat caliedImage;
	result = CaliImage(image, caliedImage, incline);
	if (result != 处理成功)
		return result;
	float status;
	double minValue = DBL_MAX;
	for (int i = 0; i < markRegion.size(); i++)
	{
		cv::Mat standard(tradeImage, markRegion[i]);//商标模板的区域
		cv::Rect boostRegion = BoostRegion(markRegion[i], tradeImage);
		uint8_t result = CheckMode_1(caliedImage(boostRegion), standard, status);
		if (status < minValue)
		{
			minValue = status;
		}
	}

	if (minValue == -1)
	{
		matchRate = 1;
	}
	else if (minValue < 0.94)
	{
		matchRate = 2;
	}
	else
	{
		matchRate = 0;
	}
	return 处理成功;
}

uint8_t TemplateTrade::CaliImage(const cv::Mat& image, cv::Mat& caliedImage,float& incline)
{
	uint8_t result;
	//外接矩ROI区域特征点提取
	vector<cv::KeyPoint>  keypointsROI;
	cv::Mat descriptorsROI;
	result = GetPointAndDescriptor(image, keypointsROI, descriptorsROI);
	if (result != 处理成功)
		return result;
	//模板特征点匹配
	vector<cv::DMatch> matches;
	result = ScreenMatchPoints(tradeDescriptor, descriptorsROI, matches);
	if (result != 处理成功)
		return result;
	//变换并截取图像
	result = TransImage(tradeKeypoints, keypointsROI, matches, image, tradeImage.size(), caliedImage, incline);
	if (result != 处理成功)
		return result;
	else
		return 处理成功;
}

uint8_t  TemplateTrade::GetCheckShootRegion()
{
	int neceRegionLeft = min(shootingRegion.x, tradeOnBackground.x);
	int neceRegionRight = max(shootingRegion.x + shootingRegion.width, tradeOnBackground.x + tradeOnBackground.width);
	int neceRegionWidth = neceRegionRight - neceRegionLeft;

	cv::Point lt, br;
	lt.y = max(0, int(shootingRegion.y - .5*shootingRegion.height));
	br.y = min(background.rows - 1, int(shootingRegion.y + 1.5*shootingRegion.height));
	lt.x = max(0, int((background.cols - neceRegionWidth) / 2));
	br.x = min(background.cols - 1, int((background.cols + neceRegionWidth) / 2));
	cv::Rect tempCheckRegion(lt, br);
	defaultShooltRegion = tempCheckRegion;

	return 处理成功;
}


uint8_t TemplateTrade::GetSelfKeyPoints()
{
	uint8_t result;
	if (tradeImage.empty())
		return 加载图片失败;
	else
	{
		result = GetPointAndDescriptor(tradeImage, tradeKeypoints, tradeDescriptor);
		return result;
	}
}

uint8_t TemplateTrade::CheckMode_1(const cv::Mat& inputImage, const cv::Mat& templateImage, float &status)
{
	//该方法用来判断模板和待测区域之间的相似度
	//待测区域已经过放大

	//计算比值
	int c2rRatio = ceil((float)templateImage.cols / (float)templateImage.rows); //列与行的比值
	int r2cRatio = ceil((float)templateImage.rows / (float)templateImage.cols); //行与列的比值

	vector<cv::Mat> blockArray;
	int mode = 0;//切割方式,1为针对图像宽且矮
	//2为图像高且窄
	//3为4分切割法

	#pragma region 分割进入blockArray
	if (c2rRatio >= 3 && r2cRatio == 1)
	{
		//mode1:特征区域的列比高大两倍以上，将长条切成等比的N份
		mode = 1;
		for (int i = 0; i < c2rRatio; i++)
		{
			float str = (float)i / (float)c2rRatio;
			float end = ((float)i + 1) / (float)c2rRatio;
			cv::Mat tempMat = templateImage.colRange(floor(str*templateImage.cols), floor(end*templateImage.cols - 1));
			blockArray.push_back(tempMat);
		}
	}
	else if (r2cRatio >= 3 && c2rRatio == 1)
	{
		//mode2:特征区域的高比列大两倍以上，将竖条切成等比的N份
		mode = 2;
		for (int i = 0; i < r2cRatio; i++)
		{
			float str = (float)i / (float)r2cRatio;
			float end = (float)(i + 1) / (float)r2cRatio;
			cv::Mat tempMat = templateImage.rowRange(floor(str*templateImage.rows), floor(end*templateImage.rows - 1));
			blockArray.push_back(tempMat);
		}
	}
	else
	{
		//mode3:特征区域宽高比在.5~2之间，将其切割为4份，左上，右上，左下，右下。
		mode = 3;
		cv::Mat tempBlock1(templateImage, cv::Rect(0, 0, templateImage.cols / 2, templateImage.rows / 2));
		blockArray.push_back(tempBlock1);
		cv::Mat tempBlock2(templateImage, cv::Rect(templateImage.cols / 2, 0, templateImage.cols / 2 - 1, templateImage.rows / 2));
		blockArray.push_back(tempBlock2);
		cv::Mat tempBlock3(templateImage, cv::Rect(0, templateImage.rows / 2, templateImage.cols / 2, templateImage.rows / 2 - 1));
		blockArray.push_back(tempBlock3);
		cv::Mat tempBlock4(templateImage, cv::Rect(templateImage.cols / 2, templateImage.rows / 2, templateImage.cols / 2 - 1, templateImage.rows / 2 - 1));
		blockArray.push_back(tempBlock4);
	}
	#pragma endregion

	vector<double> matchValue;
	vector<int> position;
	int correctDis;
	vector<int> xPosition;
	vector<int> yPosition;
	int correctDisX, correctDisY;

	#pragma region 计算分区匹配位置
	double minVal, maxVal;
	cv::Point minLoc, maxLoc;
	for (int i = 0; i < blockArray.size(); i++)
	{

		int result_cols = inputImage.cols - blockArray[i].cols + 1;
		int result_rows = inputImage.rows - blockArray[i].rows + 1;

		cv::Mat result = cv::Mat(result_cols, result_rows, CV_32FC1);
		cv::matchTemplate(inputImage, blockArray[i], result, CV_TM_CCORR_NORMED);

		cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());

		if (mode == 1)
		{
			position.push_back(maxLoc.x);
			correctDis = blockArray[i].cols;
		}
		else if (mode == 2)
		{
			position.push_back(maxLoc.y);
			correctDis = blockArray[i].rows;
		}
		else if (mode == 3)
		{
			xPosition.push_back(maxLoc.x);
			yPosition.push_back(maxLoc.y);
			correctDisX = blockArray[i].cols;
			correctDisY = blockArray[i].rows;
		}
		else
		{
			return 标记区域错误;
		}
		double tempDouble = maxVal;
		matchValue.push_back(tempDouble);
	}


	//for (int i = 0; i < position.size(); i++)
	//{
	//	cout << position[i] << "  ";
	//}
	//cout << endl;


	bool isOK = true;
	if (mode == 1 || mode == 2)
	{
		for (int i = 0; i < position.size() - 1; i++)
		{
			if (abs(correctDis - (position[i + 1] - position[i])) <= ceil(.1*correctDis))
			{
				continue;
			}
			else
			{
				isOK = false;
			}
		}
	}
	else if (mode == 3)
	{
		if (abs(correctDisX - (xPosition[1] - xPosition[0])) <= ceil(.1*correctDisX) \
			&& abs(correctDisX - (xPosition[3] - xPosition[2])) <= ceil(.1*correctDisX)\
			&& abs(xPosition[1] - xPosition[3]) <= ceil(.1*correctDisX)\
			&& abs(correctDisY - (yPosition[2] - yPosition[0])) <= ceil(.1*correctDisY)\
			&& abs(correctDisY - (yPosition[3] - yPosition[1])) <= ceil(.1*correctDisY)\
			&& abs(yPosition[2] - yPosition[3]) <= ceil(.1*correctDisY))
		{
			isOK = true;
		}
		else
		{
			isOK = false;
		}
	}
	#pragma endregion

	double minValue = DBL_MAX;

	for (int i = 0; i < matchValue.size(); i++)
	{
		if (matchValue[i]<minValue)
			minValue = matchValue[i];
	}

	if (isOK)
	{
		status = minValue;
		return 处理成功;
	}
	else
	{
		status = -1;
		return 处理成功;
	}
}

uint8_t TemplateTrade::CheckMode_2(const cv::Mat& inputImage, const cv::Mat& templateImage, float &status, cv::Point& tlPoint)
{
	//该方法用来判断模板和待测区域之间的相似度
	//待测区域已经过放大

	//计算比值
	int c2rRatio = ceil((float)templateImage.cols / (float)templateImage.rows); //列与行的比值
	int r2cRatio = ceil((float)templateImage.rows / (float)templateImage.cols); //行与列的比值

	vector<cv::Mat> blockArray;
	int mode = 0;//切割方式,1为针对图像宽且矮
	//2为图像高且窄
	//3为4分切割法

	#pragma region 分割进入blockArray
	if (c2rRatio >= 3 && r2cRatio == 1)
	{
		//mode1:特征区域的列比高大两倍以上，将长条切成等比的N份
		mode = 1;
		for (int i = 0; i < c2rRatio; i++)
		{
			float str = (float)i / (float)c2rRatio;
			float end = ((float)i + 1) / (float)c2rRatio;
			cv::Mat tempMat = templateImage.colRange(floor(str*templateImage.cols), floor(end*templateImage.cols - 1));
			blockArray.push_back(tempMat);
		}
	}
	else if (r2cRatio >= 3 && c2rRatio == 1)
	{
		//mode2:特征区域的高比列大两倍以上，将竖条切成等比的N份
		mode = 2;
		for (int i = 0; i < r2cRatio; i++)
		{
			float str = (float)i / (float)r2cRatio;
			float end = (float)(i + 1) / (float)r2cRatio;
			cv::Mat tempMat = templateImage.rowRange(floor(str*templateImage.rows), floor(end*templateImage.rows - 1));
			blockArray.push_back(tempMat);
		}
	}
	else
	{
		//mode3:特征区域宽高比在.5~2之间，将其切割为4份，左上，右上，左下，右下。
		mode = 3;
		cv::Mat tempBlock1(templateImage, cv::Rect(0, 0, templateImage.cols / 2, templateImage.rows / 2));
		blockArray.push_back(tempBlock1);
		cv::Mat tempBlock2(templateImage, cv::Rect(templateImage.cols / 2, 0, templateImage.cols / 2 - 1, templateImage.rows / 2));
		blockArray.push_back(tempBlock2);
		cv::Mat tempBlock3(templateImage, cv::Rect(0, templateImage.rows / 2, templateImage.cols / 2, templateImage.rows / 2 - 1));
		blockArray.push_back(tempBlock3);
		cv::Mat tempBlock4(templateImage, cv::Rect(templateImage.cols / 2, templateImage.rows / 2, templateImage.cols / 2 - 1, templateImage.rows / 2 - 1));
		blockArray.push_back(tempBlock4);
	}
	#pragma endregion

	vector<double> matchValue;
	vector<int> position;
	int correctDis;
	vector<int> xPosition;
	vector<int> yPosition;
	int correctDisX, correctDisY;

	#pragma region 计算分区匹配位置
	double minVal, maxVal;
	cv::Point minLoc, maxLoc;
	for (int i = 0; i < blockArray.size(); i++)
	{

		int result_cols = inputImage.cols - blockArray[i].cols + 1;
		int result_rows = inputImage.rows - blockArray[i].rows + 1;

		cv::Mat result = cv::Mat(result_cols, result_rows, CV_32FC1);
		cv::matchTemplate(inputImage, blockArray[i], result, CV_TM_CCORR_NORMED);
		cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());
		if (i == 0)
		{
			tlPoint.x = maxLoc.x;
			tlPoint.y = maxLoc.y;
		}

		if (mode == 1)
		{
			position.push_back(maxLoc.x);
			correctDis = blockArray[i].cols;
		}
		else if (mode == 2)
		{
			position.push_back(maxLoc.y);
			correctDis = blockArray[i].rows;
		}
		else if (mode == 3)
		{
			xPosition.push_back(maxLoc.x);
			yPosition.push_back(maxLoc.y);
			correctDisX = blockArray[i].cols;
			correctDisY = blockArray[i].rows;
		}
		else
		{
			return 标记区域错误;
		}
		double tempDouble = maxVal;
		matchValue.push_back(tempDouble);
	}

	bool isOK = true;
	if (mode == 1 || mode == 2)
	{
		for (int i = 0; i < position.size() - 1; i++)
		{
			if (abs(correctDis - (position[i + 1] - position[i])) <= ceil(.1*correctDis))
			{
				continue;
			}
			else
			{
				isOK = false;
			}
		}
	}
	else if (mode == 3)
	{
		if (abs(correctDisX - (xPosition[1] - xPosition[0])) <= ceil(.1*correctDisX) \
			&& abs(correctDisX - (xPosition[3] - xPosition[2])) <= ceil(.1*correctDisX)\
			&& abs(xPosition[1] - xPosition[3]) <= ceil(.1*correctDisX)\
			&& abs(correctDisY - (yPosition[2] - yPosition[0])) <= ceil(.1*correctDisY)\
			&& abs(correctDisY - (yPosition[3] - yPosition[1])) <= ceil(.1*correctDisY)\
			&& abs(yPosition[2] - yPosition[3]) <= ceil(.1*correctDisY))
		{
			isOK = true;
		}
		else
		{
			isOK = false;
		}
	}
	#pragma endregion

	double minValue = DBL_MAX;

	for (int i = 0; i < matchValue.size(); i++)
	{
		if (matchValue[i]<minValue)
			minValue = matchValue[i];
	}

	if (isOK)
	{
		status = minValue;
		return 处理成功;
	}
	else
	{
		status = -1;
		return 处理成功;
	}
}

uint8_t TemplateTrade::GetSmoothRegions()
{
	// tradeImage
	int DETECTLENGTH = 6;
	int DETECTACCURACY = 10;
	int detect_lower_index = int(4 * DETECTLENGTH * DETECTLENGTH * 0.15);
	int detect_upper_index = int(4 * DETECTLENGTH * DETECTLENGTH * 0.85);
	int width = tradeImage.cols;
	int height = tradeImage.rows;

	smoothDetectMask.create(tradeImage.size(), tradeImage.type());
	smoothDetectMask.setTo(0);

	for (int r = DETECTLENGTH; r < height - DETECTLENGTH; r++) {
		uchar* row_ptr = smoothDetectMask.ptr<uchar>(r);
		for (int c = DETECTLENGTH; c < width - DETECTLENGTH; c++) {
			uchar _min = 255, _max = 0;
			bool is_smooth_pixel = true;
			std::vector<uchar> temp_container;
			temp_container.reserve(4 * DETECTLENGTH*DETECTLENGTH);
			for (int i = r - DETECTLENGTH; i <= r + DETECTLENGTH; i++){
				for (int j = c - DETECTLENGTH; j <= c + DETECTLENGTH; j++) {
					temp_container.push_back(tradeImage.at<uchar>(i, j));	
				}
			}
			std::sort(temp_container.begin(), temp_container.end());
			if (temp_container[detect_upper_index] - temp_container[detect_lower_index] < DETECTACCURACY) {
				row_ptr[c] = 255;
			}
		}
	}
	cv::imwrite("tradeImage.png", tradeImage);
	cv::imwrite("smoothDetectMask.png", smoothDetectMask);
	return 处理成功;



}