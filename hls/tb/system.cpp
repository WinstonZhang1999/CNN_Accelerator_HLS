#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <stdio.h>

#include "mojo.h"
#include "dwarf.h"
#include "system.hpp"

#include "accelerated_sim.hpp"

std::ofstream ofs;

// Mojo network
mojo::network *cnn;

#include "datatypes.hpp"
#include "register_map.hpp"

#define IMAGE_SIZE 3 * 32 * 32

// -- Utilities

#define __round_mask(x, y) ((__typeof__(x))((y)-1))
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)

// float -> FPDATA -> sc_bv
template<typename T, size_t FP_WL>
void system_t::load_fpdata(float *in, uint32_t base_address,
                           uint32_t size, uint32_t actual_size)
{
    for (uint32_t i = 0; i < size / DMA_ADJ; i++)
    {
        sc_dt::sc_bv<DATA_WIDTH> data_dma;

        for (uint32_t word = 0; word < DMA_ADJ; word++)
        {
            const uint32_t index = DMA_ADJ * i + word;

            if (index >= actual_size)
            { break; }

            float data = in[index];
            T data_fp(data);
            sc_dt::sc_bv<FP_WL> data_bv;
            fp2bv<T, FP_WL, FP_WL>(data_fp, data_bv);
            data_dma.range((word + 1) * FP_WL - 1, word * FP_WL) = data_bv;
        }

        memory->data[base_address / DMA_ADJ + i] = data_dma;
    }
}

// sc_bv -> FPDATA -> float
void system_t::convert_output(int target_layer)
{
    float *output = cnn->layer_sets[target_layer]->node.x;

#ifdef RUN_SIM
    ofs.open("test.txt", std::ofstream::out);
#endif
#ifdef RUN_ACCELERATED_SIM
    ofs.open("accelerated_test.txt", std::ofstream::out);
#endif

    for (uint32_t i = 0; i < o_size / DMA_ADJ; i++)
    {
        sc_dt::sc_bv<DATA_WIDTH> data_dma = memory->data[o_addr / DMA_ADJ + i];

        for (uint32_t word = 0; word < DMA_ADJ; word++)
        {
            const uint32_t index = DMA_ADJ * i + word;

            if (index >= actual_o_size)
            { break; }

#ifdef FIXED
            sc_dt::sc_bv<FPDATA_WL> data_bv = data_dma.range((word + 1) * FPDATA_WL - 1,
                                              word * FPDATA_WL);
            FPDATA data_fp;
            bv2fp<FPDATA, FPDATA_WL, FPDATA_WL>(data_bv, data_fp);
#else
            sc_dt::sc_bv<32> data_bv = data_dma.range((word + 1) * 32 - 1,
                                              word * 32);
            FPDATA data_fp;
            bv2fp<FPDATA, 32, 32>(data_bv, data_fp);
#endif
            float data_float;
            fp2native(data_fp, data_float);
            output[index] = data_float;

#ifdef RUN_SIM
            // Print meaningful results for validation
            // Compare Fixed-point behavioral with RTL simulation
            if(target_layer == 5 || target_layer == 6)
            {
                uint32_t o_size = cnn->W[target_layer-1]->rows;
                float *output = cnn->layer_sets[target_layer]->node.x;
                if(index < o_size)
                    ofs << i << ": " << output[index] << std::endl;
            }
            else if(target_layer != 4)
            {
                uint32_t rows = cnn->layer_sets[target_layer+1]->node.rows;
                uint32_t cols = cnn->layer_sets[target_layer+1]->node.cols;
                uint32_t chans = cnn->layer_sets[target_layer]->node.chans;
                uint32_t o_size = rows*cols*chans;
                float *output = cnn->layer_sets[target_layer]->node.x;
                if(index < o_size)
                    ofs << i << ": " << output[index] << std::endl;

            }
            else if(target_layer == 4)
            {
                uint32_t o_size = cnn->W[target_layer]->cols;
                float *output = cnn->layer_sets[target_layer]->node.x;
                if(index < o_size)
                    ofs << i << ": " << output[index] << std::endl;
            }

#endif
#ifdef RUN_ACCELERATED_SIM
            // Print meaningful partial results for validation
            // Compare Fixed-point behavioral with RTL simulation (both accelerated)
            if (index < pool_size * pool_size * reduced_chans)
            { ofs << index << ": " << output[index] << std::endl; };
#endif
        }
    }

#ifdef RUN_SIM
    ofs.close();
#endif
#ifdef RUN_ACCELERATED_SIM
    ofs.close();
#endif

}

void system_t::run_pv(int layer, bool fully_connected = false)
{
    if (fully_connected)
        fc_compute(cnn->layer_sets[layer]->node.x,
                   cnn->layer_sets[layer - 1]->node.x,
                   cnn->W[layer - 1]->x,
                   cnn->layer_sets[layer]->bias.x,
                   cnn->W[layer - 1]->cols,
                   cnn->W[layer - 1]->rows,
                   cnn->layer_sets[layer]->relu);
    else
        convolution_compute(cnn->layer_sets[layer]->node.x,
                            cnn->layer_sets[layer]->bias.x,
                            cnn->layer_sets[layer - 1]->node.x,
                            cnn->W[layer - 1]->x,
                            cnn->layer_sets[layer]->node.cols,
                            cnn->layer_sets[layer]->node.rows,
                            cnn->layer_sets[layer - 1]->node.chans,
                            cnn->layer_sets[layer]->node.chans,
                            get_pool_size(cnn->layer_sets[layer]->node.cols,
                                          cnn->layer_sets[layer]->node.rows,
                                          cnn->layer_sets[layer]->do_pool,
                                          cnn->layer_sets[layer]->do_pad),
                            get_pool_stride(cnn->layer_sets[layer]->node.cols,
                                            cnn->layer_sets[layer]->node.rows,
                                            cnn->layer_sets[layer]->do_pool,
                                            cnn->layer_sets[layer]->do_pad),
                            cnn->layer_sets[layer]->do_pool,
                            cnn->layer_sets[layer]->do_pad);
}

void system_t::move_weights(int target_layer, bool fully_connected)
{
    float *w = cnn->W[target_layer - 1]->x;
    int src_chans;
    int dst_chans;
    int r = reduced_chans;

    if (fully_connected)
    {
        src_chans = cnn->W[target_layer - 1]->cols;
        dst_chans = cnn->W[target_layer - 1]->rows;

        for (int i = 1; i < r; i++)
            for (int j = 1; j < r; j++)
            { w[r * i + j] = w[src_chans * i + j]; }
    }
    else
    {
        src_chans = cnn->layer_sets[target_layer - 1]->node.chans;
        dst_chans = cnn->layer_sets[target_layer]->node.chans;

        int index = 0;
        for (int src_chan = 0; src_chan < src_chans; src_chan++) {
            for (int dst_chan = 0; dst_chan < r; dst_chan++) {
                for (int k = 0; k < 9; k++) {
                    w[index * 9 + k] = w[(src_chan * dst_chans + dst_chan) * 9 + k];
                }
                index++;
            }
        }
    }
}

void system_t::set_configuration_param(int target_layer, bool fully_connected)
{
    // ======================   ^  <------------- i_addr
    // |       input        |   | i_size
    // ======================   -  <------------- w_addr
    // |    input weights   |   | w_size
    // ======================   -  <------------- b_addr
    // |     output bias    |   | o_size
    // ======================   -  <------------- o_addr
    // |       output       |   | o_size
    // ======================   v

    if (fully_connected)
    {
        w_cols = cnn->W[target_layer - 1]->cols;
        w_rows = cnn->W[target_layer - 1]->rows;

        actual_i_size = w_cols;
        i_size = round_up(actual_i_size, DMA_ADJ);

        actual_w_size = w_cols * w_rows;
        w_size = round_up(actual_w_size, DMA_ADJ);

        actual_b_size = w_rows;
        b_size = round_up(actual_b_size, DMA_ADJ);

        actual_o_size = w_rows;
        o_size = round_up(actual_o_size, DMA_ADJ);
    }
    else
    {
        cols = cnn->layer_sets[target_layer]->node.cols;
        rows = cnn->layer_sets[target_layer]->node.rows;

        src_chans = cnn->layer_sets[target_layer - 1]->node.chans;
        dst_chans = cnn->layer_sets[target_layer]->node.chans;

        do_pool = cnn->layer_sets[target_layer]->do_pool;
        do_pad = cnn->layer_sets[target_layer]->do_pad;
        pool_size = get_pool_size(cols, rows, do_pool, do_pad);
        pool_stride = get_pool_stride(cols, rows, do_pool, do_pad);

        actual_i_size = cols * rows * src_chans;
        i_size = round_up(actual_i_size, DMA_ADJ);

        actual_w_size = 9 * src_chans * dst_chans;
        w_size = round_up(actual_w_size, DMA_ADJ);

        actual_b_size = dst_chans;
        b_size = round_up(actual_b_size, DMA_ADJ);

        //
        // WARNING: the convolution requires an output memory as large as the input memory
        //          to place temporary results. After performing max_pooling, however, the
        //          actual output is much smaller (pool_size), thus we can save time on
        //          DMA transactions by only transfering useful data.
        //
        actual_o_size = cols * rows * dst_chans;
        o_size = round_up(actual_o_size, DMA_ADJ);
    }

    // Compute memory mapping
    i_addr = 0;
    w_addr = i_size;
    b_addr = i_size + w_size;
    o_addr = i_size + w_size + b_size;
}

// -- Functions

void system_t::load_regs(bool fully_connected)
{
    // -- Pointers to the memory

    assert(driver->do_write(IN_BASE_ADDR_REG,    i_addr << 2));
    assert(driver->do_write(IN_W_BASE_ADDR_REG,  w_addr << 2));
    assert(driver->do_write(OUT_B_BASE_ADDR_REG, b_addr << 2));
    assert(driver->do_write(OUT_BASE_ADDR_REG,   o_addr << 2));

    // -- Configuration parameters

    assert(driver->do_write(IN_SIZE_REG,         i_size));
    assert(driver->do_write(IN_W_SIZE_REG,       w_size));
    assert(driver->do_write(OUT_SIZE_REG,        o_size));

    if (fully_connected)
    {
#ifdef RUN_ACCELERATED_SIM
        assert(driver->do_write(NUM_W_COLS_REG,  reduced_chans));
        assert(driver->do_write(NUM_W_ROWS_REG,  reduced_chans));
#else // NORMAL_SIMULATION
        assert(driver->do_write(NUM_W_COLS_REG,  w_cols));
        assert(driver->do_write(NUM_W_ROWS_REG,  w_rows));
#endif
    }
    else
    {
        assert(driver->do_write(OUT_B_SIZE_REG,  b_size));
        assert(driver->do_write(NUM_COLS_REG,    cols));
        assert(driver->do_write(NUM_ROWS_REG,    rows));
#ifdef RUN_ACCELERATED_SIM
#ifdef FIXED
        assert(driver->do_write(SRC_CHANS_REG,   reduced_chans));
#else
        assert(driver->do_write(SRC_CHANS_REG,   src_chans));
#endif
        assert(driver->do_write(DST_CHANS_REG,   reduced_chans));
#else // NORMAL_SIMULATION
        assert(driver->do_write(SRC_CHANS_REG,   src_chans));
        assert(driver->do_write(DST_CHANS_REG,   dst_chans));
#endif
        assert(driver->do_write(DO_POOL_REG,     do_pool));
        assert(driver->do_write(DO_PAD_REG,      do_pad));
        assert(driver->do_write(POOL_SIZE_REG,   pool_size));
        assert(driver->do_write(POOL_STRIDE_REG, pool_stride));
    }
}

void system_t::setup_memory(int target_layer, bool fully_connected)
{
    wait();

    REPORT_INFO("Software inference completed");

    REPORT_INFO("Testing target layer %d", target_layer);

    // Compact weights in memory if running accelerated RTL test

#ifdef RUN_ACCELERATED_SIM
#ifdef FIXED
    reduced_chans = REDUCED_CHANS(target_layer);
#else
    reduced_chans = REDUCED_CHANS_NATIVE(target_layer);
#endif
    move_weights(target_layer, fully_connected);
#endif

    // Determine configuration parameters from CNN model
    set_configuration_param(target_layer, fully_connected);

    // ======================   ^  <------------- i_addr
    // |       input        |   | i_size
    // ======================   -  <------------- w_addr
    // |    input weights   |   | w_size
    // ======================   -  <------------- b_addr
    // |     output bias    |   | o_size
    // ======================   -  <------------- o_addr
    // |       output       |   | o_size
    // ======================   v

    memory->data = new sc_dt::sc_bv<DATA_WIDTH>[
     (i_size + w_size + o_size + b_size)];

#ifdef FIXED
    load_fpdata<FPDATA, FPDATA_WL>(cnn->layer_sets[target_layer - 1]->node.x,
                                   i_addr, i_size, actual_i_size);
    load_fpdata<W_FPDATA, W_FPDATA_WL>(cnn->W[target_layer - 1]->x,
                                       w_addr, w_size, actual_w_size);
    load_fpdata<FPDATA, FPDATA_WL>(cnn->layer_sets[target_layer]->bias.x,
                                   b_addr, b_size, actual_b_size);
#else
    load_fpdata<FPDATA, 32>(cnn->layer_sets[target_layer - 1]->node.x,
                                   i_addr, i_size, actual_i_size);
    load_fpdata<W_FPDATA, 32>(cnn->W[target_layer - 1]->x,
                                       w_addr, w_size, actual_w_size);
    load_fpdata<FPDATA, 32>(cnn->layer_sets[target_layer]->bias.x,
                                   b_addr, b_size, actual_b_size);
#endif

    REPORT_INFO("Load memory completed");

    wait();
}

void system_t::load_memory(void)
{
    // -- Load model

    REPORT_INFO("Configuration");

    cnn = new mojo::network();
    assert(cnn->read(model_path));

    REPORT_INFO("DWARF7 model loaded");

    // -- Read image

    float *image = new float[IMAGE_SIZE];
    std::ifstream infile(image_path.c_str(), std::ifstream::binary);
    assert(infile.read((char *) image, IMAGE_SIZE * sizeof(float)));
    cnn->forward_setup(image);
    delete[] image;

    REPORT_INFO("DWARF7 image: %s", image_path.c_str());

    //
    // Run inference
    //

    // Input layer -> nothing to be done
    // input 34x34 (including pad of 2 pixels), 3 channels

    //
    // Convolution type 1
    //
#ifndef TARGET_LAYER_1
    // convolution 34x34 (including pad of 2 pixels), kernel
    //  size 3x3, 32 output channels, relu
    run_pv(1);

    //
    // Convolution type 2
    //
#ifndef TARGET_LAYER_2
    // convolution 34x34 (including pad of 2 pixels), kernel
    // size 3x3, 32 output channels, relu, max pooling 2x2 (pad on output -> 18x18)
    run_pv(2);

    //
    // Convolution type 3
    //
#ifndef TARGET_LAYER_3
    // convolution 18x18 (including pad of 2 pixels), kernel
    //  size 3x3, 64 output channels, relu, max pooling 2x2 (pad on output -> 10x10)
    run_pv(3);

    //
    // Convolution type 4
    //
#ifndef TARGET_LAYER_4
    // convolution 10x10 (including pad of 2 pixels), kernel
    //  size 3x3, 128 output channels, relu, max pooling 2x2 (no pad on output -> 4x4)
    run_pv(4);

    //
    // Fully Connected
    //
#ifndef TARGET_LAYER_5
    // fully_connected 1x1, 2048 to 64 channels, relu
    run_pv(5, true);
#ifndef TARGET_LAYER_6
    // fully_connected 1x1, 64 to 10 channels, brokemax
    run_pv(6, true);

#else
    setup_memory(6, true);
#endif // TARGET_LAYER_6
#else
    setup_memory(5, true);
#endif // TARGET_LAYER_5
#else
    setup_memory(4, false);
#endif // TARGET_LAYER_4
#else
    setup_memory(3, false);
#endif // TARGET_LAYER_3
#else
    setup_memory(2, false);
#endif // TARGET_LAYER_2
#else
    setup_memory(1, false);
#endif // TARGET_LAYER_1

}

void system_t::dump_memory(void)
{
#ifdef TARGET_LAYER_6
    convert_output(6);
#else
#ifdef TARGET_LAYER_5
    convert_output(5);
#else
#ifdef TARGET_LAYER_4
    convert_output(4);
#else
#ifdef TARGET_LAYER_3
    convert_output(3);
#else
#ifdef TARGET_LAYER_2
    convert_output(2);
#else
#ifdef TARGET_LAYER_1
    convert_output(1);
#else
    run_pv(1, false);
#endif // TARGET_LAYER_1
    run_pv(2, false);
#endif // TARGET_LAYER_2
    run_pv(3, false);
#endif // TARGET_LAYER_3
    run_pv(4, false);
#endif // TARGET_LAYER_4
    run_pv(5, true);
#endif // TARGET_LAYER_5
    run_pv(6, true);
#endif // TARGET_LAYER_6

    //
    // Inference Complete
    //

    REPORT_INFO("Dump memory completed");
}

#define ERROR_THRESHOLD 0.20

void system_t::validate(void)
{
    int first, second;
    int tot_errors = 0;
    std::stringstream stm;

    // -- This is the final output of the DWARF

    float *output = cnn->layer_sets[6]->node.x;

    // -- Inference is complete

    REPORT_INFO("Inference completed");

    first = mojo::arg_max(output, cnn->out_size());
    REPORT_INFO("#1: %s (%f)", labels[first], output[first]);

    output[first] = -1; // find the next best
    second = mojo::arg_max(output, cnn->out_size());
    REPORT_INFO("#2: %s (%f)", labels[second], output[second]);
}

void system_t::clean_up(void)
{
    delete[] memory->data;
    delete cnn;
}
