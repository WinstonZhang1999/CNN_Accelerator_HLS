/* Copyright 2021 Columbia University SLD Group */

#ifndef __FC_LAYER_FUNCTIONS_HPP__
#define __FC_LAYER_FUNCTIONS_HPP__

void fc_layer::fcrelu_accumulate_signal(uint32_t w_row, uint32_t num_w_cols, bool pingpong)
{
    FPDATA_WORD out_word;
    FPDATA_WORD out_b_word = output_b.port2[0][w_row];
    FPDATA out_fp = 0;
    FPDATA out_b_fp;

    for (uint32_t i = 0; i < num_w_cols; i++)
    {
        FPDATA_WORD in_word = input.port2[0][i];
        W_FPDATA_WORD in_w_word;

        if (pingpong)
        {
            in_w_word = input_w_0.port2[0][i];
        }
        else
        {
            in_w_word = input_w_1.port2[0][i];
        }

        FPDATA in_fp;
        W_FPDATA in_w_fp;

        cynw_interpret_float(in_word, in_fp);
        cynw_interpret_float(in_w_word, in_w_fp);

        out_fp += in_fp * in_w_fp;
    }

    cynw_interpret_float(out_b_word, out_b_fp);

    if (out_fp + out_b_fp < 0)
    {
        out_fp = 0;
    }
    else
    {
        out_fp += out_b_fp;
    }

    cynw_interpret_float(out_fp, out_word);

    output.port1[0][w_row] = out_word;
}


#endif /* __FC_LAYER_FUNCTIONS_HPP__ */
