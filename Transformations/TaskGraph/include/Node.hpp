#pragma once
#include "PotTaskGraphInterface.hpp"
#include "clang/AST/Decl.h"
using namespace clang;
struct Node {

  static int idCounter;
  int id;
  std::unordered_map<std::shared_ptr<Node>,
                     std::unordered_map<const NamedDecl *, NodeDependency>>
      next;
  std::unordered_set<std::shared_ptr<Node>> prev;
  std::vector<std::shared_ptr<struct Graph>> graph;
  std::string instruction;
  std::vector<const Instruction *> instructionPtr;
  void dump() {
    llvm::errs() << "Instructions: " << instruction << "\n";
    for (auto &instr : instructionPtr) {

      instr->dump();
    }
    llvm::errs() << "\n";
  }
  void addLink(std::shared_ptr<Node> curN, std::shared_ptr<Node> n, bool isRead,
               bool isWrite, const NamedDecl *arg) {
    if (arg == nullptr)
      return;
    if (this->next.count(n) == 0)
      this->next.insert(
          {n, std::unordered_map<const NamedDecl *, NodeDependency>()});
    if (this->next.at(n).count(arg) == 0) {
      this->next.at(n).insert({arg, {isRead, isWrite}});
      n->prev.insert(curN);
    } else {
      this->next.at(n).at(arg).isRead =
          this->next.at(n).at(arg).isRead || isRead;
      this->next.at(n).at(arg).isWrite =
          this->next.at(n).at(arg).isWrite || isWrite;
    }
  }
  inline void addReadLink(std::shared_ptr<Node> curN, std::shared_ptr<Node> n,
                          const NamedDecl *arg) {
    addLink(curN, n, true, false, arg);
  }
  inline void addWriteLink(std::shared_ptr<Node> curN, std::shared_ptr<Node> n,
                           const NamedDecl *arg) {
    addLink(curN, n, false, true, arg);
  }
};
