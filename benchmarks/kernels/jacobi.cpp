#include <algorithm>
#include <utility>

void relax_rows(const double *source, double *output, int size, int first_row,
                int block_rows) {
  int end = std::min(first_row + block_rows, size - 1);
  for (int i = first_row; i < end; ++i)
    for (int j = 1; j < size - 1; ++j)
      output[(i - first_row) * size + j] =
          0.25 * (source[(i - 1) * size + j] + source[(i + 1) * size + j] +
                  source[i * size + j - 1] + source[i * size + j + 1]);
}
void jacobi_step(const double *source, double *output, int size,
                 int block_rows) {
  for (int row = 1; row < size - 1; row += block_rows) {
    relax_rows(&source[0], &output[row * size], size, row, block_rows);
  }
}
void jacobi(double *current, double *next, int size, int steps,
            int block_rows) {
  for (int step = 0; step < steps; ++step) {
    jacobi_step(current, next, size, block_rows);
    std::swap(current, next);
  }
}
