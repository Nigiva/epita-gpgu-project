#include <iostream>
#include "utils.hpp"
#include "render.hpp"
#include <math.h>
#include <stdlib.h>

void gaussian_kernel(double *kernel, double sigma, int size){
    double r, s = 2.0 * sigma * sigma;
    double sum = 0.0;

    int mid = size / 2;
    for (int x = -mid; x <= mid; x++) {
        for (int y = -mid; y <= mid; y++) {
            r = sqrt(x * x + y * y);
            kernel[(y + mid) * size + (x + mid)] = (exp(-(r * r) / s)) / (M_PI * s);
            sum += kernel[(y + mid) * size + (x + mid)];
        }
    }

    // normalising the Kernel
    for (int i = 0; i < size; ++i)
    {
        for (int j = 0; j < size; ++j)
        {
            kernel[i * size + j] /= sum;
        }
    }
}

std::uint8_t abs(std::uint8_t a, std::uint8_t b)
{
    int res = a - b;
    if (res < 0){
        res = -res;
    }
    return (std::uint8_t) res;
}

void erosion_dilation(char *img_buffer, int width, int height, int stride, int radius, bool is_square, bool is_erosion){

    rgba8_t tmp_buffer[height][width];

    // iteration on each pixels
    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            char* base_ptr = img_buffer + y * stride;

            std::uint8_t val = (is_erosion) ? 255 : 0;

            // iteration on each pixel around current pixel
            for (int i = -radius; i <= radius; i++){
                for (int j = -radius; j <= radius; j++){
                    // if outside the image
                    if (i + x < 0 or i + x >= width or j + y < 0 or j + y >= height)
                        continue;

                    // if disc and not in it
                    //if (!is_square and !(abs(i) + abs(j) <= radius))
                    if (!is_square and !((int)sqrt(i*i + j*j) <= radius))
                        continue;
                    std::uint8_t cell = ((rgba8_t*)(base_ptr + j * stride))[i + x].r;
                    if (is_erosion and val > cell){
                        val = cell;
                    }
                    else if (!is_erosion and val < cell){
                        val = cell;
                    }
                }
            }
            tmp_buffer[y][x] = rgba8_t{val, val, val, 255};
        }
    }

    // copy to img_buffer
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            ((rgba8_t*)(img_buffer + y * stride))[x] = tmp_buffer[y][x];
        }
    }

}


int* histo(char *img_buffer, int width, int height, int stride){
    int* buffer = (int*)calloc(256, sizeof(int));

    for (int y = 0; y < height; y++){
        rgba8_t* lineptr = (rgba8_t*)(img_buffer + y * stride);
        for (int x = 0; x < width; x++){
            auto cell = lineptr[x];
            buffer[cell.r] += 1;
        }
    }
    return buffer;
}

int otsu(int width, int height, int* hist){
    int sum = 0;

    for (int i = 0; i <= 255; i++){
        sum += i * hist[i];
    }

    int q1 = 0;
    int q2 = 0;
    double mu1 = 0.0;
    double mu2 = 0.0;
    int N = width * height;
    int cur_sum = 0;
    double var;
    double var_max = 0.0;
    int threshold = 0;

    for (int i = 0; i <= 255; i++){
        q1 += hist[i];
        if (q1 == 0)
            continue;

        q2 = N - q1;
        cur_sum += i * hist[i];
        mu1 = (double)cur_sum / q1;
        mu2 = (double)(sum - cur_sum) / q2;
        var = (mu1 - mu2) * (mu1 - mu2) * q1 * q2;

        if (var > var_max){
            threshold = i;
            var_max = var;
        }
    }
    return threshold;
}

void hysteresis(char* img_buffer, int width, int height, int stride, int* threshold_1, int* threshold_2){

    int* hist = histo(img_buffer, width, height, stride);
    *threshold_1 = otsu(width, height, hist);
    int N = 0;

    for (int i = 0; i <= *threshold_1; i++){
        N += hist[i];
        hist[i] = 0;
    }

    *threshold_2 = otsu(1, width * height - N, hist);
    free(hist);

}

