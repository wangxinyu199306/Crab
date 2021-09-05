# encoding: utf-8
import os,sys
from ctypes import *

here = os.path.abspath(os.path.dirname(__file__))
dll_path = os.path.join(here, '..\Release\hrTradeMark.dll')


if __name__ == "__main__":
    dll = windll.LoadLibrary(dll_path)

    background_path = os.path.join(here, '../../../Dataset/DedupData1/IMG_3566.JPG')
    print(background_path)
    ret = dll.LoadTemplate(c_int(0), background_path.encode(),c_int(373),c_int(613),c_int(1909),c_int(2685))
    print("step 1: Load Background: {}".format(c_ubyte(ret)))
    
    status_holder = []
    status_holder.append(c_ubyte(dll.LoadTemplate(c_int(2), background_path.encode(), c_int(637),c_int(1361),c_int(833),c_int(1553))))  # 圆标
    status_holder.append(c_ubyte(dll.LoadTemplate(c_int(2), background_path.encode(), c_int(981),c_int(2241),c_int(1309),c_int(2285)))) # 15W40
    status_holder.append(c_ubyte(dll.LoadTemplate(c_int(2), background_path.encode(), c_int(669),c_int(1821),c_int(877),c_int(1933)))) # 新配方新品上市
    status_holder.append(c_ubyte(dll.LoadTemplate(c_int(2), background_path.encode(), c_int(793),c_int(1949),c_int(1425),c_int(2213)))) # 潍柴专用机油
    print("step 2: Set Region: {}".format(status_holder))

    dll.StatusConfirm()
    for order in range(39, 67):
        input_path = os.path.join(here,"../../../Dataset/DedupData1/IMG_35{}.JPG".format(order))
        value = pointer(c_float(0.0))
        incline = pointer(c_float(0.0))
        ret = dll.CheckTradeStatus(input_path.encode(), value, incline)
        print("order:{}, status:{}, value:{}, incline:{}".format(order, c_ubyte(ret),  value.contents.value, incline.contents.value))
    

