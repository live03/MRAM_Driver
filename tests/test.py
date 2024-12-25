import numpy as np
from numpy import ndarray
def bitSlice(array: ndarray, bits: int = 8):
    # 0.将int8类型转化为uint8类型,供给unpackbits函数使用,会自动将负数转化为补码
    array_u8 = array.astype(np.uint8)
    # 1.如果小于512行则扩充到512
    array_filled = np.pad(array_u8, ((0, 0), (0, 512-array_u8.shape[-1])), mode='constant', constant_values=0)

    # 2.1 将所有数据转化为二进制表示,目前只支持uint8
    # 转化为二进制后数据shape为 [col][512][8]
    array_filled_binary = np.unpackbits(array_filled[:, :, np.newaxis], axis=2, bitorder='little')
    # 2.2 将所有数据的第n位按照32bit切片
    # 切片后的数据shape为 [col][bits][16]
    res_array = np.zeros((array.shape[0], bits, 16), dtype=np.uint32)
    for bit in range(bits):
        for idx in range(16):
            binary_as_int = np.dot(array_filled_binary[:, idx*32:(idx+1)*32, bit], 2 ** np.arange(32))
            res_array[:, bit, idx] = binary_as_int
    return res_array

def intToBin(number, index, feature=True):
    # index为该数据位宽,number为待转换数据,feature为True则进行十进制转二进制，为False则进行二进制转十进制。"""
    if feature:  # 十进制转换为二进制
        if number >= 0:
            b = bin(number)
            b = '0' * (index + 2 - len(b)) + b
        else:
            b = 2 ** index + number
            b = bin(b)
            b = '1' * (index + 2 - len(b)) + b  # 注意这里算出来的结果是补码
        b = b.replace("0b", '')
        b = b.replace('-', '')
        return b
    elif not feature:  # 二进制转换为十进制
        i = int(str(number), 2)
        if i >= 2 ** (index - 1):  # 如果是负数
            i = -(2 ** index - i)
            return i
        else:
            return i

inbits = 5
# input vector
ind_ = np.random.randint(low=-2 ** (inbits - 1), high=2 ** (inbits - 1), size=(1, 8), dtype=np.int8)
# weight matrix
wd_ = np.random.randint(low=-2 ** (inbits - 1), high=2 ** (inbits - 1 ), size=(4, 8), dtype=np.int8)

soft_result = np.matmul(ind_,wd_.T,dtype=np.int32)

print(intToBin(-5,8))