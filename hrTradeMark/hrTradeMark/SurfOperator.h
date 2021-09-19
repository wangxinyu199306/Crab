
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
	uint8_t GetPointAndDescriptor(const cv::Mat& image, vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors);

	uint8_t ScreenMatchPoints(const cv::Mat& descriptors1, const cv::Mat& descriptors2, vector<cv::DMatch>& goodMatches);

	uint8_t CalibrationImage(const cv::Mat& templateImage, vector<cv::KeyPoint>& keypoints1, vector<cv::KeyPoint>& keypoints2, vector<cv::DMatch>& goodMatches, const cv::Mat& oriImage, cv::Mat& imageRoi);

	void CalculHomoMatrix(const vector<cv::KeyPoint>& keyPoint1, const vector<cv::KeyPoint>& keyPoint2, const vector<cv::DMatch>& matchePoints, cv::Mat& homo);

	uint8_t TransImage(const vector<cv::KeyPoint>& tradeKeypoints, const vector<cv::KeyPoint>& keypointsROI, const vector<cv::DMatch>& matches, const cv::Mat& bottlePart, const cv::Size& outSize, cv::Mat& tradeRegion, float &incline);

	cv::Rect BoostRegion(const cv::Rect& oriRegion, const cv::Mat& image, float ratio = .1);

	double CalculateSimilarity(const cv::Mat& standard, const cv::Mat& measuring);
private:
	const int minHessian = 400;

	const int minFeaturePoint = 4;//对一张图/一个区域寻找特征点时，结果不能小于这个值
};




#endif