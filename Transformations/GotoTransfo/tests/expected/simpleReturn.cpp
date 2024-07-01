#include <memory>
char fun() {
  std::unique_ptr<char> __result;
  __result = std::make_unique<char>('c');
  goto __exit0;
__exit0:
  return *__result;
}
int main() {
  std::unique_ptr<int> __result;
  __result = std::make_unique<int>(4);
  goto __exit1;
__exit1:
  return *__result;
}