# encoding: utf-8
import os,sys
from ctypes import *

here = os.path.abspath(os.path.dirname(__file__))
dll_path = os.path.join(here, '..\Release\hrTradeMark.dll')
dll = windll.LoadLibrary(dll_path)

background_path = os.path.join(here, '../../../Dataset/WrinkleData2/Template1.bmp')
# background_path = os.path.join(here, '../../../Dataset/WrinkleData2/IMG_3566.JPG')

print(background_path)
# ret = dll.LoadTemplate(c_int(0), background_path.encode(),c_int(373),c_int(613),c_int(1909),c_int(2685))  # IMG_3566.JPG
ret = dll.LoadTemplate(c_int(0), background_path.encode(),c_int(460),c_int(300),c_int(885),c_int(700)); # Template1.bmp
ret2 = dll.LoadTemplate(c_int(3), background_path.encode(),c_int(300),c_int(460),c_int(700),c_int(885)); # Template1.bmp


print(c_ubyte(ret))

print(c_ubyte(ret2))
dll.StatusConfirm()

