#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "mram_api.h"
#include "mram_config.h"
#include "dma_utils.h"

void print_buf(void *buf, int size)
{
    uint8_t *buf8 = NULL;
    uint16_t *buf16 = NULL;
    uint32_t *buf32 = NULL;
    int pos = 0;
    if (size == 8)
    {
        buf8 = (uint8_t *)buf;
        while (buf8[pos])
            printf("0x%2X ", buf8[pos++]);
    }
    else if (size == 16)
    {
        buf16 = (uint16_t *)buf;
        while (buf16[pos])
            printf("0x%04X ", buf16[pos++]);
    }
    else if (size == 32)
    {
        buf32 = (uint32_t *)buf;
        while (buf32[pos])
            printf("0x%08X ", buf32[pos++]);
    }
    printf("\n");
}

void test_mram()
{
    uint32_t reg_buf;
    char char_buffer[128] = {0};
    uint32_t u32_buffer[4] = {1, 2, 3, 4};
    uint32_t u32_buffer2[4] = {0,0,0,0};
    int pos = 0;
    int fd, i, j;
    int cmd_size = sizeof(Controll_Command);

    // initial file size
    // char *ram_name = "./storage/device_ram.bin";
    // char macro_name[4][4][32] = {
    //     {"./storage/device_macro_00.bin", "./storage/device_macro_01.bin", "./storage/device_macro_02.bin", "./storage/device_macro_03.bin"},
    //     {"./storage/device_macro_10.bin", "./storage/device_macro_11.bin", "./storage/device_macro_12.bin", "./storage/device_macro_13.bin"},
    //     {"./storage/device_macro_20.bin", "./storage/device_macro_21.bin", "./storage/device_macro_22.bin", "./storage/device_macro_23.bin"},
    //     {"./storage/device_macro_30.bin", "./storage/device_macro_31.bin", "./storage/device_macro_32.bin", "./storage/device_macro_33.bin"}};

    // fd = open(H2C_DEVICE, O_RDWR | O_CREAT | O_SYNC, 0664);
    // ftruncate(fd, 2 * 1024 * 1024);
    // fd = open(C2H_DEVICE, O_RDWR | O_CREAT | O_SYNC, 0664);
    // ftruncate(fd, 2 * 1024 * 1024);
    // fd = open(CTRL_DEVICE, O_RDWR | O_CREAT | O_SYNC, 0664);
    // ftruncate(fd, 10 * 4);
    // close(fd);
    // fd = open(ram_name, O_RDWR | O_CREAT | O_SYNC, 0664);
    // ftruncate(fd, 2 * 1024 * 1024);
    // close(fd);
    // for (i = 0; i < 4; i++)
    // {
    //     for (j = 0; j < 4; j++)
    //     {
    //         fd = open(macro_name[i][j], O_RDWR | O_CREAT | O_SYNC, 0664);
    //         ftruncate(fd, 32 * 32 * 1024);
    //         close(fd);
    //     }
    // }

    printf("\n--------------------------------reg function test--------------------------------\n");
    Sldi(CTRL_DEVICE, 0x00215555, 0);
    Sld(CTRL_DEVICE, 0, 2);
    printf("%X\n", u32_buffer);
    Ldr(CTRL_DEVICE, 2, u32_buffer);
    printf("%X\n", u32_buffer);

    printf("\n--------------------------------RAM function test--------------------------------\n");
    Store(H2C_DEVICE, 0x0000, u32_buffer, 4);
    printf("write success: %X\n", u32_buffer[0]);
    Load(C2H_DEVICE, 0x0000, u32_buffer, 1);
    printf("read success: %X\n", u32_buffer[0]);

    printf("\n--------------------------------Macro function test--------------------------------\n");
    Send(H2C_DEVICE, 0, 0, 0, 0, u32_buffer, 4);
    print_buf(u32_buffer, 32);
    Recv(H2C_DEVICE, C2H_DEVICE, 0, 0, 0, 0, u32_buffer2, 4);
    print_buf(u32_buffer2, 32);

    // printf("\n--------------------------------Calc function test--------------------------------\n");
}

int *bitslice(uint32_t *arr, int arr_size, int bit_size)
{
    int i, j, k;
    // 分配返回数组的内存
    int *res = (int *)malloc(bit_size * (arr_size / 16) * sizeof(int));
    if (!res)
    {
        perror("Memory allocation failed");
        return NULL;
    }

    // 初始化res数组
    for (i = 0; i < bit_size; i++)
    {
        res[i] = 0;
    }

    // 按位切片
    for (i = 0; i < bit_size; i++)
    {
        k = 0;
        for (j = 0; j < arr_size; j++)
        {
            // 提取每个数字的第i位（从低位到高位）
            int bit = (arr[j] >> i) & 1;
            // 将第i位的结果构造到res[i]中，从高到低
            res[i + k / 16] = (res[i] << 1) | bit;
            k = k + 1;
        }
    }

    return res;
}

int main(int argc, char const *argv[])
{
    test_mram();
    return 0;
}
