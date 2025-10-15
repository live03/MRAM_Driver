import numpy as np
from numpy import ndarray
import ctypes
import random
import time
import logging

H2C_DEVICE = b"/dev/xdma0_h2c_0"
C2H_DEVICE = b"/dev/xdma0_c2h_0"
CTRL_DEVICE = b"/dev/xdma0_user"
mram_dll = ctypes.CDLL('./libmram_api.so')

# seed = 172329
seed = 8456

logging.basicConfig(
    level=logging.INFO,  
    format="[%(asctime)s] [%(levelname)s] %(message)s", 
    datefmt="%Y-%m-%d %H:%M:%S",
)

log = logging.getLogger()
log.addHandler(logging.FileHandler("mram_test.log", "w"))

def eq(a, b):
    for i in range(len(a)):
        if a[i]!=b[i]:
            return False
    return True

def program_verify(retry_times,send_data, data_size):
    recv_data = (ctypes.c_int32 * data_size)()
    device_ram_addr = random.randrange(0x0,0x4FFFFFF+1,4)
    for _ in range(retry_times):
        err = mram_dll.Store_Everywhere(H2C_DEVICE, device_ram_addr, send_data, data_size)
        if err:
            log.info("Write Failed!")
            continue
        mram_dll.Load_Everywhere(C2H_DEVICE,device_ram_addr,recv_data,data_size)
        if not eq(send_data, recv_data):
            log.info("Read Failed!")
            continue
        log.info("Success!")
        return True
    return False
 


if __name__ == "__main__":
    # 芯片初始化
    mram_dll.Init()
    retry_times=1
    cmd = (ctypes.c_uint32 * 3)(0x1, 0x2,0x3)
    data_size=3
    program_verify(retry_times,cmd, data_size)