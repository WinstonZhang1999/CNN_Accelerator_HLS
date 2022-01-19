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
//    dwarf.cpp:  Simple example using pre-trained DWARF7 model.
// ==================================================================== mojo ==

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <fstream>
#include <stdio.h>


#include <mojo.h>
#include <mojo_utils.h>

std::string image_path = "airplane.bmp";

int main(int argc, char **argv)
{
	if (argc > 1) image_path = std::string(argv[1]);

	// read image
	cv::Mat im = cv::imread(image_path);
	if(im.empty() || im.cols<1) { std::cout << "Failed to read a valid image. (" << image_path <<")"<<std::endl; return 1;}

	// convert packed BGR to planar BGR and subtract DWARF mean (while converting to float)
	cv::Mat bgr[3];
	cv::split(im,bgr);
	const int img_size = 3 * 32 * 32;
	float *img = new float[img_size];

        float data_mean = 120.70748;
        float data_std = 64.150024;

	for(int i=0; i<32*32;i++)
	{
            img[i+32*32*0]= ((float)bgr[2].data[i] - data_mean)/(data_std + 1e-7);
            img[i+32*32*1]= ((float)bgr[1].data[i] - data_mean)/(data_std + 1e-7);
            img[i+32*32*2]= ((float)bgr[0].data[i] - data_mean)/(data_std + 1e-7);
	}


	// dump binary
	std::string output_path = image_path.replace(image_path.end() - 3, image_path.end(), "bin");
	std::ofstream outfile(output_path.c_str(), std::ofstream::binary);
	outfile.write((char *) img, img_size * sizeof(float));

	delete [] img;

	return 0;
}
