// Ce bench fait du hash sur un tableau de 200 millions d'entiers et divise le travail en 4 tâches.
// Il fait des appels imbriques de fonctions ce qui permet de tester la capacite d'APAC à detecter les var read-only dans des chaines d'appels.

long hash_element(int *arr, int idx) {
    return (long)arr[idx] * (long)arr[idx] + (long)arr[idx] * 31 + 17;
}

long hash_reduce(int *arr, int start, int end) {
    if (end - start <= 1) {
        return hash_element(arr, start);
    }
    int mid;
    mid = start + (end - start) / 2;
    long left;
    left = hash_reduce(arr, start, mid);
    long right;
    right = hash_reduce(arr, mid, end);
    return left ^ right;
}

long full_hash(int *arr, int offset, int len) {
    if (len <= 1) {
        return hash_element(arr, offset);
    }
    int half;
    half = len / 2;
    long base;
    base = hash_reduce(arr, offset, offset + len);
    long check;
    check = hash_reduce(arr, offset, offset + half);
    return base ^ check;
}

volatile long result_sink = 0;

int main() {
    int N;
    N = 200000000;
    int *data;
    data = new int[N];
    int quarter;
    quarter = N / 4;
    long h1;
    h1 = full_hash(data, 0, quarter);
    long h2;
    h2 = full_hash(data, quarter, quarter);
    long h3;
    h3 = full_hash(data, quarter * 2, quarter);
    long h4;
    h4 = full_hash(data, quarter * 3, quarter);
    long total;
    total = h1 ^ h2 ^ h3 ^ h4;
    result_sink = total;
    return 0;
}
