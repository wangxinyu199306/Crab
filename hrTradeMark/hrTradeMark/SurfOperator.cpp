#include "stdafx.h"
#include "SurfOperator.h"

const uint8_t 处理成功 = 0; //PROCESSOK
const uint8_t 加载模板错误 = 1; //LOADTEMPLATEERROR
const uint8_t 加载图片失败 = 2; //LOADIMAGEERROR
const uint8_t 特征不够 = 3;//INSUFFICIENTFEATURE
const uint8_t 特征不匹配 = 4; //FEATUREDISMATCH
const uint8_t 标记区域错误 = 5;//INVAILDCOORDINATE
const uint8_t 没有提取到前景 = 6; //NOFOREGROUND
const uint8_t 透视变换失败 = 7; //HOMOFAILED

const float PI = 3.1415926;

uint8_t SurfOperator::GetPointAndDescriptor(const cv::Mat& image, vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors)
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
		return 特征不够;
	else
		return 处理成功;
}

uint8_t SurfOperator::ScreenMatchPoints(const cv::Mat& descriptors1, const cv::Mat& descriptors2, vector<cv::DMatch>& goodMatches)
{
	cv::BFMatcher matcher(cv::NORM_L2);
	goodMatches.clear();
	vector<vector< cv::DMatch>> matches2;
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

uint8_t SurfOperator::CalibrationImage(const cv::Mat& templateImage, vector<cv::KeyPoint>& keypoints1, vector<cv::KeyPoint>& keypoints2, vector<cv::DMatch>& goodMatches, const cv::Mat& oriImage, cv::Mat& imageRoi)
{
	vector<cv::Point2f> points1, points2;

	if (goodMatches.size() < minFeaturePoint)
	{
		return 特征不匹配;
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

			cv::Rect imgRoi(lt, br);
			cv::Mat tempRoi(oriImage, imgRoi);
			imageRoi = tempRoi.clone();

			//incline = (360 * (atan((points1Projected[1].x - points1Projected[0].x) / (points1Projected[1].y - points1Projected[0].y)) / PI)) - 180;

			return 0;
		}
		else
		{
			return 透视变换失败;
		}
	}
	else
	{
		return 透视变换失败;
	}
}

void SurfOperator::CalculHomoMatrix(const vector<cv::KeyPoint>& keyPoint1, const vector<cv::KeyPoint>& keyPoint2, const vector<cv::DMatch>& matchePoints, cv::Mat& homo)
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

uint8_t SurfOperator::TransImage(const vector<cv::KeyPoint>& tradeKeypoints, const vector<cv::KeyPoint>& keypointsROI, const vector<cv::DMatch>& matches, const cv::Mat& bottlePart, const cv::Size& outSize, cv::Mat& tradeRegion, float &incline)
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

	incline = 0;
	incline += 180 * atan((points1Projected[1].y - points1Projected[0].y) / (points1Projected[1].x - points1Projected[0].x+0.0001)) / PI;
	if (points1Projected[2].y - points1Projected[0].y <= 0)
	{
		incline = incline > 0 ? incline + 180 : incline - 180;
		
	}

	corners[2].x = corners[2].x >= correctBottle.cols ? correctBottle.cols - 1 : corners[2].x;
	corners[2].y = corners[2].y >= correctBottle.rows ? correctBottle.rows - 1 : corners[2].y;
	cv::Mat tempTradeRegion(correctBottle, cv::Rect(corners[0], corners[2]));

	cv::resize(tempTradeRegion, tempTradeRegion, outSize);
	tradeRegion = tempTradeRegion.clone();
	return 处理成功;
}

cv::Rect SurfOperator::BoostRegion(const cv::Rect& oriRegion, const cv::Mat& image, float ratio)
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

	newTLx = oriRegion.x - floor(xRatio*oriWidth) < 0 ? 0 : oriRegion.x - floor(xRatio*oriWidth);
	newTLy = oriRegion.y - floor(yRatio*oriHeight) < 0 ? 0 : oriRegion.y - floor(yRatio*oriHeight);
	newBRx = oriRegion.x + floor((xRatio + 1)*oriWidth) >= image.cols ? image.cols - 1 : oriRegion.x + floor((xRatio + 1)*oriWidth);
	newBRy = oriRegion.y + floor((yRatio + 1)*oriHeight) >= image.rows ? image.rows - 1 : oriRegion.y + floor((yRatio + 1)*oriHeight);

	cv::Rect aRect(cv::Point(newTLx, newTLy), cv::Point(newBRx, newBRy));
	return aRect;
}