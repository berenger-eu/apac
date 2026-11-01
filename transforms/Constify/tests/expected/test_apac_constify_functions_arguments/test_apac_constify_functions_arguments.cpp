#include <stdlib.h>

// free requires mutable pointed-to storage; the pointer itself may be const.
void e(int *const a) { free(a); }

int f(const int a) {
  int b = a;
  b = 1;
  return b;
}

void g(int *const a) { a[f(1)]--; }

void h(int &a) { a = 1; }

void j(int &a) { h(a); }

void k(int *const a, int *const b, const int c) {
  int *d = a;
  d = b;
  *d = 1;
}

void l1(int &a) { a += 1; }

void l2(int &a) { l1(a); }

void l3(int &a) {
  int &b = a;
  l2(b);
}

void m(const int &a, int *const b, const int *const &c) {
  int *const d = a + b;
  d[0] = 1;
}

int *n1(int *const a, const int &b) {
  int *const c = a;
  return c;
}

int *n2(int *const a, const int &b) {
  int *const c = a;
  return n1(c, b);
}

int &n3(int &a, const int &b) { return a; }

int &n4(int &a, const int &b) {
  int &c = a;
  return n3(c, b);
}

void n5(int *const a, int &b, const int c) {
  const int *const d = n2(a, c);
  const int &e = n4(b, c);
}

void o(int a, int b, const int c) {
  int &d = a;
  int *const e = &b;

  d = 1;
  *e = 1;
}

void c1(int *const a, const int *const b, const int *const c, int *const d) {
  int *const aa = a;
  const int *const bb = b;
  const int *const cc = c;
  int *const dd = d;

  *aa = 42;
  *dd = 12;
}

void c2(int *const a, const int *const b, const int *const c, int *const d) {
  int *const aa = a;
  int *const dd = d;

  const int *const bb = b;
  const int *const cc = c;

  *aa = 42;
  *dd = 12;
}

void c3(int *const a, const int *const b, const int *const c, int *const d) {
  int *const aa = a;
  int *const dd = d;

  const int *const bb = b;
  const int *const cc = c;
  const int e = 2;

  *aa = 42;
  *dd = 12;
}

namespace BB {
void h(int &a) { a = 1; }
} // namespace BB

namespace AA {
void f(int &a) { a = 1; }

void g(int a, int b, const int c) {
  f(a);
  BB::h(b);
}
} // namespace AA

void p(int a, const int b) { AA::f(a); }

class CC {
public:
  int *i;
  int *j;

  void f(int *const a, const int b) {
    i = a;
    *j = 42;
    p(i, j);
  }

  int q(int a) const;

  void p(int *const a, const int *const b) const { *a = *b + 1; }
};

int CC::q(const int a) const { return a; }

void q(CC a, int b, const int c) { a.f(&b, 1); }
