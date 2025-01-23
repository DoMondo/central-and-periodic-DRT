#pragma once

#include "Halide.h"

class CentralDRTf {
 private:
   typedef int32_t dir_type;
   Halide::Func fm[MAX_IT];
   Halide::Expr dispDST(Halide::Expr m, Halide::Expr s, bool is_first) {
      using namespace Halide;
      const int32_t n = MAX_IT - 1;
      Expr snm1 = s & 1;
      Expr _s = s >> 1;
      Expr sigma = select(m > 0 && !is_first, _s & ((1 << m) - 1), 0);
      Expr v = select(is_first, 0.5f, cast<float>(_s >> m));
      Expr phi = ((1 << (m + 1) - (m < (n - 1)))) *
                 ((1 << abs(n - m - 2)) - v) /
                 (1 << (m + 1) - 1);
      Expr dd = cast<dir_type>(
         select((m == n - 1), floor((s + 1.f) * .5f),
                (is_first) || (s == 0) || s >= (1 << (n - 1)),
                0, (2.f * sigma + snm1) * phi));
      return dd;
   }

 public:
   void drtf(Halide::Func &in, Halide::Func &out, Halide::Var &x,
             Halide::Var &y, Halide::Var &c) {
      using namespace Halide;
      fm[0](x, y, c) =
         in(
            Halide::clamp(x, 0, VAL_N - 1), 
            Halide::clamp(y, 0, VAL_N - 1),
            Halide::clamp(c, 0, 3)
           );
      for (int32_t m = 0; m < MAX_IT - 1; m++) {
         Expr dispDSTout("dispDSTout");
         Expr dispDSTS0("dispDSTS0");
         Expr dispDSTS1("dispDSTS1");
         dispDSTout = dispDST(m, x, false);
         Expr snm1 = x & 1;
         Expr ss = x >> 1;
         Expr sigma = select(m > 0, ss & ((1 << m) - 1), 0);
         Expr v = ss >> m;
         Expr src_S0 = (v << (m + 1)) + sigma;
         dispDSTS0 = dispDST(abs(m - 1), src_S0, m == 0);
         Expr src_S1 = src_S0 + (1 << m);
         dispDSTS1 = dispDST(abs(m - 1), src_S1, m == 0);
         Expr deltaD_S1 = sigma + snm1;
         Expr ds0 = y + dispDSTout - dispDSTS0;
         Expr ds1 = y + dispDSTout - deltaD_S1 - dispDSTS1;
         Expr s0 = select(ds0 >= 0 && ds0 < VAL_N,
                          fm[m](clamp(src_S0, 0, VAL_N - 1),
                                clamp(ds0, 0, VAL_N - 1), 
                                clamp(c, 0, 3)),
                          0);
         Expr s1 = select(ds1 >= 0 && ds1 < VAL_N,
                          fm[m](clamp(src_S1, 0, VAL_N - 1),
                                clamp(ds1, 0, VAL_N - 1), 
                                clamp(c, 0, 3)),
                          0);
         fm[m + 1](x, y, c) = s0 + s1;
      }
      out(x, y, c) = fm[MAX_IT - 1](
         Halide::clamp(x, 0, VAL_N - 1), 
         Halide::clamp(y, 0, VAL_N - 1),
         Halide::clamp(c, 0, 3));
   }

   void sched_cpu(Halide::Var &x, Halide::Var &y, Halide::Var &c) {
      for (int i = 0; i < MAX_IT; i++) {
         //fm[i].compute_root().parallel(c).vectorize(x, 16);
         fm[i].compute_root();
      }
   }

   void sched_opencl(Halide::Var &x, Halide::Var &y, Halide::Var &c) {
      Halide::Var xo, yo, xi, yi;
      for (int i = 0; i < MAX_IT; i++) {
         fm[i]
            .bound(x, 0, VAL_N)
            .bound(y, 0, VAL_N)
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
            //.bound(y, 0, VAL_N)
            //.bound(c, 0, 4)
            //.unroll(c)
            //.compute_root()
            //.glsl(x, y, c);
      //}
   }
};
