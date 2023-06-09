#pragma once
#include <cmath>

/// \param kernel The kernel buffer
/// \param sigma The standard deviation
/// \param size The kernel size
void gaussian_kernel(double* kernel, double sigma, int size);

/// \param a first value
/// \param b second value
std::uint8_t abs(std::uint8_t a, std::uint8_t b);

/// \param img_buffer The image to change
/// \param width The image's width
/// \param height The image's height
/// \param stride The image's stride
/// \param radius The window's radius
/// \param is_square True if the window is a square, False for a disc
/// \param is_erosion True for the erosion algorithm, False for dilation
void erosion_dilation(char *img_buffer, int width, int height, int stride, int radius, bool is_square, bool is_erosion);


/// \param img_buffer the image to change
/// \param width the image's width
/// \param height the image's height
/// \param stride the image's stride
int* histo(char *img_buffer, int width, int height, int stride);

/// \param width the image's width
/// \param height the image's height
/// \param hist the histogram of the image
int otsu(int width, int height, int* hist);

/// \param img_buffer The image to change
/// \param width the image's width
/// \param height the image's height
/// \param stride The image's stride
/// \param threshold_1 address of the variable that will contain the first threshold
/// \param threshold_2 address of the variable that will contain the second threshold
void hysteresis(char* img_buffer, int width, int height, int stride, int* threshold_1, int* threshold_2);
