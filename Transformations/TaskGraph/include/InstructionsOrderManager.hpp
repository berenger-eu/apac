#pragma once
#include "Node.hpp"
#include "common.hpp"
#include "clang/AST/Stmt.h"
#include "clang/Lex/Lexer.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <deque>
#include <set>
#include <unordered_map>
// Key will be the instruction
// value will be the list of instructions that will be put in place of the
// instruction Each key in the map will correspond to a #pragma scope This
// structure will be used to easily fuse and swap instructions
using namespace clang;

struct StmtOrder;
struct SmallerBeginLocation {
  bool operator()(
      const std::pair<const Stmt *, std::shared_ptr<StmtOrder>> &a,
      const std::pair<const Stmt *, std::shared_ptr<StmtOrder>> &b) const {
    return a.first->getBeginLoc() <= b.first->getBeginLoc();
  }
};
struct StmtOrder {
  bool isLooped;
  int groupCounter = 0;
  // The key is the instruction, the value is its associated group number
  std::map<const Stmt *, int> instructionLinks;
  // The key is the group number, the value is a set of pairs of instructions
  // and sub groups(with their own sub order)
  std::map<int, std::set<std::pair<const Stmt *, std::shared_ptr<StmtOrder>>,
                         SmallerBeginLocation>>
      instructionGroups;
  // The key is the group number, the value is the corresponding node
  std::map<int, std::shared_ptr<Node>> nodesGroup;
  StmtOrder() : isLooped(false) {}
  inline void addInstructionToManager(const Stmt *key) {
    instructionLinks.insert({key, groupCounter});
    std::set<std::pair<const Stmt *, std::shared_ptr<StmtOrder>>,
             SmallerBeginLocation>
        temp;
    instructionGroups.insert({groupCounter, temp});
    // Looped stmts are treated differently
    std::shared_ptr<StmtOrder> tempOrder;
    if (isa<ForStmt>(key)) {
      tempOrder = std::make_shared<StmtOrder>();
      tempOrder->isLooped = true;
    }
    // All of the other non loops instructions adding a new scope (CompoundStmt
    // might be better to use, but it might lead to other issues with other
    // elements)
    else if (isa<IfStmt>(key) || isa<CompoundStmt>(key)) {
      tempOrder = std::make_shared<StmtOrder>();
    }
    instructionGroups.at(groupCounter++).insert({{key, tempOrder}});
  }
  inline void addNodeToGroup(const std::shared_ptr<Node> &node) {
    if (node == nullptr || node->instructionPtr.empty())
      return;
    auto st = node->instructionPtr.front()->instruction;

    assert(instructionLinks.count(st) != 0 &&
           "Instruction not found in the order manager\n");
    if (instructionLinks.count(st) == 0)
      return;
    int groupNum = instructionLinks.at(st);
    if (nodesGroup.count(groupNum) == 0)
      nodesGroup.insert({groupNum, node});
  }
  // Find the Sub order of an instruction and return it
  // Warning on nullptr found (which most likely means an error)
  inline std::shared_ptr<StmtOrder> getSubStmtOrder(const Stmt *key) const {
    assert(instructionLinks.count(key) != 0);
    if (instructionLinks.count(key) == 0)
      return nullptr;
    int temp = instructionLinks.at(key);
    assert(instructionGroups.count(temp) != 0);
    if (instructionGroups.count(temp) == 0)
      return nullptr;
    //
    for (const auto &instr : instructionGroups.at(temp))
      if (instr.first == key) {
        if (instr.second == nullptr)
          llvm::errs()
              << "Warning : Looking for a subStmtOrder and found nullptr\n";
        return instr.second;
      }
    return nullptr;
  }
  inline std::shared_ptr<Node> getNode(const Stmt *key) const {
    if (instructionLinks.count(key) == 0 ||
        nodesGroup.count(instructionLinks.at(key)) == 0)
      return nullptr;
    return nodesGroup.at(instructionLinks.at(key));
  }

  void dump() const {
    llvm::errs() << "Dumping StmtOrder\n";
    for (const auto &instruction : instructionGroups) {
      llvm::errs() << "Group " << instruction.first << "\n";
      for (const auto &instr : instruction.second) {
        instr.first->dump();
        if (instr.second != nullptr) {
          instr.second->dump();
        }
      }
    }
  }
  // Moves the group of the instruction closer to the start of the code to the
  // group of the instruction closer to the end of the code
  void moveInstruction(const Stmt *key1, const Stmt *key2);
} typedef StmtOrder;
// typedef std::map<const Stmt*,std::deque<const Stmt*>> instructionsOrder;
// To fuse the groups of each given instruction together
void fuseInstructions(const std::vector<const Stmt *>,
                      StmtOrder &instructionsOrderManager);