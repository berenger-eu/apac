int funct() { return 0; }
int main_apacSeq() {
  funct();
  return 0;
}
int main() {
  bool __apac_depth_ok = true;
  if (__apac_depth_ok) {

    funct();
  } else {
    main_apacSeq();
  }
  return 0;
}