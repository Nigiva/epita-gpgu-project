#include <cstddef>
#include <memory>

#include <png.h>
#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>
#include "render.hpp"

char* read_png(const char*filename,
                    int* file_width,
                    int* file_height,
                    int* file_stride)
{
  // read the file
  FILE *fp = fopen(filename, "rb");

  png_byte bit_depth;
  png_byte color_type;
  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info = png_create_info_struct(png);
  png_init_io(png, fp);
  png_read_info(png, info);

  int width = png_get_image_width(png, info);
  if (file_width != NULL)
      *file_width = width;
  int height = png_get_image_height(png, info);
  if (file_height != NULL)
      *file_height = height;
  color_type = png_get_color_type(png, info);
  bit_depth = png_get_bit_depth(png, info);

  if(bit_depth == 16)
    png_set_strip_16(png);
  if(color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_palette_to_rgb(png);
  if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
      png_set_expand_gray_1_2_4_to_8(png);
  if(png_get_valid(png, info, PNG_INFO_tRNS))
      png_set_tRNS_to_alpha(png);
  if(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
  if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      png_set_gray_to_rgb(png);

  png_read_update_info(png, info);

  int numchan = 4;
  if (file_stride != NULL)
      *file_stride = width * numchan;

  // Set up row pointer
  png_bytep *row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  int i, j;
  for (i = 0; i < height; i++)
      row_pointers[i] = (png_bytep)malloc(png_get_rowbytes(png,info));
  png_read_image(png, row_pointers);

  // Put row pointers data into image
  unsigned char *image = (unsigned char *) malloc (numchan*width*height);
  int count = 0;
  for (i = 0 ; i < height ; i++)
  {
      for (j = 0 ; j < numchan*width ; j++)
      {
          image[count] = row_pointers[i][j];
          count += 1;
      }
  }
  fclose(fp);
  for (i = 0; i < height; i++)
      free(row_pointers[i]);
  free(row_pointers);

  return (char*)(image);
}


void write_png(char* buffer,
               int width,
               int height,
               int stride,
               const char* filename)
{
  png_structp png_ptr =
    png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

  if (!png_ptr)
    return;

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    png_destroy_write_struct(&png_ptr, nullptr);
    return;
  }

  FILE* fp = fopen(filename, "wb");
  png_init_io(png_ptr, fp);

  png_set_IHDR(png_ptr, info_ptr,
               width,
               height,
               8,
               PNG_COLOR_TYPE_RGB_ALPHA,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png_ptr, info_ptr);
  for (int i = 0; i < height; ++i)
  {
    png_write_row(png_ptr, reinterpret_cast<png_const_bytep>(buffer));
    buffer += stride;
  }

  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, nullptr);
  fclose(fp);
}


// Usage: ./detect
int main(int argc, char** argv)
{
  (void) argc;
  (void) argv;

  std::string reference_filename;
  std::vector<std::string> images_filename;
  std::string mode;

  CLI::App app{"detect"};
  app.add_option("reference", reference_filename, "reference image")->required()->check(CLI::ExistingFile);
  app.add_option("inputs", images_filename, "input images")->required()->check(CLI::ExistingFile);
  app.add_set("-m", mode, {"GPU", "CPU"}, "Either 'GPU' or 'CPU'");

  CLI11_PARSE(app, argc, argv);

  //json::value result;

  // Create buffer
  int width;
  int height;
  int stride;
  char* ref_buffer = read_png(reference_filename.c_str(), &width, &height, &stride);
  gray_scale(ref_buffer, width, height, stride);
  write_png(ref_buffer, width, height, stride, "output42.png");

  char* img_buffer;

  // Rendering
  spdlog::info("Runnging {} mode with (w={},h={}).", mode, width, height);
  for (int i = 0; i < images_filename.size(); i += 1)
  {
    img_buffer = read_png(images_filename[i].c_str(), NULL, NULL, NULL);

    if (mode == "CPU")
    {
      render_cpu(ref_buffer, width, height, stride, img_buffer);
    }
    else if (mode == "GPU")
    {
      render(ref_buffer, width, height, stride, img_buffer);
    }
  }

  //std::cout << result.asString() << "\n";
}

