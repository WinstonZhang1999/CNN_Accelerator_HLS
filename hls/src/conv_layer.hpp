/* Copyright 2021 Columbia University SLD Group */

#include "utils.hpp"
#include "handshake.hpp"
#include "axi_traits.hpp"
#include "datatypes.hpp"

#include "conv_layer_directives.hpp"

#include "plm_block_51200x32_1w1r.hpp"
#include "plm_block_12x32_1w1r.hpp"
#include "plm_block_512x32_1w1r.hpp"
#include "plm_block_51200x32_1w2r.hpp"

#ifndef __CONV_LAYER_HPP__
#define __CONV_LAYER_HPP__

#define PLM_INPUT plm_block_51200x32_1w1r_t<FPDATA_WORD, PLM_INPUT_SIZE>
#define PLM_WEIGHTS plm_block_12x32_1w1r_t<W_FPDATA_WORD, PLM_INPUT_WEIGHT_SIZE>
#define PLM_BIAS plm_block_512x32_1w1r_t<FPDATA_WORD, PLM_OUTPUT_BIAS_SIZE>
#define PLM_OUTPUT plm_block_51200x32_1w2r_t<FPDATA_WORD, PLM_OUTPUT_SIZE>

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
#define round_down(x, y) ((x) & ~__round_mask(x, y))

class conv_layer
    : public sc_module
    , public axi4_lite::axi4_lite_single_rw_if<reg_if_traits>
{

    public:

        // -- Input ports

        // Clock signal
        sc_in<bool> clk;

        // Reset signal
        sc_in<bool> resetn;

        // -- Output ports

        // Interrupt signals
        sc_out<bool> irq;
        sc_out<bool> load_irq;
        sc_out<bool> compute_irq;
        sc_out<bool> store_irq;

        // -- Handshakes

        // The input is loaded
        handshake_t input_ready;

        // The output is ready
        handshake_t output_ready;

        // -- AXI (see axi_traits.hpp)

        // Target for the regs
        reg_target_t reg_target;

        // Adaptor for the target
        reg_adaptor_t srw_adaptor;

        // Initiator for dma r/w transactions
        dma_initiator_t dma_initiator;

        // -- Constructor

        SC_HAS_PROCESS(conv_layer);
        conv_layer(sc_module_name name)
            : sc_module(name)
            , clk("clk")
            , resetn("resetn")
            , irq("irq")
            , load_irq("load_irq")
            , compute_irq("compute_irq")
            , store_irq("store_irq")
            , input_ready("input_ready")
            , output_ready("output_ready")
            , reg_target("reg_target")
            , srw_adaptor("srw_adaptor")
            , dma_initiator("dma_initiator")
            , cmd_sig("cmd_sig")
            , in_base_addr_sig("in_base_addr_sig")
            , in_w_base_addr_sig("in_w_base_addr_sig")
            , out_b_base_addr_sig("out_b_base_addr_sig")
            , out_base_addr_sig("out_base_addr_sig")
            , in_size_sig("in_size_sig")
            , in_w_size_sig("in_w_size_sig")
            , out_size_sig("out_size_sig")
            , out_b_size_sig("out_b_size_sig")
            , num_cols_sig("num_cols_sig")
            , num_rows_sig("num_rows_sig")
            , src_chans_sig("src_chans_sig")
            , dst_chans_sig("dst_chans_sig")
            , do_pool_sig("do_pool_sig")
            , do_pad_sig("do_pad_sig")
            , pool_size_sig("pool_size_sig")
            , pool_stride_sig("pool_stride_sig")
        {
            // Process to read the input data
            // Configure it to match dma traits
            SC_THREAD_CLOCK_RESET_TRAITS(load_input, clk, resetn, dma_if_traits::put_get_traits);

            // Process to encrypt the input data
            SC_CTHREAD(compute_kernel, clk.pos());
            reset_signal_is(resetn, false);

            // Process to store the output data
            SC_THREAD_CLOCK_RESET_TRAITS(store_output, clk, resetn, dma_if_traits::put_get_traits);

            // Bind the clock and reset signals
            reg_target.clk_rst(clk, resetn);
            dma_initiator.clk_rst(clk, resetn);

            // Bind the adaptor to the target
            reg_target.target_port(srw_adaptor);
            srw_adaptor.target_port(*this);

            // Map arrays to registers (flatten) or memories
            input_0.clk(this->clk);
            input_1.clk(this->clk);
            input_w_0.clk(this->clk);
            input_w_1.clk(this->clk);
            output_b.clk(this->clk);
            output_0.clk(this->clk);
            output_1.clk(this->clk);
        }

        // -- Processes

        // Load input from memory
        void load_input(void);

        // Perform the computation
        void compute_kernel(void);

        // Store output in memory
        void store_output(void);

        // -- Functions (kernel)
        void convolution_compute(uint32_t num_cols, uint32_t num_rows,
                                 uint32_t src_chans, uint32_t dst_chans,
                                 int src_chan, int dst_chan,
                                 bool do_pool, bool do_pad,
                                 uint32_t pool_size, uint32_t pool_stride,
                                 bool pingpong, bool out_pingpong);

        // -- Functions (utils)

        // Wait for configuration
        inline void wait_for_config(void);

        // Wait for IRQ to be cleard
        inline void clear_irq(void);

        //send IRQ from processes
        inline void irq_load(void);
        inline void irq_compute(void);
        inline void irq_store(void);

        // Process has complete. Wait for reset.
        inline void process_done(void);

        // Handshake callable by load_input
        inline void load_compute_handshake(void);

        // Handshake callable by compute_kernel
        inline void compute_load_handshake(void);

        // Handshake callable by compute_kernel
        inline void compute_store_handshake(void);

        // Handshake callable by store_output
        inline void store_compute_handshake(void);

        // To read the input data
        template<class T, size_t WL>
        void load_data(T &array, uint32_t base_addr, uint32_t size);

        // To store the output data
        template<class T>
        void store_data(T &array, uint32_t base_addr, uint32_t size);

        // -- Functions (registers)

        // To reset the read side
        virtual void reset_single_read(void);

        // To read a register
        virtual void single_read(reg_target_t::archan_t &archan,
                                 reg_target_t::rchan_t &rchan);

        // To know if we support single read
        virtual bool can_single_read(void) const;

        // To reset the write side
        virtual void reset_single_write(void);

        // To write a register
        virtual void single_write(reg_target_t::awchan_t &awchan,
                                  const reg_target_t::wchan_t &wchan,
                                  reg_target_t::bchan_t &bchan);

        // To know if we support single write
        virtual bool can_single_write(void) const;

        // -- Private memories (input)

        PLM_INPUT input_0;
        PLM_INPUT input_1;
        PLM_WEIGHTS input_w_0;
        PLM_WEIGHTS input_w_1;
        PLM_BIAS output_b;

        // -- Private memories (output)

        PLM_OUTPUT output_0;
        PLM_OUTPUT output_1;

        // -- Configuration registers

        sc_signal<uint32_t> cmd_sig;

        sc_signal<uint32_t> in_base_addr_sig;
        sc_signal<uint32_t> in_w_base_addr_sig;
        sc_signal<uint32_t> out_b_base_addr_sig;
        sc_signal<uint32_t> out_base_addr_sig;

        sc_signal<uint32_t> in_size_sig;
        sc_signal<uint32_t> in_w_size_sig;
        sc_signal<uint32_t> out_size_sig;
        sc_signal<uint32_t> out_b_size_sig;

        sc_signal<uint32_t> num_cols_sig;
        sc_signal<uint32_t> num_rows_sig;
        sc_signal<uint32_t> src_chans_sig;
        sc_signal<uint32_t> dst_chans_sig;
        sc_signal<bool> do_pool_sig;
        sc_signal<bool> do_pad_sig;
        sc_signal<uint32_t> pool_size_sig;
        sc_signal<uint32_t> pool_stride_sig;
};

#endif /* __CONV_LAYER_HPP__ */
