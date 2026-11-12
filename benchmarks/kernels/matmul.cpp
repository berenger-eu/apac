#include <algorithm>

// Update C(i,j) with the product of tiles A(i,k) and B(k,j).
void multiply_tile(const double *a, const double *b, double *c, int size,
                   int tile_size, int first_row, int first_column,
                   int first_inner) {
  int rows = std::min(tile_size, size - first_row);
  int columns = std::min(tile_size, size - first_column);
  int inner = std::min(tile_size, size - first_inner);
  for (int i = 0; i < rows; ++i)
    for (int k = 0; k < inner; ++k)
      for (int j = 0; j < columns; ++j)
        c[i * size + j] += a[i * size + k] * b[k * size + j];
}
void multiply_matrices(const double *a, const double *b, double *c, int size,
                       int tile_size) {
  for (int i = 0; i < size; i += tile_size) {
    for (int j = 0; j < size; j += tile_size) {
      for (int k = 0; k < size; k += tile_size) {
        multiply_tile(&a[i * size + k], &b[k * size + j], &c[i * size + j],
                      size, tile_size, i, j, k);
      }
    }
  }
}
