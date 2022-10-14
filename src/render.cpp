#include "render.hpp"
#include "utils.hpp"


void gray_scale(char* buffer, int width, int height, int stride){

  for (int i = 0; i < height; i++){
    rgba8_t*  lineptr = (rgba8_t*)(buffer + i * stride);
    for (int j =0; j < width; j++){

      auto cell = *(lineptr + j);
      std::uint8_t r = cell.r, g = cell.g, b = cell.b;

      auto gray = static_cast<std::uint8_t>(0.3 * r + 0.59 * g + 0.11 * b);

      lineptr[j] = rgba8_t{gray, gray, gray, 255};
    }
  }
}

void gaussian_blur(char* buffer, int width, int height, int stride){  
  
  int size = 3;
  double sigma = 1.0;
  double* kernel = (double*)malloc(sizeof(double) * size * size);
  gaussian_kernel(kernel, sigma, size);

  //////gaussian blur/////////
  for (int i = 0; i < height - size; i++) {
    rgba8_t* lineptr_1 = (rgba8_t*)(buffer + i * stride);
    rgba8_t* lineptr_2 = (rgba8_t*)(buffer + (i + 1) * stride);
    rgba8_t* lineptr_3 = (rgba8_t*)(buffer + (i + 2) * stride);
    for (int j = 0; j < width - size; j++) {
      double gaussian_pixel = 0.0;
      for (int x = 0; x < size; x++) {
	std::uint8_t cell_1 = (*(lineptr_1 + j + x)).r;
	std::uint8_t cell_2 = (*(lineptr_2 + j + x)).r;
	std::uint8_t cell_3 = (*(lineptr_3 + j + x)).r;
	gaussian_pixel += kernel[x] * cell_1 + kernel[size + x] * cell_2 + kernel[size * 2 + x] * cell_3;
      }
      auto cast_gaussian_pixel  = static_cast<std::uint8_t>(gaussian_pixel);
      lineptr_1[j] = rgba8_t{cast_gaussian_pixel, cast_gaussian_pixel, cast_gaussian_pixel, 255};
      lineptr_2[j] = rgba8_t{cast_gaussian_pixel, cast_gaussian_pixel, cast_gaussian_pixel, 255};
      lineptr_3[j] = rgba8_t{cast_gaussian_pixel, cast_gaussian_pixel, cast_gaussian_pixel, 255};
    }
  }
  free(kernel);
}
void render_cpu(char* ref_buffer, int width, int height, std::ptrdiff_t stride, char* img_buffer)
{
}
