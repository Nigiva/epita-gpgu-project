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

void images_difference(char *ref_buffer, int width, int height, int stride, char* img_buffer)
{
    for (int i = 0; i < height; i++)
    {
        rgba8_t *lineptr_ref = (rgba8_t*)(ref_buffer + i * stride);
        rgba8_t *lineptr_img = (rgba8_t*)(img_buffer + i * stride);

        for (int j = 0; j < width; j++)
        {
            auto cell_ref = *(lineptr_ref + j);
            auto cell_img = *(lineptr_img + j);

            std::uint8_t r = cell_ref.r - cell_img.r;
            std::uint8_t g = cell_ref.g - cell_img.g;
            std::uint8_t b = cell_ref.b - cell_img.b;
            std::uint8_t a = cell_img.a;

            lineptr_img[j] = rgba8_t{r, g, b, a};
        }
    }
}

void gaussian_blur(char* buffer, int width, int height, int stride, int kernel_size){  
  
  // get the gaussian kernel
  double sigma = 1.0;
  double* kernel = (double*)malloc(sizeof(double) * kernel_size * kernel_size);
  gaussian_kernel(kernel, sigma, kernel_size);

  //////gaussian blur/////////
  int mid_kernel = (kernel_size - 1) / 2;
  rgba8_t tmp_buffer[height][width];

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
        char* base_ptr = buffer + y * stride;

        double gaussian_pixel = 0.0;
        for (int i = -mid_kernel; i <= mid_kernel; i++) {
            for (int j = -mid_kernel; j <= mid_kernel; j++) {
                if (i + x < 0 or i + x >= width or j + y < 0 or j + y >= height)
                    continue;
                std::uint8_t cell = ((rgba8_t*)(base_ptr + j * stride))[i + x].r;
                gaussian_pixel += kernel[kernel_size * (j + mid_kernel) + (i + mid_kernel)] * cell;
            }
        }
        std::uint8_t cast_gaussian_pixel = (std::uint8_t) gaussian_pixel;
        tmp_buffer[y][x] = rgba8_t{cast_gaussian_pixel, cast_gaussian_pixel, cast_gaussian_pixel, 255};
    }
  }

  // copy the blured buffer
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
        ((rgba8_t*)(buffer + y * stride))[x] = tmp_buffer[y][x];
    }
  }

  free(kernel);
}

void render_cpu(char* ref_buffer, int width, int height, std::ptrdiff_t stride, char* img_buffer)
{
}
