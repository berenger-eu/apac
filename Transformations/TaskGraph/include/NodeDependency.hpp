enum class Access { READ, WRITE };
struct NodeDependency {
  bool isRead;
  bool isWrite;
};
inline NodeDependency operator+(const NodeDependency &lhs,
                                const NodeDependency &rhs) {
  return NodeDependency{lhs.isRead || rhs.isRead, lhs.isWrite || rhs.isWrite};
}