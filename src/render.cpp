#include "render.hpp"
#include "utils.hpp"
#include <map>
#include <iostream>
#include <cstring>

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

void images_diff(char *ref_buffer, int width, int height, int stride, char* img_buffer)
{
    for (int i = 0; i < height; i++)
    {
        rgba8_t *lineptr_ref = (rgba8_t*)(ref_buffer + i * stride);
        rgba8_t *lineptr_img = (rgba8_t*)(img_buffer + i * stride);

        for (int j = 0; j < width; j++)
        {
            auto cell_ref = lineptr_ref[j];
            auto cell_img = lineptr_img[j];

            std::uint8_t r = abs(cell_ref.r - cell_img.r);
            std::uint8_t g = abs(cell_ref.g - cell_img.g);
            std::uint8_t b = abs(cell_ref.b - cell_img.b);
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

void opening(char* img_buffer, int width, int height, int stride, int radius, bool is_square){
  erosion_dilation(img_buffer, width, height, stride, radius, is_square, true);
  erosion_dilation(img_buffer, width, height, stride, radius, is_square, false);
}
void closing(char* img_buffer, int width, int height, int stride, int radius, bool is_square){
  erosion_dilation(img_buffer, width, height, stride, radius, is_square, false);
  erosion_dilation(img_buffer, width, height, stride, radius, is_square, true);
}


void bbox(char *img_buffer, int width, int height, int stride, int threshold, int peak){

  int L[height][width];

  // lower threshold
  for (int y = 0; y < height; y++){
    rgba8_t* lineptr = (rgba8_t*)(img_buffer + y * stride);
    for (int x = 0; x < width; x++){
      if (lineptr[x].r < threshold){
	L[y][x] = 0;
      }
      else {
	L[y][x] = y * width + x;
      }
    }
  }

  // get components
  bool is_changed = true;
  int mid_kernel = 1;

  while (is_changed){
    is_changed = false;
    for (int y = 0; y < height; y++){
      for (int x = 0; x < width; x++){
        for (int i = -mid_kernel; i <= mid_kernel; i++) {
	  for (int j = -mid_kernel; j <= mid_kernel; j++) {
	    if (i + x < 0 or i + x >= width or j + y < 0 or j + y >= height)
	      continue;
	    if (L[y][x] == 0)
	      continue;
	    if (L[j+y][i+x] == 0)
	      continue;
	    if (L[j+y][i+x] < L[y][x]){
	      L[y][x] = L[j+y][i+x];
	      is_changed = true;
	    }
	  }
	}
      }
    }
  }


  // count compnents and max values
  std::map<int, uint8_t> components;
  for (int y = 0; y < height; y++){
    for (int x = 0; x < width; x++){
      if (L[y][x] == 0)
	continue;
      uint8_t val = ((rgba8_t*)(img_buffer + y * stride))[x].r;
      if (components.find(L[y][x]) == components.end())
	components.insert({L[y][x], val});
      else if (components[L[y][x]] < val)
	components[L[y][x]] = val;
    }
  }

  // upper threshold
  uint8_t number_components = 0;
  for (auto i = components.begin(); i != components.end(); i++){
    if (i->second <= peak)
      i->second = 0;
    else {
      number_components++;
      i->second = number_components;
    }
  }

  // assign component number
  for (int y = 0; y < height; y++){
    for (int x = 0; x < width; x++){
      if (L[y][x] == 0)
	continue;
      L[y][x] = components[L[y][x]];
    }
  }
  
  for (int y = 0; y < height; y++){
    rgba8_t* lineptr = (rgba8_t*)(img_buffer + y * stride);
    for (int x = 0; x < width; x++){
      if (L[y][x] == 0){
	lineptr[x] = rgba8_t{0,0,0,255};
      }
      else {
	lineptr[x] = rgba8_t{255,255,255,255};
      }
    }
  }

  // get bbox coordinates
  int bbox[number_components][4];
  memset(bbox, -1, sizeof(int) * number_components * 4);

  for (int y = 0; y < height; y++){
    for (int x = 0; x < width; x++){
      if (L[y][x] == 0)
	continue;
      int cur_components = L[y][x] - 1;
      if(bbox[cur_components][0] == -1 or x < bbox[cur_components][0])
	bbox[cur_components][0] = x;
      if(bbox[cur_components][1] == -1 or y < bbox[cur_components][1])
	bbox[cur_components][1] = y;
      if(bbox[cur_components][2] == -1 or x > bbox[cur_components][2])
	bbox[cur_components][2] = x;
      if(bbox[cur_components][3] == -1 or y > bbox[cur_components][3])
	bbox[cur_components][3] = y;
    }
  }
  for (int i = 0; i < number_components; i++){
    bbox[i][2] -= bbox[i][0];
    bbox[i][3] -= bbox[i][1];
  }

  // add box to image
  for (int i = 0; i < number_components; i++){
    rgba8_t* lineptr = (rgba8_t*)(img_buffer + bbox[i][1] * stride);
    for (int j = 0; j <= bbox[i][2]; j++){
      lineptr[j + bbox[i][0]] = rgba8_t{255, 0, 0, 255};
    }

    lineptr = (rgba8_t*)(img_buffer + (bbox[i][1] + bbox[i][3]) * stride);
    for (int j = 0; j <= bbox[i][2]; j++){
      lineptr[j + bbox[i][0]] = rgba8_t{255, 0, 0, 255};
    }

    lineptr = (rgba8_t*)(img_buffer + (bbox[i][1]) * stride);
    for (int j = 0; j <= bbox[i][3]; j++){
      lineptr[bbox[i][0]] = rgba8_t{255, 0, 0, 255};
      lineptr[bbox[i][0] + bbox[i][2]] = rgba8_t{255, 0, 0, 255};
      lineptr = (rgba8_t*)((char*)lineptr + stride);
    }
  }
}


void render_cpu(char* ref_buffer, int width, int height, std::ptrdiff_t stride, char* img_buffer){}
