#include <memory>
bool h() {
  std::unique_ptr<bool> __result;

  __result = std::make_unique<bool>(true);
  goto __exit0;
__exit0:
  return *__result;
}