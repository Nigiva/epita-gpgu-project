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
