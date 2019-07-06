# encoding: utf-8
import os,sys
from ctypes import *

here = os.path.abspath(os.path.dirname(__file__))
dll_path = os.path.join(here, '..\Release\hrTradeMark.dll')
dll = windll.LoadLibrary(dll_path)

background_path = os.path.join(here, '../../../Dataset/DedupData1/IMG_3566.JPG')
print(background_path)
ret = dll.LoadTemplate(c_int(0), background_path.encode(),c_int(373),c_int(613),c_int(1909),c_int(2685))
print(c_ubyte(ret))
dll.StatusC`onfirm()