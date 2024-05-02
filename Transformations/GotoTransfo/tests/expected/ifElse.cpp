#include <memory>
int main() {
  std::unique_ptr<int> __result;

  if (1) {
    __result = std::make_unique<int>(4);
    goto __exit0;
  } else if (1) {
    __result = std::make_unique<int>(5);
    goto __exit0;
  } else {
    __result = std::make_unique<int>(1);
    goto __exit0;
  }
__exit0:
  return *__result;
}