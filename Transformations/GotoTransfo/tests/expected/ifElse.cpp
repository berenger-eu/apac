int main() {
  int __result;

  if (1) {
    __result = 4;
    goto __exit1;
  } else if (1) {
    __result = 5;
    goto __exit1;
  } else {
    __result = 1;
    goto __exit1;
  }
  if (1) {
    __result = 8;
    goto __exit1;
  } else if (4) {
    __result = 7;
    goto __exit1;
  } else {
    __result = 9;
    goto __exit1;
  }
__exit1:
  return __result;
}