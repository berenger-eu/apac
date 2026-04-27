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

int main() {
    int *data;
    data = new int[100];
    long r1;
    r1 = sum_reduce(data, 0, 50);
    long r2;
    r2 = sum_reduce(data, 50, 100);
    long total;
    total = r1 + r2;
    return 0;
}
