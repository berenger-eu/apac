#pragma once

// Emit a self-contained runtime: generated sources must not require APAC's
// development headers on the target compute machine.
inline const char *apacDepthIncludes() {
  return "#include <omp.h>\n#include <cerrno>\n#include <climits>\n"
         "#include <cstdio>\n#include <cstdlib>\n";
}
inline const char *apacDepthRuntime() {
  return R"apac(
inline int __apac_default_depth(unsigned int threads) {
  int depth = 1;
  for (unsigned int remaining = threads > 1 ? threads - 1 : 0;
       remaining; remaining >>= 1)
    ++depth;
  return depth;
}
const int nb_cores = omp_get_max_threads();
const int parallel_depth = __apac_default_depth(nb_cores);
int __apac_depth = 0;
#pragma omp threadprivate(__apac_depth)
const static int __apac_depth_max = [] {
  const char *value = std::getenv("APAC_DEPTH_MAX");
  if (!value) return parallel_depth;
  char *end = nullptr;
  errno = 0;
  long depth = std::strtol(value, &end, 10);
  if (errno == ERANGE || end == value || *end || depth < 0 || depth > INT_MAX) {
    std::fprintf(stderr, "APAC_DEPTH_MAX must be an integer between 0 and %d\n", INT_MAX);
    std::exit(2);
  }
  return static_cast<int>(depth);
}();
struct __apac_depth_guard {
  int previous;
  explicit __apac_depth_guard(int depth) : previous(__apac_depth) {
    __apac_depth = depth;
  }
  ~__apac_depth_guard() { __apac_depth = previous; }
  __apac_depth_guard(const __apac_depth_guard &) = delete;
  __apac_depth_guard &operator=(const __apac_depth_guard &) = delete;
};
)apac";
}
