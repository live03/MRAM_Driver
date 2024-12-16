#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "mram_config.h"
#include "mram_api.h"
#include "dma_utils.h"

/* ltoh: little endian to host */
/* htol: host to little endian */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ltohl(x) (x)
#define ltohs(x) (x)
#define htoll(x) (x)
#define htols(x) (x)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define ltohl(x) __bswap_32(x)
#define ltohs(x) __bswap_16(x)
#define htoll(x) __bswap_32(x)
#define htols(x) __bswap_16(x)
#endif

#define PAGE_SIZE sysconf(_SC_PAGESIZE)

// initial command buffer
Start_Command START_CMD = {0, 0, 0, 0, 0, CODE_START};
Calculate_Command CALC_CMD = {0, 0, 0, 0, CODE_CALC};
Input_Command INPUT_CMD = {0, 0, CODE_INPUT};
Addr_Command ADDR_CMD = {0, 0, CODE_ADDR};
End_Command END_CMD = {0x015555, CODE_ENDINS};

// global Instruction Address Register
static uint32_t IAR = INST_AREA_START_OFFSET;

//****************************************print message******************************************************** */

void print_cmd(Controll_Command cmd)
{
#ifdef DEBUG_MODE
    char mode[3][6] = {"read", "write", "calc"};
    char sel[3][4] = {"one", "row", "all"};
    switch (cmd.start.code)
    {
    case CODE_START:
        printf("Start Command 0x%X = {MODE:%s, SEL:%s macro, CS:(row %d, col %d)}\n",
               cmd.start, mode[cmd.start.MODE], sel[cmd.start.SEL], cmd.start.CS >> 2, cmd.start.CS && 3);
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
        printf("Address Command 0x%X = {ip_addr:%d, mram_addr:%X}\n",
               cmd.addr, cmd.addr.ip, cmd.addr.addr);
        break;
    case CODE_ENDINS:
        printf("End Command 0x%X\n", cmd.end);
        break;
    default:
        break;
    }

#endif
}

//************************************************************************************************************* */

//****************************************reg read/write******************************************************** */
typedef struct mmap_reg_param_t
{
    int err;
    int fd;
    int *map;
} mmap_reg_param;

/**
 * @brief
 *
 * @param device_name
 * @param reg_addr
 * @return mmap_reg_param
 */
mmap_reg_param map_reg(char *device_name, int reg_addr)
{
    int fd;
    off_t target = reg_addr;
    off_t target_aligned, offset;
    int *map;
    mmap_reg_param res = {0, 0, 0};

    // check for target page alignment
    // usually, page size is 4k
    // get an in-page offset of the address
    offset = target & (PAGE_SIZE - 1);
    // If the address exceeds the page size, get the number of pages include
    target_aligned = target & (~(PAGE_SIZE - 1));
    printf("device: %s,base address:0x%lx, map range: 0x%lx ~ 0x%lx, access %s.\n",
           device_name, target, target_aligned, target_aligned + PAGE_SIZE, "read/write");

    // open xdma device
    if ((fd = open(device_name, O_RDWR | O_SYNC)) == -1)
    {
        fprintf(stderr, "character device %s opened failed: %s.\n",
                device_name, strerror(errno));
        res.err = errno;
        return res;
    }
    printf("character device %s opened.\n", device_name);

    // only map to the specified address and the last four bytes
    // map = mmap(NULL, offset + 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target_aligned);
    map = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target_aligned);
    if (map == (int *)-1)
    {
        fprintf(stderr, "Memory 0x%lx mapped failed: %s.\n",
                target, strerror(errno));
        close(fd);
        res.err = 1;
        return res;
    }
    printf("Memory 0x%lx mapped at address %p.\n",
           target_aligned, map);

    res.fd = fd;
    res.map = map;

    return res;
}

int umap_reg(mmap_reg_param mrp, int reg_addr)
{
    if (munmap(mrp.map, PAGE_SIZE) == -1)
    {
        fprintf(stderr, "Memory 0x%lx mapped failed: %s.\n",
                reg_addr, strerror(errno));
        return 1;
    }
    close(mrp.fd);
    return 0;
}

int Sldi(char *device_name, unsigned int imm, int reg_addr)
{
    int *map;

    mmap_reg_param mrp = map_reg(device_name, reg_addr);
    if (mrp.err)
        return 1;
    map = mrp.map;

    printf("Write 32-bits immediate value 0x%08x to 0x%lx (0x%p)\n",
           (unsigned int)imm, reg_addr, map);
    /* swap 32-bit endianess if host is not little-endian */
    imm = htoll(imm);
    *((uint32_t *)map + reg_addr) = imm;

    return umap_reg(mrp, reg_addr);
}

int Sld(char *device_name, int reg_addr_from, int reg_addr_to)
{
    int err;
    int *map_from, *map_to;
    off_t target_aligned_from, target_aligned_to;
    mmap_reg_param mrp_from, mrp_to;

    target_aligned_from = reg_addr_from & (~(PAGE_SIZE - 1));
    target_aligned_to = reg_addr_to & (~(PAGE_SIZE - 1));

    mrp_from = map_reg(device_name, reg_addr_from);
    mrp_to = target_aligned_from == target_aligned_to ? mrp_from : map_reg(device_name, reg_addr_to);
    if (mrp_from.err || mrp_to.err)
        return 1;

    map_from = mrp_from.map;
    map_to = mrp_to.map;

    printf("move 32-bit reg value from 0x%lx (0x%p) to 0x%lx (0x%p)\n",
           reg_addr_from, map_from, reg_addr_to, map_to);

    *((uint32_t *)map_to + reg_addr_to) = *((uint32_t *)map_from + reg_addr_from);

    err = umap_reg(mrp_from, reg_addr_from);
    err = err | umap_reg(mrp_to, reg_addr_to);

    return err;
}

int Ldr(char *device_name, int reg_addr, unsigned int *host_addr)
{
    int *map;

    mmap_reg_param mrp = map_reg(device_name, reg_addr);
    if (mrp.err)
        return 1;
    map = mrp.map;

    *host_addr = *((uint32_t *)map + reg_addr);

    printf("Read 32-bits value 0x%08x to 0x%lx (0x%p)\n",
           (unsigned int)*host_addr, reg_addr, map);

    return umap_reg(mrp, reg_addr);
}

static int ring_ctrl(int start_type, int addr, int len)
{
    int i, each_addr, each_len;
    int total_count = len * 4;
    int left_count = len % 128 == 0 ? len / 128 : len / 128 + 1;
    mmap_reg_param mrp;
    int *map;
    // mmap to regs
    mrp = map_reg(CTRL_DEVICE, CTRL_REG_START_OFFSET);
    if (mrp.err)
        return 1;
    map = mrp.map;
    for (i = 0; i < left_count; i++)
    {
        each_addr = addr + 128 * 4 * i;
        each_len = i == left_count - 1 ? len % 128 : 128;

        // write LEN and ADDR regs
        printf("Write (addr, len) : (0x%08x, %d) to reg (0x%lx, 0x%lx) (0x%p)\n",
               each_addr, each_len * 4, CTRL_REG_ADDR_OFFSET, CTRL_REG_LEN_OFFSET, map);
        *((uint32_t *)map + CTRL_REG_ADDR_OFFSET) = htoll(each_addr);
        *((uint32_t *)map + CTRL_REG_LEN_OFFSET) = htoll(each_len);

        // write START reg
        printf("Write start value 0x%08x to 0x%lx (0x%p)\n",
               start_type, CTRL_REG_START_OFFSET, map);
        /* swap 32-bit endianess if host is not little-endian */
        *((uint32_t *)map + CTRL_REG_START_OFFSET) = htoll(start_type);

        // read status
        while (*((uint32_t *)map + CTRL_REG_STATUS_OFFSET) != CTRL_REG_STATUS_SUCCESS)
            ;
    }

    umap_reg(mrp, CTRL_REG_START_OFFSET);

    printf("Ring and Process Success!\n");
}
//************************************************************************************************************* */

//****************************************RAM <---> Macro****************************************************** */

int Vread(char *device_name, int macro_row, int macro_col, int ip_addr, int mram_addr, int data_size);

int Vwrite(char *device_name, int macro_row, int macro_col, int ip_addr, int mram_addr, int data_size);

//************************************************************************************************************* */

//****************************************Host <---> Device RAM************************************************ */
// Allow data to be stored anywhere in the onboard RAM
static int Load_Everywhere(char *device_name, int device_ram_addr, unsigned int *host_addr, int data_size)
{
    int fd;
    ssize_t rc = 0;
    size_t bytes_done = 0;
    int underflow = 0;

    fd = open(device_name, O_RDWR);
    if (fd < 0)
    {
        fprintf(stderr, "unable to open device %s, %d.\n",
                device_name, fd);
        perror("open device");
        return -EINVAL;
    }

    rc = read_to_buffer(device_name, fd, (char *)host_addr, data_size * 4, device_ram_addr);
    if (rc < 0)
        goto out;
    bytes_done = rc;

    if (bytes_done < data_size)
    {
        fprintf(stderr, "underflow %ld/%ld.\n",
                bytes_done, data_size);
        underflow = 1;
    }

out:
    close(fd);
    if (rc < 0)
        return rc;
    return underflow ? -EIO : 0;
}

// only allow data to be stored in the data area in the onboard RAM
int Load(char *device_name, int device_ram_addr, unsigned int *host_addr, int data_size)
{
    if (device_ram_addr % 4 != 0)
    {
        printf("The address must be 4-byte aligned\n");
        return 1;
    }
    if (device_ram_addr >= INST_AREA_START_OFFSET)
    {
        printf("Out of data area 0x0000_0000 ~ 0x0007_FFFF\n");
        return 1;
    }
    return Load_Everywhere(device_name, device_ram_addr, host_addr, data_size);
}

// Allow data to be stored anywhere in the onboard RAM
static int Store_Everywhere(char *device_name, int device_ram_addr, unsigned int *host_addr, int data_size)
{
    int fd;
    ssize_t rc = 0;
    size_t bytes_done = 0;
    int underflow = 0;

    fd = open(device_name, O_RDWR);
    if (fd < 0)
    {
        fprintf(stderr, "unable to open device %s, %d.\n",
                device_name, fd);
        perror("open device");
        return -EINVAL;
    }

    /* write buffer to AXI MM address using SGDMA */
    rc = write_from_buffer(device_name, fd, (char *)host_addr, data_size * 4, device_ram_addr);
    if (rc < 0)
        goto out;

    bytes_done = rc;

    if (bytes_done < data_size)
    {
        printf("underflow %ld/%ld.\n",
               bytes_done, data_size);
        underflow = 1;
    }

out:
    close(fd);
    if (rc < 0)
        return rc;
    /* treat underflow as error */
    return underflow ? -EIO : 0;
}

// only allow data to be stored in the data area in the onboard RAM
int Store(char *device_name, int device_ram_addr, unsigned int *host_addr, int data_size)
{
    if (device_ram_addr % 4 != 0)
    {
        printf("The address must be 4-byte aligned\n");
        return 1;
    }
    if (device_ram_addr >= INST_AREA_START_OFFSET)
    {
        printf("Out of data area 0x0000_0000 ~ 0x0007_FFFF\n");
        return 1;
    }
    return Store_Everywhere(device_name, device_ram_addr, host_addr, data_size);
}
//************************************************************************************************************* */

//********************************************Host <---> Mcro************************************************** */
int Send(char *device_name, int macro_row, int macro_col, int ip_addr, int mram_addr, unsigned int *host_data_addr, int data_size)
{
    int i;
    ssize_t rc = 0;
    unsigned int *cmd_buffer = NULL;
    int pos = 0;
    int cmd_size = sizeof(Controll_Command);
    // avoid platform difference
    uint32_t *data_src = (uint32_t *)host_data_addr;

    // 4byte start_cmd + 4byte addr_cmd + (4byte input_cmd)*(2 * data_size) + 4byte end_cmd = 12 + 8*data_size byte
    int cmd_buf_size = 8 * data_size + 4096;
    posix_memalign((void **)&cmd_buffer, 4096 /*alignment */, cmd_buf_size);
    if (!cmd_buffer)
    {
        fprintf(stderr, "OOM %lu.\n", cmd_buf_size);
        rc = -ENOMEM;
        goto out;
    }
    printf("host buffer {size: 0x%lx, address: %p}\n",
           cmd_buf_size, cmd_buffer);

    // Start Command
    START_CMD.MODE = START_MODE_WRITE;
    START_CMD.SEL = START_SEL_ONE;
    START_CMD.CS = macro_row << 2 + macro_col;
    cmd_buffer[pos++] = *(uint32_t *)&START_CMD;
    print_cmd((Controll_Command)START_CMD);

    ADDR_CMD.ip = ip_addr;
    // Addr & Input Command
    for (i = 0; i < data_size; i++)
    {
        // addr auto increase
        ADDR_CMD.addr = mram_addr + i;
        cmd_buffer[pos++] = *(uint32_t *)&ADDR_CMD;
        print_cmd((Controll_Command)ADDR_CMD);

        // trans data high 16bit
        INPUT_CMD.data = data_src[i] >> 16;
        cmd_buffer[pos++] = *(uint32_t *)&INPUT_CMD;
        print_cmd((Controll_Command)INPUT_CMD);

        // trans data low 16bit
        INPUT_CMD.data = data_src[i] & 0x0000ffff;
        cmd_buffer[pos++] = *(uint32_t *)&INPUT_CMD;
        print_cmd((Controll_Command)INPUT_CMD);
    }

    // End Command
    cmd_buffer[pos++] = *(uint32_t *)&END_CMD;
    print_cmd((Controll_Command)END_CMD);

    // command store over, write reg to call
    rc = Store_Everywhere(device_name, IAR, cmd_buffer, pos);

    // ring controller to process cmd
    ring_ctrl(CTRL_REG_START_WRITE, IAR, pos);
    IAR = IAR + pos * 4 >= RES_DATA_AREA_START_OFFSET ? INST_AREA_START_OFFSET : IAR + pos * 4;

out:
    free(cmd_buffer);
    return rc;
}

int Recv(char *send_device_name, char *recv_device_name, int macro_row, int macro_col, int ip_addr, int mram_addr, unsigned int *host_data_addr, int data_size)
{
    int i, fd;
    ssize_t rc = 0;
    unsigned int *cmd_buffer = NULL;
    unsigned int *tmp_res_buffer = NULL;
    int pos = 0;
    int cmd_size = sizeof(Controll_Command);

    // 4Byte start_cmd + (4Byte addr_cmd)*data_size + 4Byte end_cmd = 6 + 4*data_size byte
    int cmd_buf_size = 4 * data_size + 4096;
    posix_memalign((void **)&cmd_buffer, 4096 /*alignment */, cmd_buf_size);
    if (!cmd_buffer)
    {
        fprintf(stderr, "OOM %lu.\n", cmd_buf_size);
        rc = -ENOMEM;
        goto out;
    }
    printf("host buffer {size: 0x%lx, address: %p}\n",
           cmd_buf_size, cmd_buffer);

    // Start Command
    START_CMD.MODE = START_MODE_READ;
    START_CMD.SEL = START_SEL_ONE;
    START_CMD.CS = macro_row << 2 + macro_col;
    cmd_buffer[pos++] = *(uint32_t *)&START_CMD;
    print_cmd((Controll_Command)START_CMD);

    ADDR_CMD.ip = ip_addr;
    // Addr & Input Command
    for (i = 0; i < data_size; i++)
    {
        // addr auto increase
        ADDR_CMD.addr = mram_addr + i;
        cmd_buffer[pos++] = *(uint32_t *)&ADDR_CMD;
        print_cmd((Controll_Command)ADDR_CMD);
    }

    // End Command
    cmd_buffer[pos++] = *(uint32_t *)&END_CMD;
    print_cmd((Controll_Command)END_CMD);

    // Trans command
    rc = Store_Everywhere(send_device_name, IAR, cmd_buffer, pos);

    // ring controller to process cmd
    ring_ctrl(CTRL_REG_START_WRITE, IAR, pos);
    IAR = IAR + pos * 4 >= RES_DATA_AREA_START_OFFSET ? INST_AREA_START_OFFSET : IAR + pos * 4;

    // ring controller to read res data
    // The result is 16 bits in a single return, so we need to read it twice
    ring_ctrl(CTRL_REG_START_READ, RES_DATA_AREA_START_OFFSET, data_size * 2);

    // wait to recv data
    tmp_res_buffer = (unsigned int *)malloc(data_size * 2 * sizeof(uint32_t));

    fd = open(recv_device_name, O_RDWR);
    rc = read_to_buffer(recv_device_name, fd, (char *)tmp_res_buffer, data_size * 2 * 4, RES_DATA_AREA_START_OFFSET);
    // recombination the result
    for (i = 0; i < data_size; i++)
        host_data_addr[0] = tmp_res_buffer[i * 2] + (tmp_res_buffer[i * 2 + 1] << 16);

out:
    free(cmd_buffer);
    close(fd);
    return rc;
}
//************************************************************************************************************* */

//************************************************Calculate**************************************************** */

int Vmmul(char *device_name, char *recv_device_name, unsigned int *input_vector, int input_size, int inbits, int in_group, int macro_row, int macro_col, int mram_addr, int wbits, int *host_data_addr)
{
    int fd, i, j, mr, mc, ig, ib, idx, wb, res_data_size;
    ssize_t rc = 0;
    uint32_t *cmd_buffer = NULL;
    int32_t *tmp_res_buffer = NULL;
    int pos = 0;
    int actived_macro_size = macro_row == 4 ? 16 : macro_col == 4 ? 4
                                                                  : 1;
    int actived_macro_row_num = actived_macro_size == 16 ? 4 : 1;
    int actived_macro_col_num = actived_macro_size == 1 ? 1 : 4;
    int cmd_size = sizeof(Controll_Command);

    // 4byte start_cmd + 4Byte calc_cmd + (4Byte input_cmd)*(actived_macro_row_num * fifo_length * (512 / 16))
    // + (4Byte addr_cmd)*(in_group * wbits * actived_macro_size * (512 / 32)) + 4Byte end_cmd = 12 + 3*data_size byte
    int cmd_buf_size = 4 * (actived_macro_row_num * 512 + in_group * wbits * actived_macro_size * 16) + 4096;
    posix_memalign((void **)&cmd_buffer, 4096 /*alignment */, cmd_buf_size);
    if (!cmd_buffer)
    {
        fprintf(stderr, "OOM %lu.\n", cmd_buf_size);
        rc = -ENOMEM;
        goto out;
    }
    printf("host buffer {size: 0x%lx, address: %p}\n",
           cmd_buf_size, cmd_buffer);

    // Start Command
    START_CMD.MODE = START_MODE_CALC;
    switch (actived_macro_size)
    {
    case 16:
        START_CMD.SEL = START_SEL_ALL;
        START_CMD.CS = 0;
        break;
    case 4:
        START_CMD.SEL = START_SEL_ROW;
        START_CMD.CS = macro_row << 2;
        break;
    case 1:
        START_CMD.SEL = START_SEL_ONE;
        START_CMD.CS = macro_row << 2 + macro_col;
        break;
    }

    cmd_buffer[pos++] = *(uint32_t *)&START_CMD;
    print_cmd((Controll_Command)START_CMD);

    // transfer input vector
    for (mr = 0; mr < actived_macro_row_num; mr++)
    {
        // transfer data to each macro row input buffer
        for (ig = 0; ig < in_group; ig++)
        {
            for (ib = 0; ib < inbits; ib++)
            {
                for (idx = 0; idx < 32; idx++)
                {
                    // the shape of input_vector is [actived_macro_row_num][in_group][inbits][16]
                    INPUT_CMD.data = input_vector[mr * (in_group * inbits * 16) + ig * (inbits * 16) + ib * 16 + idx];
                    cmd_buffer[pos++] = *(uint32_t *)&INPUT_CMD;
                    print_cmd((Controll_Command)INPUT_CMD);
                }
            }
        }
        // fill remained input buffer with 0
        if (in_group * inbits < FIFO_LENGTH)
        {
            for (i = 0; i < FIFO_LENGTH - in_group * inbits; i++)
            {
                for (j = 0; j < 32; j++)
                {
                    INPUT_CMD.data = 0;
                    cmd_buffer[pos++] = *(uint32_t *)&INPUT_CMD;
                    print_cmd((Controll_Command)INPUT_CMD);
                }
            }
        }
    }

    // transfer weight matrix address
    for (ig = 0; ig < in_group; ig++)
    {
        for (wb = 0; wb < wbits; wb++)
        {
            for (mr = 0; mr < actived_macro_row_num; mr++)
            {
                for (mc = 0; mc < actived_macro_size; mc++)
                {
                    // A single address retrieves 32 bits of data, so just need 16 addresses
                    for (idx = 0; idx < 16; idx++)
                    {
                        ADDR_CMD.ip = 0;
                        // weight matrix must be written sequentially
                        ADDR_CMD.addr = mram_addr + idx;
                        cmd_buffer[pos++] = *(uint32_t *)&ADDR_CMD;
                        print_cmd((Controll_Command)ADDR_CMD);
                    }
                }
            }
        }
    }

    // End Command
    cmd_buffer[pos++] = *(uint32_t *)&END_CMD;
    print_cmd((Controll_Command)END_CMD);

    // Trans command
    rc = Store_Everywhere(device_name, IAR, cmd_buffer, pos);

    // ring controller to process cmd
    ring_ctrl(CTRL_REG_START_WRITE, IAR, pos);
    IAR = IAR + pos * 4 >= RES_DATA_AREA_START_OFFSET ? INST_AREA_START_OFFSET : IAR + pos * 4;

    // ring controller to read res data
    res_data_size = in_group * actived_macro_size * 7;
    ring_ctrl(CTRL_REG_START_READ, RES_DATA_AREA_START_OFFSET, res_data_size);

    // wait to recv data
    tmp_res_buffer = (unsigned int *)malloc(res_data_size * sizeof(uint32_t));

    fd = open(recv_device_name, O_RDWR);
    rc = read_to_buffer(recv_device_name, fd, (char *)tmp_res_buffer, res_data_size * 4, RES_DATA_AREA_START_OFFSET);
    // recombination the result

    // received data
    // {data1_hi[24:9], {data1_lo[8:0],data2_hi[24:18]}, data2_me[17:2], {data2_lo[1:0], data3_hi[24:11]},
    // {data3_lo[10:0], data4_hi[24:20]}, data4_me[19:4], {data_lo[3:0], cycle_num[11:0]}}

    // target data
    // {data1[24:0], data2[24:0], data3[24:0], data4[24:0], cycle_num[11:0]}
    for (i = 0; i < in_group * actived_macro_size; i++)
    {
        host_data_addr[i * 4] = tmp_res_buffer[i * 7 + 6] << 9 + tmp_res_buffer[i * 7 + 5] >> 7;
        host_data_addr[i * 4 + 1] = (tmp_res_buffer[i * 7 + 5] & 0x3F) << 18 + tmp_res_buffer[i * 7 + 4] << 2 + tmp_res_buffer[i * 7 + 3] >> 14;
        host_data_addr[i * 4 + 2] = (tmp_res_buffer[i * 7 + 3] & 0x3FF) << 11 + tmp_res_buffer[i * 7 + 2] >> 5;
        host_data_addr[i * 4 + 3] = (tmp_res_buffer[i * 7 + 2] & 0x1F) << 20 + tmp_res_buffer[i * 7 + 1] << 4 + tmp_res_buffer[i * 7] >> 12;
    }
out:
    free(cmd_buffer);
    return rc;
}

int Vvdmu(char *device_name, int ram_vector_addr, int ram_vector_size, int inbits, int in_group, int macro_row, int macro_col, int mram_addr, int wbits, int device_ram_addr);

int Vvadd(char *device_name, unsigned int *input_vector, int input_size, int ram_vector_addr, int device_ram_addr);

//************************************************************************************************************* */

//************************************************For synchronizing******************************************** */

int Wait(int macro_row, int macro_col)
{
    printf("Macro(%d, %d) is Locked", macro_row, macro_col);
}

int Wakeup(int macro_row, int macro_col)
{
    printf("Macro(%d, %d) is Unlocked", macro_row, macro_col);
}

//************************************************************************************************************* */
