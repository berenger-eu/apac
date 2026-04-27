#include <cstring>
#include <omp.h>
struct Point {
  int x;
  int y;
};
const int nb_cores = omp_get_max_threads();
const int parallel_depth = ffs(nb_cores); // log2(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = parallel_depth;

int computeX(int v) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return v * 2;
  } else {
    return computeX_apacSeq(v);
  }
}
int computeY(int v) {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {}
    return v + 1;
  } else {
    return computeY_apacSeq(v);
  }
}

int main() {
  int __apac_depth_local = __apac_depth;
  int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);
  if (__apac_depth_ok) {

#pragma omp taskgroup
    {

      Point s;

      int result;

#pragma omp task default(shared) firstprivate(__apac_depth_local)
      {
        __apac_depth = __apac_depth_local + 1;
        s.x = computeX(3);
        s.y = computeY(7);
        result = s.x + s.y;
      }
    }
    return result;
  } else {
    return main_apacSeq();
  }
}
