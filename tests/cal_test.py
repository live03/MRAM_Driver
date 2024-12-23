import numpy as np
from numpy import ndarray
import ctypes
import time
mram_dll = ctypes.CDLL('./libmram_api.so')

# np.random.seed(172329)
# np.random.seed(312231)

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
    wrow, wcol = 512, 64
    inbits, wbits = 8, 8
    in_group = 1
    # 激活多少行、多少列进行操作
    # TODO: 开启全macro测试会卡死,卡在读
    macro_row, macro_col = 4, 4

    mram_base_addr = 0x0008

    # input vector
    ind_ = np.random.randint(low=0, high=2 ** (inbits - 1 if inbits > 1 else 1), size=(in_group, wrow), dtype=np.uint8)
    # weight matrix
    wd_ = np.random.randint(low=0, high=2 ** (wbits - 1 if wbits > 1 else 1), size=(wcol, wrow), dtype=np.uint8)
    # weight matrix addr base， 随机生成写地址
    # addr_base = np.random.randint(low=0, high=31, size=(macro_row, macro_col), dtype=np.uint8)
    # print(ind_)
    # print()
    # print(wd_)
    # print(ind_.shape)
    # print(wd_.shape)
    soft_result = np.matmul(ind_,wd_.T,dtype=np.uint32)
    print("使用 @ 运算符计算的结果:\n", soft_result)
    # print("使用 @ 运算符计算的结果:\n", result_at.shape)

    # shape of input_vector is [in_group][inbits][16]
    input_vector = bitSlice(ind_)
    # res array shape is [wcol][wbits][16]
    input_weight = bitSlice(wd_)

    weight_buffer = (ctypes.c_uint * (wbits * 16))()

    inArr = (ctypes.c_uint * (in_group * inbits * 16))()
    idx = 0
    for ig in range(in_group):
        for wb in range(inbits):
            for i in range(16):
                inArr[idx] = input_vector[ig, wb, i]
                idx = idx + 1

    # 激活的macro行数
    actived_macro_row = 1 if macro_row != 4 else 4
    # 激活的macro列数
    actived_macro_col = 1 if macro_col != 4 else 4

    for mr_i in range(actived_macro_row):
        for mc_i in range(actived_macro_col):
            # 每次计算的macro行列编号
            mr = macro_row if actived_macro_row == 1 else mr_i
            mc = macro_col if actived_macro_col == 1 else mc_i
            # 写入权重数据
            for ip in range(4):
                idx = 0
                for wb in range(wbits):
                    for i in range(16):
                        weight_buffer[idx] = input_weight[(mr_i * 16 + mc_i * 4 + ip), wb, i]
                        idx = idx + 1
                mram_dll.Send(H2C_DEVICE, mr, mc, ip, mram_base_addr, weight_buffer, wbits * 16)


    res_size = in_group * actived_macro_row * actived_macro_col * 4
    res_buff = (ctypes.c_uint * res_size)()
    mram_dll.Vmmul(H2C_DEVICE,C2H_DEVICE, inArr, in_group * inbits * 16, inbits, in_group, macro_row, macro_col, mram_base_addr, wbits, res_buff)
    
    for i in range(in_group):
        for j in range(res_size // in_group):
            print(res_buff[i * res_size // in_group + j], end= " ")
        print()

cal_test()