int funct() { return 0; }
int main() {
#pragma omp parallel num_threads(nb_cores)
#pragma omp master
  { funct(); }
}