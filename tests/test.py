import numpy as np
from numpy import ndarray
import ctypes
import time
mram_dll = ctypes.CDLL('./libmram_api.so')

np.random.seed(172329)

# array shape is [col][row]
# res array shape is [col][bits][16]


def bitSlice(array: ndarray, bits: int = 8):
    # 1.如果小于512行则扩充到512
    # array shape is [col][row_count]
    # array_filled shape is [col][512]
    array_filled = np.pad(array, ((0, 0), (0, 512-array.shape[-1])), mode='constant', constant_values=0)

    # 2.1 将所有数据转化为二进制表示,目前只支持uint8
    # array_filled_binary shape is [col][512][8]
    array_filled_binary = np.unpackbits(array_filled[:, :, np.newaxis], axis=2, bitorder='little')
    # 2.2 将所有数据的第n位按照32bit切片
    # 切片后的数据shape为 [col][bits][16]
    res_array = np.zeros((array.shape[0], bits, 16), dtype=np.uint32)
    for bit in range(bits):
        for idx in range(16):
            binary_as_int = np.dot(array_filled_binary[:, idx*32:(idx+1)*32, bit], 2 ** np.arange(32))
            res_array[:, bit, idx] = binary_as_int
    return res_array

H2C_DEVICE =  b'/dev/xdma0_h2c_0'
C2H_DEVICE =  b'/dev/xdma0_c2h_0'
CTRL_DEVICE =  b'/dev/xdma0_user'

def cal_test():
    # 定义Macro计算规模，以及输入向量、阵列数据位宽等信息
    # 4*1*1 开启1个Macro, 2*5*3 开启一行Macro, 2*8*8 开启所有Macro
    wrow, wcol = 28, 4
    inbits, wbits = 2, 2
    in_group = 1
    # 激活多少行、多少列进行操作
    macro_row, macro_col = 0, 1
    # input vector
    ind_ = np.random.randint(low=0, high=2 ** inbits - 1, size=(in_group, wrow), dtype=np.uint8)
    # weight matrix
    wd_ = np.random.randint(low=0, high=2 ** wbits - 1, size=(wcol, wrow), dtype=np.uint8)
    # weight matrix addr base， 随机生成写地址
    # addr_base = np.random.randint(low=0, high=31, size=(macro_row, macro_col), dtype=np.uint8)
    # print(ind_)
    # print(ind_.shape)
    # print(wd_.shape)
    soft_result = ind_ @ wd_.T
    print("使用 @ 运算符计算的结果:\n", soft_result)
    # print("使用 @ 运算符计算的结果:\n", result_at.shape)

    input_vector = bitSlice(ind_)
    input_weight = bitSlice(wd_)

    w1 = (ctypes.c_uint * (wbits * 16))()
    w2 = (ctypes.c_uint * (wbits * 16))()
    w3 = (ctypes.c_uint * (wbits * 16))()
    w4 = (ctypes.c_uint * (wbits * 16))()
    idx = 0
    for wb in range(wbits):
        for i in range(16):
            w1[idx] = input_weight[0,wb,i]
            w2[idx] = input_weight[1,wb,i]
            w3[idx] = input_weight[2,wb,i]
            w4[idx] = input_weight[3,wb,i]
            idx = idx + 1


    inArr = (ctypes.c_uint * (in_group * inbits*16))()
    idx = 0
    for wb in range(inbits):
        for i in range(16):
            inArr[idx] = input_vector[0,wb,i]
            idx = idx + 1

    mram_base_addr = 0x0008
    mram_dll.Send(H2C_DEVICE, macro_row, macro_col, 0, mram_base_addr, w1, wbits*16)
    time.sleep(1)
    mram_dll.Send(H2C_DEVICE, macro_row, macro_col, 1, mram_base_addr, w2, wbits*16)
    time.sleep(1)
    mram_dll.Send(H2C_DEVICE, macro_row, macro_col, 2, mram_base_addr, w3, wbits*16)
    time.sleep(1)
    mram_dll.Send(H2C_DEVICE, macro_row, macro_col, 3, mram_base_addr, w4, wbits*16)
    time.sleep(1)
    res_buff = (ctypes.c_uint * (in_group * 1 * 4))()

    mram_dll.Vmmul(H2C_DEVICE,C2H_DEVICE, inArr, in_group * inbits*16, inbits, in_group, macro_row, macro_col, mram_base_addr, wbits, res_buff)
    
    time.sleep(1)

    host_buffer = (ctypes.c_uint * 1)()
    mram_dll.Recv(H2C_DEVICE,C2H_DEVICE,macro_row,macro_col,0,mram_base_addr,host_buffer,1)
    print(res_buff)

    for i in range(4):
        print(res_buff[i])
    # print(host_buffer[0])
    # print(input_weight[0,0,0])
cal_test()
