#ifndef __MRAM_API_H__
#define __MRAM_API_H__

/**
 * @brief Read data from Macro to onboard RAM. Write data from onboard RAM to Macro. The data_size identifies the length of
 * data to be read, in 32bit
 *
 * @param device_name the device to be opened, eg. /dev/xdma0_h2c_0
 * @param macro_row the macro array shape is 4x4, row number range is [0, 3]
 * @param macro_colthe the macro array shape is 4x4, col number range is [0, 3]
 * @param ip_addr a macro contains 4 MRAM IPs, range is [0, 3]
 * @param mram_addr no need to concern about the MRAM row and col, just give a 15-bit addr
 * @param data_size identifies the length of data to be read, in 32bit
 * @return int
 */
int Vread(char *device_name, int macro_row, int macro_col, int ip_addr, int mram_addr, int data_size);

/**
 * @brief Write data from onboard RAM to Macro. The data_size identifies the length of
 * data to be write, in 32bit
 *
 * @param device_name the device to be opened, eg. /dev/xdma0_h2c_0
 * @param macro_row the macro array shape is 4x4, row number range is [0, 3]
 * @param macro_colthe the macro array shape is 4x4, col number range is [0, 3]
 * @param ip_addr a macro contains 4 MRAM IPs, range is [0, 3]
 * @param mram_addr no need to concern about the MRAM row and col, just give a 15-bit addr
 * @param data_size identifies the length of data to be read, in 32bit
 * @return int
 */
int Vwrite(char *device_name, int macro_row, int macro_col, int ip_addr, int mram_addr, int data_size);

/**
 * @brief Load data from onboard RAM to host buffer. The data_size identifies the length of
 * param 'host_data_addr'.
 * @example Load calculation result from onboard RAM address 0x0004 to host buffer
 * int device_ram_addr = 0x0004;
 * unsigned int calc_result[4];
 * Load("/dev/xdma0_c2h_0", 0x0004, calc_result, 4);
 *
 * @param device_name the device to be opened, eg. /dev/xdma0_c2h_0
 * @param device_ram_addr the RAM address on the device. If use axi-stream, this should always be 0
 * @param host_addr host buffer address, in 32bits
 * @param data_size  identifies the length of param 'host_addr', in 32bits
 * @return int not zero means error
 */
int Load(char *device_name, int device_ram_addr, unsigned int *host_addr, int data_size);

/**
 * @brief Store data to onboard RAM from host buffer. The data_size identifies the length of
 * param 'host_data_addr'.
 * @example Store some commands to onboard RAM
 * int device_ram_addr = 0x0004;
 * unsigned int cmd[3] = {0x00410001, 0x00410002, 0x00215555};
 * Load("/dev/xdma0_h2c_0", device_ram_addr, cmd, 3);
 *
 * @param device_name the device to be opened, eg. /dev/xdma0_h2c_0
 * @param device_ram_addr the RAM address on the device. If use axi-stream, this should always be 0
 * @param host_addr host buffer address, in 32bits
 * @param data_size identifies the length of param 'host_addr'
 * @return int not zero means error
 */
int Store(char *device_name, int device_ram_addr, unsigned int *host_addr, int data_size);

/**
 * @brief Send data from the host to the Macro directly. The data_size identifies the length of
 * param 'host_data_addr'.
 * @example Store 512x4 weight matrix to Macro01, offset 0x0000, assume that the weight data has a maximum length of 5 bits
 * // original weight matrix, shape is [col][row]
 * unsigned int weight[4][512] = {{28, 11,..., 24, 31},{15, 19,... 2, 26}, {30, 17,..., 11, 9}, {3, 7,..., 11, 8}};
 * // Omitting the bit slicing process.
 * // After bitwise slicing process, the shape of weight matrix is [col][bitsize][16].
 * // Here bitsize is 5. Since a macro unit stores 32 bits of data, each col has 512 bits data in total, the final length is 16.
 * unsigned int weight_in_bits[4][5][16] = {...};
 * Send("/dev/xdma0_h2c_0", 0, 1, 0, 0, weights, 0, (unsigned int *)(weight_in_bits[0]), 5 * 16);
 * Send("/dev/xdma0_h2c_0", 0, 1, 1, 0, weights, 0, (unsigned int *)(weight_in_bits[1]), 5 * 16);
 * Send("/dev/xdma0_h2c_0", 0, 1, 2, 0, weights, 0, (unsigned int *)(weight_in_bits[2]), 5 * 16);
 * Send("/dev/xdma0_h2c_0", 0, 1, 3, 0, weights, 0, (unsigned int *)(weight_in_bits[3]), 5 * 16);
 *
 * @param device_name the device to be opened, eg. /dev/xdma0_h2c_0
 * @param macro_row the macro array shape is 4x4, row number range is [0, 3]
 * @param macro_col the macro array shape is 4x4, colum number range is [0, 3]
 * @param ip_addr a macro contains 4 MRAM IPs, range is [0, 3]
 * @param mram_addr no need to concern about the MRAM row and col, just give a 15-bit addr
 * @param host_data_addr each address stores 32 bits of data, so pass in a buffer address with 32-bit entry
 * @param data_size identifies the length of param 'host_data_addr'
 * @return int not zero means error
 */
int Send(char *device_name, int macro_row, int macro_col, int ip_addr, int mram_addr, unsigned int *host_data_addr, int data_size);

/**
 * @brief Receive data from the Macro to the host directly, note that the host_data_addr
 * must be a 32bit type array, like 'int*'. The data_size identifies the length of
 * param 'host_data_addr'.
 * example: Read weight matrix from Macro01.
 * unsigned int weight_in_bits[5][16];
 * Send("/dev/xdma0_h2c_0", "/dev/xdma0_c2h_0", 0, 1, 0, 0, (unsigned int *)(weight_in_bits), 5 * 16);
 * // The data received is a bit slice and needs to be restored. Omitting the restoring process.
 * unsigned int weight[512] = restore(weight_in_bits);
 *
 * @param send_device_name the device opened to transfer command, eg. /dev/xdma0_h2c_0
 * @param recv_device_name the device opened to receive read data, eg. /dev/xdma0_c2h_0
 * @param macro_row the macro array shape is 4x4, row number range is [0, 3]
 * @param macro_col the macro array shape is 4x4, colum number range is [0, 3]
 * @param ip_addr a macro contains 4 MRAM IPs, range is [0, 3]
 * @param mram_addr no need to concern about the MRAM row and col, just give a 15-bit addr
 * @param host_data_addr each address stores 32 bits of data, so pass in a buffer address with 32-bit entry
 * @param data_size identifies the length of param 'host_data_addr'
 * @return int not zero means error
 */
int Recv(char *send_device_name, char *recv_device_name, int macro_row, int macro_col, int ip_addr, int mram_addr, unsigned int *host_data_addr, int data_size);

/**
 * @brief Vector matrix multiplication. Result are stored in the device RAM, which is specified by param device_ram_addr
 * @example Compute the multiplication of a vector[1x28] and matrix[28x4]
 * // original data are as follow
 * unsigned int vector[28] = {1,2,...28};
 * unsigned int weight[4][28] = {{28, 11,...24, 31},{15, 19,...2, 26}, {30, 17,...11, 9}, {3, 7,...11, 8}};
 *
 * // fill the original data to 512
 * unsigned int vector_filled[512] = {0, 0, 0, 0,...0, 0, 1, 2,...28};
 * unsigned int weight_filled[4][512] = {{0, 0, 0,...28, 11,...24, 31},{0, 0, 0,...15, 19,...2, 26}, {0, 0, 0, 0,...30, 17,...11, 9}, {0, 0, 0,,.3, 7,...11, 8}};
 *
 * // Omitting the bit slicing process. Assume that vector and weight matrix has a maximum length of 5 bits,
 * // After bitwise slicing process, the shape of weight matrix is [col][bitsize][16].
 * // Here bitsize is 5. Since a macro unit stores 32 bits of data, each col has 512 bits data in total, the final length is 16.
 * unsigned int vector_filledin_bits[5][16] = {...};
 * unsigned int weight_filled_in_bits[4][5][16] = {...};
 * // Store weight to macro01, offset 0x0008
 * Send("/dev/xdma0_h2c_0", 0, 1, 0, 0, weights, 0x0008, (unsigned int *)(weight_filled_in_bits[0]), 5 * 16);
 * Send("/dev/xdma0_h2c_0", 0, 1, 1, 0, weights, 0x0008, (unsigned int *)(weight_filled_in_bits[1]), 5 * 16);
 * Send("/dev/xdma0_h2c_0", 0, 1, 2, 0, weights, 0x0008, (unsigned int *)(weight_filled_in_bits[2]), 5 * 16);
 * Send("/dev/xdma0_h2c_0", 0, 1, 3, 0, weights, 0x0008, (unsigned int *)(weight_filled_in_bits[3]), 5 * 16);
 *
 * // send calculation command
 * int res_addr = 0x1000;
 * Vmmul("/dev/xdma0_h2c_0", (unsigned int *)vector_filledin_bits, 5 * 16, 5, 1, 0, 1, 0x0008, 5, res_addr)
 *
 * // read cal result
 * unsiged int calc_result[4];
 * Load("/dev/xdma0_c2h_0", res_addr, calc_result, 4);
 *
 * @param device_name the device to be opened, eg. /dev/xdma0_h2c_0
 * @param recv_device_name the device opened to receive read data, eg. /dev/xdma0_c2h_0
 * @param input_vector input vector array, each element is 32bit. Array shape is [actived_macro_row_num][in_group][inbits][16].
 * @param input_size identifies the length of param 'input_vector', in 32bits
 * @param inbits the bit width of all input element. Determined by the maximum value in the input vector.
 * @param in_group number of input vectors
 * @param macro_row value range is [0, 4]. If it's 4, means all row open, otherwise open the specified row.
 * @param macro_col value range is [0, 4]. If it's 4, means all col open, otherwise open the specified col.
 * @param mram_addr start address of weight matrix
 * @param wbits the bit width of all weight matrix element. Determined by the maximum value in the weight matrix.
 * @param host_data_addr the host address of the calculation result, length is in_group * actived_macro_num * 4
 * @return int not zero means error
 */
int Vmmul(char *device_name, char *recv_device_name, unsigned int *input_vector, int input_size, int inbits, int in_group, int macro_row, int macro_col, int mram_addr, int wbits, int *host_data_addr);

/**
 * @brief Vector vector dot product. One vector is from onborad RAM, the other is from Macro. Result are stored in the
 * device RAM, which is specified by param device_ram_addr
 * @example Compute the multiplication of a vector[1x28] and another vector[1x28]
 * TODO: Hardware is currently unable to implement this feature
 *
 * // You need to pre-load the input vector to onboard RAM
 * unsigned int ram_addr = 0x0008;
 * unsigned int ram_vector_in_bits[5][16] = {...};
 * Store("/dev/xdma0_h2c_0", ram_addr, (unsigned int *)ram_vector_in_bits, 5 * 16);
 *
 * //You need to pre-load the weight vector to Macro the method then called is basically the same as Vmmul
 * unsigned int ip_addr = 0;
 * unsigned int mram_addr = 0x0012;
 * unsigned int macro_vector_in_bits[5][16] = {...};
 * Send("/dev/xdma0_h2c_0", 0, 1, ip_addr, mram_addr, (unsigned int *)(macro_vector_in_bits), 5 * 16);
 *
 * // This method then called is basically the same as Vmmul
 * int res_addr = 0x1000;
 * Vvdmu("/dev/xdma0_h2c_0", ram_addr, 5 * 16, 5, 1, 0, 1, mram_addr, 5, res_addr);
 *
 * // read cal result, the calc_result[ip_addr] is the answer we need, the rest are useless data.
 * unsiged int calc_result[4];
 * Load("/dev/xdma0_c2h_0", res_addr, calc_result, 4);
 *
 * @param device_name the device to be opened, eg. /dev/xdma0_h2c_0
 * @param ram_vector_addr input vector array address, each element is 32bit. Array shape is [actived_macro_row_num][in_group][inbits][16].
 * @param ram_vector_size identifies the length of param 'ram_vector_addr', in 32bits
 * @param inbits the bit width of all input element. Determined by the maximum value in the input vector.
 * @param in_group number of input vectors
 * @param macro_row value range is [0, 4]. If it's 4, means all row open, otherwise open the specified row.
 * @param macro_col value range is [0, 4]. If it's 4, means all col open, otherwise open the specified col.
 * @param mram_addr start address of weight matrix
 * @param wbits the bit width of all weight matrix element. Determined by the maximum value in the weight matrix.
 * @param device_ram_addr the RAM address of the calculation result
 * @return int not zero means error
 */
int Vvdmu(char *device_name, int ram_vector_addr, int ram_vector_size, int inbits, int in_group, int macro_row, int macro_col, int mram_addr, int wbits, int device_ram_addr);

/**
 * @brief Vector vector add. One vector is from host input, the other is form onboard RAm. Results are stored in the
 * onboard RAM, which is specified by param device_ram_addr.
 * @example Calculate the addition of two vectors.
 * TODO: Hardware is currently unable to implement this feature
 * unsigned int *input[28] = {1, 2,...28};
 * ram_addr = 0x001C;
 * res_addr = 0x1000;
 * Vvadd("/dev/xdma0_h2c_0", input, 28, ram_addr, res_addr);
 *
 * @param device_name the device to be opened, eg. /dev/xdma0_h2c_0
 * @param input_vector input vector array, each element is 32bit.
 * @param input_size identifies the length of param 'input_vector'
 * @param ram_vector_addr the RAM address of another vector. The length of another vector is the same as param 'input_size'.
 * @param device_ram_addr the RAM address of the calculation result
 * @return int not zero means error
 */
int Vvadd(char *device_name, unsigned int *input_vector, int input_size, int ram_vector_addr, int device_ram_addr);

/**
 * @brief Lock the specificed macro
 * @example lock the macro01 before calculate
 * Wait(0, 1);
 * // do something with macro01...
 * Wakeup(0, 1);
 *
 * @param macro_row value range is [0, 4]. If it's 4, means all row open, otherwise open the specified row.
 * @param macro_col value range is [0, 4]. If it's 4, means all col open, otherwise open the specified col.
 * @return int not zero means lock failed
 */
int Wait(int macro_row, int macro_col);

/**
 * @brief unlock the specificed macro
 * @example lock the macro01 before calculate
 * Wait(0, 1);
 * // do something with macro01...
 * Wakeup(0, 1);
 *
 * @param macro_row value range is [0, 4]. If it's 4, means all row open, otherwise open the specified row.
 * @param macro_col value range is [0, 4]. If it's 4, means all col open, otherwise open the specified col.
 * @return int not zero means unlock failed
 */
int Wakeup(int macro_row, int macro_col);

/**
 * @brief Initialize/Write the register using the immediate value.
 * Reg_addr is defined in mram_config.h
 * @example set the value of reg WEIGHT_BASE_ADDR to 0x00010000
 * Sldi("/dev/xdma0_control", 0x00010000, REG_WEIGHT_BASE_ADDR_OFFSET)
 *
 * @param device_name the device to be opened, eg. /dev/xdma0_control
 * @param imm 32bit immediate value
 * @param reg_addr reg address/offset
 * @return int not zero means error
 */
int Sldi(char *device_name, unsigned int imm, int reg_addr);

/**
 * @brief Assigns the value of reg_addr_from to reg_addr_to.
 * Reg_addr is defined in mram_config.h
 * @example Assigns the value of MACRO_SEL to WEIGHT_BASE_ADDR.
 * Sld("/dev/xdma0_control", REG_MACRO_SEL_OFFSET, REG_WEIGHT_BASE_ADDR_OFFSET)
 *
 * @param device_name the device to be opened, eg. /dev/xdma0_control
 * @param reg_addr_from reg address/offset from
 * @param reg_addr_to reg address/offset to
 * @return int not zero means error
 */
int Sld(char *device_name, int reg_addr_from, int reg_addr_to);

/**
 * @brief Read the value of the reg_addr and pass it to host_addr.
 * Reg_addr is defined in mram_config.h
 * @example Read the value of the reg WEIGHT_BASE_ADDR
 * unsigned int weight_base_addr;
 * Ldr("/dev/xdma0_control", REG_WEIGHT_BASE_ADDR_OFFSET, &weight_base_addr);
 *
 * @param device_name the device to be opened, eg. /dev/xdma0_control
 * @param reg_addr reg address/offset
 * @param host_addr host buffer address
 * @return int not zero means error
 */
int Ldr(char *device_name, int reg_addr, unsigned int *host_addr);

#endif