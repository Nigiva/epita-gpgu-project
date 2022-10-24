#pragma once
#include <cmath>

/// \param kernel The kernel buffer
/// \param sigma The standard deviation
/// \param size The kernel size
void gaussian_kernel(double* kernel, double sigma, int size);

/// \param a first value
/// \param b second value
std::uint8_t abs(std::uint8_t a, std::uint8_t b);
