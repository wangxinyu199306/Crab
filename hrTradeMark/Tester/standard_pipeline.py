# encoding: utf-8
import os,sys
import time
from ctypes import *

here = os.path.abspath(os.path.dirname(__file__))
dll_path = os.path.join(here, '..\Release\hrTradeMark.dll')
# dll_path = os.path.join(here, '..\OldVersion\hrTradeMark.dll')

if __name__ == "__main__":
    dll = windll.LoadLibrary(dll_path)

    background_path = os.path.join(here, '../../../Dataset/DedupData1/IMG_3566.JPG')
    print(background_path)
    # 第一步通常为加载模板，加载时需要框出商标的范围
    start_time = time.time()
    ret = dll.LoadTemplate(c_int(0), background_path.encode(),c_int(373),c_int(613),c_int(1909),c_int(2685))
    step1_time = time.time()
    print("step 1: Load Background: {}".format(c_ubyte(ret)))
    
    status_holder = []
    # 第二部为框选特征区域，通常为较为清晰的文字或者是图案，当特征区域内的文字或图案相比模板发生变动时，认为错误标注。
    status_holder.append(c_ubyte(dll.LoadTemplate(c_int(2), background_path.encode(), c_int(637),c_int(1361),c_int(833),c_int(1553))))  # 圆标
    status_holder.append(c_ubyte(dll.LoadTemplate(c_int(2), background_path.encode(), c_int(981),c_int(2241),c_int(1309),c_int(2285)))) # 15W40
    status_holder.append(c_ubyte(dll.LoadTemplate(c_int(2), background_path.encode(), c_int(669),c_int(1821),c_int(877),c_int(1933)))) # 新配方新品上市
    status_holder.append(c_ubyte(dll.LoadTemplate(c_int(2), background_path.encode(), c_int(793),c_int(1949),c_int(1425),c_int(2213)))) # 潍柴专用机油
    step2_time = time.time()
    print("step 2: Set Region: {}".format(status_holder))

    dll.StatusConfirm()
    for order in range(39, 67):
        input_path = os.path.join(here,"../../../Dataset/DedupData1/IMG_35{}.JPG".format(order))
        value = pointer(c_float(0.0))
        incline = pointer(c_float(0.0))
        mismatch_region = pointer(c_int(-1))
        ret = dll.CheckTradeStatus(input_path.encode(), value, incline, mismatch_region)
        print("order:{}, status:{}, value:{}, incline:{}, mismatch_region:{}".format(order, c_ubyte(ret),  value.contents.value, incline.contents.value, mismatch_region.contents.value))
    step3_time = time.time()
    print("load image cost : {}".format(step1_time - start_time))
    print("load template cost : {}".format(step2_time - step1_time))
    print("process cost : {}, avg: {}".format(step3_time - step2_time, (step3_time - step2_time)/29.0))
       
    

