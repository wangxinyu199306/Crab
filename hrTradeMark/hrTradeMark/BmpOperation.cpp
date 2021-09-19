#include "stdafx.h"
#include "BmpOperation.h"
#include "opencv2/imgproc/imgproc.hpp"

int BmpOperation::ByteToMat(BYTE* bmpStream, int streamLength, cv::Mat& image)
{
	BYTE* tempPtr = new BYTE[streamLength];
	memcpy(tempPtr, bmpStream, streamLength);
	BYTE* ptr = tempPtr;
	BITMAPFILEHEADER* header = (BITMAPFILEHEADER*)tempPtr;
	int headerlen = (int)header->bfOffBits;
	ptr += sizeof(BITMAPFILEHEADER);
	BITMAPINFOHEADER* info = (BITMAPINFOHEADER*)ptr;
	int row = info->biHeight;
	int col = info->biWidth;
	BYTE* dataPtr = tempPtr + headerlen;
	cv::Mat tempImage(row, col, CV_8UC1, dataPtr);
	image = tempImage.clone();
	if (image.empty())
	{
		delete[] tempPtr;
		return 1;
	}
	cv::flip(image, image, 0);
	delete[] tempPtr;
	return 0;
}

int BmpOperation::GetMatLength(BYTE* originalStream, const cv::Mat& image, int &length)
{
	BITMAPFILEHEADER* header = (BITMAPFILEHEADER*)originalStream;
	int headerlen = header->bfOffBits;
	int size;
	if (image.cols % 4 == 0)
	{
		size = sizeof(BYTE)*image.rows*image.cols;
	}
	else if ((image.cols + 1) % 4 == 0)
	{
		size = sizeof(BYTE)*image.rows*(image.cols + 1);
	}
	else if ((image.cols + 2) % 4 == 0)
	{
		size = sizeof(BYTE)*image.rows*(image.cols + 2);
	}
	else if ((image.cols - 1) % 4 == 0)
	{
		size = sizeof(BYTE)*image.rows*(image.cols - 1);
	}
	length = size + headerlen;
	return 0;
}

int BmpOperation::GetHeaderLength(BYTE* imageStream, int streamLength, int & headerLength)
{
	BYTE* copyPtr = new BYTE[streamLength];
	memcpy(copyPtr, imageStream, streamLength);
	BITMAPFILEHEADER* fileHeadPtr = (BITMAPFILEHEADER*)copyPtr;
	headerLength = (int)fileHeadPtr->bfOffBits;
	delete []copyPtr;
	return 0;
}

int BmpOperation::MatToByte(BYTE* originalStream, const cv::Mat& image, BYTE* outputStream)
{
	if (image.empty())
	{
		return 1;
	}
	cv::Mat outputImage = image.clone();
	if (outputImage.cols % 4 == 0)
	{

	}
	else if ((outputImage.cols + 1) % 4 == 0)
	{
		cv::resize(outputImage, outputImage, cv::Size((image.cols + 1), image.rows));
	}
	else if ((outputImage.cols + 2) % 4 == 0)
	{
		cv::resize(outputImage, outputImage, cv::Size((image.cols + 2), image.rows));
	}
	else if ((outputImage.cols - 1) % 4 == 0)
	{
		cv::resize(outputImage, outputImage, cv::Size((image.cols - 1), image.rows));
	}
	cv::flip(outputImage, outputImage, 0);
	BITMAPFILEHEADER* header = (BITMAPFILEHEADER*)originalStream;
	int headerlen = header->bfOffBits;
	BYTE* imageHeadInfo = new BYTE[headerlen];
	memcpy(imageHeadInfo, originalStream, headerlen);
	BYTE* modiPtr = imageHeadInfo;
	modiPtr += sizeof(BITMAPFILEHEADER);
	BITMAPINFOHEADER* infoModiPtr = (BITMAPINFOHEADER*)modiPtr;
	infoModiPtr->biHeight = outputImage.rows;
	infoModiPtr->biWidth = outputImage.cols;
	std::memcpy(outputStream, imageHeadInfo, headerlen);
	int size = outputImage.rows*outputImage.cols;
	std::memcpy(outputStream + headerlen, outputImage.data, size * sizeof(BYTE));
	delete[] imageHeadInfo;
	return 0;
}

int BmpOperation::RawByteToMat(BYTE* originalStream, int streamLength, int rows, int column, cv::Mat& image)
{
	if (rows*column != streamLength)
	{
		cv::Mat tempImage(rows, column, CV_8UC3, originalStream);
		cv::cvtColor(tempImage, image, CV_BGR2GRAY);
		if (image.empty())
		{
			return 2;
		}
	}
	else
	{
		cv::Mat tempImage(rows, column, CV_8UC1, originalStream);
		image = tempImage.clone();
		if (image.empty())
		{
			return 2;
		}
	}
	return 0;
}

int BmpOperation::MatToRawByte(const cv::Mat& image, BYTE* outputStream)
{
	int size = image.rows*image.cols;
	std::memcpy(outputStream, image.data, size * sizeof(BYTE));
	return 0;
}