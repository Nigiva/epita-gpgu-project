# BENCHMARK

## benchmark CPU VS GPU v1
```
2022-11-05T10:48:13+01:00
Running ./bench
Run on (8 X 2295.66 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 64 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 8192 KiB (x8)
Load Average: 0.23, 0.13, 0.05
-------------------------------------------------------------------------------------
Benchmark                           Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------------------
BM_Rendering_cpu/real_time      46321 ms        46304 ms            1 frame_rate=0.0215884/s
BM_Rendering_gpu/real_time        861 ms          805 ms            1 frame_rate=1.16153/s
```

## Profiling
```
[2022-11-05 10:53:03.869] [info] Runnging GPU mode with (w=1920,h=1080).
==1240519== NVPROF is profiling process 1240519, command: ./detect -m GPU ../../export/reference.png ../../export/input-0424.png
==1240519== Profiling application: ./detect -m GPU ../../export/reference.png ../../export/input-0424.png
==1240519== Profiling result:
            Type  Time(%)      Time     Calls       Avg       Min       Max  Name
 GPU activities:   81.12%  248.83ms         4  62.207ms  18.666ms  105.74ms  erosion_dilation(char*, int, int, int, int, bool, bool, char*, int)
                   14.15%  43.406ms       334  129.96us  125.31us  161.24us  propagate_relabeling(int*, int, int, bool*, bool, int*)
                    2.00%  6.1323ms         2  3.0662ms  3.0653ms  3.0670ms  gpu_gaussian_blur(char*, int, int, unsigned long, double*, int, char*, unsigned long)
                    0.97%  2.9606ms         3  986.87us  1.2480us  1.5384ms  [CUDA memcpy HtoD]
                    0.59%  1.8144ms       340  5.3360us  1.0880us  1.4299ms  [CUDA memcpy DtoH]
                    0.45%  1.3668ms         1  1.3668ms  1.3668ms  1.3668ms  histogram(char*, int, int, int, int*)
                    0.33%  1.0261ms         2  513.03us  511.92us  514.13us  gpu_gray_scale(char*, int, int, unsigned long)
                    0.17%  518.80us         1  518.80us  518.80us  518.80us  gpu_difference(char*, int, int, unsigned long, char*, unsigned long)
                    0.11%  325.53us       340     957ns     896ns  1.2800us  [CUDA memset]
                    0.06%  176.60us         1  176.60us  176.60us  176.60us  get_bbox(int*, int, int, int*, int*, char*, int)
                    0.06%  175.52us         1  175.52us  175.52us  175.52us  thresholding(char*, int, int, int, int, int*)
      API calls:   70.97%  304.90ms       340  896.75us  10.349us  257.85ms  cudaMemcpy
                   26.41%  113.45ms         3  37.816ms  117.08us  113.15ms  cudaMallocPitch
                    1.10%  4.7174ms         3  1.5725ms  1.4733ms  1.6511ms  cudaMemcpy2D
                    0.83%  3.5649ms       346  10.303us  4.7790us  185.58us  cudaLaunchKernel
                    0.46%  1.9640ms       340  5.7760us  3.3060us  46.237us  cudaMemset
                    0.11%  470.79us        10  47.079us  4.4170us  177.15us  cudaFree
                    0.08%  332.10us         7  47.442us  4.4080us  173.07us  cudaMalloc
                    0.04%  162.74us       101  1.6110us     180ns  69.882us  cuDeviceGetAttribute
                    0.01%  46.457us         1  46.457us  46.457us  46.457us  cuDeviceGetName
                    0.00%  8.2250us         1  8.2250us  8.2250us  8.2250us  cuDeviceGetPCIBusId
                    0.00%  2.2340us         3     744ns     461ns  1.2730us  cuDeviceGetCount
                    0.00%  1.2120us         5     242ns     180ns     340ns  cudaPeekAtLastError
                    0.00%     972ns         2     486ns     281ns     691ns  cuDeviceGet
                    0.00%     501ns         1     501ns     501ns     501ns  cuDeviceTotalMem
                    0.00%     361ns         1     361ns     361ns     361ns  cuDeviceGetUuid
```

## Profiling Opti (Output privatizaztion)
```
==814011== Profiling result:
            Type  Time(%)      Time     Calls       Avg       Min       Max  Name
 GPU activities:   78.72%  216.08ms         4  54.020ms  17.436ms  97.375ms  erosion_dilation(char*, int, int, int, int, bool, bool, char*, int)
                   15.79%  43.335ms       334  129.75us  125.25us  138.84us  propagate_relabeling(int*, int, int, bool*, bool, int*)
                    2.23%  6.1303ms         2  3.0652ms  3.0647ms  3.0656ms  gpu_gaussian_blur(char*, int, int, unsigned long, double*, int, char*, unsigned long)
                    1.36%  3.7458ms         3  1.2486ms  1.4400us  1.9120ms  [CUDA memcpy HtoD]
                    0.68%  1.8700ms       340  5.4990us  1.1190us  1.4828ms  [CUDA memcpy DtoH]
                    0.38%  1.0372ms         1  1.0372ms  1.0372ms  1.0372ms  histogram(char*, int, int, int, int*)
                    0.37%  1.0266ms         2  513.32us  513.11us  513.52us  gpu_gray_scale(char*, int, int, unsigned long)
                    0.19%  520.95us         1  520.95us  520.95us  520.95us  gpu_difference(char*, int, int, unsigned long, char*, unsigned long)
                    0.14%  394.97us       340  1.1610us     896ns  1.6640us  [CUDA memset]
                    0.06%  172.32us         1  172.32us  172.32us  172.32us  get_bbox(int*, int, int, int*, int*, char*, int)
                    0.06%  164.83us         1  164.83us  164.83us  164.83us  thresholding(char*, int, int, int, int, int*)
      API calls:   65.00%  272.58ms       340  801.70us  11.010us  224.76ms  cudaMemcpy
                   32.45%  136.06ms         3  45.354ms  91.271us  135.63ms  cudaMallocPitch
                    1.32%  5.5378ms         3  1.8459ms  1.6687ms  1.9860ms  cudaMemcpy2D
                    0.59%  2.4713ms       346  7.1420us  5.4400us  66.033us  cudaLaunchKernel
                    0.33%  1.3765ms       340  4.0480us  3.2760us  18.454us  cudaMemset
                    0.15%  646.58us        10  64.657us  4.9780us  242.43us  cudaFree
                    0.09%  385.66us         7  55.094us  4.3380us  251.94us  cudaMalloc
                    0.04%  176.24us       101  1.7440us     169ns  86.110us  cuDeviceGetAttribute
                    0.02%  99.866us         1  99.866us  99.866us  99.866us  cuDeviceGetName
                    0.00%  9.0370us         1  9.0370us  9.0370us  9.0370us  cuDeviceGetPCIBusId
                    0.00%  3.9870us         3  1.3290us     461ns  2.9950us  cuDeviceGetCount
                    0.00%  1.3430us         5     268ns     181ns     430ns  cudaPeekAtLastError
                    0.00%  1.0720us         2     536ns     240ns     832ns  cuDeviceGet
                    0.00%     681ns         1     681ns     681ns     681ns  cuDeviceTotalMem
                    0.00%     380ns         1     380ns     380ns     380ns  cuDeviceGetUuid
```
