#include "render.hpp"
#include <spdlog/spdlog.h>
#include <cassert>
#include "utils.hpp"

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
__global__ void gpu_gaussian_blur(char* img_buffer, int width, int height, size_t img_pitch, double* gaussian_kernel, int kernel_size, char* res_buffer, size_t res_pitch)
{
    int x = blockDim.x * blockIdx.x + threadIdx.x;
    int y = blockDim.y * blockIdx.y + threadIdx.y;

    if (x >= width || y >= height)
        return;

    int mid_kernel = (kernel_size - 1) / 2;

    // get pixel value
    char* base_ptr = img_buffer + y * img_pitch;
    double gaussian_pixel = 0.0;
    for (int i = -mid_kernel; i <= mid_kernel; i++) {
        for (int j = -mid_kernel; j <= mid_kernel; j++) {
            if (i + x < 0 or i + x >= width or j + y < 0 or j + y >= height)
                continue;
            std::uint8_t cell = ((rgba8_t*)(base_ptr + j * img_pitch))[i + x].r;
            gaussian_pixel += gaussian_kernel[kernel_size * (j + mid_kernel) + (i + mid_kernel)] * cell;
        }
    }
    std::uint8_t cast_gaussian_pixel = (std::uint8_t) gaussian_pixel;

    // apply pixel value
    rgba8_t* base_ptr2 = (rgba8_t*)(res_buffer + y * res_pitch);
    base_ptr2[x] = rgba8_t{cast_gaussian_pixel, cast_gaussian_pixel, cast_gaussian_pixel, 255};
}

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

    // Allocate device memory and create/copy gaussian kernel
    double*  gaussianKernel;
    int kernel_size = 5; // gaussian kernel of size 5

    rc = cudaMalloc(&gaussianKernel, kernel_size * sizeof(double) * kernel_size);
    if (rc)
        abortError("Fail buffer allocation");

    // get the gaussian kernel
    double sigma = 1.0;
    double* kernel = (double*)malloc(sizeof(double) * kernel_size * kernel_size);
    gaussian_kernel(kernel, sigma, kernel_size);

    rc = cudaMemcpy(gaussianKernel, kernel, kernel_size * sizeof(double) * kernel_size, cudaMemcpyHostToDevice);
    if (rc)
        abortError("Fail buffer allocation");

    free(kernel);

    // Allocate device memory to store tmp images
    char* devTmpBuffer;
    char* tmp_buff;
    size_t pitchTmp, tmp_pitch;

    rc = cudaMallocPitch(&devTmpBuffer, &pitchTmp, width * sizeof(rgba8_t), height);
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

        // apply gray scale to images
        gpu_gray_scale<<<dimGrid, dimBlock>>>(devRefBuffer, width, height, pitchRef);
        if (cudaPeekAtLastError())
            abortError("Computation Error");
        gpu_gray_scale<<<dimGrid, dimBlock>>>(devImgBuffer, width, height, pitchImg);
        if (cudaPeekAtLastError())
            abortError("Computation Error");

        // apply gaussian blur to images
        gpu_gaussian_blur<<<dimGrid, dimBlock>>>(devRefBuffer, width, height, pitchRef, gaussianKernel, kernel_size, devTmpBuffer, pitchTmp);
        if (cudaPeekAtLastError())
            abortError("Computation Error");
        // Swap buffers
        tmp_buff = devRefBuffer;
        tmp_pitch = pitchRef;
        devRefBuffer = devTmpBuffer;
        pitchRef = pitchTmp;
        devTmpBuffer = tmp_buff;
        pitchTmp = tmp_pitch;

        gpu_gaussian_blur<<<dimGrid, dimBlock>>>(devImgBuffer, width, height, pitchImg, gaussianKernel, kernel_size, devTmpBuffer, pitchTmp);
        if (cudaPeekAtLastError())
            abortError("Computation Error");

        // Swap buffers
        tmp_buff = devImgBuffer;
        tmp_pitch = pitchImg;
        devImgBuffer = devTmpBuffer;
        pitchImg = pitchTmp;
        devTmpBuffer = tmp_buff;
        pitchTmp = tmp_pitch;
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

    // Free
    rc = cudaFree(gaussianKernel);
    if (rc)
        abortError("Unable to free memory");

    return {};
}
