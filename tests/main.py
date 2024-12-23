import numpy as np
from numpy import ndarray


coeFile_enable = True

# import string

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


# Macro仿真
def Macro(inb: ndarray, wb: ndarray, output_buffer: ndarray, col: int, n: int, input_lshift: int, weight_lshift: int,
          input_sign: bool, weight_sign: bool, sa_bypass: bool) -> ndarray:
    # 流水线各阶段latch
    one_bit_mul_res = np.empty(shape=(n, col), dtype=np.uint8)
    column_wise_sum = np.empty(shape=(n, 1), dtype=np.uint16)
    after_lshift_res = np.empty(shape=(n, 1), dtype=np.uint32)
    output_res = np.empty(shape=(n, 1), dtype=np.int32)
    # and gate matrix
    one_bit_mul_res = wb * inb.T
    # 流水加法树，此处为了简单起见，没有实现树形加法，而是直接调库
    column_wise_sum = one_bit_mul_res.sum(axis=1)

    if sa_bypass:  # 只用于测试能效，不用于测试1~8bit计算
        output_res = column_wise_sum
    else:
        # 左移操作
        after_lshift_res = column_wise_sum.__lshift__(input_lshift + weight_lshift)
        after_lshift_res = after_lshift_res.reshape(-1, 1)  # 此条语句纯粹是为了保证ndarray shape相同，无特殊意义
        # partial VMM result sum
        if input_sign ^ weight_sign:
            output_res = output_buffer - after_lshift_res
        else:
            output_res = output_buffer + after_lshift_res
    # output
    return output_res


# ! 本次未使用
def main():
    # 定义Macro计算规模，以及输入向量、阵列数据位宽
    col, n = 512, 4

    inbits, wbits = 1, 1

    # 随机生成输入向量以及矩阵
    ind = np.random.randint(low=-2 ** (inbits - 1), high=2 ** (inbits - 1) - 1, size=(col, 1),
                            dtype=np.int32)  # input vector
    wd = np.random.randint(low=-2 ** (wbits - 1), high=2 ** (wbits - 1) - 1, size=(n, col),
                           dtype=np.int32)  # weight matrix

    print(ind)
    print(wd)
    with open('1.txt', 'w') as f:
        f.write('bin')
    for i in range(512):
        txt_in = intToBin(ind[i, 0], inbits, True)
        with open('1.txt', 'a') as f:
            f.write('\n' + txt_in)

    with open('2.txt', 'w') as f:
        f.write('bin')
    for i in range(512):
        txt_in = intToBin(wd[0, i], wbits, True)
        with open('2.txt', 'a') as f:
            f.write('\n' + txt_in)

    with open('3.txt', 'w') as f:
        f.write('bin')
    for i in range(512):
        txt_in = intToBin(wd[1, i], wbits, True)
        with open('3.txt', 'a') as f:
            f.write('\n' + txt_in)

    with open('4.txt', 'w') as f:
        f.write('bin')
    for i in range(512):
        txt_in = intToBin(wd[2, i], wbits, True)
        with open('4.txt', 'a') as f:
            f.write('\n' + txt_in)

    with open('5.txt', 'w') as f:
        f.write('bin')
    for i in range(512):
        txt_in = intToBin(wd[3, i], wbits, True)
        with open('5.txt', 'a') as f:
            f.write('\n' + txt_in)

    # np.savetxt("C:\\Users\\ASUS-PC\\Desktop\\1.txt", ind, fmt='%d', delimiter=',')
    # np.savetxt("C:\\Users\\ASUS-PC\\Desktop\\2.txt", wd[0], fmt='%d', delimiter=',')
    # np.savetxt("C:\\Users\\ASUS-PC\\Desktop\\3.txt", wd[1], fmt='%d', delimiter=',')
    # np.savetxt("C:\\Users\\ASUS-PC\\Desktop\\4.txt", wd[2], fmt='%d', delimiter=',')
    # np.savetxt("C:\\Users\\ASUS-PC\\Desktop\\5.txt", wd[3], fmt='%d', delimiter=',')
    # 软件计算得到理论计算结果
    soft_res = np.matmul(wd, ind)
    print(soft_res)
    # 对输入向量以及矩阵进行比特划分
    inbs = np.empty(shape=(col, 1, inbits), dtype=np.uint8)
    wbs = np.empty(shape=(n, col, wbits), dtype=np.uint8)
    for i in range(wbits):
        wbs[:, :, i] = np.bitwise_and(wd, 1)
        wd = wd.__irshift__(1)
    for i in range(inbits):
        inbs[:, :, i] = np.bitwise_and(ind, 1)
        ind = ind.__irshift__(1)
    # 初始化output buffer
    output_buffer = np.zeros(shape=(n, 1), dtype=np.int32)  # 计算之前先使用latch_rst_n将计算流水线以及output buffer进行重置
    # 开始进行矩阵向量乘法计算，一次取1b，为了使得流水线可以满载，将输入向量放到内部循环
    for i in range(wbits):
        # read weight matrix from mram
        for j in range(inbits):
            # fetch input vector
            output_buffer = Macro(inbs[:, :, j], wbs[:, :, i], output_buffer, col, n, j, i, j == inbits - 1,
                                  i == wbits - 1, sa_bypass=False)
    # 得到计算结果，并与理论计算结果进行比较
    print(output_buffer)
    print((output_buffer == soft_res).all())


def data_gen():
    np.random.seed(172329)
    # 定义Macro计算规模，以及输入向量、阵列数据位宽等信息
    # 4*1*1 开启1个Macro, 2*5*3 开启一行Macro, 2*8*8 开启所有Macro
    col, n = 512, 4
    in_group = 4
    inbits, wbits = 2, 2
    macro_row, macro_col = 1, 1  # 一次激活多少行、多少列进行操作
    macro_capacity = 4  # Mb
    storage_word = 32

    code_length = 17  # 接口IO位宽
    data_length = 16
    fifo_length = 16
    insNum = 0
    calCycle = 1
    mode = {"read": "01", "write": "10", "comp": "11"}
    sel = {"one": "01", "row": "10", "all": "11"}
    coemode = {"read": "101", "write": "111", "comp": "110", "end": "000", "change": "010"}
    # end_instr = "01" * (code_length // 2) + "0" if (code_length % 2 == 1) else ""
    

    if coeFile_enable:
        trans_word = "{}{},\n"
        end_instr = "10101010101010101"
    else:
        trans_word = "P0_in=" + str(code_length) + "'b{};\n"
        end_instr = "10101_0101_0101_0101"

    trans_lat = "#20;\n"
    read_lat = "#2000;\n"
    write_lat = "#10000;\n"

    if coeFile_enable:
        test_data_file = "coe_test.coe"
    else:
        test_data_file = "test.txt"
    log_file = "log.txt"

    assert (in_group * inbits <= fifo_length)
    assert (col * n * wbits <= macro_capacity * 1024 * 1024)
    assert (macro_row <= macro_col)

    # 随机生成输入向量以及矩阵
    ind_ = np.random.randint(low=-2 ** (inbits - 1)+1, high=2 ** (inbits - 1), size=(macro_row, in_group, col, 1),
                             dtype=np.int32)  # input vector
    wd_ = np.random.randint(low=-2 ** (wbits - 1)+1, high=2 ** (wbits - 1), size=(macro_row, macro_col, n, col),
                            dtype=np.int32)  # weight matrix
    addr_base = np.random.randint(low=0, high=1, size=(macro_row, macro_col),
                                  dtype=np.int32)  # weight matrix addr base， 随机生成写地址
    cs = np.random.randint(low=0, high=3, size=(2),
                           dtype=np.int32)  # cs，当macro未全开启时，随机选择macro进行操作

    soft_res = np.zeros(shape=(macro_row, macro_col, in_group, n, 1), dtype=np.int32)
    for mr in range(macro_row):
        for mc in range(macro_col):
            for ig in range(in_group):
                soft_res[mr, mc, ig, :, :] = np.matmul(wd_[mr, mc, :, :], ind_[mr, ig, :, :])

    if inbits == 1:
        ind = np.expand_dims(np.vectorize(intToBin)(ind_, inbits), axis=2)
    else:
        ind = np.array(np.vectorize(tuple)(np.vectorize(intToBin)(ind_, inbits)))
        ind = np.transpose(np.flip(ind, axis=0), (1, 2, 0, 3, 4))  # [mr, ig, ib, col, 1]
    # ind__ = ind.reshape(macro_row, -1, col, 1)

    # ind_seq = np.full((macro_row, 16, col, 1), '0')
    # ind_seq[:, :in_group*inbits, :, :] = ind
    # ind_seq = "".join(ind_seq.flatten().tolist())
    # ind_seq += '0' * (code_length - len(ind_seq) % code_length) if (len(ind_seq) % code_length != 0) else ''

    if wbits == 1:
        wd = np.expand_dims(np.vectorize(intToBin)(wd_, wbits), axis=2)
    else:
        wd = np.array(np.vectorize(tuple)(np.vectorize(intToBin)(wd_, wbits)))
        wd = np.transpose(np.flip(wd, axis=0), (1, 2, 0, 3, 4))  # [mr, mc, wb, n, col]
    # wd_seq = "".join(wd.flatten().tolist())
    # wd_seq += '0' * (code_length - len(wd_seq) % code_length) if (len(wd_seq) % code_length != 0) else ''

    # print(ind.shape)
    # print(wd.shape)

    '''
    # 用于检验比特拆分是否正确，测试表明拆分功能实现正确
    _ind = np.vectorize(int)(ind)
    _wd = np.vectorize(int)(wd)

    # 初始化output buffer
    output_buffer = np.zeros(shape=(macro_row, macro_col, in_group, n, 1), dtype=np.int32)  # 计算之前先使用latch_rst_n将计算流水线以及output buffer进行重置
    for mr in range(macro_row):
        for mc in range(macro_col):
            for ig in range(in_group):
                # 开始进行矩阵向量乘法计算，一次取1b，为了使得流水线可以满载，将输入向量放到内部循环
                for i in range(wbits):
                    # read weight matrix from mram
                    for j in range(inbits):
                        # fetch input vector
                        # 之前已验证Macro的正确性，此处直接使用计算
                        output_buffer[mr, mc, ig, :, :] = Macro(_ind[mr, ig, j, :, :], _wd[mr, mc, i, :, :], output_buffer[mr, mc, ig, :, :], col, n, j, i, j == inbits - 1,
                                            i == wbits - 1, sa_bypass=False)

    print((output_buffer == soft_res).all())
    '''

    if macro_col == 1:
        sel_word = "one"
    elif macro_row == 1:
        sel_word = "row"
    else:
        sel_word = "all"

    # 将当前配置参数写成log文件
    with open(log_file, mode='w', encoding='utf-8') as fp:
        fp.write("{0} = {1}\n".format("col", col))
        fp.write("{0} = {1}\n".format("n", n))
        fp.write("{0} = {1}\n".format("in_group", in_group))
        fp.write("{0} = {1}\n".format("inbits", inbits))
        fp.write("{0} = {1}\n".format("wbits", wbits))
        fp.write("{0} = {1}\n".format("macro_row", macro_row))
        fp.write("{0} = {1}\n".format("macro_col", macro_col))
        fp.write("{0} = {1}\n".format("macro_capacity", macro_capacity))
        fp.write("{0} = {1}\n".format("storage_word", storage_word))
        fp.write("{0} = {1}\n".format("code_length", code_length))
        fp.write("{0} = {1}\n".format("data_length", data_length))
        fp.write("{0} = {1}\n".format("fifo_length", fifo_length))

        fp.write("{0} = {1}\n".format("addr_base", addr_base.tolist()))
        fp.write("{0} = {1}\n".format("cs", cs.tolist()))

        fp.write("{0} = {1}\n".format("ind", ind_.tolist()))
        fp.write("{0} = {1}\n".format("wd", wd_.tolist()))
        fp.write("{0} = {1}\n".format("ind_split", ind.tolist()))
        fp.write("{0} = {1}\n".format("wd_split", wd.tolist()))

        fp.write("{0} = {1}\n".format("result", soft_res.tolist()))

    with open(test_data_file, mode='w', encoding='utf-8') as fp:
        if coeFile_enable:
            # fp.write("; This .COE file specifies the contents for a block memory of depth=XXX, and width=20.\n")
            fp.write("memory_initialization_radix=2;\n")
            fp.write("memory_initialization_vector=\n")
        for mr in range(macro_row):
            for mc in range(macro_col):
                # write weight data stream
                start_instr = '0' + mode['write'] + sel["one"] + intToBin(cs[0] if macro_row == 1 else mr,
                                                                          2) + intToBin(cs[1] if macro_col == 1 else mc,
                                                                                        2)
                start_instr += "0" * (code_length - len(start_instr))

                if coeFile_enable:
                    fp.write(trans_word.format(start_instr,coemode['write']))
                    insNum += 1
                else:
                    fp.write(trans_word.format(start_instr))
                    fp.write("in_vld=1;\n")
                    fp.write(trans_lat)

                # 生成写操作流
                for wb in range(wbits):
                    for idx1 in range(n):
                        for idx2 in range(col // storage_word):
                            addr = intToBin(idx1, 2) + intToBin(addr_base[mr, mc] + wb * col // storage_word + idx2, 15)
                            wd_data = ''.join(
                                wd[mr, mc, wb, idx1, storage_word * idx2:storage_word * (idx2 + 1)].flatten().tolist())

                            # 地址
                            if coeFile_enable:
                                insNum += 3
                                fp.write(trans_word.format(addr,coemode['write']))
                                fp.write(trans_word.format('0' * (code_length - 16) + wd_data[:16],coemode['write']))
                                fp.write(trans_word.format('0' * (code_length - 16) + wd_data[16:],coemode['write']))
                            else:
                                fp.write(trans_word.format(addr))
                                fp.write("in_vld=1;\n")
                                fp.write(trans_lat)

                                fp.write(trans_word.format('0' * (code_length - 16) + wd_data[:16]))
                                fp.write(trans_lat)
                                fp.write(trans_word.format('0' * (code_length - 16) + wd_data[16:]))
                                fp.write(trans_lat)

                                # 写过程
                                fp.write("in_vld=0;\n")
                                fp.write(write_lat)
                if coeFile_enable:
                    insNum += 2
                    fp.write(trans_word.format(end_instr,coemode['end']))
                    # fp.write(trans_word.format(end_instr,coemode['end']))
                    fp.write(trans_word.format(end_instr,coemode['end']))
                else:
                    fp.write(trans_word.format(end_instr))
                    fp.write("in_vld=0;\n")
                    fp.write(trans_lat)
                    fp.write(trans_word.format('0' * code_length))
                    fp.write("#2000;\n")

        for i in range(calCycle):
            # calc data stream
            start_instr = '0' + mode['comp'] + sel[sel_word] + '1111'#intToBin(cs[0], 2) + intToBin(cs[1], 2)
            start_instr += "0" * (code_length - len(start_instr))

            if coeFile_enable:
                insNum += 1
                fp.write(trans_word.format(start_instr,coemode['comp']))
            else:
                fp.write(trans_word.format(start_instr))
                fp.write("in_vld=1;\n")
                fp.write(trans_lat)

            calc_fmt_instr = intToBin(wbits, 4) + intToBin(inbits, 4) + intToBin(in_group, 6)
            calc_fmt_instr = "0" * (code_length - len(calc_fmt_instr)) + calc_fmt_instr

            if coeFile_enable:
                insNum += 1
                fp.write(trans_word.format(calc_fmt_instr,coemode['comp']))
            else:
                fp.write(trans_word.format(calc_fmt_instr))
                fp.write(trans_lat)
            
            for mr in range(macro_row):
                for ig in range(in_group):
                    for ib in range(inbits):
                        for idx in range(col // data_length):
                            in_data = ''.join(
                                ind[mr, ig, ib, data_length * idx:data_length * (idx + 1), :].flatten().tolist())

                            # print(trans_word.format('0' * (code_length-16) + ind_seq[16 * idx : 16 * (idx + 1)]))
                            # fp.write(trans_word.format(ind_seq[code_length * idx : code_length * (idx + 1)]))
                            # 输入16bit数据+1bit前导零
                            if coeFile_enable:
                                insNum += 1
                                fp.write(trans_word.format('0' * (code_length - 16) + in_data,coemode['comp']))
                            else:
                                fp.write(trans_word.format('0' * (code_length - 16) + in_data))
                                fp.write(trans_lat)
                print(ig, ib)
                if in_group * inbits < fifo_length:
                    for i in range(fifo_length - in_group * inbits):
                        for j in range(col // data_length):
                            if coeFile_enable:
                                insNum += 1
                                fp.write(trans_word.format('0' * code_length,coemode['comp']))
                            else:                            
                                fp.write(trans_word.format('0' * code_length))
                                fp.write(trans_lat)

            if coeFile_enable:
                insNum += 2
                fp.write(trans_word.format('0' * code_length,coemode['change']))
                # fp.write(trans_word.format('0' * code_length,coemode['change']))
                fp.write(trans_word.format('0' * code_length,coemode['change']))

            for wb in range(wbits):
                # 每次替换weight bit时，需要重新给权重地址
                for mr in range(macro_row):
                    for mc in range(macro_col):
                        for idx in range(col // storage_word):
                            # print(trans_word.format(intToBin(addr_base[mr, mc]+wb*col*n//storage_word+idx, code_length)))
                            if coeFile_enable:
                                insNum += 1
                                fp.write(trans_word.format(intToBin(addr_base[mr, mc] + wb * col // storage_word + idx, code_length),coemode['comp']))
                            else:
                                fp.write(trans_word.format(intToBin(addr_base[mr, mc] + wb * col // storage_word + idx, code_length)))
                                fp.write("in_vld=1;\n")
                                fp.write(trans_lat)

                                fp.write("in_vld=0;\n")
                                fp.write(read_lat)

            if coeFile_enable:
                insNum += 1
                fp.write(trans_word.format(end_instr,coemode['end']))
                # fp.write(trans_word.format(end_instr,coemode['end']))
                # fp.write("10101010101010101000;")
                # fp.write(str(insNum))
            else:
                fp.write(trans_word.format(end_instr))
                fp.write("in_vld=0;\n")
                fp.write(trans_lat)
                fp.write(trans_word.format('0' * code_length))
                fp.write("#2000;\n")
            
        if coeFile_enable:
            insNum += 1
            fp.write("10101010101010101000;")
    
    # 读取原有内容并存储
    with open(test_data_file, 'r', encoding='utf-8') as file:
        original_content = file.read()

    # 以写模式覆盖文件，先写入插入的首行，再写入原有内容
    with open(test_data_file, 'w', encoding='utf-8') as file:
        first_line = '; This .COE file specifies the contents for a block memory of depth={}, and width=20.\n'
        file.write(first_line.format(insNum))
        file.write(original_content)


if __name__ == "__main__":
    data_gen()


