#pragma once
// Appended after transformation: initialization, oracles, timing and reporting
// are deliberately outside the code transformed by APAC.
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <omp.h>
#include <stdexcept>
#include <string>
#include <vector>
#ifndef BENCH_KIND
#error BENCH_KIND must identify the kernel
#endif
#ifndef BENCH_PARALLEL
#define BENCH_PARALLEL 0
#endif
extern "C" unsigned long long bench_entry(int, int, int, int *, int *, double *,
                                          double *, double *, double *);
namespace bench {
constexpr double guard = -987654321.25;
std::uint32_t random_value(std::uint32_t i, std::uint32_t seed) {
  std::uint32_t x = i + 0x9e3779b9U * (seed + 1U);
  x ^= x >> 16;
  x *= 0x7feb352dU;
  x ^= x >> 15;
  x *= 0x846ca68bU;
  return x ^ (x >> 16);
}
int argument(char *text, int minimum, int maximum) {
  char *end = nullptr;
  errno = 0;
  long value = std::strtol(text, &end, 10);
  if (errno || !text[0] || *end || value < minimum || value > maximum)
    throw std::runtime_error("invalid benchmark argument: " +
                             std::string(text));
  return int(value);
}
std::vector<double> buffer(std::size_t n) {
  return std::vector<double>(n + 2, guard);
}
void check_guards(const std::vector<double> &v) {
  if (v.front() != guard || v.back() != guard)
    throw std::runtime_error("floating buffer guard overwritten");
}
std::uint64_t digest = 14695981039346656037ULL;
void hash(std::uint64_t value) {
  digest ^= value;
  digest *= 1099511628211ULL;
}
void hash_double(double value) {
  std::uint64_t bits;
  std::memcpy(&bits, &value, sizeof(bits));
  hash(bits);
}
double max_error = 0;
void close(double actual, long double expected) {
  long double difference = std::fabs((long double)actual - expected);
  if (!std::isfinite(actual) ||
      difference > 2e-9L * (1 + std::fabs(expected))) {
    std::fprintf(stderr, "mismatch: actual=%.17g expected=%.21Lg\n", actual,
                 expected);
    throw std::runtime_error("floating-point oracle failed");
  }
  max_error = std::max(max_error, double(difference));
}
} // namespace bench
int main(int argc, char **argv) {
  try {
    if (argc != 6)
      throw std::runtime_error(
          "usage: executable SIZE GRAIN STEPS SEED PATTERN");
    int limit = 20000000;
    if (BENCH_KIND == 0)
      limit = 46;
    if (BENCH_KIND == 3)
      limit = 15;
    if (BENCH_KIND == 5)
      limit = 4096;
    if (BENCH_KIND == 6)
      limit = 32768;
    if (BENCH_KIND == 7)
      limit = 8192;
    int n = bench::argument(argv[1],
                            BENCH_KIND == 7   ? 2
                            : BENCH_KIND == 6 ? 1
                                              : 0,
                            limit);
    int grain = bench::argument(argv[2], 1, 20000000);
    int steps = bench::argument(argv[3], 1, 10000);
    int seed = bench::argument(argv[4], 0, 1000000);
    int pattern = bench::argument(argv[5], 0, 3);
    std::size_t count = 0;
    if (BENCH_KIND == 4)
      count = n;
    if (BENCH_KIND == 5 || BENCH_KIND == 7)
      count = std::size_t(n) * n;
    if (BENCH_KIND == 6)
      count = std::size_t(n) * 3;
    std::vector<int> keys(
        (BENCH_KIND == 1 || BENCH_KIND == 2) ? std::size_t(n) + 2 : 2,
        -2147483647);
    std::vector<int> aux(keys.size(), -2147483647);
    std::vector<int> sorted;
    auto x = bench::buffer(count), y = bench::buffer(count);
    auto z = bench::buffer(BENCH_KIND == 4                      ? 1
                           : BENCH_KIND == 5 || BENCH_KIND == 6 ? count
                                                                : 0);
    auto temp = bench::buffer(BENCH_KIND == 6 ? count : 0);
    if (BENCH_KIND == 1 || BENCH_KIND == 2) {
      for (int i = 0; i < n; ++i) {
        int value = int(bench::random_value(i, seed) % 2000001) - 1000000;
        if (pattern == 1)
          value = i - n / 2;
        if (pattern == 2)
          value = n - i;
        if (pattern == 3)
          value = int(bench::random_value(i, seed) % 16);
        keys[i + 1] = value;
      }
      sorted.assign(keys.begin() + 1, keys.end() - 1);
    }
    if (BENCH_KIND == 4 || BENCH_KIND == 5) {
      for (std::size_t i = 0; i < count; ++i) {
        double scale = BENCH_KIND == 5 ? 0.0625 : 1.0;
        x[i + 1] = (int(bench::random_value(i, seed) % 17) - 8) * scale;
        y[i + 1] = (int(bench::random_value(i, seed + 1) % 17) - 8) * scale;
      }
      std::fill(z.begin() + 1, z.end() - 1, 0);
    }
    if (BENCH_KIND == 6) {
      for (std::size_t i = 0; i < count; ++i) {
        x[i + 1] = (bench::random_value(i, seed) % 10000) / 10000.0;
        y[i + 1] = (int(bench::random_value(i, seed + 1) % 2001) - 1000) * 1e-6;
      }
      std::fill(z.begin() + 1, z.end() - 1, 0);
      std::fill(temp.begin() + 1, temp.end() - 1, 0);
    }
    if (BENCH_KIND == 7) {
      for (std::size_t i = 0; i < count; ++i)
        x[i + 1] = y[i + 1] = (bench::random_value(i, seed) % 1024) / 1024.0;
    }
    double seconds = 0;
    unsigned long long scalar = 0;
    int team = 1;
#if BENCH_PARALLEL
#pragma omp parallel
    {
#pragma omp barrier
#pragma omp single
      {
        team = omp_get_num_threads();
        double start = omp_get_wtime();
#pragma omp taskgroup
        {
          scalar = bench_entry(n, grain, steps, keys.data() + 1, aux.data() + 1,
                               x.data() + 1, y.data() + 1, z.data() + 1,
                               temp.data() + 1);
        }
        seconds = omp_get_wtime() - start;
      }
    }
#else
    double start = omp_get_wtime();
    scalar =
        bench_entry(n, grain, steps, keys.data() + 1, aux.data() + 1,
                    x.data() + 1, y.data() + 1, z.data() + 1, temp.data() + 1);
    seconds = omp_get_wtime() - start;
#endif
    // All checks, including independent reference computations, are untimed.
    for (const auto *v : {&x, &y, &z, &temp})
      bench::check_guards(*v);
    if (keys.front() != -2147483647 || keys.back() != -2147483647 ||
        aux.front() != -2147483647 || aux.back() != -2147483647)
      throw std::runtime_error("integer buffer guard overwritten");
    if (BENCH_KIND == 0) {
      unsigned long long a = 0, b = 1;
      for (int i = 0; i < n; ++i) {
        auto next = a + b;
        a = b;
        b = next;
      }
      if (scalar != a)
        throw std::runtime_error("iterative Fibonacci oracle failed");
      bench::hash(scalar);
    }
    if (BENCH_KIND == 3) {
      constexpr unsigned long long counts[] = {
          1,  1,   0,   0,    2,     10,    4,      40,
          92, 352, 724, 2680, 14200, 73712, 365596, 2279184};
      if (scalar != counts[n])
        throw std::runtime_error("N-Queens solution count failed");
      bench::hash(scalar);
    }
    if (BENCH_KIND == 1 || BENCH_KIND == 2) {
      std::sort(sorted.begin(), sorted.end());
      if (!std::equal(sorted.begin(), sorted.end(), keys.begin() + 1))
        throw std::runtime_error("std::sort oracle failed");
      for (int value : sorted)
        bench::hash(std::uint32_t(value));
    }
    if (BENCH_KIND == 4 || BENCH_KIND == 5) {
      for (std::size_t i = 0; i < count; ++i) {
        double scale = BENCH_KIND == 5 ? 0.0625 : 1.0;
        bench::close(x[i + 1],
                     (int(bench::random_value(i, seed) % 17) - 8) * scale);
        bench::close(y[i + 1],
                     (int(bench::random_value(i, seed + 1) % 17) - 8) * scale);
      }
    }
    if (BENCH_KIND == 4) {
      long double expected = 0;
      for (int i = 0; i < n; ++i)
        expected += (long double)x[i + 1] * y[i + 1];
      bench::close(z[1], expected);
      bench::hash_double(z[1]);
    }
    if (BENCH_KIND == 5) {
      for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
          long double expected = 0;
          for (int k = 0; k < n; ++k)
            expected += (long double)x[1 + i * n + k] * y[1 + k * n + j];
          bench::close(z[1 + i * n + j], expected);
          bench::hash_double(z[1 + i * n + j]);
        }
    }
    if (BENCH_KIND == 6) {
      using Triple = std::array<long double, 3>;
      std::vector<Triple> p(n), v(n), nextp(n), nextv(n);
      for (int i = 0; i < n; ++i)
        for (int d = 0; d < 3; ++d) {
          // Recreate inputs independently of the buffers mutated by the kernel.
          p[i][d] = (bench::random_value(3 * i + d, seed) % 10000) / 10000.0;
          v[i][d] =
              (int(bench::random_value(3 * i + d, seed + 1) % 2001) - 1000) *
              1e-6;
        }
      for (int step = 0; step < steps; ++step) {
        for (int i = 0; i < n; ++i) {
          Triple acceleration = {0, 0, 0};
          for (int j = n - 1; j >= 0; --j) {
            Triple delta;
            long double radius = 0.001L;
            for (int d = 0; d < 3; ++d) {
              delta[d] = p[j][d] - p[i][d];
              radius += delta[d] * delta[d];
            }
            long double weight = 1.0L / (n * radius * std::sqrt(radius));
            for (int d = 0; d < 3; ++d)
              acceleration[d] += delta[d] * weight;
          }
          for (int d = 0; d < 3; ++d) {
            nextv[i][d] = v[i][d] + 0.001L * acceleration[d];
            nextp[i][d] = p[i][d] + 0.001L * nextv[i][d];
          }
        }
        p.swap(nextp);
        v.swap(nextv);
      }
      const auto &out = steps % 2 ? z : x;
      const auto &outv = steps % 2 ? temp : y;
      for (int i = 0; i < n; ++i)
        for (int d = 0; d < 3; ++d) {
          bench::close(out[1 + 3 * i + d], p[i][d]);
          bench::close(outv[1 + 3 * i + d], v[i][d]);
          bench::hash_double(out[1 + 3 * i + d]);
          bench::hash_double(outv[1 + 3 * i + d]);
        }
    }
    if (BENCH_KIND == 7) {
      std::vector<long double> ref(count), next(count);
      for (std::size_t i = 0; i < count; ++i)
        ref[i] = next[i] = (bench::random_value(i, seed) % 1024) / 1024.0;
      for (int step = 0; step < steps; ++step) {
        for (int j = 1; j < n - 1; ++j)
          for (int i = 1; i < n - 1; ++i)
            next[i * n + j] = (ref[(i - 1) * n + j] + ref[(i + 1) * n + j] +
                               ref[i * n + j - 1] + ref[i * n + j + 1]) /
                              4;
        ref.swap(next);
      }
      const auto &out = steps % 2 ? y : x;
      for (std::size_t i = 0; i < count; ++i) {
        bench::close(out[i + 1], ref[i]);
        bench::hash_double(out[i + 1]);
      }
    }
    std::printf(
        "{\"seconds\":%.17g,\"verified\":true,\"scalar\":\"%llu\",\"checksum\":"
        "\"%016llx\",\"team_threads\":%d,\"max_abs_error\":%.17g}\n",
        seconds, scalar, (unsigned long long)bench::digest, team,
        bench::max_error);
    return 0;
  } catch (const std::exception &error) {
    std::fprintf(stderr, "benchmark: %s\n", error.what());
    return 2;
  }
}
