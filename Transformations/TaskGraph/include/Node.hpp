#pragma once
#include "Instruction.hpp"
#include "clang/AST/Decl.h"
using namespace clang;
struct Node {

  static int idCounter;
  int id;
  bool isLooped;
  std::unordered_map<std::shared_ptr<Node>,
                     std::unordered_map<const NamedDecl *, NodeDependency>>
      next;
  std::unordered_set<std::shared_ptr<Node>> prev;
  std::vector<std::shared_ptr<struct Graph>> graph;
  std::string instruction;
  std::vector<const Instruction *> instructionPtr;
  std::unordered_map<const NamedDecl *, NodeDependency> dependencies;
  std::unordered_set<const NamedDecl *> inOutDep;
  std::unordered_set<const NamedDecl *> inDep;
  Node() : id(idCounter++), isLooped(false) {}
  void dump();
  void addLink(std::shared_ptr<Node> curN, std::shared_ptr<Node> n, bool isRead,
               bool isWrite, const NamedDecl *arg);
  inline void addReadLink(std::shared_ptr<Node> curN, std::shared_ptr<Node> n,
                          const NamedDecl *arg) {
    addLink(curN, n, true, false, arg);
  }
  inline void addWriteLink(std::shared_ptr<Node> curN, std::shared_ptr<Node> n,
                           const NamedDecl *arg) {
    addLink(curN, n, false, true, arg);
  }
  inline void computeInOutInDep() {
    computeInOutDep();
    computeInDep();
  }
  void computeInOutDep();
  void computeInDep();
};
