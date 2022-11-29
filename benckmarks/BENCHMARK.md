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
==2796100== Profiling application: ./detect -m GPU -v baseline data/export/reference.png data/export/input-0424.png
==2796100== Profiling result:
            Type  Time(%)      Time     Calls       Avg       Min       Max  Name
 GPU activities:   92.47%  246.51ms         4  61.627ms  18.961ms  107.28ms  erosion_dilation(char*, int, int, int, int, bool, bool, char*, int, bool)
                    2.57%  6.8543ms        42  163.20us  135.07us  348.60us  propagate_relabeling(int*, int, int, bool*, bool, int*)
                    2.28%  6.0771ms         2  3.0385ms  3.0375ms  3.0396ms  gpu_gaussian_blur(char*, int, int, unsigned long, double*, int, char*, unsigned long)
                    1.04%  2.7665ms         3  922.15us  1.4720us  1.4028ms  [CUDA memcpy HtoD]
                    0.52%  1.3915ms        47  29.606us  1.2160us  1.3331ms  [CUDA memcpy DtoH]
                    0.39%  1.0450ms         1  1.0450ms  1.0450ms  1.0450ms  histogram(char*, int, int, int, int*)
                    0.38%  1.0200ms         2  510.00us  509.91us  510.10us  gpu_gray_scale(char*, int, int, unsigned long)
                    0.19%  518.58us         1  518.58us  518.58us  518.58us  gpu_difference(char*, int, int, unsigned long, char*, unsigned long)
                    0.07%  176.28us         1  176.28us  176.28us  176.28us  get_bbox(int*, int, int, int*, int*, char*, int)
                    0.06%  164.25us         1  164.25us  164.25us  164.25us  thresholding(char*, int, int, int, int, int*)
                    0.02%  53.087us        48  1.1050us     960ns  1.5360us  [CUDA memset]
      API calls:   64.87%  270.35ms        47  5.7520ms  11.933us  255.10ms  cudaMemcpy
                   33.48%  139.53ms         3  46.508ms  167.45us  139.00ms  cudaMallocPitch
                    1.11%  4.6123ms         3  1.5374ms  1.4914ms  1.6112ms  cudaMemcpy2D
                    0.16%  657.61us        10  65.760us  4.7890us  263.66us  cudaFree
                    0.13%  553.53us        54  10.250us  5.4610us  89.718us  cudaLaunchKernel
                    0.11%  441.27us         7  63.038us  4.5580us  254.66us  cudaMalloc
                    0.06%  266.26us        48  5.5470us  4.0280us  33.113us  cudaMemset
                    0.05%  210.05us       101  2.0790us     170ns  102.00us  cuDeviceGetAttribute
                    0.02%  73.528us         1  73.528us  73.528us  73.528us  cuDeviceGetName
                    0.01%  24.346us         5  4.8690us     201ns  23.334us  cudaPeekAtLastError
                    0.00%  10.299us         1  10.299us  10.299us  10.299us  cuDeviceGetPCIBusId
                    0.00%  3.2970us         3  1.0990us     321ns  2.5050us  cuDeviceGetCount
                    0.00%  1.0230us         2     511ns     231ns     792ns  cuDeviceGet
                    0.00%     982ns         1     982ns     982ns     982ns  cuDeviceTotalMem
                    0.00%     371ns         1     371ns     371ns     371ns  cuDeviceGetUuid
```


## benchmark CPU VS GPU v2
```
Run on (8 X 2295.64 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 64 KiB (x8)
  L2 Unified 512 KiB (x8)
  L3 Unified 8192 KiB (x8)
Load Average: 1.49, 1.23, 0.97
-------------------------------------------------------------------------------------
Benchmark                           Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------------------
BM_Rendering_cpu/real_time      47637 ms        47629 ms            1 frame_rate=0.0209922/s
BM_Rendering_gpu/real_time        852 ms          796 ms            1 frame_rate=1.17344/s
```

## Profiling Opti (Output privatizaztion)
```
==2348346== Profiling application: ./detect -m GPU data/export/reference.png data/export/input-0424.png
==2348346== Profiling result:
            Type  Time(%)      Time     Calls       Avg       Min       Max  Name
 GPU activities:   91.84%  224.35ms         4  56.087ms  17.312ms  98.905ms  erosion_dilation(char*, int, int, int, int, bool, bool, char*, int, bool)
                    2.77%  6.7774ms        42  161.37us  134.14us  349.40us  propagate_relabeling(int*, int, int, bool*, bool, int*)
                    2.49%  6.0805ms         2  3.0402ms  3.0393ms  3.0411ms  gpu_gaussian_blur(char*, int, int, unsigned long, double*, int, char*, unsigned long)
                    1.13%  2.7624ms         3  920.79us  1.6000us  1.4272ms  [CUDA memcpy HtoD]
                    0.56%  1.3562ms        47  28.854us  1.2150us  1.2979ms  [CUDA memcpy DtoH]
                    0.43%  1.0462ms         1  1.0462ms  1.0462ms  1.0462ms  histogram(char*, int, int, int, int*)
                    0.42%  1.0179ms         2  508.96us  508.82us  509.11us  gpu_gray_scale(char*, int, int, unsigned long)
                    0.21%  512.18us         1  512.18us  512.18us  512.18us  gpu_difference(char*, int, int, unsigned long, char*, unsigned long)
                    0.07%  177.44us         1  177.44us  177.44us  177.44us  get_bbox(int*, int, int, int*, int*, char*, int)
                    0.07%  165.31us         1  165.31us  165.31us  165.31us  thresholding(char*, int, int, int, int, int*)
                    0.02%  48.382us        48  1.0070us     959ns  1.2160us  [CUDA memset]
      API calls:   64.12%  246.96ms        47  5.2545ms  11.661us  232.89ms  cudaMemcpy
                   34.18%  131.66ms         3  43.886ms  122.10us  131.29ms  cudaMallocPitch
                    1.19%  4.5783ms         3  1.5261ms  1.4739ms  1.6065ms  cudaMemcpy2D
                    0.13%  516.29us        10  51.628us  4.4890us  164.86us  cudaFree
                    0.12%  460.70us        54  8.5310us  5.8610us  41.207us  cudaLaunchKernel
                    0.09%  344.97us         7  49.280us  4.2280us  209.72us  cudaMalloc
                    0.06%  246.54us        48  5.1360us  3.9380us  36.357us  cudaMemset
                    0.06%  242.34us       101  2.3990us     240ns  111.09us  cuDeviceGetAttribute
                    0.02%  93.025us         1  93.025us  93.025us  93.025us  cuDeviceGetName
                    0.01%  35.747us         5  7.1490us     210ns  34.785us  cudaPeekAtLastError
                    0.00%  11.290us         1  11.290us  11.290us  11.290us  cuDeviceGetPCIBusId
                    0.00%  3.0550us         3  1.0180us     470ns  2.0040us  cuDeviceGetCount
                    0.00%  1.2910us         2     645ns     450ns     841ns  cuDeviceGet
                    0.00%     861ns         1     861ns     861ns     861ns  cuDeviceTotalMem
                    0.00%     521ns         1     521ns     521ns     521ns  cuDeviceGetUuid  
```
