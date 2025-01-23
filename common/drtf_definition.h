#pragma once

#include "Halide.h"

class DRTf {
 private:
   typedef uint32_t dir_type;
   Halide::Func fm[MAX_IT] /*= {
      Halide::Func("fm_0"), Halide::Func("fm_1"), Halide::Func("fm_2"),
      Halide::Func("fm_3"), Halide::Func("fm_4"), Halide::Func("fm_5"),
      Halide::Func("fm_6"), Halide::Func("fm_7"), Halide::Func("fm_8"),
      Halide::Func("fm_9"), Halide::Func("fm_10")}*/
      ;

 public:
   void drtf(Halide::Func &in, Halide::Func &out, Halide::Var &x,
             Halide::Var &y, Halide::Var &c) {
      // Fill upper part
      fm[0](x, y, c) = in(x, y - VAL_N, c);
      for (dir_type m = 0; m < MAX_IT - 1; m++) {
         Halide::Expr sigma{"sigma"};
         Halide::Expr s0{"s0"};
         Halide::Expr dir_f0{"dir_f0"};
         Halide::Expr dir_f1{"dir_f1"};
         Halide::Expr f0{"f0"};
         Halide::Expr f1{"f1"};
         Halide::Expr aux{"aux"};
         sigma = (Halide::cast<dir_type>(x) & ((1 << (m + 1)) - 1)) >> 1;
         aux = Halide::cast<dir_type>(x) >> (m + 1) << (m + 1);
         // This clamp will determine the size of fm[i]
         dir_f0 =
            Halide::clamp(Halide::cast<uint16_t>(sigma + aux), 0, VAL_N - 1);
         f0 = fm[m](dir_f0, y, c);
         dir_f1 = dir_f0 + (1 << m);
         dir_f1 = Halide::clamp(dir_f1, 0, VAL_N - 1);
         s0 = Halide::cast<dir_type>(x) & 1;
         f1 = Halide::select(
            (y + sigma + s0) < (VAL_N * 2),
            fm[m](dir_f1, Halide::clamp(y + sigma + s0, 0, VAL_N * 2 - 1), c),
            0);
         fm[m + 1](x, y, c) = f0 + f1;
      }
      out(x, y, c) = fm[MAX_IT - 1](x, y, c);
   }

   void sched_cpu(Halide::Var &x, Halide::Var &y, Halide::Var &c) {
      for (int i = 0; i < MAX_IT; i++) {
         //fm[i].compute_root().parallel(c).vectorize(y, 4);
         fm[i].compute_root();
      }
   }

   void sched_opencl(Halide::Var &x, Halide::Var &y, Halide::Var &c) {
      Halide::Var xo, yo, xi, yi;
      for (int i = 0; i < MAX_IT; i++) {
         fm[i]
            .bound(x, 0, VAL_N)
            .bound(y, 0, VAL_N * 2)
            .bound(c, 0, 4)
            .unroll(c)
            .compute_root()
            .gpu_tile(x, y, xo, yo, xi, yi, 8, 8);
      }
   }

   void sched_gpu(Halide::Var &x, Halide::Var &y, Halide::Var &c) {
      //for (int i = 0; i < MAX_IT; i++) {
         //fm[i]
            //.bound(x, 0, VAL_N)
            //.bound(y, 0, VAL_N * 2)
            //.bound(c, 0, 4)
            //.unroll(c)
            //.compute_root()
            //.glsl(x, y, c);
      //}
   }
};
