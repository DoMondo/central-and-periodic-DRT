#include "Halide.h"
#include "central_drtf.h"
#include "drt2output.h"
#include "drtf.h"
#include "halide_benchmark.h"
#include "halide_image_io.h"
#include "periodic_drt2output.h"
#include "periodic_drtf.h"
#include <argp.h>
#include <string>

static char doc[] =
   "               _               _           _ _     _       \n"
   " _ __ __ _  __| | ___  _ __   | |__   __ _| (_) __| | ___  \n"
   "| '__/ _` |/ _` |/ _ \\| '_ \\  | '_ \\ / _` | | |/ _` |/ _ \\ \n"
   "| | | (_| | (_| | (_) | | | | | | | | (_| | | | (_| |  __/ \n"
   "|_|  \\__,_|\\__,_|\\___/|_| |_| |_| |_|\\__,_|_|_|\\__,_|\\___| \n"
   "                                                           \n";

struct arguments {
   char *args[2];
   std::string path;
};

static struct argp_option options[] = {};

static error_t parse_opt(const int key, char *arg, struct argp_state *state) {
   // struct arguments *arguments = (struct arguments *)state->input;
   return 0;
}

static char args_doc[] = "";
static struct argp argp = {options, parse_opt, args_doc, doc};

void test_radon_drtf() {
   std::string path("in/test1_" + std::to_string(int(VAL_N)) + ".jpg");
   printf("test_radon_drtf( %s )\n", path.c_str());
   Halide::Runtime::Buffer<uint8_t> input_ = Halide::Tools::load_image(path);
   Halide::Runtime::Buffer<uint16_t> input(VAL_N, VAL_N);
   for (int i = 0; i < input.width(); i++)
      for (int j = 0; j < input.height(); j++)
         input(i, j) = input_(i, j);
   Halide::Runtime::Buffer<uint16_t> output(input.width() * 4, input.height() * 3);
   double time =
      Halide::Tools::benchmark(2, 10, [&]() { drtf(input, output); });
   output.copy_to_host();
   printf("DRTf time:\t  %g ms\n", time * 1e3);
   Halide::Runtime::Buffer<uint8_t> output8(input.width() * 4, input.height() * 3);
   for (int i = 0; i < output.width(); i++)
      for (int j = 0; j < output.height(); j++)
         output8(i, j) = output(i, j) / VAL_N;
   Halide::Tools::save_image(output8, "out/drtf.png");
}

void test_radon_periodic_drtf() {
   std::string path("in/test1_" + std::to_string(int(VAL_N)) + ".jpg");
   printf("test_radon_periodic_drtf( %s )\n", path.c_str());
   Halide::Runtime::Buffer<uint8_t> input_ = Halide::Tools::load_image(path);
   Halide::Runtime::Buffer<uint16_t> input(VAL_N, VAL_N);
   for (int i = 0; i < input.width(); i++)
      for (int j = 0; j < input.height(); j++)
            input(i, j) = input_(i, j);
   Halide::Runtime::Buffer<uint16_t> output(input.width() * 4, input.height());
   double time =
      Halide::Tools::benchmark(2, 10, [&]() { periodic_drtf(input, output); });
   output.copy_to_host();
   Halide::Runtime::Buffer<uint8_t> output8(input.width() * 4, input.height());
   for (int i = 0; i < output.width(); i++)
      for (int j = 0; j < output.height(); j++)
         output8(i, j) = output(i, j) / VAL_N;
   printf("DRTf time:\t  %g ms\n", time * 1e3);
   Halide::Tools::save_image(output8, "out/drtf_periodic.png");
}

void test_radon_central_drtf() {
   std::string path("in/test1_" + std::to_string(int(VAL_N)) + ".jpg");
   printf("test_radon_central_drtf( %s )\n", path.c_str());
   Halide::Runtime::Buffer<uint8_t> input_ = Halide::Tools::load_image(path);
   Halide::Runtime::Buffer<uint16_t> input(VAL_N, VAL_N);
   for (int i = 0; i < input.width(); i++)
      for (int j = 0; j < input.height(); j++)
         input(i, j) = input_(i, j);
   Halide::Runtime::Buffer<uint16_t> output(input.width() * 4, input.height());
   double time =
      Halide::Tools::benchmark(2, 10, [&]() { central_drtf(input, output); });
   output.copy_to_host();
   printf("DRTf time:\t  %g ms\n", time * 1e3);
   Halide::Runtime::Buffer<uint8_t> output8(input.width() * 4, input.height());
   for (int i = 0; i < output.width(); i++)
      for (int j = 0; j < output.height(); j++)
         output8(i, j) = output(i, j) / VAL_N;
   Halide::Tools::save_image(output8, "out/drtf_central.png");
}

int main(int argc, char *argv[]) {
   struct arguments arguments;
   /* Set argument defaults */
   arguments.path = ".";
   argp_parse(&argp, argc, argv, 0, 0, &arguments);
   test_radon_drtf();
   test_radon_central_drtf();
   test_radon_periodic_drtf();
   return 0;
}
