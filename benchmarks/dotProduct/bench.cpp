// Ce bench fait du produit scalaire sur deux tableaux de 100 millions d'entiers et divise le travail en 4 tâches.

long dot_product(int *a, int *b, int start, int end) {
    if (end - start <= 1) {
        return (long)a[start] * (long)b[start];
    }
    int mid;
    mid = start + (end - start) / 2;
    long left;
    left = dot_product(a, b, start, mid);
    long right;
    right = dot_product(a, b, mid, end);
    return left + right;
}

long norm_squared(int *v, int start, int end) {
    if (end - start <= 1) {
        return (long)v[start] * (long)v[start];
    }
    int mid;
    mid = start + (end - start) / 2;
    long left;
    left = norm_squared(v, start, mid);
    long right;
    right = norm_squared(v, mid, end);
    return left + right;
}

long diff_norm(int *a, int *b, int start, int end) {
    if (end - start <= 1) {
        return ((long)a[start] - (long)b[start]) * ((long)a[start] - (long)b[start]);
    }
    int mid;
    mid = start + (end - start) / 2;
    long left;
    left = diff_norm(a, b, start, mid);
    long right;
    right = diff_norm(a, b, mid, end);
    return left + right;
}

long sum_both(int *a, int *b, int start, int end) {
    if (end - start <= 1) {
        return (long)a[start] + (long)b[start];
    }
    int mid;
    mid = start + (end - start) / 2;
    long left;
    left = sum_both(a, b, start, mid);
    long right;
    right = sum_both(a, b, mid, end);
    return left + right;
}

volatile long result_sink = 0;

int main() {
    int N;
    N = 100000000;
    int *a;
    a = new int[N];
    int *b;
    b = new int[N];
    long dp;
    dp = dot_product(a, b, 0, N);
    long na;
    na = norm_squared(a, 0, N);
    long nb;
    nb = norm_squared(b, 0, N);
    long dn;
    dn = diff_norm(a, b, 0, N);
    long sb;
    sb = sum_both(a, b, 0, N);
    long total;
    total = dp + na + nb + dn + sb;
    result_sink = total;
    return 0;
}
