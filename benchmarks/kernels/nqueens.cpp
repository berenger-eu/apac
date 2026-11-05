// Backtracking with bit masks for columns and the two diagonals.
unsigned long long search_queens(unsigned long long board_mask,
                                 unsigned long long columns,
                                 unsigned long long left_diagonal,
                                 unsigned long long right_diagonal) {
  if (columns == board_mask)
    return 1;
  unsigned long long available =
      board_mask & ~(columns | left_diagonal | right_diagonal);
  unsigned long long solutions = 0;
  while (available) {
    unsigned long long column = available & (~available + 1);
    available ^= column;
    solutions += search_queens(board_mask, columns | column,
                               (left_diagonal | column) << 1,
                               (right_diagonal | column) >> 1);
  }
  return solutions;
}
unsigned long long nqueens(int size) {
  return search_queens((1ULL << size) - 1, 0, 0, 0);
}
