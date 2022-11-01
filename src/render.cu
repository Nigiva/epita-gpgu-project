#include "render.hpp"
#include <spdlog/spdlog.h>
#include <cassert>

    [[gnu::noinline]]
void _abortError(const char* msg, const char* fname, int line)
{
    cudaError_t err = cudaGetLastError();
    spdlog::error("{} ({}, line: {})", msg, fname, line);
    spdlog::error("Error {}: {}", cudaGetErrorName(err), cudaGetErrorString(err));
    std::exit(1);
}

#define abortError(msg) _abortError(msg, __FUNCTION__, __LINE__)


// Device code
__global__ void gpu_gray_scale(char* buffer, int width, int height, size_t pitch)
{
    int x = blockDim.x * blockIdx.x + threadIdx.x;
    int y = blockDim.y * blockIdx.y + threadIdx.y;

    if (x >= width || y >= height)
        return;

    // get cell
    rgba8_t*  lineptr = (rgba8_t*)(buffer + y * pitch);
    rgba8_t cell = lineptr[x];

    // get gray scale
    std::uint8_t gray = static_cast<std::uint8_t>(0.3 * cell.r + 0.59 * cell.g + 0.11 * cell.b);

    // assign gray pixel
    lineptr[x] = {gray, gray, gray, 255};
}

std::vector<std::vector<int>> render(char* ref_buffer, int width, int height, std::ptrdiff_t stride, char* img_buffer)
{
    cudaError_t rc = cudaSuccess;

    // Allocate device memory and copy reference image to device memory
    char*  devRefBuffer;
    size_t pitchRef;

    rc = cudaMallocPitch(&devRefBuffer, &pitchRef, width * sizeof(rgba8_t), height);
    if (rc)
        abortError("Fail buffer allocation");

    rc = cudaMemcpy2D(devRefBuffer, pitchRef, ref_buffer, stride, width * sizeof(rgba8_t), height, cudaMemcpyHostToDevice);
    if (rc)
        abortError("Fail buffer allocation");

    // Allocate device memory and copy current image to device memory
    char*  devImgBuffer;
    size_t pitchImg;

    rc = cudaMallocPitch(&devImgBuffer, &pitchImg, width * sizeof(rgba8_t), height);
    if (rc)
        abortError("Fail buffer allocation");

    rc = cudaMemcpy2D(devImgBuffer, pitchImg, img_buffer, stride, width * sizeof(rgba8_t), height, cudaMemcpyHostToDevice);
    if (rc)
        abortError("Fail buffer allocation");

    // Run the kernel with blocks of size 32 x 32
    {
        int bsize = 32;
        int w     = std::ceil((float)width / bsize);
        int h     = std::ceil((float)height / bsize);

        spdlog::debug("running kernel of size ({},{})", w, h);

        dim3 dimBlock(bsize, bsize);
        dim3 dimGrid(w, h);

       gpu_gray_scale<<<dimGrid, dimBlock>>>(devRefBuffer, width, height, pitchRef);
        if (cudaPeekAtLastError())
            abortError("Computation Error");

       gpu_gray_scale<<<dimGrid, dimBlock>>>(devImgBuffer, width, height, pitchImg);
        if (cudaPeekAtLastError())
            abortError("Computation Error");
    }

    // Copy back to main memory
    rc = cudaMemcpy2D(img_buffer, stride, devImgBuffer, pitchImg, width * sizeof(rgba8_t), height, cudaMemcpyDeviceToHost);
    if (rc)
        abortError("Unable to copy buffer back to memory");

    // Free
    rc = cudaFree(devRefBuffer);
    if (rc)
        abortError("Unable to free memory");

    // Free
    rc = cudaFree(devImgBuffer);
    if (rc)
        abortError("Unable to free memory");

    return {};
}
