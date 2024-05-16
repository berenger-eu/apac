typedef int *pINT;

using ppINT = int **;

struct A {};

void e(const int a) {}

void f(const int a) {}

// TODO: Be able to use anonymous arguments, e.g. void g(float *, double &, ...)
void g(const float *const a, const double &b, const int *const *const &c) {}

void h(const int *const a, const int *const *const b) {}

void i(const A &a) {}
