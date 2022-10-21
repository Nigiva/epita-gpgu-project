#pragma once
#include <cmath>

/// \param kernel The kernel buffer
/// \param sigma The standard deviation
/// \param size The kernel size
void gaussian_kernel(double* kernel, double sigma, int size);
