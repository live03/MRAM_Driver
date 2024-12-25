#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "mram_api.h"
#include "mram_config.h"

void print_buf(void *buf, int radix, int size)
{
    int i;
    uint8_t *buf8 = NULL;
    uint16_t *buf16 = NULL;
    uint32_t *buf32 = NULL;
    if (radix == 8)
    {
        buf8 = (uint8_t *)buf;
        for (i = 0; i < size; i++)
            printf("0x%2X ", buf8[i]);
    }
    else if (radix == 10)
    {
        buf32 = (uint32_t *)buf;
        for (i = 0; i < size; i++)
            printf("%d ", buf32[i]);
    }
    else if (radix == 16)
    {
        buf16 = (uint16_t *)buf;
        for (i = 0; i < size; i++)
            printf("0x%04X ", buf16[i]);
    }
    else if (radix == 32)
    {
        buf32 = (uint32_t *)buf;
        for (i = 0; i < size; i++)
            printf("0x%08X ", buf32[i]);
    }
    printf("\n");
}

void print_cmd_test(Controll_Command cmd)
{
    char mode[4][6] = {"", "read", "write", "calc"};
    char sel[4][4] = {"", "one", "row", "all"};
    switch (cmd.start.code)
    {
    case CODE_START:
        printf("Start Command 0x%X = {MODE:%s, SEL:%s macro, CS:(%d)(row %d, col %d)}\n",
               cmd.start, mode[cmd.start.MODE], sel[cmd.start.SEL], cmd.start.CS, cmd.start.CS >> 2, cmd.start.CS && 3);
        break;
    case CODE_CALC:
        printf("Calculate Command 0x%X = {wbits:%d, inbits:%d, in_group:%d}\n",
               cmd.calculate, cmd.calculate.wbits, cmd.calculate.inbits, cmd.calculate.in_group);
        break;
    case CODE_INPUT:
        printf("Input Command 0x%X = {data:%X}\n",
               cmd.input, cmd.input.data);
        break;
    case CODE_ADDR:
        printf("Address Command 0x%X = {ip_addr:%d, mram_addr:0x%04X}\n",
               cmd.addr, cmd.addr.ip, cmd.addr.addr);
        break;
    case CODE_ENDINS:
        printf("End Command 0x%X\n", cmd.end);
        break;
    default:
        break;
    }
}

void getTime()
{
    struct tm *tm_t;
    struct timeval time;
    // char str_time[80];
    gettimeofday(&time, NULL);
    tm_t = localtime(&time.tv_sec);
    if (NULL != tm_t)
    {
        printf("%04d-%02d-%02d %02d:%02d:%02d:%03ld\n",
               tm_t->tm_year + 1900,
               tm_t->tm_mon + 1,
               tm_t->tm_mday,
               tm_t->tm_hour,
               tm_t->tm_min,
               tm_t->tm_sec,
               time.tv_usec / 1000);
    }
}

int cap_test()
{
    int macro_row, macro_col, ip_addr, mram_addr, start, i, err;
    int batch_size = 64;
    unsigned int *send_data = (unsigned int *)malloc(batch_size * sizeof(int));
    unsigned int *recv_data = (unsigned int *)malloc(batch_size * sizeof(int));
    int rows = 4, cols = 4, ips = 4, mram_addrs = 32768;
    // int rows = 4, cols = 4, ips = 4, mrams = 8192;
    FILE *fd = fopen("cap_test_64_64M.log", "w");
    srand(3213);
    getTime();
    // int rows = 1, cols = 1,ips = 1, mrams = 4096;
    for (macro_row = 0; macro_row < rows; macro_row++)
    {
        for (macro_col = 0; macro_col < cols; macro_col++)
        {

            for (ip_addr = 0; ip_addr < ips; ip_addr++)
            {
                for (mram_addr = 0; mram_addr < mram_addrs; mram_addr += batch_size)
                {
                    start = macro_row * (cols * ips * mram_addrs) + macro_col * (ips * mram_addrs) + ip_addr * mram_addrs + mram_addr;
                    for (i = 0; i < batch_size; i++)
                    {
                        // send_data[i] = 0xFFFFFFFF;
                        // send_data[i] = 0x0;
                        send_data[i] = rand() % 0xFFFFFFFF;
                        // send_data[i] = start + i;
                    }
                    // err = Send(H2C_DEVICE, macro_row, macro_col, ip_addr, mram_addr, send_data, batch_size);

                    err = Recv(H2C_DEVICE, C2H_DEVICE, macro_row, macro_col, ip_addr, mram_addr, recv_data, batch_size);
                    for (i = 0; i < batch_size; i++)
                        if (send_data[i] != recv_data[i])
                            fprintf(fd, "Macro(%d, %d) (ip, addr):(%d, 0x%08X) match failed. (Send, Recv):(%d, %d)\n",
                                    macro_row, macro_col, ip_addr, mram_addr + i, send_data[i], recv_data[i]);
                        else
                            fprintf(fd, "Macro(%d, %d) (ip, addr):(%d, 0x%08X) match success. (Send, Recv):(%d, %d)\n",
                                    macro_row, macro_col, ip_addr, mram_addr + i, send_data[i], recv_data[i]);
                }
            }
            // fprintf(fd, "Macro(%d, %d) (ip, addr):(%d, 0x%08X) match failed. (Send, Recv):(%d, %d)\n",
            //         macro_row, macro_col, ip_addr, mram_addr, send_data[0], recv_data[0]);
        }
    }
    getTime();
}

int sendCMD(void *value)
{
    Store_Everywhere(H2C_DEVICE, 0, (int *)value, 1);
    ring_ctrl(CTRL_REG_START_WRITE, 0, 1);
}

int main(int argc, char const *argv[])
{
    // sendCMD(&CTRL_RESET_CMD);
    uint32_t u32_buffer2[4096] = {0};
    uint32_t cal_res[7] = {0x0009000, 0x00000000, 0x00000020, 0x00004000, 0x0000, 0x0000, 0x0000};
    int data[7];
    int i;
    unsigned int value = 0X00029000;
    uint32_t host_data_addr[4];
    int res[7] = {130, 5201, 0, 0, 0, 1664, 0};
    Init();

    // Load_Everywhere(C2H_DEVICE, INST_AREA_START_OFFSET, &value, 1);
    // printf("%X",value);
    // sendCMD(&value);
    cap_test();

    // ring_ctrl(CTRL_REG_START_READ, RES_DATA_AREA_START_OFFSET, 1024);
    // Load_Everywhere(C2H_DEVICE, RES_DATA_AREA_START_OFFSET, u32_buffer2, 1024);
    // print_buf(u32_buffer2, 10, 1024);

    // for (i = 0; i < 1; i++)
    // {
    //     host_data_addr[0] = (cal_res[6] << 9) + (cal_res[5] >> 7);
    //     host_data_addr[1] = ((cal_res[5] & 0x7F) << 18) + (cal_res[4] << 2) + (cal_res[3] >> 14);
    //     host_data_addr[2] = ((cal_res[3] & 0x3FFF) << 11) + (cal_res[2] >> 5);
    //     host_data_addr[3] = ((cal_res[2] & 0x1F) << 20) + (cal_res[1] << 4) + (cal_res[0] >> 12);
    // }
    // print_buf(host_data_addr, 10, 4);
    // print_cmd_test(*((Controll_Command*)(&value)));
    return 0;
}