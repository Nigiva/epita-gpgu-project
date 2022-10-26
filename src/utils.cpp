#include <iostream>
#include "utils.hpp"

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
