
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
	uint8_t GetPointAndDescriptor(cv::Mat image, vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors);

	int SaveSurfPoint(vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors, char* surfKeyFile, char*surfDesFile, int& surfNumber);

	int LoadSurfPoint(int surfNumber, char* surfKeyFile, char*surfDesFile, vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors);

	uint8_t ScreenMatchPoints(cv::Mat descriptors1, cv::Mat descriptors2, vector<cv::DMatch>& goodMatches);

	int GetFinalRoi(cv::Mat templateImage,vector<cv::KeyPoint>& keypoints1, vector<cv::KeyPoint>& keypoints2, vector<cv::DMatch>& goodMatches, cv::Mat oriImage, cv::Mat& imageRoi, float& incline);

	int MultiBlockDescriptor(cv::Mat templateImage, vector<vector<cv::KeyPoint>>& blockKeypoints, vector<cv::Mat>& blockDescriptors, cv::Size& blockSize);

private:
	int minHessian = 400;
};


class FrameDif
{
public:
	void GetDifferPart(cv::Mat image, cv::Mat background, cv::Mat& differPart);

	uint8_t ExtractBottle(cv::Mat image, cv::Mat differPart, cv::Mat& bottlePart);
};


class ImageCombination
{
public:
	void GetSiftPoint(cv::Mat image, vector<cv::KeyPoint>& keyPoint, cv::Mat& imageDesc);

	void GetMatchPoint(cv::Mat imageDesc_1, cv::Mat imageDesc_2, vector<cv::DMatch>& matchePoints);

	void CalculHomoMatrix(vector<cv::KeyPoint> keyPoint1, vector<cv::KeyPoint> keyPoint2, vector<cv::DMatch> matchePoints, cv::Mat& homo);

	uint8_t TransImage(vector<cv::KeyPoint> tradeKeypoints, vector<cv::KeyPoint>  keypointsROI, vector<cv::DMatch> matches, cv::Mat bottlePart, cv::Size outSize, cv::Mat& tradeRegion);

	double CalculateSimilarity(cv::Mat standard, cv::Mat measuring);

	double CalculateSimilarityAndReturnLocation(cv::Mat standard, cv::Mat measuring, cv::Point& ltPoint);

	cv::Rect BoostRegion(cv::Rect oriRegion, cv::Mat image, float ratio = .1);
};
#endif