#include "render.hpp"

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

void render_cpu(char* ref_buffer, int width, int height, std::ptrdiff_t stride, char* img_buffer)
{
}
