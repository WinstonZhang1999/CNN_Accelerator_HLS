// == mojo ====================================================================
//
//    Copyright (c) gnawice@gnawice.com. All rights reserved.
//	  See LICENSE in root folder
//
//    Permission is hereby granted, free of charge, to any person obtaining a
//    copy of this software and associated documentation files(the "Software"),
//    to deal in the Software without restriction, including without
//    limitation the rights to use, copy, modify, merge, publish, distribute,
//    sublicense, and/or sell copies of the Software, and to permit persons to
//    whom the Software is furnished to do so, subject to the following
//    conditions :
//
//    The above copyright notice and this permission notice shall be included
//    in all copies or substantial portions of the Software.
//
//    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
//    OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
//    THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// ============================================================================

#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <stdio.h>

#include <mojo.h>
#include <mojo_utils.h>
#include <dwarf.h>

std::ofstream ofs;

std::string image_path = "../data/truck3.bmp";
std::string model_file = "../models/dwarf7.mojo";
int target_layer = 5;

int main(int argc, char **argv) {
    if (argc > 1)
        image_path = std::string(argv[1]);
    if (argc > 2)
        target_layer = atoi(argv[2]);

    std::cout << image_path << std::endl;

    // read image
    cv::Mat im = cv::imread(image_path);
    if (im.empty() || im.cols < 1) {
        std::cout << "Failed to read a valid image. (" << image_path << ")" << std::endl;
        return 1;
    }

    // resize to 32x32 BGR format
    // cv::resize(im,im,cv::Size(32,32));
    // make 3 chan
    // if (im.channels()<3) cv::cvtColor(im,im,CV_GRAY2BGR);

    // convert packed BGR to planar BGR and subtract DWARF mean (while converting to float)
    cv::Mat bgr[3];
    cv::split(im, bgr);

    float *img = new float[3 * 32 * 32];
    // for(int i=0; i<32*32;i++)
    //{
    //    std::cout  << "BGR: ";
    //    std::cout << (unsigned)bgr[2].data[i] << " ";
    //    std::cout << (unsigned)bgr[1].data[i] << " ";
    //    std::cout << (unsigned)bgr[0].data[i] << std::endl;
    //}
    float data_mean = 120.70748;
    float data_std = 64.150024;

    // normalize input data
    for (int i = 0; i < 32 * 32; i++) {
        img[i + 32 * 32 * 0] = ((float)bgr[2].data[i] - data_mean) / (data_std + 1e-7);
        img[i + 32 * 32 * 1] = ((float)bgr[1].data[i] - data_mean) / (data_std + 1e-7);
        img[i + 32 * 32 * 2] = ((float)bgr[0].data[i] - data_mean) / (data_std + 1e-7);
    }

    // create a network
    mojo::network cnn;

    // load DWARF7 model
    std::cout << "Loading DWARF7 model..." << std::endl;
    if (!cnn.read(model_file)) {
        std::cerr << "error: could not read model.\n";
        return 1;
    }

    // start a (low precision) timer
    std::cout << "Classifying image..." << std::endl;

    // Load input
    cnn.forward_setup(img);

    // ------- Run inference -------

    // Input layer -> nothing to be done
    // input 34x34 (including pad of 2 pixels), 3 channels

    // conv2d #1: 34x34 (including pad of 2 pixels), kernel size 3x3, 32 channels, relu
    convolution_compute(cnn.layer_sets[1]->node.x, cnn.layer_sets[1]->bias.x, cnn.layer_sets[0]->node.x, cnn.W[0]->x,
                        cnn.layer_sets[1]->node.cols, cnn.layer_sets[1]->node.rows, cnn.layer_sets[0]->node.chans,
                        cnn.layer_sets[1]->node.chans,
                        get_pool_size(cnn.layer_sets[1]->node.cols, cnn.layer_sets[1]->node.rows,
                                      cnn.layer_sets[1]->do_pool, cnn.layer_sets[1]->do_pad),
                        get_pool_stride(cnn.layer_sets[1]->node.cols, cnn.layer_sets[1]->node.rows,
                                        cnn.layer_sets[1]->do_pool, cnn.layer_sets[1]->do_pad),
                        cnn.layer_sets[1]->do_pool, cnn.layer_sets[1]->do_pad);

    // conv2d #2: 34x34 (including pad of 2 pixels), kernel size 3x3, 32 channels, relu, max pooling 2x2 (pad on output
    // -> 18x18)
    convolution_compute(cnn.layer_sets[2]->node.x, cnn.layer_sets[2]->bias.x, cnn.layer_sets[1]->node.x, cnn.W[1]->x,
                        cnn.layer_sets[2]->node.cols, cnn.layer_sets[2]->node.rows, cnn.layer_sets[1]->node.chans,
                        cnn.layer_sets[2]->node.chans,
                        get_pool_size(cnn.layer_sets[2]->node.cols, cnn.layer_sets[2]->node.rows,
                                      cnn.layer_sets[2]->do_pool, cnn.layer_sets[2]->do_pad),
                        get_pool_stride(cnn.layer_sets[2]->node.cols, cnn.layer_sets[2]->node.rows,
                                        cnn.layer_sets[2]->do_pool, cnn.layer_sets[2]->do_pad),
                        cnn.layer_sets[2]->do_pool, cnn.layer_sets[2]->do_pad);

    // conv2d #3: 18x18 (including pad of 2 pixels), kernel size 3x3, 32 channels, relu, max pooling 2x2 (pad on output
    // -> 10x10)
    convolution_compute(cnn.layer_sets[3]->node.x, cnn.layer_sets[3]->bias.x, cnn.layer_sets[2]->node.x, cnn.W[2]->x,
                        cnn.layer_sets[3]->node.cols, cnn.layer_sets[3]->node.rows, cnn.layer_sets[2]->node.chans,
                        cnn.layer_sets[3]->node.chans,
                        get_pool_size(cnn.layer_sets[3]->node.cols, cnn.layer_sets[3]->node.rows,
                                      cnn.layer_sets[3]->do_pool, cnn.layer_sets[3]->do_pad),
                        get_pool_stride(cnn.layer_sets[3]->node.cols, cnn.layer_sets[3]->node.rows,
                                        cnn.layer_sets[3]->do_pool, cnn.layer_sets[3]->do_pad),
                        cnn.layer_sets[3]->do_pool, cnn.layer_sets[3]->do_pad);

    // conv2d #4: 10x10 (including pad of 2 pixels), kernel size 3x3, 64 channels, relu, max pooling 2x2
    convolution_compute(cnn.layer_sets[4]->node.x, cnn.layer_sets[4]->bias.x, cnn.layer_sets[3]->node.x, cnn.W[3]->x,
                        cnn.layer_sets[4]->node.cols, cnn.layer_sets[4]->node.rows, cnn.layer_sets[3]->node.chans,
                        cnn.layer_sets[4]->node.chans,
                        get_pool_size(cnn.layer_sets[4]->node.cols, cnn.layer_sets[4]->node.rows,
                                      cnn.layer_sets[4]->do_pool, cnn.layer_sets[4]->do_pad),
                        get_pool_stride(cnn.layer_sets[4]->node.cols, cnn.layer_sets[4]->node.rows,
                                        cnn.layer_sets[4]->do_pool, cnn.layer_sets[4]->do_pad),
                        cnn.layer_sets[4]->do_pool, cnn.layer_sets[4]->do_pad);

    // fc (dense) #1: 1x1, 2048 channels, relu
    fc_compute(cnn.layer_sets[5]->node.x, cnn.layer_sets[4]->node.x, cnn.W[4]->x, cnn.layer_sets[5]->bias.x,
               cnn.W[4]->cols, cnn.W[4]->rows, cnn.layer_sets[5]->relu);


    // fc (dense) #2: 1x1, 64 channels, relu
    fc_compute(cnn.layer_sets[6]->node.x, cnn.layer_sets[5]->node.x, cnn.W[5]->x, cnn.layer_sets[6]->bias.x,
               cnn.W[5]->cols, cnn.W[5]->rows, cnn.layer_sets[6]->relu);

    float *out = cnn.layer_sets[6]->node.x;

    std::cout << "TARGET_LAYER: " << target_layer << std::endl;
    ofs.open("test.txt", std::ofstream::out);
    
    if(target_layer == 5 || target_layer == 6)
    {
        int o_size = cnn.W[target_layer-1]->rows;
        float *output = cnn.layer_sets[target_layer]->node.x;
        for(int i=0; i < o_size; i++)
            ofs << i << ": " << output[i] << std::endl;
    }
    else if(target_layer != 4)
    {
        int rows = cnn.layer_sets[target_layer+1]->node.rows;
        int cols = cnn.layer_sets[target_layer+1]->node.cols;
        int chans = cnn.layer_sets[target_layer]->node.chans;
        int o_size = rows*cols*chans;
        float *output = cnn.layer_sets[target_layer]->node.x;
        std::cout << rows << "x" << cols << "x" << chans << std::endl;
        for(int i=0; i < o_size; i++)
            ofs << i << ": " << output[i] << std::endl;

    }
        else if(target_layer == 4)
    {
        int o_size = cnn.W[target_layer]->cols;
        float *output = cnn.layer_sets[target_layer]->node.x;
        std::cout << o_size << std::endl;
        for(int i=0; i < o_size; i++)
            ofs << i << ": " << output[i] << std::endl;
    }
    ofs.close();


    // ------- Finish inference -------

    std::cout << "Results:" << std::endl;
    // find the best result
    int first = mojo::arg_max(out, cnn.out_size());
    std::cout << "  1: label|score: \t\"" << labels[first] << "\" | " << out[first] << std::endl;

    // find next best
    out[first] = -1;
    int second = mojo::arg_max(out, cnn.out_size());
    std::cout << "  2: label|score: \t\"" << labels[second] << "\" | " << out[second] << std::endl;

    // std::cout << std::endl << "Click on an image, then hit any key to exit." << std::endl;
    // std::cout << std::endl;

    // enum mojo_palette{ gray=0, hot=1, tensorglow=2, voodoo=3, saltnpepa=4};
    // mojo::show(mojo::draw_cnn_weights(cnn, "1.1", mojo::tensorglow), 3, "Weights 1.1",1);
    // mojo::show(mojo::draw_cnn_state(cnn, "1.1", mojo::hot), 0.25, "Map 1.1",1);

    // std::string str;
    // cv::imshow(labels[first], im);
    // cv::waitKey(0);

    delete[] img;

    return 0;
}
