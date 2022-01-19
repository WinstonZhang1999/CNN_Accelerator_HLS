/* Copyright 2021 Columbia University SLD Group */

#ifndef __SYSTEM_HPP__
#define __SYSTEM_HPP__

#include <stdlib.h>
#include <string.h>

#include "utils.hpp"

#include "driver.hpp"
#include "memory.hpp"

#include "axi_traits.hpp"

#if TEST_FULLY_CONNECTED
#include "fc_layer_wrap.h"
#elif TEST_CONVOLUTION
#include "conv_layer_wrap.h"
#else
#error Unimplemented layer type
#endif

class system_t : public sc_module
{
    public:

        // -- Modules

        // Memory's model
        memory_t *memory;

        // Driver's model
        driver_t *driver;

        // Accelerator's wrapper
#if TEST_FULLY_CONNECTED
        fc_layer_wrapper *acc;
#elif TEST_CONVOLUTION
        conv_layer_wrapper *acc;
#endif

        // -- Input ports

        // Clock signal
        sc_in<bool> clk;

        // Reset signal
        sc_in<bool> resetn;

        // -- Output ports

        // Interrupt signals
        sc_signal<bool> irq;
        sc_signal<bool> load_irq;
        sc_signal<bool> compute_irq;
        sc_signal<bool> store_irq;

        // -- Internal Channels

        // Channels to handle the DMA transactions
        axi4::axi4_channel<dma_if_traits> dma_channel;

        // Channels to handle the write/read reqs for the registers
        axi4_lite::axi4_lite_channel<reg_if_traits> reg_channel;

        // -- Constructor

        SC_HAS_PROCESS(system_t);
        system_t(sc_module_name name,
                 std::string model_path,
                 std::string image_path)
            : sc_module(name)
            , clk("clk")
            , resetn("resetn")
            , irq("irq")
            , load_irq("load_irq")
            , compute_irq("compute_irq")
            , store_irq("store_irq")
            , reg_channel("reg_channel")
            , dma_channel("dma_channel")
            , model_path(model_path)
            , image_path(image_path)
        {
            // Instantiate the driver's model
            driver = new driver_t("driver");

            // Instantiate the memory's model
            memory = new memory_t("memory");

            // Instantiate the accelerator
#if TEST_FULLY_CONNECTED
            acc = new fc_layer_wrapper("fc_layer");
#elif TEST_CONVOLUTION
            acc = new conv_layer_wrapper("conv_layer");
#endif // TEST_FULLY_CONNECTED

            // Binding the driver's model
            driver->clk(clk);
            driver->resetn(resetn);
            driver->irq(irq);
            driver->load_irq(load_irq);
            driver->compute_irq(compute_irq);
            driver->store_irq(store_irq);
            driver->reg_initiator(reg_channel);

            // Binding the memory's model
            memory->clk(clk);
            memory->resetn(resetn);
            dma_channel(memory->dma_target);

            // Binding the acceleratorr
            acc->clk(clk);
            acc->resetn(resetn);
            acc->irq(irq);
            acc->load_irq(load_irq);
            acc->compute_irq(compute_irq);
            acc->store_irq(store_irq);
            reg_channel(acc->reg_target);
            acc->dma_initiator(dma_channel);

            driver->system_ref = this;
        }

        // -- Functions

        // Load the value of the registers
        void load_regs(bool fully_connected);

        // Load the input values in memory
        void load_memory(void);
        void setup_memory(int target_layer, bool fully_connected);

        // Read the output values from memory
        void dump_memory(void);

        // Verify that the results are correct
        void validate(void);

        // Optionally free resources (arrays)
        void clean_up(void);

        // Load a float vector in memory
        template<typename T, size_t FP_WL>
        void load_fpdata(float *in,
                         uint32_t base_addr,
                         uint32_t size,
                         uint32_t actual_size);

        // Convert output from memory back to float
        void convert_output(int target_layer);

        // Call programmer's view on one layer of the CNN
        void run_pv(int layer, bool fully_connected);

        // Set configuration parameters from CNN model
        void set_configuration_param(int target_layer, bool fully_connected);

        // Rearrange weights in memory for accelerated partial simulation
        void move_weights(int target_layer, bool fully_connected);

        // -- Private data

        // I/O Dimensions
        uint32_t actual_w_size;
        uint32_t actual_i_size;
        uint32_t actual_b_size;
        uint32_t actual_o_size;

        uint32_t w_size;
        uint32_t i_size;
        uint32_t b_size;
        uint32_t o_size;

        // Configuration parameters
        uint32_t w_cols;
        uint32_t w_rows;
        uint32_t cols;
        uint32_t rows;
        uint32_t src_chans;
        uint32_t dst_chans;
        bool do_pool;
        bool do_pad;
        uint32_t pool_size;
        uint32_t pool_stride;

        uint32_t reduced_chans;

        // Base addresses
        uint32_t w_addr;
        uint32_t i_addr;
        uint32_t b_addr;
        uint32_t o_addr;

        // Path for the model
        std::string model_path;

        // Image to be classified
        std::string image_path;
};

#endif /* __SYSTEM_HPP__ */
