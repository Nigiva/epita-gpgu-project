#pragma once
#include <cstddef>
#include <memory>
#include <cstdint>
#include <vector>
#include <png.h>

struct rgba8_t {
  std::uint8_t r;
  std::uint8_t g;
  std::uint8_t b;
  std::uint8_t a;
};


/// \param buffer The RGBA24 image buffer
/// \param width Image width
/// \param height Image height
/// \param stride Number of bytes between two lines
void gray_scale(char* buffer, int width, int height, int stride);

/// \param buffer The RGBA24 image buffer
/// \param width Image width
/// \param height Image height
/// \param stride Number of bytes between two lines
/// \param kernel_size Size of gaussian kernel
void gaussian_blur(char* buffer, int width, int height, int stride, int kernel_size);


/// \param buffer The RGBA24 image buffer
/// \param width Image width
/// \param height Image height
/// \param stride Number of bytes between two lines
std::vector<std::vector<int>> render_cpu(char* ref_buffer, int width, int height, int stride, char* img_buffer);



/// \param buffer The RGBA24 image buffer
/// \param width Image width
/// \param height Image height
/// \param stride Number of bytes between two lines
std::vector<std::vector<int>> render(char* ref_buffer, int width, int height, std::ptrdiff_t stride, char* img_buffer);

/// \param buffer The RGBA24 image buffer
/// \param width Image width
/// \param height Image height
/// \param stride Number of bytes between two lines
void images_diff(char *ref_buffer, int width, int height, int stride, char* img_buffer);


/// \param img_buffer The image to change
/// \param width The image's width
/// \param height The image's height
/// \param stride The image's stride
/// \param radius The window's radius
/// \param is_square True if the window is a square, False for a disc
void opening(char* img_buffer, int width, int height, int stride, int radius, bool is_square);

/// \param img_buffer The image to change
/// \param width The image's width
/// \param height The image's height
/// \param stride The image's stride
/// \param radius The window's radius
/// \param is_square True if the window is a square, False for a disc
void closing(char* img_buffer, int width, int height, int stride, int radius, bool is_square);


/// \param img_buffer The image to change
/// \param width The image's width
/// \param height The image's height
/// \param stride The image's stride
/// \param threshold minimum difference
/// \param peak boxes must contain a value >= peak
std::vector<std::vector<int>> bbox(char *img_buffer, int width, int height, int stride, int threshold, int peak);


/// \param filename The path to the file
/// \param file_width A pointer points to file's width
/// \param file_height A pointer points to file' height
/// \param file_stride A pointer points to file's stride
char* read_png(const char*filename, int* file_width, int* file_height, int* file_stride);


/// \param buffer The image to change
/// \param width The image's width
/// \param height The image's height
/// \param stride The image's stride
/// \param filename The path to the file
void write_png(char* buffer, int width, int height, int stride, const char* filename);
