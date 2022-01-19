/* Copyright 2021 Columbia University SLD Group */

// -- Module definition

#include "fc_layer.hpp"

// -- Functions (regs)

#include "fc_layer_regs.hpp"

// -- Functions (utils)

#include "fc_layer_utils.hpp"

// -- Functions (kernel)

#include "fc_layer_functions.hpp"

// -- Processes

void fc_layer::load_input(void)
{
    bool pingpong; uint32_t index;
    uint32_t in_base_addr, in_size;
    uint32_t out_b_base_addr, out_size;
    uint32_t in_w_base_addr, num_w_cols, num_w_rows;

    // Reset

    {
        HLS_DEFINE_PROTOCOL("load-input-reset");

        dma_initiator.archan->reset_put();
        dma_initiator.rchan->reset_get();

        input_ready.req.reset_req();

        input.port1.reset();
        input_w_0.port1.reset();
        input_w_1.port1.reset();
        output_b.port1.reset();

        wait();
    }

    while (true) {
        // Config
        {
            HLS_DEFINE_PROTOCOL("load-input-config");

            wait_for_config(); // memory-mapped regs

            in_base_addr    = in_base_addr_sig.read();
            in_w_base_addr  = in_w_base_addr_sig.read();
            out_b_base_addr = out_b_base_addr_sig.read();
            num_w_cols      = num_w_cols_sig.read();
            num_w_rows      = num_w_rows_sig.read();
            in_size         = in_size_sig.read();
            out_size        = out_size_sig.read();

            pingpong = false;

        }

        // Load

        {
            // -- Inputs
            load_data<FPDATA_WORD>(input, in_base_addr, in_size);

            // -- Biases
            load_data<FPDATA_WORD>(output_b, out_b_base_addr, out_size);

            // -- Weights
            index = in_w_base_addr;

            for (uint32_t j = 0; j < num_w_rows; ++j)
            {
                //HLS_UNROLL_LOOP(CONSERVATIVE, num_w_rows-1, "loop");
                irq_load();//START LOAD

                if (pingpong)
                    load_data<W_FPDATA_WORD>(input_w_0, index, num_w_cols);
                else // !pingpong
                    load_data<W_FPDATA_WORD>(input_w_1, index, num_w_cols);

                irq_load();//FINISH LOAD
                // Handshake with compute
                load_compute_handshake();

                // Update pingpong var
                pingpong = !pingpong;

                // Update mem address
                index += (num_w_cols) << 2;
            }
        }

        // Conclude

        {
            HLS_DEFINE_PROTOCOL("load-input-done");

            process_done();
        }
    }
}

void fc_layer::compute_kernel(void)
{
    bool pingpong;
    uint32_t num_w_rows;
    uint32_t num_w_cols;

    // Reset

    {
        HLS_DEFINE_PROTOCOL("compute-kernel-reset");

        input_ready.ack.reset_ack();
        output_ready.req.reset_req();

        input.port2.reset();
        input_w_0.port2.reset();
        input_w_1.port2.reset();
        output_b.port2.reset();
        output.port1.reset();

        wait();
    }


    while (true) {
        // Config
        {
            HLS_DEFINE_PROTOCOL("compute-kernel-config");

            wait_for_config(); // memory-mapped regs

            num_w_rows = num_w_rows_sig.read();
            num_w_cols = num_w_cols_sig.read();

            pingpong = false;
        }

        // Compute

        {
            for (uint32_t j = 0; j < num_w_rows; j++)
            {
                // Handshake with load_input
                compute_load_handshake();
                irq_compute();//START COMPUTE

                // Execute the computational kernel
                fcrelu_accumulate_signal(j, num_w_cols, pingpong);

                // Update pingpong var
                pingpong = !pingpong;
                irq_compute();//FINISH COMPUTE
            }

            // Handshake with store_output
            compute_store_handshake();
        }

        // Conclude
        {
            HLS_DEFINE_PROTOCOL("compute-kernel-done");

            process_done();
        }
    }
}

void fc_layer::store_output(void)
{
    uint32_t out_base_addr, out_size;

    // Reset

    {
        HLS_DEFINE_PROTOCOL("store-output-reset");

        dma_initiator.awchan->reset_put();
        dma_initiator.wchan->reset_put();
        dma_initiator.bchan->reset_get();

        output_ready.ack.reset_ack();

        irq.write(false);

        output.port2.reset();

        wait();
    }

    while (true) {
        // Config
        {
            HLS_DEFINE_PROTOCOL("store-output-config");

            wait_for_config(); // memory-mapped regs

            out_base_addr = out_base_addr_sig.read();
            out_size  = out_size_sig.read();
        }

        // Store

        {
            // Handshake with compute
            store_compute_handshake();
            irq_store();//START STORE

            store_data(output, out_base_addr, out_size);
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