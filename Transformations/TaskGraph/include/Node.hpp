#pragma once
#include "AliasArg.hpp"
#include "Instruction.hpp"
#include "clang/AST/Decl.h"
using namespace clang;
struct Node {

  static int idCounter;
  int id;
  bool isLooped;
  std::unordered_map<
      std::shared_ptr<Node>,
      std::unordered_map<std::shared_ptr<aliasArg>, NodeDependency>>
      next;
  std::unordered_set<std::shared_ptr<Node>> prev;
  // Subgraph(s)
  std::vector<std::shared_ptr<struct Graph>> graph;
  std::string instruction;
  std::vector<const Instruction *> instructionPtr;
  std::unordered_map<std::shared_ptr<aliasArg>, NodeDependency> dependencies;
  std::unordered_set<std::shared_ptr<aliasArg>> inOutDep;
  std::unordered_set<std::shared_ptr<aliasArg>> inDep;
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
