#include "HalideBuffer.h"
#include "HalideRuntime.h"
#include "central_drtf.h"
#include "drtf.h"
#include "halide_benchmark.h"
#include "periodic_drtf.h"
#include <cmath>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

bool have_opencl_or_metal() {
#ifdef _WIN32
   return LoadLibrary("OpenCL.dll") != NULL;
#elif __APPLE__
   return dlopen("/System/Library/Frameworks/Metal.framework/Versions/Current/"
                 "Metal",
                 RTLD_LAZY) != NULL;
#else
   return dlopen("libOpenCL.so", RTLD_LAZY) != NULL;
#endif
}

void test_radon_drtf() {
   printf("test_radon_drtf\n");
   Halide::Runtime::Buffer<uint16_t> input(VAL_N, VAL_N);
   Halide::Runtime::Buffer<uint16_t> output(VAL_N * 4, VAL_N * 3);
   double time =
      Halide::Tools::benchmark(2, 10, [&]() { drtf(input, output); });
   printf("Original DRTf time:\t  %g ms\n", time * 1e3);
}

void test_radon_periodic_drtf() {
   printf("test_radon_drtf\n");
   Halide::Runtime::Buffer<uint16_t> input(VAL_N, VAL_N);
   Halide::Runtime::Buffer<uint16_t> output(VAL_N * 4, VAL_N);
   double time =
      Halide::Tools::benchmark(2, 10, [&]() { periodic_drtf(input, output); });
   printf("Periodic DRTf time:\t  %g ms\n", time * 1e3);
}

void test_radon_central_drtf() {
   printf("test_radon_drtf\n");
   Halide::Runtime::Buffer<uint16_t> input(VAL_N, VAL_N);
   Halide::Runtime::Buffer<uint16_t> output(VAL_N * 4, VAL_N);
   double time =
      Halide::Tools::benchmark(2, 10, [&]() { central_drtf(input, output); });
   printf("Central DRTf time:\t  %g ms\n", time * 1e3);
}

int main(int argc, char *argv[]) {
   printf("N = %d\n", VAL_N);
   test_radon_drtf();
   test_radon_periodic_drtf();
   test_radon_central_drtf();
   return 0;
}
