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

def program_verify(retry_times, H2C_DEVICE, macro_row, macro_col, ip_addr, mram_addr, send_data, batch_size):
    recv_data = (ctypes.c_int32 * batch_size)()
    for _ in range(retry_times):
        err = mram_dll.Send(H2C_DEVICE, macro_row, macro_col, ip_addr, mram_addr, send_data, batch_size)
        if err:
            continue
        mram_dll.Recv(H2C_DEVICE,C2H_DEVICE, macro_row, macro_col, ip_addr, mram_addr, recv_data, batch_size)
        if not eq(send_data, recv_data):
            continue
        return True
    return False

def capacity_write(rows, cols, ips, mram_addrs, type, batch_size):
    total_time = 0
    random.seed(seed)
    log.info("Start Writing......")
    for macro_row in range(rows):
        for macro_col in range(cols):
            for ip_addr in range(ips):
                for mram_addr in range(0, mram_addrs, batch_size):
                    start = macro_row * (cols * ips * mram_addrs) + macro_col * (ips * mram_addrs) + ip_addr * mram_addrs + mram_addr;
                    send_data = (ctypes.c_int32 * batch_size)()
                    for i in range(batch_size):
                        match type:
                            case "Random":
                                send_data[i] = random.randint(0, 0xFFFFFFFF)
                            case "Zero":
                                send_data[i] = 0x0
                            case "One":
                                send_data[i] = 0xFFFFFFFF
                            case "Sequence":
                                send_data[i] = start + i
                            case _:
                                send_data[i] = 0x0
                    start_time = time.time()
                    # err = mram_dll.Send(H2C_DEVICE, macro_row, macro_col, ip_addr, mram_addr, send_data, batch_size)
                    succ = program_verify(1, H2C_DEVICE, macro_row, macro_col, ip_addr, mram_addr, send_data, batch_size)
                    total_time += (time.time() - start_time)
                    # if not succ:
                    #     log.error(f'(macro_row, macro_col, ip_addr, mram_addr):({macro_col}, {macro_col}, {ip_addr}, {mram_addr}) capacity write error!')
                        # raise ValueError("MRAM_DLL Send Function Error")
    log.info("Write Success!")
    log.info(f"Total write time is {total_time:.3f}")

def capacity_read(rows, cols, ips, mram_addrs, type, batch_size):
    retry_times = 3
    total_time = 0
    error_addrs = 0
    random.seed(seed)
    cap_log_file = open(f"Capacity_Test_{type}.log", "w")
    log.info("Start Reading......")
    for macro_row in range(rows):
        for macro_col in range(cols):
            for ip_addr in range(ips):
                for mram_addr in range(0, mram_addrs, batch_size):
                    start = macro_row * (cols * ips * mram_addrs) + macro_col * (ips * mram_addrs) + ip_addr * mram_addrs + mram_addr;
                    send_data = (ctypes.c_int32 * batch_size)()
                    for i in range(batch_size):
                        match type:
                            case "Random":
                                send_data[i] = random.randint(0, 0xFFFFFFFF)
                            case "Zero":
                                send_data[i] = 0x0
                            case "One":
                                send_data[i] = 0xFFFFFFFF
                            case "Sequence":
                                send_data[i] = start + i
                            case _:
                                send_data[i] = 0x0

                    recv_data = (ctypes.c_int32 * batch_size)()
                    start_time = time.time()
                    err = mram_dll.Recv(H2C_DEVICE, C2H_DEVICE, macro_row, macro_col, ip_addr, mram_addr, recv_data, batch_size)
                    total_time += (time.time() - start_time)
                    if err != 0:
                        log.error(f'(macro_row, macro_col, ip_addr, mram_addr):({macro_col}, {macro_col}, {ip_addr}, {mram_addr}) capacity read error!')
                        # raise ValueError("MRAM_DLL Recv Function Error")
                    for i in range(batch_size):
                        if send_data[i] != recv_data[i]:
                            cap_log_file.write(f'Macro({macro_row}, {macro_col}) (ip, addr):({ip_addr},{mram_addr:0>8X}) match failed. (Send, Recv):({send_data[i]}, {recv_data[i]})\n')
                            error_addrs += 1
                        else:
                            cap_log_file.write(f'Macro({macro_row}, {macro_col}) (ip, addr):({ip_addr},{mram_addr:0>8X}) match success. (Send, Recv):({send_data[i]}, {recv_data[i]})\n')
                            break
    cap_log_file.close()
    log.info("Read Success!")
    log.info(f"Total read time is {total_time:.3f}s")
    log.info(f"The number of incorrect addresses is {error_addrs}")


def capacity_test(rows, cols, ips, mram_addrs, type, batch_size = 64):
    log.info("----------------------------Capacity Test Start----------------------------")
    log.info(f'Capacity Test Params (Macro_Rows:{rows}, Macro_Cols:{cols}, IPs:{ips}, MRAM_Addrs:{mram_addrs}, Data_Type:"{type}")')
    capacity_write(rows, cols, ips, mram_addrs, type, batch_size)
    time.sleep(1)
    capacity_read(rows, cols, ips, mram_addrs, type, batch_size)
    log.info("-----------------------------Capacity Test End-----------------------------\n")
    


def bitSlice(array: ndarray, bits: int = 8):
    # 0.将int8类型转化为uint8类型,供给unpackbits函数使用,会自动将负数转化为补码
    array_u8 = array.astype(np.uint8)

    # 1.如果小于512行则扩充到512
    array_filled = np.pad(array_u8, ((0, 0), (0, 512 - array_u8.shape[-1])), mode='constant', constant_values=0)

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

def calculate_test(inbits, wbits, in_group, macro_row, macro_col, mram_base_addr = 0x0000):
    log.info("----------------------------Calculate Test Start----------------------------")
    log.info(f'Calculate Test Params (Actived_Macro_Row:{macro_row}, Actived_Macro_Row:{macro_col}, inbits:{inbits}, wbits:{wbits})')
    # 激活的macro行数
    actived_macro_row = 1 if macro_row != 4 else 4
    # 激活的macro列数
    actived_macro_col = 1 if macro_col != 4 else 4
    # 生成数据大小
    wrow, wcol = 512,(4 * actived_macro_row * actived_macro_col)
    np.random.seed(seed)
    # input vector
    ind_ = np.random.randint(low=-2 ** (inbits - 1), high=2 ** (inbits - 1), size=(in_group, wrow), dtype=np.int8)
    # weight matrix
    wd_ = np.random.randint(low=-2 ** (wbits - 1), high=2 ** (wbits - 1 ), size=(wcol, wrow), dtype=np.int8)
    # 理论计算结果
    soft_result = np.matmul(ind_,wd_.T,dtype=np.int32)
    # soft_result = np.astype(np.int32)    

    # shape of input_vector is [in_group][inbits][16]
    input_vector = bitSlice(ind_, inbits)
    # shape of input_weight is [wcol][wbits][16]
    input_weight = bitSlice(wd_, wbits)

    # 公共的权重输入buffer
    weight_buffer = (ctypes.c_uint * (wbits * 16))()

    # 向量输入buffer
    inArr = (ctypes.c_uint * (in_group * inbits * 16))()
    for ig in range(in_group):
        for wb in range(inbits):
            for idx in range(16):
                inArr[ig * inbits * 16 + wb * 16 + idx] = input_vector[ig, wb, idx]

    log.info("Writing Weight Matrix......")
    # 写入权重
    for mr_i in range(actived_macro_row):
        for mc_i in range(actived_macro_col):
            # 每次计算的macro行列编号
            mr = macro_row if actived_macro_row == 1 else mr_i
            mc = macro_col if actived_macro_col == 1 else mc_i
            # 为每个IP写入权重数据
            for ip in range(4):
                for wb in range(wbits):
                    for i in range(16):
                            weight_buffer[wb * 16 + i] = input_weight[(mr_i * 16 + mc_i * 4 + ip), wb, i]
                mram_dll.Send(H2C_DEVICE, mr, mc, ip, mram_base_addr, weight_buffer, wbits * 16)
    log.info("Writing Weight Matrix Success!")

    log.info("Start Calculating......")
    # 计算结果个数
    res_size = in_group * actived_macro_row * actived_macro_col * 4
    res_buff = (ctypes.c_int * res_size)()
    mram_dll.Vmmul(H2C_DEVICE,C2H_DEVICE, inArr, in_group * inbits * 16, inbits, in_group, macro_row, macro_col, mram_base_addr, wbits, res_buff)
    log.info("Calculate Success!")

    log.info("Chip Returned Result:")
    log_str = ""
    for i in range(res_size):
        log_str += (str(res_buff[i]) + " ")
    log.info(log_str)
    
    log.info("Theoretical Result:")
    log_str = ""
    for i in range(res_size):
        # soft_result[0, i] = soft_result[0, i] & 0x1FFFFFF
        log_str += (str(soft_result[0, i]) + " ")
    log.info(log_str)
    
    for i in range(res_size):
        if res_buff[i] != soft_result[0, i]:
            log.warning("Calculate Result Match Failed!")
            log.info("-----------------------------Calculate Test End-----------------------------\n")
            return
    
    log.info("Calculate Result Match Success!")
    log.info("-----------------------------Calculate Test End-----------------------------\n")
    

if __name__ == "__main__":
    # 芯片初始化
    mram_dll.Init()

    # 容量测试: 全0数据
    # capacity_test(rows=4, cols=4, ips=4, mram_addrs=32768, type="Zero")

    # # # 容量测试: 全1数据
    # capacity_test(rows=4, cols=4, ips=4, mram_addrs=32768, type="One")

    # # # 容量测试: 随机数据
    # capacity_test(rows=4, cols=4, ips=4, mram_addrs=32768, type="Random")
    
    # # 计算测试: 随机单Macro 1-8bit计算
    for inbits in [1, 2, 4, 8]:
        for wbits in [1, 2, 4, 8]:
            calculate_test(inbits=inbits, wbits=wbits, in_group=1, macro_row=random.randint(0,3), macro_col=random.randint(0,3))

    # # 计算测试: 16 Macro 1-8bit计算
    for inbits in [1, 2, 4, 8]:
        for wbits in [1, 2, 4, 8]:
            calculate_test(inbits=inbits, wbits=wbits, in_group=1, macro_row=4, macro_col=4)
    
    # # 计算测试: 单Macro 8bit计算
    for mr in range(4):
        for mc in range(4):
            calculate_test(inbits=8, wbits=8, in_group=1, macro_row=mr, macro_col=mc)
