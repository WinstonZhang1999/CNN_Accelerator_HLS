/* Copyright 2021 Columbia University SLD Group */

#include "utils.hpp"
#include "register_map.hpp"

#ifndef __FC_LAYER_REGS_HPP__
#define __FC_LAYER_REGS_HPP__

// -- Functions (reg)

void fc_layer::reset_single_read(void)
{
    // Nothing to do
}

void fc_layer::single_read(reg_target_t::archan_t &archan,
                      reg_target_t::rchan_t &rchan)
{
    rchan.resp = axi4_lite::AXI_OK_RESPONSE;

    switch (archan.addr & 0xfff)
    {
    // -- Pointers to memory

    case IN_BASE_ADDR_REG:
        REPORT_DEBUG("IN_BASE_ADDR_REG: 0x%08x", in_base_addr_sig.read());
        rchan.data = in_base_addr_sig.read();
        break;

    case IN_W_BASE_ADDR_REG:
        REPORT_DEBUG("IN_W_BASE_ADDR_REG: 0x%08x", in_w_base_addr_sig.read());
        rchan.data = in_w_base_addr_sig.read();
        break;

    case OUT_B_BASE_ADDR_REG:
        REPORT_DEBUG("OUT_B_BASE_ADDR_REG: 0x%08x", out_b_base_addr_sig.read());
        rchan.data = out_b_base_addr_sig.read();
        break;

    case OUT_BASE_ADDR_REG:
        REPORT_DEBUG("OUT_BASE_ADDR_REG: 0x%08x", out_base_addr_sig.read());
        rchan.data = out_base_addr_sig.read();
        break;

    // -- Configuration parameter

    case NUM_W_COLS_REG:
        REPORT_DEBUG("NUM_W_COLS_REG: 0x%08x", num_w_cols_sig.read());
        rchan.data = num_w_cols_sig.read();
        break;

    case NUM_W_ROWS_REG:
        REPORT_DEBUG("NUM_W_ROWS_REG: 0x%08x", num_w_rows_sig.read());
        rchan.data = num_w_rows_sig.read();
        break;

    case IN_SIZE_REG:
        REPORT_DEBUG("IN_SIZE_REG: 0x%08x", in_size_sig.read());
        rchan.data = in_size_sig.read();
        break;

    case IN_W_SIZE_REG:
        REPORT_DEBUG("IN_W_SIZE_REG: 0x%08x", in_w_size_sig.read());
        rchan.data = in_w_size_sig.read();
        break;

    case OUT_SIZE_REG:
        REPORT_DEBUG("OUT_SIZE_REG: 0x%08x", out_size_sig.read());
        rchan.data = out_size_sig.read();
        break;

    // -- Status and command registers

    case CMD_REG:
        if (irq.read())
            rchan.data = ACCELERATOR_STATUS_DONE;
        else
            cmd_sig.read();
        break;

    default:
        REPORT_DEBUG("REGISTER NOT MAPPED");
        rchan.resp = axi4_lite::AXI_SLVERR_RESPONSE;
        break;
    }
}

bool fc_layer::can_single_read(void) const
{
    return true;
}

void fc_layer::reset_single_write(void)
{
    this->cmd_sig.write(0);
    this->in_base_addr_sig.write(0);
    this->in_w_base_addr_sig.write(0);
    this->out_b_base_addr_sig.write(0);
    this->out_base_addr_sig.write(0);
    this->num_w_cols_sig.write(0);
    this->num_w_rows_sig.write(0);
    this->in_size_sig.write(0);
    this->in_w_size_sig.write(0);
    this->out_size_sig.write(0);
}

#define ALIGNMENT_MASK ((DATA_WIDTH / 8) - 1)

void fc_layer::single_write(reg_target_t::awchan_t &awchan, const
                       reg_target_t::wchan_t &wchan, reg_target_t::bchan_t &bchan)
{
    bchan.resp = axi4_lite::AXI_OK_RESPONSE;

    const uint32_t wdata = wchan.data.to_uint();
    const bool aligned = (wdata & ALIGNMENT_MASK) ? false : true;
    const bool is_non_zero = (wdata != 0);
    // const bool is_non_zero_pow2 = wdata && (!(wdata & (wdata - 1)));

    switch (awchan.addr & 0xfff)
    {

    // -- Pointers to memory

    case IN_BASE_ADDR_REG:
        // We force aligned transactions over the bus
        if (!aligned) {
            REPORT_DEBUG("INVALID ARGUMENT: address is not aligned (w)");
            bchan.resp = axi4_lite::AXI_SLVERR_RESPONSE;
        } else {
            in_base_addr_sig = wdata;
            REPORT_DEBUG("IN_BASE_ADDR_REG: 0x%08x (w)", wdata);
        }
        break;

    case IN_W_BASE_ADDR_REG:
        if (!aligned) {
            REPORT_DEBUG("INVALID ARGUMENT: address is not aligned (w)");
            bchan.resp = axi4_lite::AXI_SLVERR_RESPONSE;
        } else {
            in_w_base_addr_sig = wdata;
            REPORT_DEBUG("IN_W_BASE_ADDR_REG: 0x%08x (w)", wdata);
        }
        break;

    case OUT_B_BASE_ADDR_REG:
        if (!aligned) {
            REPORT_DEBUG("INVALID ARGUMENT: address is not aligned (w)");
            bchan.resp = axi4_lite::AXI_SLVERR_RESPONSE;
        } else {
            out_b_base_addr_sig = wdata;
            REPORT_DEBUG("OUT_B_BASE_ADDR_REG: 0x%08x (w)", wdata);
        }
        break;

    case OUT_BASE_ADDR_REG:
        if (!aligned) {
            REPORT_DEBUG("INVALID ARGUMENT: address is not aligned (w)");
            bchan.resp = axi4_lite::AXI_SLVERR_RESPONSE;
        } else {
            out_base_addr_sig = wdata;
            REPORT_DEBUG("OUT_BASE_ADDR_REG: 0x%08x (w)", wdata);
        }
        break;

    // -- Configuration parameters

    case NUM_W_COLS_REG:
        if (!is_non_zero) {
            REPORT_DEBUG("INVALID ARGUMENT: num_w_cols_reg cannot be 0 (w)");
            bchan.resp = axi4_lite::AXI_SLVERR_RESPONSE;
        } else if (wdata > PLM_SIZE) {
            REPORT_DEBUG("INVALID ARGUMENT: num_w_cols_reg cannot be larger than PLM_SIZE (w)");
            bchan.resp = axi4_lite::AXI_SLVERR_RESPONSE;
        } else {
            num_w_cols_sig = wdata;
            REPORT_DEBUG("NUM_W_COLS_REG: 0x%08x (w)", wdata);
        }
        break;

    case NUM_W_ROWS_REG:
        if (!is_non_zero) {
            REPORT_DEBUG("INVALID ARGUMENT: num_w_rows_reg cannot be 0 (w)");
            bchan.resp = axi4_lite::AXI_SLVERR_RESPONSE;
        } else {
            num_w_rows_sig = wdata;
            REPORT_DEBUG("NUM_W_ROWS_REG: 0x%08x (w)", wdata);
        }
        break;


    case IN_SIZE_REG:
        if (!is_non_zero) {
            REPORT_DEBUG("INVALID ARGUMENT: in_size_reg cannot be 0 (w)");
            bchan.resp = axi4_lite::AXI_SLVERR_RESPONSE;
        } else if (wdata > PLM_SIZE) {
            REPORT_DEBUG("INVALID ARGUMENT: in_size_reg cannot be larger than PLM_SIZE (w)");
            bchan.resp = axi4_lite::AXI_SLVERR_RESPONSE;
        } else {
            in_size_sig = wdata;
            REPORT_DEBUG("IN_SIZE_REG: 0x%08x (w)", wdata);
        }
        break;


    case IN_W_SIZE_REG:
        if (!is_non_zero) {
            REPORT_DEBUG("INVALID ARGUMENT: in_w_size_reg cannot be 0 (w)");
            bchan.resp = axi4_lite::AXI_SLVERR_RESPONSE;
        } else {
            in_w_size_sig = wdata;
            REPORT_DEBUG("IN_W_SIZE_REG: 0x%08x (w)", wdata);
        }
        break;


    case OUT_SIZE_REG:
        if (!is_non_zero) {
            REPORT_DEBUG("INVALID ARGUMENT: out_size_reg cannot be 0 (w)");
            bchan.resp = axi4_lite::AXI_SLVERR_RESPONSE;
        } else if (wdata > PLM_SIZE) {
            REPORT_DEBUG("INVALID ARGUMENT: out_size_reg cannot be larger than PLM_SIZE (w)");
            bchan.resp = axi4_lite::AXI_SLVERR_RESPONSE;
        } else {
            out_size_sig = wdata;
            REPORT_DEBUG("OUT_SIZE_REG: 0x%08x (w)", wdata);
        }
        break;


    // -- Status and command registers

    case CMD_REG:
        if (wdata == ACCELERATOR_CMD_CLEARIRQ) {
            REPORT_DEBUG("CMD_REG:  0x%08x (w)", wdata);
            this->cmd_sig.write(wdata);
        } else if (wdata == ACCELERATOR_CMD_GO) {
            REPORT_DEBUG("CMD_REG:  0x%08x (w)", wdata);
            this->cmd_sig.write(wdata);
        } else {
            REPORT_DEBUG("INVALID COMMAND (w)");
            bchan.resp = axi4_lite::AXI_SLVERR_RESPONSE;
        }
        break;

    default:
        REPORT_DEBUG("REGISTER NOT MAPPED (w)");
        bchan.resp = axi4_lite::AXI_SLVERR_RESPONSE;
        break;
    }
}

bool fc_layer::can_single_write(void) const
{
    return true;
}

#endif /* __FC_LAYER_REGS_HPP__ */
