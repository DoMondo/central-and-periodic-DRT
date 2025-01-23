#include "Halide.h"

// A full forward discrete radon transform (1024x1024)
namespace {

class drt2output_generator : public Halide::Generator<drt2output_generator> {

 public:
   Var x, y, c;
   Input<Buffer<uint8_t>> in{"in", 3};
   Input<Buffer<uint8_t>> in_src{"in_src", 2};
   Output<Buffer<uint8_t>> out{"out", 2};

   void generate() {
      Func cin = Halide::BoundaryConditions::constant_exterior(in, 0);
      Func cin_src = Halide::BoundaryConditions::constant_exterior(in_src, 0);
      const int N = VAL_N;
      out(x, y) = cast<uint8_t>(0);
      Expr outN = out.width() / 4;
      Expr xfactor = 4.0f * N / out.width();
      Expr yfactor = 3.0f * N / out.height();
      Expr xdir0 = clamp(cast<uint16_t>(x * xfactor), 0, N);
      Expr xdir1 = clamp(cast<uint16_t>(x * xfactor - 1 * N), 0, N);
      Expr xdir2 = clamp(cast<uint16_t>(x * xfactor - 2 * N), 0, N);
      Expr xdir3 = clamp(cast<uint16_t>(x * xfactor - 3 * N), 0, N);
      Expr ydir0 = clamp(cast<uint16_t>(y * yfactor), 0, N * 2);
      Expr ydir1 = clamp(cast<uint16_t>(y * yfactor - N), 0, N * 2);
      Expr xdir_insrc = clamp(cast<uint16_t>(x * xfactor - 3 * N), 0, N);
      Expr ydir_insrc = clamp(cast<uint16_t>(y * xfactor), 0, N);
      // clang-format off
      out(x, y) = Halide::select(
         (x >= 0) && (x < outN), 
         cin(xdir0, ydir0, 0),
         (x < outN * 2) && (x >= outN),
         cin(xdir1, ydir0, 1),
         (x < outN * 3) && (x >= outN * 2), 
         cin(xdir2, ydir1, 2),
         (x >= outN * 3) && (y <= outN),
         cin_src(xdir_insrc, ydir_insrc), 
         (x >= outN * 3),
         cin(xdir3, ydir1, 3),
         0);
      // clang-format on
   }

   void schedule() {
      if (auto_schedule) {
         const int N = VAL_N;
         in.dim(0).set_estimate(0, N * 2);
         in.dim(1).set_estimate(0, N * 2);
         in.dim(2).set_estimate(0, 4);
         in_src.dim(0).set_estimate(0, N);
         in_src.dim(1).set_estimate(0, N);
         out.set_estimate(x, 0, N).set_estimate(y, 0, N * 2);
      } else {
         out.compute_root();
      }
   }
};

} // namespace

HALIDE_REGISTER_GENERATOR(drt2output_generator, drt2output)
