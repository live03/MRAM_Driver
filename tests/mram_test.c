#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

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
            printf("0x%08d ", buf32[pos++]);
    }
    printf("\n");
}

void test_mram()
{
    uint32_t reg_buf;
    char char_buffer[128] = {0};
    uint32_t u32_buffer[4] = {123, 456, 789, 101};
    uint32_t u32_buffer2[4] = {0, 0, 0, 0};
    int pos = 0;
    int fd, i, j;
    int cmd_size = sizeof(Controll_Command);

    // printf("\n--------------------------------reg function test--------------------------------\n");
    // Sldi(CTRL_DEVICE, 0x00215555, 0);
    // Sld(CTRL_DEVICE, 0, 2);
    // printf("%X\n", u32_buffer);
    // Ldr(CTRL_DEVICE, 2, u32_buffer);
    // printf("%X\n", u32_buffer);

    // printf("\n--------------------------------RAM function test--------------------------------\n");
    // Store(H2C_DEVICE, 0x0000, u32_buffer, 4);
    // printf("write success\n");
    // print_buf(u32_buffer,32,4);
    // Load(C2H_DEVICE, 0x0000, u32_buffer2, 4);
    // printf("read success\n");
    // print_buf(u32_buffer2,32,4);

    printf("\n--------------------------------Macro function test--------------------------------\n");
    Send(H2C_DEVICE, 0, 0, 0, 0, u32_buffer, 4);
    print_buf(u32_buffer, 32, 4);

    // Load_Everywhere(C2H_DEVICE, INST_AREA_START_OFFSET, u32_buffer2, 4);

    printf("---------Read Out------------\n");
    Recv(H2C_DEVICE, C2H_DEVICE, 0, 0, 0, 0, u32_buffer2, 4);
    print_buf(u32_buffer2, 32, 4);

    // printf("\n--------------------------------Calc function test--------------------------------\n");
}

int main(int argc, char const *argv[])
{
    uint32_t value = 0x003E0000;
    Store_Everywhere(H2C_DEVICE, 0, &value, 1);
    ring_ctrl(CTRL_REG_START_WRITE, 0, 1);
    // test_mram();
    return 0;
}
