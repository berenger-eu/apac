#pragma once
#include "clang/AST/Decl.h"
#include <vector>
struct key_struct {
  const clang::NamedDecl *decl;
  std::vector<int> indices;
  void dump() const {
    // Dump the contents of the key_struct
    llvm::errs() << decl->getNameAsString();
    for (const auto &index : indices) {
      llvm::errs() << " [" << index << "]";
    }
    llvm::errs() << "\n";
  }
};
struct key_hash {
  std::size_t operator()(const key_struct p) const {
    auto h1 = std::hash<const clang::NamedDecl *>{}(p.decl);
    auto h2 = 0;
    for (const auto &element : p.indices) {
      h2 ^= std::hash<int>{}(element);
    }
    return h1 ^ h2;
  }
};

struct key_equal {
  bool operator()(const key_struct p1, key_struct p2) const {
    return p1.decl == p2.decl; //&& p1.indices == p2.indices;
  }
};
