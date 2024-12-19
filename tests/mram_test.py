import ctypes
import logging as log

log.basicConfig(
    level=log.INFO,  
    format="[%(asctime)s] [%(levelname)s] [%(funcName)s] %(message)s", 
    datefmt="%Y-%m-%d %H:%M:%S",
    handlers=[
        # log.StreamHandler(), 
        log.FileHandler("test.log","w")
    ],
)

mram_dll = ctypes.CDLL('./libmram_api.so')

def capacity_test():
    batch_size = 1
    
    # rows,cols,ips,mrams = 4,4,4,1048576
    rows,cols,ips,mrams = 1,1,1,4096
    for macro_row in range(rows):
        for macro_col in range(cols):
            for ip_addr in range(ips):
                for mram_addr in range(mrams // batch_size):
                    start = macro_row*(4 * 4 * 256) + macro_col*(4*256) + ip_addr*256 + mram_addr
                    send_data = (ctypes.c_int32 * batch_size)()
                    for i in range(batch_size):
                        send_data[i] = start + i
                    err = mram_dll.Send(macro_row,macro_col,ip_addr,mram_addr,send_data,batch_size)
                    if err !=0:
                        print('error1!')
                    recv_data = (ctypes.c_int32 * batch_size)()
                    err = mram_dll.Recv(macro_row,macro_col,ip_addr,mram_addr,recv_data,batch_size)
                    if err !=0:
                        print(err,'error2!')
                    for i in range(batch_size):
                        if send_data[i] != recv_data[i]:
                            log.error(f'macro({macro_row,macro_col}) ip({ip_addr} mram_addr({mram_addr:0>8X})) idx({i}) write weight:{send_data[i]}, but recv weight: {recv_data[i]}')
                        else:
                            log.info(f'macro({macro_row,macro_col}) ip({ip_addr}) mram_addr({mram_addr:0>8X} ~ {(mram_addr+batch_size):0>8X}) write/read verify success')

capacity_test()

# mram_dll.Wait(1,1)