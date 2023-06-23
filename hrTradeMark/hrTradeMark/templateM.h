

#ifndef TEMPLATEM_H_
#define	TEMPLATEM_H_

#include "stdafx.h"
#include "opencv.hpp"
#include "nonfree/nonfree.hpp"
#include <stdint.h>
#include "SurfOperator.h"

using namespace std;
//using namespace cv;

class TemplateTrade:public SurfOperator
{
public:
	cv::Mat background;//背景模板
	cv::Rect tradeOnBackground;//商标在背景上的位置
	cv::Mat tradeImage;//商标模板

	cv::Rect checkShootRegion;//在该区域中寻找模板
	cv::Rect shootingRegion;//在背景中模板的位置
	cv::Mat shootingImage;//从背景上切下来的模板的图片
	cv::Rect defaultShooltRegion;//检测区
	cv::Mat smoothDetectMask;


	vector<cv::KeyPoint> tradeKeypoints;//商标模板的特征点
	cv::Mat tradeDescriptor; //商标模板的特征描述符

	vector<cv::Rect> markRegion;//标记区域在商标模板上的位置
	vector<cv::Mat> markImage;

	//加载背景图，加载商标位置
	uint8_t LoadImageWithPos(const cv::Mat& backgroundImage, const cv::Rect& tradePos);
	
	//加载位置，模式一为加载全局位置，用于检查是否拍照
	//模式二为加载特征位置，用于判断细节
	//模式三为自动检测出大面积的纯色区域，用于阴影检测
	uint8_t SetRegion(cv::Rect region, int mode);

	//通过商标在模板上的位置，确定每次检测是否拍照时需要匹配的区域 -- 已废弃
	uint8_t GetCheckShootRegion();

	//清空所有预存数据
	uint8_t ClearAll();

	//检测是否需要拍照，如果是，则进行判读，如果不是，判定不是后立刻返回 isShoot = false -- 已废弃
	uint8_t CheckShootingAndEva(const cv::Mat& image, bool &isShoot, float& matchRate, float& incline);//检查当前图片是否拍照

	uint8_t EvaAnImage(const cv::Mat& image, float& matchRate, float& incline, int& mismatch_region);

	//把目标图片矫正到 shootingImage相同的坐标系上，输出与尺寸shootImage相同
	uint8_t CaliImage(const cv::Mat& image, cv::Mat& caliedImage, float& incline);

	uint8_t CheckMarkRegion(const cv::Mat& caliedImage);
	// 自动检测模板中的平滑区域
	uint8_t GetSmoothRegions();

protected:
	uint8_t GetSelfKeyPoints();

	uint8_t CheckMatch(const cv::Mat& image, float &status);

	uint8_t CheckMode_1(const cv::Mat& inputImage, const cv::Mat& tempLateImage, float& status);

	uint8_t CheckMode_2(const cv::Mat& inputImage, const cv::Mat& tempLateImage, float& status, cv::Point& tlPoint);
	// 检测平滑区域是否有褶皱出现
	uint8_t CheckMode_3(const cv::Mat& inputImage, float& status);

private:

};

#endif