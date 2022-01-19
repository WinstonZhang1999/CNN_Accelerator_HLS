/* Copyright 2021 Columbia University SLD Group */

#include "register_map.hpp"

#ifndef __CONV_LAYER_UTILS_HPP__
#define __CONV_LAYER_UTILS_HPP__

inline void conv_layer::load_compute_handshake(void)
{
    {
        HLS_DEFINE_PROTOCOL("load-compute-handshake");

        input_ready.req.req();
    }
}

inline void conv_layer::compute_load_handshake(void)
{
    {
        HLS_DEFINE_PROTOCOL("compute-load-handshake");

        input_ready.ack.ack();
    }
}

inline void conv_layer::compute_store_handshake(void)
{
    {
        HLS_DEFINE_PROTOCOL("compute-store-handshake");

        output_ready.req.req();
    }
}

inline void conv_layer::store_compute_handshake(void)
{
    {
        HLS_DEFINE_PROTOCOL("store-compute-handshake");

        output_ready.ack.ack();
    }
}

void conv_layer::wait_for_config(void)
{
    bool go = false;

    do
    {
        go  = cmd_sig.read()             == ACCELERATOR_CMD_GO;
        go &= num_cols_sig.read()      != 0;
        go &= num_rows_sig.read()      != 0;
        go &= in_size_sig.read()         != 0;
        go &= in_w_size_sig.read()       != 0;
        go &= out_size_sig.read()        != 0;
        wait();
    } while (!go);

}

void conv_layer::clear_irq(void)
{
    process_done();

    irq.write(false);
    wait();
}

void conv_layer::irq_load(void)
{
    HLS_DEFINE_PROTOCOL("load-irq");

    load_irq.write(true);
    wait();
    load_irq.write(false);
}

void conv_layer::irq_compute(void)
{
    HLS_DEFINE_PROTOCOL("compute-irq");

    compute_irq.write(true);
    wait();
    compute_irq.write(false);
}

void conv_layer::irq_store(void)
{
    HLS_DEFINE_PROTOCOL("store-irq");

    store_irq.write(true);
    wait();
    store_irq.write(false);
}

void conv_layer::process_done(void)
{
    do { wait(); }
    while (cmd_sig.read() != ACCELERATOR_CMD_CLEARIRQ);
}

#define AXI4_BURST_MAX_LEN 16

template<class T, size_t WL>
void conv_layer::load_data(T &array, uint32_t base_addr, uint32_t size)
{
    uint32_t index = 0;
    uint32_t beat_id = 1;
    uint32_t mem_index = 0;
    uint32_t mem_off = base_addr;
    dma_initiator_t::archan_t archan;

    archan.burst = axi4::AXI_INCR_ADDR_BURST;

    for (index = size; index > 0; )
    {
        HLS_DEFINE_PROTOCOL("load-data-protocol");

        uint32_t j = 0;
        uint32_t beats = (index < 16) ? index : 16;

        archan.tid = beat_id;
        archan.len = beats - 1;
        archan.addr = mem_off;

        while (!dma_initiator.archan->nb_put(archan)) { wait(); }

        for (; j < beats; ++j)
        {
            dma_initiator_t::rchan_t rchan;

            while (!dma_initiator.rchan->nb_get(rchan)) { wait(); }

            // TODO: handle different DATA WIDTH
            array.port1[0][mem_index++] = rchan.data;

            wait();
        }

        // TODO: handle different DATA WIDTH
        mem_off += (beats << 2);
        index -= beats;
        ++beat_id;

        wait();
    }
}

template<class T>
void conv_layer::store_data(T &array, uint32_t base_addr, uint32_t size)
{
    uint32_t index = 0;
    uint32_t beat_id = 1;
    uint32_t mem_index = 0;
    uint32_t mem_off = base_addr;
    dma_initiator_t::awchan_t awchan;

    awchan.burst = axi4::AXI_INCR_ADDR_BURST;

    for (index = size; index > 0; )
    {
        HLS_DEFINE_PROTOCOL("store-data-protocol");

        uint32_t j = 0;
        uint32_t beats = (index < 16) ? index : 16;

        awchan.tid = beat_id;
        awchan.len = beats - 1;
        awchan.addr = mem_off;

        while (!dma_initiator.awchan->nb_put(awchan)) { wait(); }

        for (; j < beats; ++j)
        {
            dma_initiator_t::wchan_t wchan;

            wait();

            // TODO: handle different DATA WIDTH
            wchan.data = array.port3[0][mem_index];

            wchan.tid = awchan.tid;
            wchan.last = (j == beats - 1);

            while (!dma_initiator.wchan->nb_put(wchan)) { wait(); }

            mem_index++;

            wait();
        }

        dma_initiator_t::bchan_t bchan;

        while (!dma_initiator.bchan->nb_get(bchan)) { wait(); }

        // TODO: handle different DATA WIDTH
        mem_off += (beats << 2);
        index -= beats;
        ++beat_id;

        wait();
    }
}

#endif // __CONV_LAYER_UTILS_HPP__
