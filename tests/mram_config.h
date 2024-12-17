#ifndef __MRAM_COMMAND_H__
#define __MRAM_COMMAND_H__

#include <stdint.h>

#define DEBUG_MODE 0

// common device name
#define H2C_DEVICE "/dev/xdma0_h2c_0"
#define C2H_DEVICE "/dev/xdma0_c2h_0"
#define CTRL_DEVICE "/dev/xdma0_user"

#define FIFO_LENGTH 16

// control code
#define CODE_START 0x1
#define CODE_CALC 0x2
#define CODE_INPUT 0x4
#define CODE_ADDR 0x8
#define CODE_ENDINS 0x10

// start command 'mode' field
#define START_MODE_READ 1
#define START_MODE_WRITE 2
#define START_MODE_CALC 3

// start command 'sel' field
#define START_SEL_ONE 1
#define START_SEL_ROW 2
#define START_SEL_ALL 3

// reg offset field
#define CTRL_REG_START_OFFSET 0x00
#define CTRL_REG_LEN_OFFSET 0x04
#define CTRL_REG_ADDR_OFFSET 0x08
#define CTRL_REG_STATUS_OFFSET 0x0C

// start reg type
#define CTRL_REG_START_READ 1
#define CTRL_REG_START_WRITE 2

// status reg type
#define CTRL_REG_STATUS_SUCCESS 0
#define CTRL_REG_STATUS_TIMEOUT 5

#define REG_ADDR_OFFSET 0x40000000
#define BRAM_ADDR_OFFSET 0xC0000000
// Memory is divided into instruction areas and data areas
// total memroy space is 1MB, memory map as follows:
// 0x0000_0000 ~ 0x0000_FFFF --> data areas
// 0x0001_0000 ~ 0x0001_7FFF --> instruction areas
// 0x0001_8000 ~ 0x0001_FFFF --> res data areas
// over 0x0001_FFFF means out of memory area
//
// DATA_AREA_START_OFFSET means data area start offset, also means max instruction area size
#define INST_AREA_START_OFFSET 0x00010000
#define RES_DATA_AREA_START_OFFSET 0x00018000

// #define REG_WEIGHT_GRANULARITY_OFFSET 0x00
// #define REG_INPUT_GRANULARITY_OFFSET 0x04
// #define REG_MACRO_SEL_OFFSET 0x08
// #define REG_WEIGHT_BASE_ADDR_OFFSET 0x0C
// #define REG_WEIGHT_LENGTH_OFFSET 0x10
// #define REG_CHIP_MODE_OFFSET 0x14

// *********************************************MRAM Command*********************************************

#pragma pack(1)

typedef struct Start_Command_T
{
    uint32_t reserved_lo : 8;
    uint32_t CS : 4;
    uint32_t SEL : 2;
    uint32_t MODE : 2;
    uint32_t reserved_hi : 1;
    uint32_t code : 15;
} Start_Command;

extern Start_Command START_CMD;

typedef struct Calculate_Command_T
{
    uint32_t in_group : 6;
    uint32_t inbits : 4;
    uint32_t wbits : 4;
    uint32_t reserved : 3;
    uint32_t code : 15;
} Calculate_Command;

extern Calculate_Command CALC_CMD;

typedef struct Input_Command_T
{
    uint32_t data : 16;
    uint32_t reserved : 1;
    uint32_t code : 15;
} Input_Command;

extern Input_Command INPUT_CMD;

typedef struct Addr_Command_T
{
    uint32_t addr : 15;
    uint32_t ip : 2;
    uint32_t code : 15;
} Addr_Command;

extern Addr_Command ADDR_CMD;

// default is 0x1555
typedef struct End_Command_T
{
    uint32_t end : 17;
    uint32_t code : 15;
} End_Command;

extern End_Command END_CMD;
extern End_Command PLL_RESET_CMD;
extern End_Command PLL_SET_CMD;
extern End_Command CTRL_RESET_CMD;

typedef union Controll_Command_T
{
    Start_Command start;
    Calculate_Command calculate;
    Input_Command input;
    Addr_Command addr;
    End_Command end;
} Controll_Command;

// ****************************************************************************************************

#endif