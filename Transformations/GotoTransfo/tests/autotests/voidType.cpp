void f();
void f() {
  int a = 1;
  return;
}
void g() {
  int b = 1;
  if (b)
    return;
  else
    return;
}

int main() {
  f();
  g();
  return 0;
}