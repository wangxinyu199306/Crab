#ifndef BMPOPERATION_H_
#define	BMPOPERATION_H_

#include "stdafx.h"
#include <windows.h>
#include "opencv2/core/core.hpp"

class BmpOperation
{
public:
	//transform the byte stream of bmp image to cv::Mat(only keep the data)
	static int ByteToMat(BYTE* bmpStream, int streamLength, cv::Mat& image);
	//the processed image will be transfromed into bmp byte stream, get the length of the byte stream
	//the number of image columns of a bmp must be 4X
	static int GetMatLength(BYTE* originalStream, const cv::Mat& image, int &length);
	//get the length of bmp byte stream header, the header is got from an original byte stream
	static int GetHeaderLength(BYTE* imageStream, int streamLength, int & headerLength);
	//transform the cv::Mat into bmp byte stream, the file header will copy from an original stream
	static int MatToByte(BYTE* originalStream, const cv::Mat& image, BYTE* outputStream);
	//transform the image byte stream(without filehead) to cv::Mat
	static int RawByteToMat(BYTE* originalStream, int streamLength, int rows, int column, cv::Mat& image);
	//transform the  cv::Mat to image byte stream(without filehead)
	static int MatToRawByte(const cv::Mat& image, BYTE* outputStream);
};
#endif