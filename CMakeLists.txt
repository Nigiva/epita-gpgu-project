cmake_minimum_required (VERSION 3.14)
project (GPGPU CUDA CXX)

include(FetchContent)

# Download external dependencies
FetchContent_Declare(
  CLI11
  URL https://github.com/CLIUtils/CLI11/archive/v1.9.1.tar.gz
  )

FetchContent_Declare(
  spdlog
  URL https://github.com/gabime/spdlog/archive/v1.9.2.tar.gz
  )

FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
  )



FetchContent_MakeAvailable(spdlog)
FetchContent_MakeAvailable(CLI11)
FetchContent_MakeAvailable(json)

# Create data Dir and download videos
file(DOWNLOAD "https://cloud.lrde.epita.fr/s/oE5t9TpweK3cDBj/download?path=&files=Aled.mp4" "data/Aled.mp4")

# Check all packages are available
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR} )
find_package(PNG REQUIRED)
find_package(TBB REQUIRED)
find_package(benchmark REQUIRED)

# Define the build flags
set(CMAKE_CUDA_ARCHITECTURES "75")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Debug mode
# You must run `cmake -DCMAKE_BUILD_TYPE=Debug ..`
if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
  set(CMAKE_CUDA_FLAGS ${CMAKE_CUDA_FLAGS} "-g -G")
  message(">>> Building in debug mode ! <<<")
endif()


add_library(renderer)
target_compile_features(renderer PUBLIC cxx_std_14)
target_link_libraries(renderer PRIVATE spdlog::spdlog)


target_sources(renderer PRIVATE src/render.hpp src/render.cu src/render.cpp src/utils.hpp src/utils.cpp)
set_target_properties(renderer PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

add_library(render_cpu_ref SHARED IMPORTED)
target_link_libraries(render_cpu_ref INTERFACE tbb)
set_target_properties(render_cpu_ref PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/librender_cpu_ref.so)


### Create the executable ###
add_executable(detect src/detect.cpp)
target_compile_features(detect PUBLIC cxx_std_17)
target_link_libraries(detect PRIVATE render_cpu_ref PNG::PNG CLI11::CLI11 spdlog::spdlog renderer nlohmann_json::nlohmann_json)

add_executable(bench src/bench.cpp)
target_compile_features(bench PUBLIC cxx_std_17)
target_link_libraries(bench PRIVATE render_cpu_ref PNG::PNG spdlog::spdlog benchmark::benchmark renderer nlohmann_json::nlohmann_json)
