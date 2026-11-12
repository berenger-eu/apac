// Sequential fallback declarations supplied by DuplicateFunctions in the pipeline.
// Isolated TaskGraph snapshots only need signatures. Execution tests use definitions.
#pragma once
int main_apacSeq();
int *inPtrs_apacSeq(int *, int *, int, int **);
int &inRef_apacSeq(int &, int &, char);
void fdummy_apacSeq(int *);
void fdummy_apacSeq(int &);
void fdummyRef_apacSeq(int &);
int work_apacSeq(int);
int increment_apacSeq(int);
int square_apacSeq(int);
int cube_apacSeq(int);
int double_val_apacSeq(int);
int negate_val_apacSeq(int);
int f_apacSeq(int);
int g_apacSeq(int);
int h_apacSeq(int, int);
int add_apacSeq(int, int);
void writeResult_apacSeq(int *, int);
long sum_reduce_apacSeq(int *, int, int);
int computeX_apacSeq(int);
int computeY_apacSeq(int);
int heavy_apacSeq(int);
int *minPtr_apacSeq(int *, int *);
