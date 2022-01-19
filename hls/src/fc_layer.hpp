/* Copyright 2021 Columbia University SLD Group */

#include "utils.hpp"
#include "handshake.hpp"
#include "axi_traits.hpp"
#include "datatypes.hpp"

#include "fc_layer_directives.hpp"

#include "plm_data32_1w1r.hpp"


#ifndef __FC_LAYER_HPP__
#define __FC_LAYER_HPP__

class fc_layer
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

    // Interrupt signal
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

    SC_HAS_PROCESS(fc_layer);
    fc_layer(sc_module_name name)
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
        , in_base_addr_sig("in_base_addr_sig")
        , in_w_base_addr_sig("in_w_base_addr_sig")
        , out_b_base_addr_sig("out_b_base_addr_sig")
        , out_base_addr_sig("out_base_addr_sig")
        , num_w_cols_sig("num_w_cols_sig")
        , num_w_rows_sig("num_w_rows_sig")
        , in_size_sig("in_size_sig")
        , in_w_size_sig("in_w_size_sig")
        , out_size_sig("out_size_sig")
        , cmd_sig("cmd_sig")
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
        input.clk(this->clk);
        input_w_0.clk(this->clk);
        input_w_1.clk(this->clk);
        output_b.clk(this->clk);
        output.clk(this->clk);
    }

    // -- Processes

    // Load input from memory
    void load_input(void);

    // Perform the computation
    void compute_kernel(void);

    // Store output in memory
    void store_output(void);

    // -- Functions (kernel)
    void fcrelu_accumulate_signal(uint32_t w_row, uint32_t num_w_cols, bool pingpong);

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
    template<class T>
    void load_data(plm_data32_1w1r_t<T, PLM_SIZE> &array,
                   uint32_t mem_base, uint32_t burst_size);

    // To store the output data
    void store_data(plm_data32_1w1r_t<FPDATA_WORD, PLM_SIZE> &array,
                    uint32_t mem_base, uint32_t burst_size);

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

    plm_data32_1w1r_t<FPDATA_WORD, PLM_SIZE> input;
    plm_data32_1w1r_t<W_FPDATA_WORD, PLM_SIZE> input_w_0;
    plm_data32_1w1r_t<W_FPDATA_WORD, PLM_SIZE> input_w_1;
    plm_data32_1w1r_t<FPDATA_WORD, PLM_SIZE> output_b;

    // -- Private memories (output)

    plm_data32_1w1r_t<FPDATA_WORD, PLM_SIZE> output;

    // -- Configuration registers

    // Pointers to locations in memory
    sc_signal<uint32_t> in_base_addr_sig;
    sc_signal<uint32_t> in_w_base_addr_sig;
    sc_signal<uint32_t> out_b_base_addr_sig;
    sc_signal<uint32_t> out_base_addr_sig;

    // Offset for loading the input
    sc_signal<uint32_t> num_w_cols_sig;
    sc_signal<uint32_t> num_w_rows_sig;
    sc_signal<uint32_t> in_size_sig;
    sc_signal<uint32_t> in_w_size_sig;
    sc_signal<uint32_t> out_size_sig;
    sc_signal<uint32_t> cmd_sig;
};

#endif /* __FC_LAYER_HPP__ */
