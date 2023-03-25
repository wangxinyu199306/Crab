# encoding: utf-8
import os,sys
from ctypes import *

here = os.path.abspath(os.path.dirname(__file__))
dll_path = os.path.join(here, '..\Release\hrIntegrityDetection.dll')
dll = windll.LoadLibrary(dll_path)

ret = dll.TestSum(c_int(2), c_int(3)); # Template1.bmp

print(c_int(ret))
# print(c_ubyte(ret))


