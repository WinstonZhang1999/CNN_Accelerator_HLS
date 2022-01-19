/* Copyright 2021 Columbia University SLD Group */

#ifndef __CONV_LAYER_FUNCTIONS_HPP__
#define __CONV_LAYER_FUNCTIONS_HPP__

void conv_layer::convolution_compute(uint32_t num_cols, uint32_t num_rows,
                                     uint32_t src_chans, uint32_t dst_chans,
                                     int src_chan, int dst_chan,
                                     bool do_pool, bool do_pad,
                                     uint32_t pool_size, uint32_t pool_stride,
                                     bool pingpong, bool out_pingpong)
{
    const int chan_stride = num_cols * num_rows;

    // Do aligned DMA accesses, but keep track of where useful data begin
    const int non_aligned_input_ptr = chan_stride * src_chan;
    const int non_aligned_input_w_ptr = 9 * (src_chan * dst_chans + dst_chan);
    const int non_aligned_output_ptr = chan_stride * dst_chan;
    const int non_aligned_output_pool_ptr = pool_stride * dst_chan;

    const int input_ptr = non_aligned_input_ptr - round_down(non_aligned_input_ptr, DMA_ADJ);
    const int input_w_ptr = non_aligned_input_w_ptr - round_down(non_aligned_input_w_ptr,
                            DMA_ADJ);
    const int output_bias_ptr = dst_chan;
    const int output_ptr = non_aligned_output_ptr - round_down(non_aligned_output_ptr,
                           DMA_ADJ);
    const int output_pool_ptr = non_aligned_output_pool_ptr - round_down(
                                    non_aligned_output_pool_ptr, DMA_ADJ);

    const bool last = (src_chan == src_chans - 1);

    W_FPDATA_WORD filter_word[9];
    W_FPDATA filter[9];
    HLS_FLATTEN_ARRAY(filter_word);
    HLS_FLATTEN_ARRAY(filter);

    for (int i = 0 ; i < 9; i++)
    {
        HLS_UNROLL_LOOP(ON);

        if (pingpong)
        { filter_word[i] = input_w_0.port2[0][input_w_ptr + i]; }
        else
        { filter_word[i] = input_w_1.port2[0][input_w_ptr + i]; }

        cynw_interpret_float(filter_word[i], filter[i]);
    }

    FPDATA_WORD b_word = output_b.port2[0][output_bias_ptr];
    FPDATA b;
    cynw_interpret_float(b_word, b);

    //
    // Convolution
    //

    const int cols = num_cols;

    // dotsum_3x3
    for (int j = 1; j < cols - 1; j++) // intput h
        for (int i = 1; i < cols - 1; i++) // intput w
        {
            const int index_out_p0 = (j - 1) * cols + i - 1;
            const int index_out_p1 = j * cols + i - 1;
            const int index_out_p2 = (j - 1) * cols + i;
            const int index_out_p3 = j * cols + i;
            const int index_out = index_out_p3;

            int index_out_pool;

            if (do_pad)
            { index_out_pool = j / 2 * pool_size + i / 2; }
            else
            { index_out_pool = (j / 2 - 1) * pool_size + i / 2 - 1; }

            FPDATA_WORD img_word[9];
            FPDATA img[9];
            HLS_FLATTEN_ARRAY(img_word);
            HLS_FLATTEN_ARRAY(img);

            if (pingpong)
            {
                img_word[0] = input_0.port2[0][input_ptr + index_out - 1 - 1 * cols  ];
                img_word[1] = input_0.port2[0][input_ptr + index_out + 0 - 1 * cols  ];
                img_word[2] = input_0.port2[0][input_ptr + index_out + 1 - 1 * cols  ];
                img_word[3] = input_0.port2[0][input_ptr + index_out - 1             ];
                img_word[4] = input_0.port2[0][input_ptr + index_out + 0             ];
                img_word[5] = input_0.port2[0][input_ptr + index_out + 1             ];
                img_word[6] = input_0.port2[0][input_ptr + index_out - 1 + 1 * cols  ];
                img_word[7] = input_0.port2[0][input_ptr + index_out + 0 + 1 * cols  ];
                img_word[8] = input_0.port2[0][input_ptr + index_out + 1 + 1 * cols  ];
            }
            else
            {
                img_word[0] = input_1.port2[0][input_ptr + index_out - 1 - 1 * cols  ];
                img_word[1] = input_1.port2[0][input_ptr + index_out + 0 - 1 * cols  ];
                img_word[2] = input_1.port2[0][input_ptr + index_out + 1 - 1 * cols  ];
                img_word[3] = input_1.port2[0][input_ptr + index_out - 1             ];
                img_word[4] = input_1.port2[0][input_ptr + index_out + 0             ];
                img_word[5] = input_1.port2[0][input_ptr + index_out + 1             ];
                img_word[6] = input_1.port2[0][input_ptr + index_out - 1 + 1 * cols  ];
                img_word[7] = input_1.port2[0][input_ptr + index_out + 0 + 1 * cols  ];
                img_word[8] = input_1.port2[0][input_ptr + index_out + 1 + 1 * cols  ];
            }

            for (int k = 0; k < 9; k++)
            {
                HLS_UNROLL_LOOP(ON);
                cynw_interpret_float(img_word[k], img[k]);
            }

            FPDATA_WORD out_word;
            FPDATA out;

            if (out_pingpong)
            { out_word = output_0.port2[0][output_ptr + index_out]; }
            else
            { out_word = output_1.port2[0][output_ptr + index_out]; }

            if (src_chan == 0)
            { out = 0; }
            else
            { cynw_interpret_float(out_word, out); }

            out +=
                img[0] * filter[0] +
                img[1] * filter[1] +
                img[2] * filter[2] +
                img[3] * filter[3] +
                img[4] * filter[4] +
                img[5] * filter[5] +
                img[6] * filter[6] +
                img[7] * filter[7] +
                img[8] * filter[8];

            // Activation
            if (last)
            {
                if (out + b < 0)
                { out = 0; }
                else
                { out = out + b; }
            }

            {
                HLS_DEFINE_PROTOCOL("separate-read-write-1");
                wait();
            }

            cynw_interpret_float(out, out_word);

            if (out_pingpong)
            { output_0.port1[0][output_ptr + index_out] = out_word; }
            else
            { output_1.port1[0][output_ptr + index_out] = out_word; }

            {
                HLS_DEFINE_PROTOCOL("separate-read-write-2");
                wait();
            }

            if (last)
            {
                // Max Pool 2x2
                if ((j % 2 == 0) && (i % 2 == 0) && do_pool)
                {

                    FPDATA_WORD p0_word, p1_word, p2_word, p3_word;
                    FPDATA p0, p1, p2, p3;

                    if (out_pingpong)
                    {
                        p0_word = output_0.port2[0][output_ptr + index_out_p0];
                        p1_word = output_0.port2[0][output_ptr + index_out_p1];
                        p2_word = output_0.port2[0][output_ptr + index_out_p2];
                        p3_word = output_0.port2[0][output_ptr + index_out_p3];
                    }
                    else
                    {
                        p0_word = output_1.port2[0][output_ptr + index_out_p0];
                        p1_word = output_1.port2[0][output_ptr + index_out_p1];
                        p2_word = output_1.port2[0][output_ptr + index_out_p2];
                        p3_word = output_1.port2[0][output_ptr + index_out_p3];
                    }

                    {
                        HLS_DEFINE_PROTOCOL("separate-read-write-3");
                        wait();
                    }

                    cynw_interpret_float(p0_word, p0);
                    cynw_interpret_float(p1_word, p1);
                    cynw_interpret_float(p2_word, p2);
                    cynw_interpret_float(p3_word, p3);

                    FPDATA max = p0;

                    if (max < p1)
                    { max = p1; }

                    if (max < p2)
                    { max = p2; }

                    if (max < p3)
                    { max = p3; }

                    cynw_interpret_float(max, out_word);

                    if (out_pingpong)
                    { output_0.port1[0][output_pool_ptr + index_out_pool] = out_word; }
                    else
                    { output_1.port1[0][output_pool_ptr + index_out_pool] = out_word; }

                    {
                        HLS_DEFINE_PROTOCOL("separate-read-write-4");
                        wait();
                    }
                }
            }
        }

    // Pad (resize)
    if (last && do_pad)
    {
        for (int i = 0; i < pool_size; i ++)

            // first row
            if (out_pingpong)
            { output_0.port1[0][output_pool_ptr + i] = 0; }
            else
            { output_1.port1[0][output_pool_ptr + i] = 0; }

        for (int i = 0; i < pool_size; i ++)

            // last row
            if (out_pingpong)
            { output_0.port1[0][output_pool_ptr + pool_size * (pool_size - 1) + i] = 0; }
            else
            { output_1.port1[0][output_pool_ptr + pool_size * (pool_size - 1) + i] = 0; }


        for (int i = 0; i < pool_size; i ++)

            // first column
            if (out_pingpong)
            { output_0.port1[0][output_pool_ptr + pool_size * i] = 0; }
            else
            { output_1.port1[0][output_pool_ptr + pool_size * i] = 0; }

        for (int i = 0; i < pool_size; i ++)

            // last column  
            if (out_pingpong)
            { output_0.port1[0][output_pool_ptr + pool_size * i + pool_size - 1] = 0; }
            else
            { output_1.port1[0][output_pool_ptr + pool_size * i + pool_size - 1] = 0; }
    }
}

#endif /* __CONV_LAYER_FUNCTIONS_HPP__ */
