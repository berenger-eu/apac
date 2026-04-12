// Ce bench fait 4 reductions divide-and-conquer sur un tableau de 500M entiers.

long sum_reduce(int *data, int start, int end) {
    if (end - start <= 1) {
        return data[start];
    }
    int mid;
    mid = start + (end - start) / 2;
    long left;
    left = sum_reduce(data, start, mid);
    long right;
    right = sum_reduce(data, mid, end);
    return left + right;
}

long weighted_reduce(int *data, int start, int end) {
    if (end - start <= 1) {
        return data[start] * data[start];
    }
    int mid;
    mid = start + (end - start) / 2;
    long left;
    left = weighted_reduce(data, start, mid);
    long right;
    right = weighted_reduce(data, mid, end);
    return left + right;
}

long alt_reduce(int *data, int start, int end) {
    if (end - start <= 1) {
        return data[start] * 3 + 7;
    }
    int mid;
    mid = start + (end - start) / 2;
    long left;
    left = alt_reduce(data, start, mid);
    long right;
    right = alt_reduce(data, mid, end);
    return left + right;
}

long combo_reduce(int *data, int start, int end) {
    if (end - start <= 1) {
        return data[start] + data[start] * data[start];
    }
    int mid;
    mid = start + (end - start) / 2;
    long left;
    left = combo_reduce(data, start, mid);
    long right;
    right = combo_reduce(data, mid, end);
    return left + right;
}

volatile long result_sink = 0;

int main() {
    int N;
    N = 500000000;
    int *data;
    data = new int[N];
    int i;
    i = 0;
    int val;
    val = 1;
    long r1;
    r1 = sum_reduce(data, 0, N);
    long r2;
    r2 = weighted_reduce(data, 0, N);
    long r3;
    r3 = alt_reduce(data, 0, N);
    long r4;
    r4 = combo_reduce(data, 0, N);
    long total;
    total = r1 + r2 + r3 + r4;
    result_sink = total;
    return 0;
}
