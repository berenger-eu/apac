#pragma once
#include "AliasArg.hpp"
#include "Instruction.hpp"
#include "clang/AST/Decl.h"
#include <set>
using namespace clang;
struct Node;
struct aliasArgComparator {
  bool operator()(const std::shared_ptr<aliasArg> &lhs,
                  const std::shared_ptr<aliasArg> &rhs) const {
    return lhs->id < rhs->id;
  }
};
using NextAliasMap =
    std::map<std::shared_ptr<aliasArg>, NodeDependency, aliasArgComparator>;
struct Node {
  int graphId;
  static int idCounter;
  int id;
  bool isLooped;
  std::unordered_map<std::shared_ptr<Node>, NextAliasMap> next;
  std::unordered_set<std::shared_ptr<Node>> prev;
  // Subgraph(s)
  std::vector<std::shared_ptr<struct Graph>> graph;
  std::string instruction;
  std::vector<const Instruction *> instructionPtr;
  std::unordered_map<std::shared_ptr<aliasArg>, NodeDependency> dependencies;
  std::set<std::shared_ptr<aliasArg>, aliasArgComparator> inOutDep;
  std::set<std::shared_ptr<aliasArg>, aliasArgComparator> inDep;
  Node() : id(idCounter++), isLooped(false) {}
  void dump();
  void addLink(std::shared_ptr<Node> curN, std::shared_ptr<Node> n, bool isRead,
               bool isWrite, std::shared_ptr<aliasArg> arg);
  inline void addReadLink(std::shared_ptr<Node> curN, std::shared_ptr<Node> n,
                          std::shared_ptr<aliasArg> arg) {
    addLink(curN, n, true, false, arg);
  }
  inline void addWriteLink(std::shared_ptr<Node> curN, std::shared_ptr<Node> n,
                           std::shared_ptr<aliasArg> arg) {
    addLink(curN, n, false, true, arg);
  }
  inline void computeInOutInDep() {
    computeInOutDep();
    computeInDep();
  }
  void computeInOutDep();
  void computeInDep();
};
