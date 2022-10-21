#include "render.hpp"
#include <iostream> 

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
        std::cout << "diff" << std::endl;
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


void render_cpu(char* ref_buffer, int width, int height, std::ptrdiff_t stride, char* img_buffer)
{
    gray_scale(img_buffer, width, height, stride);
    images_difference(ref_buffer, width, height, stride, img_buffer);
}
