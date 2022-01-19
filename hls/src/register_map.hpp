/* Copyright 2021 Columbia University SLD Group */

#ifndef __REGISTER_MAP_HPP__
#define __REGISTER_MAP_HPP__

// -- Status and command registers

#define CMD_REG                  0x00

// -- Command encoding

#define ACCELERATOR_CMD_CLEARIRQ 0x00
#define ACCELERATOR_CMD_GO       0x01
#define ACCELERATOR_STATUS_DONE  0x02

// -- Pointers to memory

#define IN_BASE_ADDR_REG         0x04
#define IN_W_BASE_ADDR_REG       0x08
#define OUT_B_BASE_ADDR_REG      0x0c
#define OUT_BASE_ADDR_REG        0x10

// -- Configuration parameters

#define IN_SIZE_REG              0x1c
#define IN_W_SIZE_REG            0x20
#define OUT_SIZE_REG             0x24

// -- Fully-connected only

#define NUM_W_COLS_REG           0x14
#define NUM_W_ROWS_REG           0x18

// -- Convolutional only

#define OUT_B_SIZE_REG           0x28
#define NUM_COLS_REG             0x2c
#define NUM_ROWS_REG             0x30
#define SRC_CHANS_REG            0x34
#define DST_CHANS_REG            0x38
#define DO_POOL_REG              0x3c
#define DO_PAD_REG               0x40
#define POOL_SIZE_REG            0x44
#define POOL_STRIDE_REG          0x48

#endif /* __REGISTER_MAP_HPP__ */
