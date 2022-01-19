/* Copyright 2021 Columbia University SLD Group */

#ifndef __DRIVER_HPP__
#define __DRIVER_HPP__

// Forward declaration
class system_t;

#include "axi_traits.hpp"

class driver_t : public sc_module
{
    public:

        // -- Input ports

        // Clock signal
        sc_in<bool> clk;

        // Reset signal
        sc_in<bool> resetn;

        // Interrupt signals
        sc_in<bool> irq;
        sc_in<bool> load_irq;
        sc_in<bool> compute_irq;
        sc_in<bool> store_irq;

        // -- Communication channels

        // To program the accelerators
        reg_initiator_t reg_initiator;

        // -- References to other modules

        // To call the system functions
        system_t *system_ref;

        // -- Module constructor

        SC_HAS_PROCESS(driver_t);
        driver_t(sc_module_name name)
            : sc_module(name)
            , clk("clk")
            , resetn("resetn")
            , irq("irq")
            , load_irq("load_irq")
            , compute_irq("compute_irq")
            , store_irq("store_irq")
            , reg_initiator("reg_initiator")
        {
            // CTHREAD to handle the requests
            // Configure it to match register traits
            SC_THREAD_CLOCK_RESET_TRAITS(driver_thread, clk,
                resetn, reg_if_traits::put_get_traits);

            // Binding the clock and reset
            reg_initiator.clk_rst(clk, resetn);
        }

        // -- Processes

        // To handle read and write requests
        void driver_thread(void);

        // -- Functions (read)

        // To read a particular register
        bool do_read(reg_initiator_t::addr_t addr, reg_initiator_t::data_t &data);

        // -- Functions (write)

        // To write a particular register
        bool do_write(reg_initiator_t::addr_t addr, reg_initiator_t::data_t data);
};

#endif /* __DRIVER_HPP__ */
