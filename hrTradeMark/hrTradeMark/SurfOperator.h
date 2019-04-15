
#ifndef SURFOPERATOR_H_
#define	SURFOPERATOR_H_

#include "stdafx.h"
#include "opencv.hpp"
#include <stdint.h>
#include "nonfree/nonfree.hpp"

using namespace std;

class SurfOperator
{
public:
	uint8_t GetPointAndDescriptor(cv::Mat image, vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors);

	uint8_t ScreenMatchPoints(cv::Mat descriptors1, cv::Mat descriptors2, vector<cv::DMatch>& goodMatches);

	uint8_t CalibrationImage(cv::Mat templateImage, vector<cv::KeyPoint>& keypoints1, vector<cv::KeyPoint>& keypoints2, vector<cv::DMatch>& goodMatches, cv::Mat oriImage, cv::Mat& imageRoi);

	void CalculHomoMatrix(vector<cv::KeyPoint> keyPoint1, vector<cv::KeyPoint> keyPoint2, vector<cv::DMatch> matchePoints, cv::Mat& homo);

	uint8_t TransImage(vector<cv::KeyPoint> tradeKeypoints, vector<cv::KeyPoint>  keypointsROI, vector<cv::DMatch> matches, cv::Mat bottlePart, cv::Size outSize, cv::Mat& tradeRegion, float &incline);

	cv::Rect BoostRegion(cv::Rect oriRegion, cv::Mat image, float ratio = .1);

	double CalculateSimilarity(cv::Mat standard, cv::Mat measuring);
private:
	const int minHessian = 400;

	const int minFeaturePoint = 4;//对一张图/一个区域寻找特征点时，结果不能小于这个值
};




#endif