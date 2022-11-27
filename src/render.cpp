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


std::vector<std::vector<int>> bbox(char *img_buffer, int width, int height, int stride, int threshold, int peak){

    int L[height][width];

    // lower threshold
    for (int y = 0; y < height; y++){
        rgba8_t* lineptr = (rgba8_t*)(img_buffer + y * stride);
        for (int x = 0; x < width; x++){
            if (lineptr[x].r < threshold){
                L[y][x] = 0;
            }
            else {
                L[y][x] = y * width + x + 1;
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

    std::vector<std::vector<int>> result = {};

    for (int i = 0; i < number_components; i++){
        result.push_back({bbox[i][0], bbox[i][1], bbox[i][2], bbox[i][3]});
    }

    return result;
}


std::vector<std::vector<int>> render_cpu(char* ref_buffer, int width, int height, int stride, char* img_buffer){
    // pretretment on current image
    gray_scale(img_buffer, width, height, stride);
    gaussian_blur(img_buffer, width, height, stride, 5);

    // difference between reference and current image
    images_diff(ref_buffer, width, height, stride, img_buffer);


    // calculate adaptative closing opening radius
    double closing_radius = width * height * 10 / (1920 * 1080);
    double opening_radius = width * height * 25 / (1920 * 1080);

    // perform morphology closing and opening
    closing(img_buffer, width, height, stride, (int)closing_radius, false);
    opening(img_buffer, width, height, stride, (int)opening_radius, false);

    // hysteresis (threasholding) with otsu method
    int threshold;
    int peak;
    hysteresis(img_buffer, width, height, stride, &threshold, &peak);
    //min threshold for very similar images
    if (threshold < 5)
        threshold = 5;
    //min peak for very similar images
    if (peak < 10)
        peak = 10;
    // get bounding boxes
    return bbox(img_buffer, width, height, stride, threshold, peak);
}

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
