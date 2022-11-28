#include "render.hpp"
#include <spdlog/spdlog.h>
#include <cassert>
#include "utils.hpp"
#include <math.h>
#include <thrust/device_vector.h>
#include <iostream>

#define ABS_MIN(a, b) ((a>=b)?(a-b):(b-a))

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


__global__ void gpu_difference(char* ref_buffer, int width, int height, size_t ref_pitch, char* img_buffer, size_t img_pitch)
{
    int x = blockDim.x * blockIdx.x + threadIdx.x;
    int y = blockDim.y * blockIdx.y + threadIdx.y;

    if (x >= width || y >= height)
        return;

    // get reference and image cell
    rgba8_t* ref_lineptr = (rgba8_t*)(ref_buffer + y * ref_pitch);
    rgba8_t ref_cell = ref_lineptr[x];
    rgba8_t* img_lineptr = (rgba8_t*)(img_buffer + y * img_pitch);
    rgba8_t img_cell = img_lineptr[x];

    std::uint8_t r = ABS_MIN(ref_cell.r, img_cell.r);
    std::uint8_t g = ABS_MIN(ref_cell.g, img_cell.g);
    std::uint8_t b = ABS_MIN(ref_cell.b, img_cell.b);

    // assign diff pixel to image buffer
    img_lineptr[x] = {r, g, b, 255};
}

__global__ void erosion_dilation(char *img_buffer, int width, int height, int img_pitch, int radius, bool is_square, bool is_erosion, char* res_buffer, int res_pitch, bool is_baseline)
{
    if (!is_baseline){
        // Define shared memory

        extern __shared__ std::uint8_t tile[];


        int x = blockDim.x * blockIdx.x + threadIdx.x;
        int y = blockDim.y * blockIdx.y + threadIdx.y;

        if (x >= width || y >= height)
            return;

        int base_x = blockDim.x * blockIdx.x;
        int base_y = blockDim.y * blockIdx.y;

        std::uint8_t* block_ptr = (std::uint8_t*)((img_buffer + (base_x - radius) * sizeof(rgba8_t)) + (base_y - radius) * img_pitch);
        int base_width = blockDim.x + 2 * radius;

        for (int j = threadIdx.y;  j < base_width; j+= blockDim.y){
            for (int i = threadIdx.x;  i < base_width; i+= blockDim.x){
                if (base_x - radius + i >= 0 and base_y - radius + j >= 0 and base_x - radius + i < width and base_y - radius + j < height)
                    tile[i + j * base_width] = block_ptr[i * sizeof(rgba8_t) + j * img_pitch];
            }
        }
        __syncthreads();

        std::uint8_t val = (is_erosion) ? 255 : 0;

        // iteration on each pixel around current pixel
        for (int i = -radius; i <= radius; i++){
            for (int j = -radius; j <= radius; j++){
                // if outside the image
                if (i + x < 0 or i + x >= width or j + y < 0 or j + y >= height)
                    continue;

                // if disc and not in it
                if (!is_square and !((int)sqrtf(i*i + j*j) <= radius))
                    continue;

                std::uint8_t cell = tile[x - base_x + i + radius + (y - base_y + j + radius) * base_width];
                if (is_erosion and val > cell){
                    val = cell;
                }
                else if (!is_erosion and val < cell){
                    val = cell;
                }
            }
        }

        // apply pixel value
        rgba8_t* base_ptr2 = (rgba8_t*)(res_buffer + y * res_pitch);
        base_ptr2[x] = rgba8_t{val, val, val, 255};
    }
    else{
        int x = blockDim.x * blockIdx.x + threadIdx.x;
        int y = blockDim.y * blockIdx.y + threadIdx.y;

        if (x >= width || y >= height)
            return;

        char* base_ptr = img_buffer + y * img_pitch;

        std::uint8_t val = (is_erosion) ? 255 : 0;

        // iteration on each pixel around current pixel
        for (int i = -radius; i <= radius; i++){
            for (int j = -radius; j <= radius; j++){
                // if outside the image
                if (i + x < 0 or i + x >= width or j + y < 0 or j + y >= height)
                    continue;

                // if disc and not in it
                if (!is_square and !((int)sqrtf(i*i + j*j) <= radius))
                    continue;
                std::uint8_t cell = ((rgba8_t*)(base_ptr + j * img_pitch))[i + x].r;
                if (is_erosion and val > cell){
                    val = cell;
                }
                else if (!is_erosion and val < cell){
                    val = cell;
                }
            }
        }

        // apply pixel value
        rgba8_t* base_ptr2 = (rgba8_t*)(res_buffer + y * res_pitch);
        base_ptr2[x] = rgba8_t{val, val, val, 255};
    }
}

__global__ void histogram(char* img_buffer, int width, int height, int img_pitch, int* histo)
{
    int x = blockDim.x * blockIdx.x + threadIdx.x;
    int y = blockDim.y * blockIdx.y + threadIdx.y;

    if (x >= width || y >= height)
        return;

    std::uint8_t cellValue = ((rgba8_t*)(img_buffer + y * img_pitch))[x].r;
    atomicAdd(histo + cellValue, 1);
}

__global__ void thresholding(char* img_buffer, int width, int height, int img_pitch, int threshold, int* L)
{
    int x = blockDim.x * blockIdx.x + threadIdx.x;
    int y = blockDim.y * blockIdx.y + threadIdx.y;

    if (x >= width || y >= height)
        return;


    rgba8_t* lineptr = (rgba8_t*)(img_buffer + y * img_pitch);
    if (lineptr[x].r < threshold){
        L[y * width + x] = 0;
    }
    else {
        L[y * width + x] = y * width + x + 1;
    }
}

__global__ void propagate_relabeling(int* L, int width, int height, bool* is_changed, bool relabeling, int* nb_components){

    __shared__ bool changed;
    int x = blockDim.x * blockIdx.x + threadIdx.x;
    int y = blockDim.y * blockIdx.y + threadIdx.y;

    if (x >= width || y >= height)
        return;

    if (L[y * width + x] == 0)
        return;

    if (relabeling and L[y * width + x] == y * width + x + 1){
        *is_changed = true;
        // relabeling
        L[y * width + x] = atomicAdd(nb_components, 1) + 1;
        return;
    }

    int mid_kernel = 1;

    // Propagate
    do
    {
        changed = false;
        __syncthreads();
        // look pixels around
        for (int i = -mid_kernel; i <= mid_kernel; i++) {
            for (int j = -mid_kernel; j <= mid_kernel; j++) {
                if (i + x < 0 or i + x >= width or j + y < 0 or j + y >= height)
                    continue;
                if (L[(j+y) * width + i+x] == 0)
                    continue;
                if (L[(j+y) * width + i+x] < L[y * width + x]){
                    L[y * width + x] = L[(j+y) * width + i+x];
                    *is_changed = true;
                    changed = true;
                }
            }
        }
        __syncthreads();
    }while (changed);
}
__global__ void get_bbox(int* L, int width, int height, int* max_values, int* bbox, char* img_buffer, int pitch)
{
    int x = blockDim.x * blockIdx.x + threadIdx.x;
    int y = blockDim.y * blockIdx.y + threadIdx.y;

    if (x >= width || y >= height)
        return;
    if (L[y * width + x] == 0)
        return;

    int component = L[y * width + x];

    uint8_t value = ((rgba8_t*)(img_buffer + y * pitch))[x].r;

    // put max value for the current component
    atomicMax(max_values + component - 1, (int)value);

    // change bbox values if necessary

    if (y - 1 >= 0 and value != ((rgba8_t*)(img_buffer + (y - 1) * pitch))[x].r)
        atomicMin(bbox + (component - 1) * 4 + 1, y);
    if (y + 1 < height and value != ((rgba8_t*)(img_buffer + (y + 1) * pitch))[x].r)
        atomicMax(bbox + (component - 1) * 4 + 3, y);
    if (x - 1 >= 0 and value != ((rgba8_t*)(img_buffer + y * pitch))[x - 1].r)
        atomicMin(bbox + (component - 1) * 4, x);
    if (x + 1 < width and value != ((rgba8_t*)(img_buffer + y * pitch))[x + 1].r)
        atomicMax(bbox + (component - 1) * 4 + 2, x);
}

std::vector<std::vector<int>> render(char* ref_buffer, int width, int height, std::ptrdiff_t stride, char* img_buffer, bool is_baseline)
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

    // Allocate device memory for histogram
    int* histoBuffer;

    rc = cudaMalloc(&histoBuffer, 256 * sizeof(int));
    if (rc)
        abortError("Fail buffer allocation");
    rc = cudaMemset(histoBuffer, 0, 256 * sizeof(int));
    if (rc)
        abortError("Fail buffer allocation");

    // Allocate device memory for L
    int* L;

    rc = cudaMalloc(&L, width * height * sizeof(int));
    if (rc)
        abortError("Fail buffer allocation");

    // Allocate device memory for share information between CPU and GPU
    bool* is_changed;

    rc = cudaMalloc(&is_changed, sizeof(bool));
    if (rc)
        abortError("Fail buffer allocation");
    rc = cudaMemset(is_changed, true, sizeof(bool));
    if (rc)
        abortError("Fail buffer allocation");

    int* nb_components;

    rc = cudaMalloc(&nb_components, sizeof(int));
    if (rc)
        abortError("Fail buffer allocation");
    rc = cudaMemset(nb_components, 0, sizeof(int));
    if (rc)
        abortError("Fail buffer allocation");

    std::vector<std::vector<int>> result;


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

        // difference
        gpu_difference<<<dimGrid, dimBlock>>>(devRefBuffer, width, height, pitchRef, devImgBuffer, pitchImg);

        // calculate adaptative closing opening radius
        double closing_radius = width * height * 10 / (1920 * 1080);
        double opening_radius = width * height * 25 / (1920 * 1080);

        // Run optimized version or not
        if (!is_baseline){
            // perform morphology closing and opening
            // closing
            erosion_dilation<<<dimGrid, dimBlock, (bsize + 2 * (int)closing_radius) * (bsize + 2 * (int)closing_radius) * sizeof(std::uint8_t)>>>(devImgBuffer, width, height, pitchImg, (int)closing_radius, false, false, devTmpBuffer, pitchTmp, false);
            erosion_dilation<<<dimGrid, dimBlock, (bsize + 2 * (int)closing_radius) * (bsize + 2 * (int)closing_radius) * sizeof(std::uint8_t)>>>(devTmpBuffer, width, height, pitchTmp, (int)closing_radius, false, true, devImgBuffer, pitchImg, false);
            //opening
            erosion_dilation<<<dimGrid, dimBlock, (bsize + 2 * (int)opening_radius) * (bsize + 2 * (int)opening_radius) * sizeof(std::uint8_t)>>>(devImgBuffer, width, height, pitchImg, (int)opening_radius, false, true, devTmpBuffer, pitchTmp, false);
            erosion_dilation<<<dimGrid, dimBlock, (bsize + 2 * (int)opening_radius) * (bsize + 2 * (int)opening_radius) * sizeof(std::uint8_t)>>>(devTmpBuffer, width, height, pitchTmp, (int)opening_radius, false, false, devImgBuffer, pitchImg, false);
        }
        else{
            // closing
            erosion_dilation<<<dimGrid, dimBlock>>>(devImgBuffer, width, height, pitchImg, (int)closing_radius, false, false, devTmpBuffer, pitchTmp, true);
            erosion_dilation<<<dimGrid, dimBlock>>>(devTmpBuffer, width, height, pitchTmp, (int)closing_radius, false, true, devImgBuffer, pitchImg, true);
            //opening
            erosion_dilation<<<dimGrid, dimBlock>>>(devImgBuffer, width, height, pitchImg, (int)opening_radius, false, true, devTmpBuffer, pitchTmp, true);
            erosion_dilation<<<dimGrid, dimBlock>>>(devTmpBuffer, width, height, pitchTmp, (int)opening_radius, false, false, devImgBuffer, pitchImg, true);

        }
        // get histogram of the image
        histogram<<<dimGrid, dimBlock>>>(devImgBuffer, width, height, pitchImg, histoBuffer);

        // copy histogram from device to host
        int* histoHostBuffer = (int*)malloc(256 * sizeof(int));
        // Here is the probleme
        rc = cudaMemcpy(histoHostBuffer, histoBuffer, 256 * sizeof(int), cudaMemcpyDeviceToHost);

        if (rc)
            abortError("Unable to copy buffer back to memory");

        // otsu: first threshold
        // (cumpute on Host !)
        int threshold_1 = otsu(width, height, histoHostBuffer);

        if (threshold_1 < 5)
            threshold_1 = 5;

        // puts zeros in the histogram for elements in [0:threshold_1]
        int N = 0;
        for (int i = 0; i <= threshold_1; i++){
            N += histoHostBuffer[i];
            histoHostBuffer[i] = 0;
        }

        // otsu: second threshold
        // (cumpute on Host !)
        int threshold_2 = otsu(1, width * height - N, histoHostBuffer);

        if (threshold_2 < 10)
            threshold_2 = 10;

        free(histoHostBuffer);


        // apply thresholding
        thresholding<<<dimGrid, dimBlock>>>(devImgBuffer, width, height, pitchImg, threshold_1, L);

        // Apply propagate
        bool* is_changed_host = (bool*)malloc(sizeof(bool));
        *is_changed_host = true;

        bool create_comp = false;
        for (int i = 0; i <= 1; i++){
            create_comp = (bool) i;
            while (*is_changed_host){
                rc = cudaMemset(is_changed, false, sizeof(bool));
                if (rc)
                    abortError("Fail buffer allocation");
                propagate_relabeling<<<dimGrid, dimBlock>>>(L, width, height, is_changed, create_comp, nb_components);
                create_comp = false;

                rc = cudaMemcpy(is_changed_host, is_changed, sizeof(bool), cudaMemcpyDeviceToHost);
                if (rc)
                    abortError("Unable to copy buffer back to memory");
            }
            *is_changed_host = true;
        }
        free(is_changed_host);


        int* nb_components_host = (int*)malloc(sizeof(int));
        rc = cudaMemcpy(nb_components_host, nb_components, sizeof(int), cudaMemcpyDeviceToHost);
        if (rc)
            abortError("Unable to copy buffer back to memory");

        if (*nb_components_host != 0){
            // Apply bbox
            int* bbox;
            rc = cudaMalloc(&bbox, 4 * (*nb_components_host) * sizeof(int));
            if (rc)
                abortError("Fail buffer allocation");
            rc = cudaMemset(bbox, 0, 4 * (*nb_components_host) * sizeof(int));
            if (rc)
                abortError("Fail buffer allocation");

            for (int i = 0; i < *nb_components_host; i++)
            {
                rc = cudaMemset(bbox + i * 4, 127, 2 * sizeof(int));
                if (rc)
                    abortError("Fail buffer allocation");
            }

            int* max_values;
            rc = cudaMalloc(&max_values, (*nb_components_host) * sizeof(int));
            if (rc)
                abortError("Fail buffer allocation");
            rc = cudaMemset(max_values, 0, (*nb_components_host) * sizeof(int));
            if (rc)
                abortError("Fail buffer allocation");

            get_bbox<<<dimGrid, dimBlock>>>(L, width, height, max_values, bbox, devImgBuffer, pitchImg);
            if (cudaPeekAtLastError())
                abortError("Computation Error");

            int* bbox_host = (int*)malloc(4 * (*nb_components_host) * sizeof(int));
            int* max_values_host = (int*)malloc((*nb_components_host) * sizeof(int));

            rc = cudaMemcpy(bbox_host, bbox, 4 * (*nb_components_host) * sizeof(int), cudaMemcpyDeviceToHost);
            if (rc)
                abortError("Unable to copy buffer back to memory");

            rc = cudaMemcpy(max_values_host, max_values, (*nb_components_host) * sizeof(int), cudaMemcpyDeviceToHost);
            if (rc)
                abortError("Unable to copy buffer back to memory");

            for (int i = 0; i < *nb_components_host; i++)
            {
                if (max_values_host[i] >= threshold_2)
                {
                    std::vector<int> cur_bbox = {bbox_host[i * 4], bbox_host[i * 4 + 1], bbox_host[i * 4 + 2] - bbox_host[i * 4], bbox_host[i * 4 + 3] - bbox_host[i * 4 + 1]};
                    result.push_back(cur_bbox);
                }
            }

            // Free
            rc = cudaFree(bbox);
            if (rc)
                abortError("Unable to free memory");
            rc = cudaFree(max_values);
            if (rc)
                abortError("Unable to free memory");

            free(bbox_host);
            free(max_values_host);
        }

        free(nb_components_host);
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

    // Free
    rc = cudaFree(devTmpBuffer);
    if (rc)
        abortError("Unable to free memory");

    // Free
    rc = cudaFree(histoBuffer);
    if (rc)
        abortError("Unable to free memory");

    // Free
    rc = cudaFree(L);
    if (rc)
        abortError("Unable to free memory");
    // Free
    rc = cudaFree(is_changed);
    if (rc)
        abortError("Unable to free memory");

    rc = cudaFree(nb_components);
    if (rc)
        abortError("Unable to free memory");
    return result;
}
