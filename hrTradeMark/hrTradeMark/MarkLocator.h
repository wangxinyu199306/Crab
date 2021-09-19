
#ifndef MARKLOCATOR_H_
#define	MARKLOCATOR_H_

#include "stdafx.h"
#include "opencv.hpp"
#include "nonfree/nonfree.hpp"
#include "nonfree/features2d.hpp"
#include <stdint.h>

using namespace std;



class SURFOperator
{
public:
	uint8_t GetPointAndDescriptor(const cv::Mat& image, vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors);

	int SaveSurfPoint(vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors, char* surfKeyFile, char*surfDesFile, int& surfNumber);

	int LoadSurfPoint(int surfNumber, char* surfKeyFile, char*surfDesFile, vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors);

	uint8_t ScreenMatchPoints(const cv::Mat& descriptors1, const cv::Mat& descriptors2, vector<cv::DMatch>& goodMatches);

	int GetFinalRoi(const cv::Mat& templateImage, vector<cv::KeyPoint>& keypoints1, vector<cv::KeyPoint>& keypoints2, vector<cv::DMatch>& goodMatches, const cv::Mat& oriImage, cv::Mat& imageRoi, float& incline);

	int MultiBlockDescriptor(const cv::Mat& templateImage, vector<vector<cv::KeyPoint>>& blockKeypoints, vector<cv::Mat>& blockDescriptors, cv::Size& blockSize);

private:
	int minHessian = 400;
};


class FrameDif
{
public:
	void GetDifferPart(const cv::Mat& image, const cv::Mat& background, cv::Mat& differPart);

	uint8_t ExtractBottle(const cv::Mat& image, const cv::Mat& differPart, cv::Mat& bottlePart);
};

class ImageCombination
{
public:
	void GetSiftPoint(const cv::Mat& image, vector<cv::KeyPoint>& keyPoint, cv::Mat& imageDesc);

	void GetMatchPoint(const cv::Mat& imageDesc_1, const cv::Mat& imageDesc_2, vector<cv::DMatch>& matchePoints);

	void CalculHomoMatrix(const vector<cv::KeyPoint>& keyPoint1, const vector<cv::KeyPoint>& keyPoint2, const vector<cv::DMatch>& matchePoints, cv::Mat& homo);

	uint8_t TransImage(const vector<cv::KeyPoint>& tradeKeypoints, const vector<cv::KeyPoint>& keypointsROI, const vector<cv::DMatch>& matches, const cv::Mat& bottlePart, const cv::Size& outSize, cv::Mat& tradeRegion);

	double CalculateSimilarity(const cv::Mat& standard, const cv::Mat& measuring);

	double CalculateSimilarityAndReturnLocation(const cv::Mat& standard, const cv::Mat& measuring, cv::Point& ltPoint);

	cv::Rect BoostRegion(const cv::Rect& oriRegion, const cv::Mat& image, float ratio = .1);
};
#endif