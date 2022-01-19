/* Copyright 2021 Columbia University SLD Group */

#include "utils.hpp"
#include "driver.hpp"
#include "system.hpp"

#include "register_map.hpp"

// -- Processes

void driver_t::driver_thread(void)
{
    // Reset

    {
        reg_initiator.awchan->reset_put();
        reg_initiator.archan->reset_put();
        reg_initiator.wchan->reset_put();
        reg_initiator.rchan->reset_get();
        reg_initiator.bchan->reset_get();

        wait();
    }

    const reg_initiator_t::addr_t addr_cmd = CMD_REG;
    const reg_initiator_t::data_t clearirq = ACCELERATOR_CMD_CLEARIRQ;
    const reg_initiator_t::data_t go = ACCELERATOR_CMD_GO;
    const reg_initiator_t::data_t done = ACCELERATOR_STATUS_DONE;

    REPORT_INFO("=== TEST BEGIN ===");

    // Configure

    {
        system_ref->load_memory();

#if defined(TARGET_LAYER_5) || defined(TARGET_LAYER_6)
        // FULLY CONNECTED
        system_ref->load_regs(true);
#else
        // CONVOLUTION
        system_ref->load_regs(false);
#endif // FULLY_CONNECTED

        assert(do_write(addr_cmd, go));

        wait();
    }

    // Computation

    {
        double e_begin_time = sc_time_stamp().value() / 1000.0;
        reg_initiator_t::data_t rdata;
        REPORT_TIME(e_begin_time, "ns BEGIN - ACC");
        double load_begin_time, load_end_time;
        int load_count = 0;
        double compute_begin_time, compute_end_time;
        int compute_count = 0;
        double store_begin_time, store_end_time;
        int store_count = 0;

        // Wait for acc to finish
        do {
            if(load_irq.read()){
                if(load_count == 0){
                    load_begin_time = sc_time_stamp().value() / 1000.0;
                    load_count++;
                }
                else if(load_count == 1){
                    load_end_time = sc_time_stamp().value() / 1000.0;
                    load_count++;
                }
            }
            if(compute_irq.read()){
                if(compute_count == 0){
                    compute_begin_time = sc_time_stamp().value() / 1000.0;
                    compute_count++;
                }
                else if(compute_count == 1){
                    compute_end_time = sc_time_stamp().value() / 1000.0;
                    compute_count++;
                }
            }
            if(store_irq.read()){
                if(store_count == 0){
                    store_begin_time = sc_time_stamp().value() / 1000.0;
                    store_count++;
                }
                else if(store_count == 1){
                    store_end_time = sc_time_stamp().value() / 1000.0;
                    store_count++;
                }
            }
            wait();
        }
        while (!irq.read());

        assert(do_read(addr_cmd, rdata));
        assert(rdata == done);
        do_write(addr_cmd, clearirq);

        double e_end_time = sc_time_stamp().value() / 1000.0;
        REPORT_TIME(e_end_time, "ns END - ACC");

        double load_time = load_end_time - load_begin_time;
        REPORT_TIME(load_time, "ns LOAD - TIME");

        double compute_time = compute_end_time - compute_begin_time;
        REPORT_TIME(compute_time, "ns COMPUTE - TIME");

        double store_time = store_end_time - store_begin_time;
        REPORT_TIME(store_time, "ns STORE - TIME");

        system_ref->dump_memory();
        system_ref->validate();

        system_ref->clean_up();
    }

    // Conclude

    {
        sc_stop();
    }
}

// -- Functions (read)

bool driver_t::do_read(reg_initiator_t::addr_t addr, reg_initiator_t::data_t &data)
{
    data = 0;
    reg_initiator_t::archan_t archan;
    reg_initiator_t::rchan_t rchan;

    archan.addr = addr;

    while (!reg_initiator.archan->nb_put(archan)) { wait(); }

    wait(); // To avoid error nb_get called multiple times

    while (!reg_initiator.rchan->nb_get(rchan)) { wait(); }

    data = rchan.data;

    return (rchan.resp == axi4_lite::AXI_OK_RESPONSE);
}

// -- Functions (write)

bool driver_t::do_write(reg_initiator_t::addr_t addr, reg_initiator_t::data_t data)
{
    reg_initiator_t::awchan_t awchan;
    reg_initiator_t::wchan_t wchan;
    reg_initiator_t::bchan_t bchan;

    awchan.addr = addr;

    while (!reg_initiator.awchan->nb_put(awchan)) { wait(); }

    wchan.data = data;

    while (!reg_initiator.wchan->nb_put(wchan)) { wait(); }

    do
    {
        wait();

    } while (!reg_initiator.bchan->nb_get(bchan));

    return (bchan.resp == axi4_lite::AXI_OK_RESPONSE);
}
