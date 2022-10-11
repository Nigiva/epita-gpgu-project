#pragma once
#include <cstddef>
#include <memory>
#include <cstdint>

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
void render_cpu(char* buffer, int width, int height, std::ptrdiff_t stride);



/// \param buffer The RGBA24 image buffer
/// \param width Image width
/// \param height Image height
/// \param stride Number of bytes between two lines
void render(char* buffer, int width, int height, std::ptrdiff_t stride);
