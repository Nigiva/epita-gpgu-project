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

void gaussian_blur(char* buffer, int width, int height, int stride, int kernel_size){  
  
  double sigma = 1.0;
  double* kernel = (double*)malloc(sizeof(double) * kernel_size * kernel_size);
  gaussian_kernel(kernel, sigma, kernel_size);

  //////gaussian blur/////////
  for (int i = 0; i < height - kernel_size; i++) {
    rgba8_t* array_lineptr[kernel_size];

    for (int y = 0; y < kernel_size; y++){
      array_lineptr[y] = (rgba8_t*)(buffer + (i+y) * stride);
    }

    // Compute gaussian pixel
    for (int j = 0; j < width - kernel_size; j++) {
      double gaussian_pixel = 0.0;
      for (int x = 0; x < kernel_size; x++) {
	for (int y = 0; y < kernel_size; y++){
	  std::uint8_t cell_1 = (*(array_lineptr[y] + j + x)).r;
	  gaussian_pixel += kernel[kernel_size * y + x] * cell_1;
	}
      }
      auto cast_gaussian_pixel  = static_cast<std::uint8_t>(gaussian_pixel);

      // Assign gaussian pixel to image
      for (int y = 0; y< kernel_size; y++){
	array_lineptr[y][j] = rgba8_t{cast_gaussian_pixel, cast_gaussian_pixel, cast_gaussian_pixel, 255};
      }
    }
  }
  free(kernel);
}
void render_cpu(char* ref_buffer, int width, int height, std::ptrdiff_t stride, char* img_buffer)
{
}
