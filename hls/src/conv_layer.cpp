/* Copyright 2021 Columbia University SLD Group */

// -- Module definition

#include "conv_layer.hpp"

// -- Functions (regs)

#include "conv_layer_regs.hpp"

// -- Functions (utils)

#include "conv_layer_utils.hpp"

// -- Functions (kernel)

#include "conv_layer_functions.hpp"

// -- Processes

void conv_layer::load_input(void)
{
    bool pingpong = true;

    uint32_t in_base_addr;
    uint32_t in_w_base_addr;
    uint32_t out_b_base_addr;

    uint32_t num_cols;
    uint32_t num_rows;
    uint32_t src_chans;
    uint32_t dst_chans;

    // Reset

    {
        HLS_DEFINE_PROTOCOL("load-input-reset");

        dma_initiator.archan->reset_put();
        dma_initiator.rchan->reset_get();

        input_ready.req.reset_req();

        input_0.port1.reset();
        input_1.port1.reset();
        input_w_0.port1.reset();
        input_w_1.port1.reset();
        output_b.port1.reset();

        wait();
    }

    while (true)
    {

        // Config

        {
            HLS_DEFINE_PROTOCOL("load-input-config.");

            wait_for_config(); // memory-mapped regs

            in_base_addr    = in_base_addr_sig.read();
            in_w_base_addr  = in_w_base_addr_sig.read();
            out_b_base_addr = out_b_base_addr_sig.read();
            num_cols        = num_cols_sig.read();
            num_rows        = num_rows_sig.read();
            src_chans       = src_chans_sig.read();
            dst_chans       = dst_chans_sig.read();
        }

        // Load

        {
            int in_index;
            const int in_len = num_cols * num_rows;

            int in_w_index;
            const int in_w_len = 9;

            // -- Load biases
            load_data<PLM_BIAS, FPDATA_WL  > (output_b, out_b_base_addr, dst_chans);

            for (int dst_chan = 0; dst_chan < dst_chans; dst_chan++)
            {
                for (int src_chan = 0; src_chan < src_chans; src_chan++)
                {
                    irq_load();//START LOAD

                    // Update input offset
                    in_index = in_base_addr + (src_chan * num_cols * num_rows << 2);

                    // Update weights offset
                    in_w_index = in_w_base_addr + (9 * (src_chan * dst_chans + dst_chan) << 2);

                    if (pingpong)
                    {
                        // -- Load inputs
                        load_data< PLM_INPUT, FPDATA_WL >(input_0, in_index, in_len);

                        // -- Load weigths
                        load_data< PLM_WEIGHTS, W_FPDATA_WL >(input_w_0, in_w_index, in_w_len);
                    }
                    else
                    {
                        // -- Load inputs
                        load_data< PLM_INPUT, FPDATA_WL >(input_1, in_index, in_len);

                        // -- Load weights
                        load_data< PLM_WEIGHTS, W_FPDATA_WL >(input_w_1, in_w_index, in_w_len);
                    }

                    // Flip input pingpong variable
                    pingpong = !pingpong;

                    irq_load();//FINISH LOAD

                    // Handshaking w/ compute
                    load_compute_handshake();
                }
            }

            // Conclude

            {
                HLS_DEFINE_PROTOCOL("load-input-done");

                process_done();
            }
        }
    }
}

void conv_layer::compute_kernel(void)
{
    bool pingpong = true;
    bool out_pingpong = true;

    bool do_pad;
    bool do_pool;

    uint32_t num_cols;
    uint32_t num_rows;
    uint32_t src_chans;
    uint32_t dst_chans;
    uint32_t pool_size;
    uint32_t pool_stride;

    // Reset

    {
        HLS_DEFINE_PROTOCOL("compute-kernel-reset");

        input_ready.ack.reset_ack();
        output_ready.req.reset_req();

        input_0.port2.reset();
        input_1.port2.reset();
        input_w_0.port2.reset();
        input_w_1.port2.reset();
        output_b.port2.reset();
        output_0.port1.reset();
        output_1.port1.reset();
        output_0.port2.reset();
        output_1.port2.reset();

        wait();
    }

    while (true)
    {
        // Config

        {
            HLS_DEFINE_PROTOCOL("compute-kernel-config");

            wait_for_config(); // memory-mapped regs

            do_pad          = do_pad_sig.read();
            do_pool         = do_pool_sig.read();
            num_cols        = num_cols_sig.read();
            num_rows        = num_rows_sig.read();
            src_chans       = src_chans_sig.read();
            dst_chans       = dst_chans_sig.read();
            pool_size       = pool_size_sig.read();
            pool_stride     = pool_stride_sig.read();
        }

        // Compute

        for (int dst_chan = 0; dst_chan < dst_chans; dst_chan++)
        {
            for (int src_chan = 0; src_chan < src_chans; src_chan++)
            {
                // Handshake with load_input
                compute_load_handshake();
                irq_compute();//START COMPUTE

                // Execute the computational kernel
                convolution_compute(num_cols, num_rows,
                                    src_chans, dst_chans, src_chan, dst_chan,
                                    do_pool, do_pad, pool_size, pool_stride,
                                    pingpong, out_pingpong);

                // Update pingpong var
                pingpong = !pingpong;
                irq_compute();//FINISH COMPUTE
            }

            // Flip out_pingpong variable
            out_pingpong = !out_pingpong;

            // Handshaking w/ store process
            compute_store_handshake();
        }

        // Conclude

        {
            HLS_DEFINE_PROTOCOL("compute-kernel-done");

            process_done();
        }
    }
}

void conv_layer::store_output(void)
{
    bool out_pingpong = true;

    uint32_t out_base_addr;
    uint32_t out_size;

    uint32_t dst_chans;
    uint32_t pool_size;

    // Reset

    {
        HLS_DEFINE_PROTOCOL("store-output-reset");

        dma_initiator.awchan->reset_put();
        dma_initiator.wchan->reset_put();
        dma_initiator.bchan->reset_get();

        output_ready.ack.reset_ack();

        irq.write(false);

        output_0.port3.reset();
        output_1.port3.reset();

        wait();
    }

    while (true)
    {

        // Config

        {
            HLS_DEFINE_PROTOCOL("store-output-config");

            wait_for_config(); // memory-mapped regs

            out_base_addr   = out_base_addr_sig.read();
            out_size        = out_size_sig.read();
            dst_chans       = dst_chans_sig.read();
            pool_size       = pool_size_sig.read();
        }

        // Store

        int out_index;
        const int out_len = round_up(pool_size * pool_size, DMA_ADJ);

        for (int dst_chan = 0; dst_chan < dst_chans; dst_chan++)
        {
            // Update input offset
            const int out_index_offset = dst_chan * pool_size * pool_size << 2;

            out_index = out_base_addr + out_index_offset;

            // Handshaking w/ compute
            store_compute_handshake();
            irq_store();//START STORE

            if (out_pingpong)
            {
                // -- Store outputs
                store_data<PLM_OUTPUT > (output_0, out_index, out_len);
            }
            else
            {
                // -- Store outputs
                store_data<PLM_OUTPUT > (output_1, out_index, out_len);
            }

            // Flip out_pingpong variable
            out_pingpong = !out_pingpong;
            irq_store();//FINISH STORE
        }

        // Conclude

        {
            HLS_DEFINE_PROTOCOL("store-output-done");

            // Set IRQ
            irq.write(true);

            // Wait for IRQ clear
            clear_irq();

            process_done();
        }
    }
}
