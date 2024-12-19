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
    int pos = 0;
    if (radix == 8)
    {
        buf8 = (uint8_t *)buf;
        for (i = 0; i < size; i++)
            printf("0x%2X ", buf8[pos++]);
    }
    else if (radix == 16)
    {
        buf16 = (uint16_t *)buf;
        for (i = 0; i < size; i++)
            printf("0x%04X ", buf16[pos++]);
    }
    else if (radix == 32)
    {
        buf32 = (uint32_t *)buf;
        for (i = 0; i < size; i++)
            // printf("%d ", buf32[pos++]);
            printf("0x%08X ", buf32[pos++]);
    }
    printf("\n");
}

void test_mram()
{
    uint32_t reg_buf;
    char char_buffer[128] = {0};
    int batch_size = 1;
    int32_t *i32_buffer;
    int32_t *i32_buffer2;
    int pos = 0;
    int fd, i, j;

    i32_buffer = (int32_t *)malloc(batch_size * sizeof(int32_t));
    i32_buffer2 = (int32_t *)malloc(batch_size * sizeof(int32_t));
    // printf("\n--------------------------------reg function test--------------------------------\n");
    // Sldi(CTRL_DEVICE, 0x00215555, 0);
    // Sld(CTRL_DEVICE, 0, 2);
    // printf("%X\n", i32_buffer);
    // Ldr(CTRL_DEVICE, 2, i32_buffer);
    // printf("%X\n", i32_buffer);

    // printf("\n--------------------------------RAM function test--------------------------------\n");
    // Store(H2C_DEVICE, 0x0000, i32_buffer, 4);
    // printf("write success\n");
    // print_buf(i32_buffer,32,4);
    // Load(C2H_DEVICE, 0x0000, i32_buffer2, 4);
    // printf("read success\n");
    // print_buf(i32_buffer2,32,4);

    printf("\n--------------------------------Macro function test--------------------------------\n");
    for (i = 0; i < batch_size; i++)
        i32_buffer[i] = i;
    Send(0, 0, 0, 0, i32_buffer, batch_size);
    // print_buf(i32_buffer, 32, 4);

    // Load_Everywhere(C2H_DEVICE, INST_AREA_START_OFFSET, i32_buffer2, 4);

    printf("---------Read Out------------\n");
    Recv(0, 0, 0, 0, i32_buffer2, batch_size);
    print_buf(i32_buffer2, 32, batch_size);

    // printf("\n--------------------------------Calc function test--------------------------------\n");
}

int init()
{
    int i;
    uint32_t value;
    uint32_t data[] = {
        // 0x003C0000,
        0x003E0000, // 00000000001111100000000000000000 //配置PLL
        0x00029200, // 00000000000000101001001000000000
        0x00100F00, // 00000000000100000000111100000000
        0x000840CE, // 00000000000010000100000011001110
        0x000840CC, // 00000000000010000100000011001100
        0x00100F0F, // 00000000000100000000111100001111
        0x000840CE, // 00000000000010000100000011001110
        0x000840FE, // 00000000000010000100000011001110
        0x00215555, // 00000000001000010101010101010101
        // 0x00025200, // 00000000000000100101001000000000
        // 0x00100F00, // 00000000000100000000111100000000
        // 0x00100F0F, // 00000000000100000000111100001111
        // 0x00215555  // 00000000001000010101010101010101

    };
    for (i = 0; i < sizeof(data) / sizeof(data[0]); i++)
    {
        value = data[i];
        Store_Everywhere(H2C_DEVICE, INST_AREA_START_OFFSET, &value, 1);
        ring_ctrl(CTRL_REG_START_WRITE, INST_AREA_START_OFFSET, 1);
    }
    sleep(1);
    value = 0X00380000;
    Store_Everywhere(H2C_DEVICE, INST_AREA_START_OFFSET, &value, 1);
    ring_ctrl(CTRL_REG_START_WRITE, INST_AREA_START_OFFSET, 1);
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
    int *send_data = (int *)malloc(batch_size * sizeof(int));
    int *recv_data = (int *)malloc(batch_size * sizeof(int));
    int rows = 4, cols = 4, ips = 4, mrams = 1048576;
    // int rows = 4, cols = 4,ips = 4, mrams = 4096;
    // int rows = 1, cols = 1, ips = 1, mrams = 4096;
    FILE *fd = fopen("cap_test_64_128_64M.log", "w");
    getTime();
    // int rows = 1, cols = 1,ips = 1, mrams = 4096;
    for (macro_row = 0; macro_row < rows; macro_row++)
        for (macro_col = 0; macro_col < cols; macro_col++)
            for (ip_addr = 0; ip_addr < ips; ip_addr++)
                for (mram_addr = 0; mram_addr < mrams; mram_addr += batch_size)
                {
                    start = macro_row * (cols * ips * mrams) + macro_col * (ips * mrams) + ip_addr * mrams + mram_addr;
                    for (i = 0; i < batch_size; i++)
                        send_data[i] = start + i;
                    // printf("Write Start!\n");
                    err = Send(macro_row, macro_col, ip_addr, mram_addr, send_data, batch_size);
                    if (err != 0)
                        printf("error1!\n");
                    // printf("Read Start!\n");
                    err = Recv(macro_row, macro_col, ip_addr, mram_addr, recv_data, batch_size);

                    if (err != 0)
                        printf("error2!\n");
                    for (i = 0; i < batch_size; i++)
                        if (send_data[i] != recv_data[i])
                            fprintf(fd, "Macro(%d, %d) (ip, addr):(%d, 0x%08X) match failed. (Send, Recv):(%d, %d)\n",
                                    macro_row, macro_col, ip_addr, mram_addr + i, send_data[i], recv_data[i]);
                        else
                            fprintf(fd, "Macro(%d, %d) (ip, addr):(%d, 0x%08X) match success. (Send, Recv):(%d, %d)\n",
                                    macro_row, macro_col, ip_addr, mram_addr + i, send_data[i], recv_data[i]);
                }
    getTime();
}

int sendCMD(void *value)
{
    Store_Everywhere(H2C_DEVICE, 0, (int *)value, 1);
    ring_ctrl(CTRL_REG_START_WRITE, 0, 1);
}

int calc_test()
{
    int i, value;
    int res[7] = {0};
    int host_data_addr[4] = {0};
    uint32_t data[] = {
        // 0x003E0000,
        0x00029200,
        0x00100F00,
        0x000840CE,
        0x000840CC,
        0x00100F0F,
        0x000840FF,
        0x000840CE,
        0x00100F0C,
        0x000840CE,
        0x000840CE,
        0x00100F03,
        0x000840CE,
        0x000840CE,
        0x00100E8F,
        0x000840CE,
        0x000840CE,
        0x00100F8F,
        0x000840CE,
        0x000840CE,
        0x00100F4F,
        0x000840CE,
        0x000840CE,
        0x00100F0F,
        0x000840CE,
        0x000840CE,
        0x00100300,
        0x000840CE,
        0x000840CE,
        0x00100C0B,
        0x000840CE,
        0x000840CE,
        0x00100D08,
        0x000840CE,
        0x000840CE,
        0x00100E03,
        0x000840CE,
        0x000840CE,
        0x00103E8F,
        0x000840CE,
        0x000840CE,
        0x00101F83,
        0x000840CE,
        0x000840CE,
        0x00100F4F,
        0x000840CE,
        0x000840CE,
        0x00102F0F,
        0x000840CE,
        0x000840CE,
        0x00215555,
        0x0002D200,
        0x00040441,
        0x0008D20E,
        0x000840CE,
        0x000840CE,
        0x000801F0,
        0x00083E00,
        0x00080000,
        0x00084848,
        0x0008031E,
        0x00081878,
        0x00080048,
        0x00081800,
        0x00080600,
        0x00080600,
        0x00080080,
        0x00081120,
        0x00080C60,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00080000,
        0x00100F00,
        0x00100F0F,
        0x00100F0C,
        0x00100F03,
        0x00100E8F,
        0x00100F8F,
        0x00100F4F,
        0x00100F0F,
        0x00100300,
        0x00100C0B,
        0x00100D08,
        0x00100E03,
        0x00103E8F,
        0x00101F83,
        0x00100F4F,
        0x00102F0F,
        0x00215555};

    for (i = 0; i < sizeof(data) / sizeof(data[0]); i++)
    {
        value = data[i];
        Store_Everywhere(H2C_DEVICE, INST_AREA_START_OFFSET, &value, 1);
        ring_ctrl(CTRL_REG_START_WRITE, INST_AREA_START_OFFSET, 1);
    }

    ring_ctrl(CTRL_REG_START_READ, RES_DATA_AREA_START_OFFSET, 7);
    Load_Everywhere(C2H_DEVICE, RES_DATA_AREA_START_OFFSET, res, 7);
    print_buf(res, 32, 7);

    host_data_addr[0] = res[6] << 9 + res[5] >> 7;
    host_data_addr[1] = (res[5] & 0x3F) << 18 + res[4] << 2 + res[3] >> 14;
    host_data_addr[2] = (res[3] & 0x3FF) << 11 + res[2] >> 5;
    host_data_addr[3] = (res[2] & 0x1F) << 20 + res[1] << 4 + res[0] >> 12;
    print_buf(host_data_addr, 32, 4);
}

int main(int argc, char const *argv[])
{
    // sendCMD(&CTRL_RESET_CMD);
    // uint32_t u32_buffer2[8] = {0};
    int data[7];
    int value = 0X00380000;

    init();
    // test_mram();
    calc_test();
    // test_mram();

    // cap_test();

    // ring_ctrl(CTRL_REG_START_READ, 0x1C, 2);
    // Load_Everywhere(C2H_DEVICE,0x1C,data,2);
    // print_buf(data,32,2);
    // getTime(buf);
    return 0;
}
