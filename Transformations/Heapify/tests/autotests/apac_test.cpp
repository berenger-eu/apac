// Test cases from Marek Felšöci

int i = 1;
int x = 2;

int main() {
  {

    int a = 1;
    int b = i;
    int const c = 1;
    int const d = i;
    int &e = i;
    int const &f = 1;
    int const &g = i;
    int const h[5] = {1, 2, 3, 4, 5};
    int j[5] = {1, 2, 3, 4, 5};

    int k = 1;
    int l = i;

    const int m = 1;
    const int n = i;

    int &o = i;
    int &p = x;

    const int &r = 1;
    const int &s = i;

    const int t[5] = {1, 2, 3, 4, 5};
    const int u[2] = {1, 2};
    const int v[1] = {0};

    int w[5] = {1, 2, 3, 4, 5};
    int y[2] = {1, 2};
    int z[1] = {0};

    int *aa = &i;
    int *ab = &x;
    int *ac = 0;

    const int *ad = &i;
    const int *ae = &i;
    const int *const af = &x;

    int const *const ag = af;
    int const *const ah = &x;

    int *po[10];
    int *pl[2] = {aa, ab};

    int *pg[2] = {aa, ab};
    int *ai = new int(1);

    int aj[2][2][2] = {{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}}};
  }

  return 0;
}