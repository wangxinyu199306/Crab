// hrIntegrityDetection.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <iostream>
#include "opencv.hpp"
#include "nonfree/nonfree.hpp"
#include "nonfree/features2d.hpp"
#include <tchar.h>
#include <stdint.h>

using namespace std;

uint8_t __stdcall Initialization() {
	return 0;
}

int __stdcall TestSum(int x1, int x2) {
	return x1 + x2;
}