class machin {
public:
  void fun1(int a) const {}
  void fun1(int b) {}
};
class machin2 : public machin {
public:
  void fun1(int c) {}
};