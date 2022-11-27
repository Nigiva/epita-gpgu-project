#include <cstddef>
#include <memory>
#include <iostream>
#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <nlohmann/json.hpp>
#include "render.hpp"
#include "utils.hpp"

using json = nlohmann::json;

// Usage: ./detect
int main(int argc, char** argv)
{
    (void) argc;
    (void) argv;

    std::string reference_filename;
    std::vector<std::string> images_filename;
    std::string mode;
    std::string baseline;
    CLI::App app{"detect"};
    app.add_option("reference", reference_filename, "reference image")->required()->check(CLI::ExistingFile);
    app.add_option("inputs", images_filename, "input images")->required()->check(CLI::ExistingFile);
    app.add_set("-m", mode, {"GPU", "CPU"}, "Either 'GPU' or 'CPU'");
    app.add_set("-v", baseline, {"Baseline", "Opt"}, "Either 'Baseline' or 'Optimiser'")->default_val("Opt");

    CLI11_PARSE(app, argc, argv);

    //json::value result;

    // Create buffer
    int width;
    int height;
    int stride;
    char* ref_buffer = read_png(reference_filename.c_str(), &width, &height, &stride);
    char* img_buffer;

    // pretretment on refference image
    if (mode == "CPU")
    {
        gray_scale(ref_buffer, width, height, stride);
        gaussian_blur(ref_buffer, width, height, stride, 5);
    }
    else if (mode == "GPU")
    {
    }

    // json object containg cordonates
    json res;

    // Rendering
    //spdlog::info("Runnging {} mode with (w={},h={}).", mode, width, height);
    for (size_t i = 0; i < images_filename.size(); i += 1)
    {
        // read image
        img_buffer = read_png(images_filename[i].c_str(), NULL, NULL, NULL);

        if (mode == "CPU")
        {
            res[images_filename[i].c_str()] = render_cpu(ref_buffer, width, height, stride, img_buffer);

        }
        else if (mode == "GPU")
        {
            if (baseline == "Baseline"){
                res[images_filename[i].c_str()] = render(ref_buffer, width, height, stride, img_buffer, true);
            }
            else if (baseline == "Opt") {
                res[images_filename[i].c_str()] = render(ref_buffer, width, height, stride, img_buffer, false);
            }
      //      std::cerr << images_filename[i].c_str() << std::endl;
        }

        //write_png(img_buffer, width, height, stride, std::string("output").append(std::to_string(i)).append(std::string(".png")).c_str());
    }

    // print result
    std::cout << res.dump(2) << std::endl;
}
