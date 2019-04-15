
#include "stdafx.h"
#include <stdint.h>
#include <iostream>
#include <tchar.h>
#include <windows.h>
#include "MarkLocator.h"

#define PI 3.1415926

using namespace std;
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

uint8_t SURFOperator::GetPointAndDescriptor(cv::Mat image, vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors)
{
	cv::SurfFeatureDetector detector(minHessian);
	detector.detect(image, keypoints);
	cv::SurfDescriptorExtractor extractor;
	extractor.compute(image, keypoints, descriptors);

	#pragma region draw circle
	//for (int i = 0;  i < keypoints.size(); i++)
	//{
	//	cv::circle(image, keypoints[i].pt, 3, cv::Scalar(0));
	//}
	//cout << "截取的点数" << keypoints.size() << endl;
	//cv::imwrite("E://bottle.bmp", image);
	#pragma endregion

	if (keypoints.size() < minFeaturePoint)
	{
		return 特征不够;
	}
	else
	{
		return 处理成功;
	}
}

uint8_t SURFOperator::ScreenMatchPoints(cv::Mat descriptors1, cv::Mat descriptors2, vector<cv::DMatch>& goodMatches)
{
	cv::BFMatcher matcher(cv::NORM_L2);
	goodMatches.clear();
	vector <vector< cv::DMatch>> matches2;
	matcher.knnMatch(descriptors1, descriptors2, matches2, 2);

	if (matches2.size() < minFeaturePoint)
	{
		return 特征不匹配;
	}
	else if (matches2.size() == minFeaturePoint)
	{
		goodMatches.push_back(matches2[0][0]);
		goodMatches.push_back(matches2[1][0]);
		goodMatches.push_back(matches2[2][0]);
		goodMatches.push_back(matches2[3][0]);
		return 处理成功;
	}
	else
	{
		for (float scr = 0.8; scr <= 1; scr += 0.05)
		{
			vector<vector<cv::DMatch>>::iterator it;
			for (it = matches2.begin(); it != matches2.end(); ++it)
			{
				if ((*it)[0].distance / (*it)[1].distance <= scr)
					goodMatches.push_back((*it)[0]);
			}
			if (goodMatches.size() < minFeaturePoint)
			{
				goodMatches.clear();
				continue;
			}
			else
			{
				return 处理成功;
			}
		}
	}
}



int SURFOperator::GetFinalRoi(cv::Mat templateImage, vector<cv::KeyPoint>& keypoints1, vector<cv::KeyPoint>& keypoints2, vector<cv::DMatch>& goodMatches, cv::Mat oriImage, cv::Mat& imageRoi, float& incline)
{
	vector<cv::Point2f> points1, points2;

	if (goodMatches.size() < 4)
	{
		return 1;
	}

	for (int i = 0; i < goodMatches.size(); i++)
	{
		cv::Point2f tempPoint1(keypoints1[goodMatches[i].queryIdx].pt.x, keypoints1[goodMatches[i].queryIdx].pt.y);
		points1.push_back(tempPoint1);
		cv::Point2f tempPoint2(keypoints2[goodMatches[i].trainIdx].pt.x, keypoints2[goodMatches[i].trainIdx].pt.y);
		points2.push_back(tempPoint2);
	}

	cv::Mat H = cv::findHomography(cv::Mat(points1), cv::Mat(points2), CV_RANSAC);

	vector<cv::Point2f> corners;
	corners.push_back(cv::Point2f(0, 0));
	corners.push_back(cv::Point2f(templateImage.cols - 1, 0));
	corners.push_back(cv::Point2f(templateImage.cols - 1, templateImage.rows - 1));
	corners.push_back(cv::Point2f(0, templateImage.rows - 1));

	vector<cv::Point2f> points1Projected;
	perspectiveTransform(corners, points1Projected, H);

	//perspective out of boundary
	for (int i = 0; i < 4; i++)
	{
		if (points1Projected[i].x > oriImage.cols || points1Projected[i].y > oriImage.rows)
			return 1;
	}

	//the order is invaild for a rectangular
	if (((points1Projected[1].x - points1Projected[0].x)>0) && ((points1Projected[2].x - points1Projected[0].x) > 0) && ((points1Projected[3].x - points1Projected[2].x) < 0))
	{
		if (((points1Projected[2].y - points1Projected[0].y) > 0) && ((points1Projected[3].y - points1Projected[0].y) > 0) && ((points1Projected[1].y - points1Projected[2].y) < 0))
		{
			cv::Point lt, br;
			if (points1Projected[3].x > points1Projected[0].x)
				lt.x = points1Projected[0].x;
			else
				lt.x = points1Projected[3].x;

			if (points1Projected[1].y > points1Projected[0].y)
				lt.y = points1Projected[0].y;
			else
				lt.y = points1Projected[1].y;

			if (points1Projected[3].y > points1Projected[2].y)
				br.y = points1Projected[3].y;
			else
				br.y = points1Projected[2].y;

			if (points1Projected[1].x > points1Projected[2].x)
				br.x = points1Projected[1].x;
			else
				br.x = points1Projected[2].x;

			cv::Rect imgRoi(lt,br);
			cv::Mat tempRoi(oriImage, imgRoi);
			imageRoi = tempRoi.clone();

			incline = (360*(atan((points1Projected[1].x - points1Projected[0].x) / (points1Projected[1].y - points1Projected[0].y))/PI)) - 180;

			return 0;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		return 1;
	}
}

int SURFOperator::MultiBlockDescriptor(cv::Mat templateImage, vector<vector<cv::KeyPoint>>& blockKeypoints, vector<cv::Mat>& blockDescriptors, cv::Size& blockSize)
{
	return 0;
}

void FrameDif::GetDifferPart(cv::Mat image, cv::Mat background, cv::Mat& differPart)
{
	differPart.create(background.size(), background.type());
	differPart.setTo(0);
	for (int r = 0; r < background.rows; r++)
	{
		uchar* backPtr = background.ptr<uchar>(r);
		uchar* imagePtr = image.ptr<uchar>(r);
		uchar* diffPtr = differPart.ptr<uchar>(r);
		for (int c = 0; c < background.cols; c++)
		{
			if (abs(backPtr[c] - imagePtr[c]) > 5)
				diffPtr[c] = 255;
		}
	}
	cv::Mat element = getStructuringElement(cv::MORPH_RECT, cv::Size(15, 15));
	morphologyEx(differPart, differPart, cv::MORPH_CLOSE, element);
	cv::Mat openElement = getStructuringElement(cv::MORPH_RECT, cv::Size(15, 150));
	morphologyEx(differPart, differPart, cv::MORPH_OPEN, openElement);
}

uint8_t FrameDif::ExtractBottle(cv::Mat image, cv::Mat differPart, cv::Mat& bottlePart)
{
	vector<vector<cv::Point>> contours_out;
	cv::findContours(differPart, contours_out, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
	//std::cout << "contours.size:" << contours_out.size() << endl;
	if (contours_out.size() == 0)
	{
		return 没有提取到前景;
	}

	double maxarea = 0;
	int maxAreaIdx;
	for (int index = contours_out.size() - 1; index >= 0; index--)
	{
		double tmparea = fabs(contourArea(contours_out[index]));
		if (tmparea>maxarea)
		{
			maxarea = tmparea;
			maxAreaIdx = index;//记录最大轮廓的索引号  
		}
	}

	cv::Rect area = cv::boundingRect(contours_out[maxAreaIdx]);
	cv::Mat showImage(image, area);
	bottlePart = showImage;
	return 处理成功;
}


void ImageCombination::GetSiftPoint(cv::Mat image, vector<cv::KeyPoint>& keyPoint, cv::Mat& imageDesc)
{
	if (image.channels() == 1)
	{
	}
	else
	{
		cv::cvtColor(image, image, CV_RGB2GRAY);
	}

	//提取特征点    
	cv::SiftFeatureDetector siftDetector(400); // 海塞矩阵阈值  
	siftDetector.detect(image, keyPoint);
	cv::SiftDescriptorExtractor siftDescriptor;
	siftDescriptor.compute(image, keyPoint, imageDesc);
}

void ImageCombination::GetMatchPoint(cv::Mat imageDesc_1, cv::Mat imageDesc_2, vector<cv::DMatch>& matchePoints)
{
	cv::BFMatcher matcher(cv::NORM_L2);
	std::vector<cv::DMatch> matches;
	vector <vector< cv::DMatch>> matches2;
	matcher.knnMatch(imageDesc_1, imageDesc_2, matches2, 2);

	vector<vector<cv::DMatch>>::iterator it;
	for (it = matches2.begin(); it != matches2.end(); ++it)
	{
		if ((*it)[0].distance / (*it)[1].distance < 0.8)
			matches.push_back((*it)[0]);
	}


	sort(matches.begin(), matches.end()); //特征点排序 



	int dotCount;
	if (matches.size() >= 20)
	{
		dotCount = 20;
	}
	else
	{
		dotCount = matches.size();
	}

	for (int i = 0; i < dotCount; i++)
	{
		matchePoints.push_back(matches[i]);
	}


}

void ImageCombination::CalculHomoMatrix(vector<cv::KeyPoint> keyPoint1, vector<cv::KeyPoint> keyPoint2, vector<cv::DMatch> matchePoints, cv::Mat& homo)
{
	vector<cv::Point2f> imagePoints1, imagePoints2;
	for (int i = 0; i < matchePoints.size(); i++)
	{
		imagePoints1.push_back(keyPoint1[matchePoints[i].queryIdx].pt);
		imagePoints2.push_back(keyPoint2[matchePoints[i].trainIdx].pt);
	}
	//获取图像1到图像2的投影映射矩阵，尺寸为3*3  
	homo = findHomography(imagePoints1, imagePoints2, CV_RANSAC);
}

double ImageCombination::CalculateSimilarity(cv::Mat standard, cv::Mat measuring)
{
	//该方法用来判断模板和待测区域之间的相似度
	//待测区域已经过放大

	//计算比值
	int c2rQuade = ceil((float)standard.cols / (float)standard.rows); //列与行的比值
	int r2cQuade = ceil((float)standard.rows / (float)standard.cols); //行与列的比值

	vector<cv::Mat> dBlock;
	int mode = 0;//切割方式,1为针对图像宽且矮
			 //2为图像高且窄
			 //3为4分切割法

	#pragma region 分割进入dBlock
	if (c2rQuade >= 3 && r2cQuade == 1)
	{
		//mode1:特征区域的列比高大两倍以上，将长条切成等比的N份
		mode = 1;
		for (int i = 0; i < c2rQuade; i++)
		{
			float str = (float)i / (float)c2rQuade;
			float end = ((float)i + 1) / (float)c2rQuade;
			cv::Mat tempMat = standard.colRange(floor(str*standard.cols), floor(end*standard.cols - 1));
			dBlock.push_back(tempMat);
		}
	}
	else if (r2cQuade >= 3 && c2rQuade == 1)
	{
		//mode2:特征区域的高比列大两倍以上，将竖条切成等比的N份
		mode = 2;
		for (int i = 0; i < r2cQuade; i++)
		{
			float str = (float)i / (float)r2cQuade;
			float end = ((float)i + 1) / (float)r2cQuade;
			cv::Mat tempMat = standard.rowRange(floor(str*standard.rows), floor(end*standard.rows - 1));
			dBlock.push_back(tempMat);
		}
	}
	else
	{
		//mode3:特征区域宽高比在.5~2之间，将其切割为4份，左上，右上，左下，右下。
		mode = 3;
		cv::Mat tempBlock1(standard, cv::Rect(0, 0, standard.cols / 2, standard.rows / 2));
		dBlock.push_back(tempBlock1);
		cv::Mat tempBlock2(standard, cv::Rect(standard.cols / 2, 0, standard.cols / 2 - 1, standard.rows / 2));
		dBlock.push_back(tempBlock2);
		cv::Mat tempBlock3(standard, cv::Rect(0, standard.rows / 2, standard.cols / 2, standard.rows / 2 - 1));
		dBlock.push_back(tempBlock3);
		cv::Mat tempBlock4(standard, cv::Rect(standard.cols / 2, standard.rows / 2, standard.cols / 2 - 1, standard.rows / 2 - 1));
		dBlock.push_back(tempBlock4);
	}
	#pragma endregion


	vector<double> matchValue;
	vector<int> position;
	int correctDis;
	vector<int> xPosition;
	vector<int> yPosition;
	int correctDisX, correctDisY; 

	for (int i = 0; i < dBlock.size(); i++)
	{

		int result_cols = measuring.cols - dBlock[i].cols + 1;
		int result_rows = measuring.rows - dBlock[i].rows + 1;

		cv::Mat result = cv::Mat(result_cols, result_rows, CV_32FC1);
		cv::matchTemplate(measuring, dBlock[i], result, CV_TM_CCORR_NORMED);

		double minVal, maxVal;
		cv::Point minLoc, maxLoc;
		cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());

		if (mode == 1)
		{
			position.push_back(maxLoc.x);
			correctDis = dBlock[i].cols;

		}
		else if (mode == 2)
		{
			position.push_back(maxLoc.y);
			correctDis = dBlock[i].rows;
	
		}
		else if (mode == 3)
		{
			xPosition.push_back(maxLoc.x);
			yPosition.push_back(maxLoc.y);
			correctDisX = dBlock[i].cols;
			correctDisY = dBlock[i].rows;
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



	double minValue = DBL_MAX;

	for (int i = 0; i < matchValue.size(); i++)
	{
		if (matchValue[i]<minValue)
		    minValue = matchValue[i];
	}

	if (isOK)
	{
		return minValue;
	}
	else
	{
		return -1;
	}


	return 1;
}

double ImageCombination::CalculateSimilarityAndReturnLocation(cv::Mat standard, cv::Mat measuring, cv::Point& ltPoint)
{
	//该方法用来判断模板和待测区域之间的相似度
	//待测区域已经过放大

	//计算比值
	int c2rQuade = ceil((float)standard.cols / (float)standard.rows);
	int r2cQuade = ceil((float)standard.rows / (float)standard.cols);

	vector<cv::Mat> dBlock;
	int mode = 0;//切割方式,1为针对图像宽且矮
	//2为图像高且窄
	//3为4分切割法

#pragma region 分割进入dBlock
	if (c2rQuade >= 3 && r2cQuade == 1)
	{
		//mode1:特征区域的列比高大两倍以上，将长条切成等比的N份
		mode = 1;
		for (int i = 0; i < c2rQuade; i++)
		{
			float str = (float)i / (float)c2rQuade;
			float end = ((float)i + 1) / (float)c2rQuade;
			cv::Mat tempMat = standard.colRange(floor(str*standard.cols), floor(end*standard.cols - 1));
			dBlock.push_back(tempMat);
		}
	}
	else if (r2cQuade >= 3 && c2rQuade == 1)
	{
		//mode2:特征区域的高比列大两倍以上，将竖条切成等比的N份
		mode = 2;
		for (int i = 0; i < r2cQuade; i++)
		{
			float str = (float)i / (float)r2cQuade;
			float end = ((float)i + 1) / (float)r2cQuade;
			cv::Mat tempMat = standard.rowRange(floor(str*standard.rows), floor(end*standard.rows - 1));
			dBlock.push_back(tempMat);
		}
	}
	else
	{
		//mode3:特征区域宽高比在.5~2之间，将其切割为4份，左上，右上，左下，右下。
		mode = 3;
		cv::Mat tempBlock1(standard, cv::Rect(0, 0, standard.cols / 2, standard.rows / 2));
		dBlock.push_back(tempBlock1);
		cv::Mat tempBlock2(standard, cv::Rect(standard.cols / 2, 0, standard.cols / 2 - 1, standard.rows / 2));
		dBlock.push_back(tempBlock2);
		cv::Mat tempBlock3(standard, cv::Rect(0, standard.rows / 2, standard.cols / 2, standard.rows / 2 - 1));
		dBlock.push_back(tempBlock3);
		cv::Mat tempBlock4(standard, cv::Rect(standard.cols / 2, standard.rows / 2, standard.cols / 2 - 1, standard.rows / 2 - 1));
		dBlock.push_back(tempBlock4);
	}
#pragma endregion


	vector<double> matchValue;
	int correctDis;
	vector<int> xPosition;
	vector<int> yPosition;
	int correctDisX, correctDisY;

	for (int i = 0; i < dBlock.size(); i++)
	{

		int result_cols = measuring.cols - dBlock[i].cols + 1;
		int result_rows = measuring.rows - dBlock[i].rows + 1;

		cv::Mat result = cv::Mat(result_cols, result_rows, CV_32FC1);
		cv::matchTemplate(measuring, dBlock[i], result, CV_TM_CCORR_NORMED);

		double minVal, maxVal;
		cv::Point minLoc, maxLoc;
		cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());

		if (mode == 1)
		{
			xPosition.push_back(maxLoc.x);
			yPosition.push_back(maxLoc.y);
			correctDis = dBlock[i].cols;

		}
		else if (mode == 2)
		{
			xPosition.push_back(maxLoc.x);
			yPosition.push_back(maxLoc.y);
			correctDis = dBlock[i].rows;

		}
		else if (mode == 3)
		{
			xPosition.push_back(maxLoc.x);
			yPosition.push_back(maxLoc.y);
			correctDisX = dBlock[i].cols;
			correctDisY = dBlock[i].rows;
		}
		else
		{
			return 标记区域错误;
		}
		double tempDouble = maxVal;
		matchValue.push_back(tempDouble);
	}

	bool isOK = true;
	if (mode == 1 )
	{
		for (int i = 0; i < xPosition.size() - 1; i++)
		{
			if (abs(correctDis - (xPosition[i + 1] - xPosition[i])) <= ceil(.1*correctDis))
			{
				continue;
			}
			else
			{
				isOK = false;
			}

		}
	}
	else if (mode == 2)
	{
		for (int i = 0; i < yPosition.size() - 1; i++)
		{
			if (abs(correctDis - (yPosition[i + 1] - yPosition[i])) <= ceil(.1*correctDis))
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

	ltPoint.x = xPosition[0];
	ltPoint.y = yPosition[0];

	double minValue = DBL_MAX;

	for (int i = 0; i < matchValue.size(); i++)
	{
		if (matchValue[i]<minValue)
			minValue = matchValue[i];
	}

	if (isOK)
	{
		return minValue;
	}
	else
	{
		return -1;
	}


	return 1;
}

uint8_t ImageCombination::TransImage(vector<cv::KeyPoint> tradeKeypoints, vector<cv::KeyPoint>  keypointsROI, vector<cv::DMatch> matches, cv::Mat bottlePart, cv::Size outSize, cv::Mat& tradeRegion)
{
	//与模板的透视变换
	cv::Mat H;
	CalculHomoMatrix(tradeKeypoints, keypointsROI, matches, H);

	vector<cv::Point2f> corners;
	corners.push_back(cv::Point2f(0, 0));
	corners.push_back(cv::Point2f(outSize.width - 1, 0));
	corners.push_back(cv::Point2f(outSize.width - 1, outSize.height - 1));
	corners.push_back(cv::Point2f(0, outSize.height - 1));


	vector<cv::Point2f> points1Projected;
	perspectiveTransform(corners, points1Projected, H);
	cv::Mat Q = cv::findHomography(cv::Mat(points1Projected), cv::Mat(corners), CV_RANSAC);

	cv::Mat correctBottle;
	cv::warpPerspective(bottlePart, correctBottle, Q, cv::Size());

	if (corners[2].x >= correctBottle.cols || corners[2].y >= correctBottle.rows)
	{
		return 透视变换失败;
	}
	cv::Mat tempTradeRegion(correctBottle, cv::Rect(corners[0], corners[2]));
	cv::resize(tempTradeRegion, tempTradeRegion, outSize);
	tradeRegion = tempTradeRegion;
	return 处理成功;
}

cv::Rect ImageCombination::BoostRegion(cv::Rect oriRegion, cv::Mat image, float ratio)
{
	int oriWidth = oriRegion.width;
	int oriHeight = oriRegion.height;
	int newTLx, newTLy, newBRx, newBRy;
	float xRatio, yRatio;
	if (oriWidth > oriHeight)
	{
		yRatio = 2 * ratio;
		xRatio = ratio;
	}
	else
	{
		xRatio = 2 * ratio;
		yRatio = ratio;
	}

	if (oriRegion.x - floor(xRatio*oriWidth) < 0)
	{
		newTLx = 0;
	}
	else
	{
		newTLx = oriRegion.x - floor(xRatio*oriWidth);
	}

	if (oriRegion.y - floor(yRatio*oriHeight) < 0)
	{
		newTLy = 0;
	}
	else
	{
		newTLy = oriRegion.y - floor(yRatio*oriHeight);
	}

	if (oriRegion.x + floor((xRatio + 1)*oriWidth) >= image.cols)
	{
		newBRx = image.cols - 1;
	}
	else
	{
		newBRx = oriRegion.x + floor((xRatio + 1)*oriWidth);
	}

	if (oriRegion.y + floor((yRatio + 1)*oriHeight) >= image.rows)
	{
		newBRy = image.rows - 1;
	}
	else
	{
		newBRy = oriRegion.y + floor((yRatio + 1)*oriHeight);
	}

	cv::Rect aRect(cv::Point(newTLx, newTLy), cv::Point(newBRx, newBRy));
	return aRect;
}

//temporary unusing
int SURFOperator::SaveSurfPoint(vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors, char* surfKeyFile, char*surfDesFile, int& surfNumber)
{
	if (keypoints.size() == 0 || keypoints.size() != descriptors.rows)
	{
		return 1;
	}
	surfNumber = keypoints.size();

	WCHAR surfKeyPath[256];
	memset(surfKeyPath, 0, sizeof(surfKeyPath));
	MultiByteToWideChar(CP_ACP, 0, surfKeyFile, strlen(surfKeyFile) + 1, surfKeyPath,
		sizeof(surfKeyPath) / sizeof(surfKeyPath[0]));

	WCHAR surfDesPath[256];
	memset(surfDesPath, 0, sizeof(surfDesPath));
	MultiByteToWideChar(CP_ACP, 0, surfDesFile, strlen(surfDesFile) + 1, surfDesPath,
		sizeof(surfDesPath) / sizeof(surfDesPath[0]));


	char pointId[256];
	WCHAR wPointId[256];
	for (int i = 0; i < keypoints.size(); i++)
	{
		sprintf(pointId, "Point-%d", i);
		memset(wPointId, 0, sizeof(wPointId));
		MultiByteToWideChar(CP_ACP, 0, pointId, strlen(pointId) + 1, wPointId,
			sizeof(wPointId) / sizeof(wPointId[0]));
		TCHAR tmp[256] = { 0 };
		_stprintf_s(tmp, 256, L"%f", keypoints[i].pt.x);
		if (TRUE != WritePrivateProfileString(wPointId, L"pt.x", tmp, surfKeyPath))
			return 1;
		_stprintf_s(tmp, 256, L"%f", keypoints[i].pt.y);
		if (TRUE != WritePrivateProfileString(wPointId, L"pt.y", tmp, surfKeyPath))
			return 1;
		_stprintf_s(tmp, 256, L"%f", keypoints[i].size);
		if (TRUE != WritePrivateProfileString(wPointId, L"size", tmp, surfKeyPath))
			return 1;
		_stprintf_s(tmp, 256, L"%f", keypoints[i].angle);
		if (TRUE != WritePrivateProfileString(wPointId, L"angle", tmp, surfKeyPath))
			return 1;
		_stprintf_s(tmp, 256, L"%f", keypoints[i].response);
		if (TRUE != WritePrivateProfileString(wPointId, L"response", tmp, surfKeyPath))
			return 1;
		_stprintf_s(tmp, 256, L"%d", keypoints[i].octave);
		if (TRUE != WritePrivateProfileString(wPointId, L"octave", tmp, surfKeyPath))
			return 1;
		_stprintf_s(tmp, 256, L"%d", keypoints[i].class_id);
		if (TRUE != WritePrivateProfileString(wPointId, L"class_id", tmp, surfKeyPath))
			return 1;
	}
	///////////
	char colId[256];
	WCHAR wColId[256];
	for (int i = 0; i < descriptors.rows; i++)
	{
		sprintf(pointId, "Point-%d", i);
		memset(wPointId, 0, sizeof(wPointId));
		MultiByteToWideChar(CP_ACP, 0, pointId, strlen(pointId) + 1, wPointId,
			sizeof(wPointId) / sizeof(wPointId[0]));

		for (int c = 0; c < descriptors.cols; c++)
		{
			sprintf(colId, "Col-%d", c);
			memset(wColId, 0, sizeof(wColId));
			MultiByteToWideChar(CP_ACP, 0, colId, strlen(colId) + 1, wColId,
				sizeof(wColId) / sizeof(wColId[0]));

			TCHAR tmp[256] = { 0 };
			_stprintf_s(tmp, 256, L"%f", descriptors.at<float>(i, c));
			if (TRUE != WritePrivateProfileString(wPointId, wColId, tmp, surfDesPath))
				return 1;
		}
	}
	return 0;
}

int SURFOperator::LoadSurfPoint(int surfNumber, char* surfKeyFile, char*surfDesFile, vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors)
{
	descriptors.create(cv::Size(64, surfNumber), CV_32F);

	WCHAR surfKeyPath[256];
	memset(surfKeyPath, 0, sizeof(surfKeyPath));
	MultiByteToWideChar(CP_ACP, 0, surfKeyFile, strlen(surfKeyFile) + 1, surfKeyPath,
		sizeof(surfKeyPath) / sizeof(surfKeyPath[0]));

	WCHAR surfDesPath[256];
	memset(surfDesPath, 0, sizeof(surfDesPath));
	MultiByteToWideChar(CP_ACP, 0, surfDesFile, strlen(surfDesFile) + 1, surfDesPath,
		sizeof(surfDesPath) / sizeof(surfDesPath[0]));

	char pointId[256];
	WCHAR wPointId[256];
	for (int i = 0; i < surfNumber; i++)
	{
		cv::KeyPoint tempKeyPoint;
		sprintf(pointId, "Point-%d", i);
		memset(wPointId, 0, sizeof(wPointId));
		MultiByteToWideChar(CP_ACP, 0, pointId, strlen(pointId) + 1, wPointId,
			sizeof(wPointId) / sizeof(wPointId[0]));
		TCHAR data[256] = { 0 };
		GetPrivateProfileString(wPointId, L"pt.x", L"0.0", data, 256, surfKeyPath);
		tempKeyPoint.pt.x = _wtof(data);
		GetPrivateProfileString(wPointId, L"pt.y", L"0.0", data, 256, surfKeyPath);
		tempKeyPoint.pt.y = _wtof(data);
		GetPrivateProfileString(wPointId, L"size", L"0.0", data, 256, surfKeyPath);
		tempKeyPoint.size = _wtof(data);
		GetPrivateProfileString(wPointId, L"angle", L"0.0", data, 256, surfKeyPath);
		tempKeyPoint.angle = _wtof(data);
		GetPrivateProfileString(wPointId, L"response", L"0.0", data, 256, surfKeyPath);
		tempKeyPoint.response = _wtof(data);
		GetPrivateProfileString(wPointId, L"octave", L"0", data, 256, surfKeyPath);
		tempKeyPoint.octave = _wtoi(data);
		GetPrivateProfileString(wPointId, L"class_id", L"0", data, 256, surfKeyPath);
		tempKeyPoint.class_id = _wtoi(data);
		keypoints.push_back(tempKeyPoint);
	}

	char colId[256];
	WCHAR wColId[256];
	for (int i = 0; i < descriptors.rows; i++)
	{
		sprintf(pointId, "Point-%d", i);
		memset(wPointId, 0, sizeof(wPointId));
		MultiByteToWideChar(CP_ACP, 0, pointId, strlen(pointId) + 1, wPointId,
			sizeof(wPointId) / sizeof(wPointId[0]));

		for (int c = 0; c < descriptors.cols; c++)
		{
			sprintf(colId, "Col-%d", c);
			memset(wColId, 0, sizeof(wColId));
			MultiByteToWideChar(CP_ACP, 0, colId, strlen(colId) + 1, wColId,
				sizeof(wColId) / sizeof(wColId[0]));

			TCHAR tmp[256] = { 0 };
			GetPrivateProfileString(wPointId, wColId, L"0.0", tmp, 256, surfDesPath);
			descriptors.at<float>(i, c) = _wtof(tmp);
		}
	}
	return 0;
}