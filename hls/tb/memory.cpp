/* Copyright 2021 Columbia University SLD Group */

#include "utils.hpp"

#include "memory.hpp"

// -- Functions (read)

void memory_t::reset_multi_read()
{
    // Nothing to do
}

bool memory_t::do_read(dma_target_t::data_t &val, unsigned index)
{
    unsigned b_index = index / 4;
    dma_delay();//DO NOT MODIFY

    // TODO: to be modified if you change DATA_WIDTH
    val.range(31, 0) = data[b_index].to_uint();

    return true;
}

void memory_t::multi_read(dma_target_t::archan_t &archan,
                          dma_target_t::rchan_t &rchan,
                          bool &burst_done)
{
    if (roffset == 0)
    { rlen = archan.len; }

    bool ret = do_read(rchan.data, archan.addr + roffset);

    if (!ret)
    {
        rchan.resp = axi4::AXI_SLVERR_RESPONSE;
    }

    if (rlen == 0)
    {
        burst_done = true;
        roffset = 0;
    }
    else
    {
        // TODO: to be modified if you change DATA_WIDTH
        roffset += 4;
        rlen -= 1;
    }
}

bool memory_t::can_multi_read() const
{
    return true;
}

// -- Functions (write)

void memory_t::reset_multi_write()
{
    // Nothing to do
}

bool memory_t::do_write(dma_target_t::data_t val, unsigned index)
{
    unsigned b_index = index / 4;
    dma_delay();//DO NOT MODIFY

    // TODO: to be modified if you change DATA_WIDTH
    data[b_index] = val.range(31, 0).to_uint();

    return true;
}

void memory_t::multi_write(dma_target_t::awchan_t &awchan,
                           const dma_target_t::wchan_t &wchan,
                           dma_target_t::bchan_t &bchan,
                           bool &burst_done)
{
    if (woffset == 0)
    { wlen = awchan.len; }

    if (!do_write(wchan.data, awchan.addr + woffset))
    {
        bchan.resp = axi4::AXI_SLVERR_RESPONSE;
    }

    if (wlen == 0)
    {
        burst_done = true;
        woffset = 0;
    }
    else
    {
        // TODO: DATA_WIDTH of the bust != from 32 bits.
        woffset += 4;
        wlen -= 1;
    }
}

bool memory_t::can_multi_write() const
{
    return true;
}
