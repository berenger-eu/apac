enum class Access { READ, WRITE };
struct NodeDependency {
  bool isRead;
  bool isWrite;
  void dump() const {
    llvm::errs() << "Read: " << isRead << " Write: " << isWrite << "\n";
  }
};
inline NodeDependency operator+(const NodeDependency &lhs,
                                const NodeDependency &rhs) {
  return NodeDependency{lhs.isRead || rhs.isRead, lhs.isWrite || rhs.isWrite};
}