#include "../central_drtf_definition.h"
#include "Halide.h"

// A quad of forward discrete radon transform (1024x1024) made for each color
// channel
namespace {

class CentralDRTf_generator : public Halide::Generator<CentralDRTf_generator> {
 private:
   CentralDRTf q;
   Func cin;
   Func q_out{"q_out"};
   Func clamped_in{"clamped_in"};
   Func out4_channels{"out4_channels"};
   Var i, j;

 public:
   Var x{"x"}, y{"y"}, c{"c"};
   Input<Buffer<uint16_t>> in{"in", 2};
   Output<Buffer<uint16_t>> out{"out_drtf", 2};

   void generate() {
      int Nm1 = VAL_N - 1;
      // clang-format off
      clamped_in(x, y, c) = Halide::select(
            c == 0, in(
               Halide::clamp(y, 0, VAL_N - 1), 
               Halide::clamp(Nm1 - x, 0, VAL_N - 1)
               ),
            c == 1, in(
               Halide::clamp(x, 0, VAL_N - 1), 
               Halide::clamp(Nm1 - y, 0, VAL_N - 1)
               ),
            c == 2, in(
               Halide::clamp(x, 0, VAL_N - 1), 
               Halide::clamp(y, 0, VAL_N - 1)
               ),
            c == 3, in(
               Halide::clamp(y, 0, VAL_N - 1), 
               Halide::clamp(x, 0, VAL_N - 1)
               ), 0
            );
      // clang-format on
      q.drtf(clamped_in, q_out, x, y, c);
      // clang-format off
      out4_channels(x, y, c) =  Halide::select(
         c == 0, q_out(
            Halide::clamp(x, 0, VAL_N - 1),
            Halide::clamp((VAL_N - 1) - y, 0, VAL_N - 1),
            Halide::clamp(c, 0, 3)
            ),
         c == 1, q_out(
            Halide::clamp(Nm1 - x, 0, VAL_N - 1),
            Halide::clamp(Nm1 - y, 0, VAL_N - 1),
            Halide::clamp(c, 0, 3)
            ),
         c == 2, q_out(
            Halide::clamp(x, 0, VAL_N - 1),
            Halide::clamp(y, 0, VAL_N - 1),
            Halide::clamp(c, 0, 3)
            ),
         c == 3, q_out(
            Halide::clamp(Nm1 - x, 0, VAL_N - 1),
            Halide::clamp(y, 0, VAL_N - 1),
            Halide::clamp(c, 0, 3)
            ), 0
         );
      out(i, j) = out4_channels(
            Halide::clamp(i % VAL_N, 0, VAL_N - 1), 
            Halide::clamp(j, 0, VAL_N - 1), 
            Halide::clamp(i / VAL_N, 0, 3)
      );
      // clang-format on
   }

   void schedule() {
      if (auto_schedule) {
         in.dim(0).set_estimate(0, VAL_N);
         in.dim(1).set_estimate(0, VAL_N);
         out.set_estimate(i, 0, VAL_N * 4).set_estimate(j, 0, VAL_N);
         //q.auto_schedule(i, j);
      } else {
         //if (get_target().has_feature(Halide::Target::OpenGL)) {
            //in.dim(0).set_bounds(0, VAL_N);
            //in.dim(1).set_bounds(0, VAL_N);
            //in.dim(2).set_bounds(0, 4);
            //q.sched_gpu(x, y, c);
            //out.compute_root();
            //out.bound(c, 0, 4);
            //out.bound(x, 0, VAL_N);
            //out.bound(y, 0, VAL_N);
            //out.unroll(c);
            //out.glsl(x, y, c);
         //} else if (get_target().has_feature(Halide::Target::OpenCL)) {
            //Halide::Var xo, yo, xi, yi;
            //in.dim(0).set_bounds(0, VAL_N);
            //in.dim(1).set_bounds(0, VAL_N);
            //in.dim(2).set_bounds(0, 4);
            //q.sched_opencl(x, y, c);
            //out.bound(x, 0, VAL_N)
               //.bound(y, 0, VAL_N)
               //.bound(c, 0, 4)
               //.unroll(c)
               //.compute_root()
               //.gpu_tile(x, y, xo, yo, xi, yi, 8, 8);
         //} else {
            q.sched_cpu(x, y, c);
            //out.compute_root().vectorize(i, 4);
            out.compute_root();
         //}
      }
   }
};

} // namespace

HALIDE_REGISTER_GENERATOR(CentralDRTf_generator, central_drtf)
