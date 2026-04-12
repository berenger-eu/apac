#include "_apac_header.hpp"
int max(int a, int b) { return (a > b) ? a : b; }

int clamp(int v, int lo, int hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }
