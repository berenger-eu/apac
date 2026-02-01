#pragma once
#include "AliasArg.hpp"
#include "NodeDependency.hpp"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include <sstream>
#include <unordered_set>
#include <vector>

struct AliasesDependencyHash {
  std::size_t operator()(const std::pair<std::shared_ptr<aliasArg>,
                                         std::shared_ptr<aliasArg>> &p) const {
    return std::hash<std::shared_ptr<aliasArg>>{}(p.first) ^
           std::hash<std::shared_ptr<aliasArg>>{}(p.second);
  }
};
struct AliasesDependencyEqual {
  bool operator()(
      const std::pair<std::shared_ptr<aliasArg>, std::shared_ptr<aliasArg>> &p1,
      const std::pair<std::shared_ptr<aliasArg>, std::shared_ptr<aliasArg>> &p2)
      const {
    return p1.first == p2.first && p1.second == p2.second;
  }
};

struct Instruction {
  clang::Stmt *instruction;
  std::string instructionString; // Instruction string
  bool complexInstruction;

  // If true, the instruction will not be fused with the next one
  // Mostly used when an instruction requires a taskwait (delete,
  // complexInstructions)
  bool noFusion = false;

  unsigned int scopedInstructionsNumber; // Also takes in account number of
                                         // instructions in ComplexInstructions
  // Use the aliasArg as its adress is unique and will contain all available
  // information about the variable (more than necessary)
  std::unordered_map<std::shared_ptr<aliasArg>, NodeDependency> dependencies;
  std::vector<Instruction> scopedInstructions;
  // Contains pairs of variables
  // first element is the alias used in the instruction, second is the variable
  // that is aliased
  // TODO : Modify to use aliasArg (for arrays)
  std::unordered_set<
      std::pair<std::shared_ptr<aliasArg>, std::shared_ptr<aliasArg>>,
      AliasesDependencyHash, AliasesDependencyEqual>
      curAliases;
  Instruction(clang::Stmt *instr = nullptr, std::string instrString = "",
              bool isComplex = false, unsigned int scopedInstrNumber = 0)
      : instruction(instr), instructionString(instrString),
        complexInstruction(isComplex), scopedInstructionsNumber(0) {}
  void dumpDep() const {
    llvm::errs() << "Dependencies for instruction: " << instructionString
                 << "\n";
    for (const auto &dep : dependencies) {
      std::stringstream ss;
      ss << (dep.second.isRead ? "READ " : "")
         << (dep.second.isWrite ? "WRITE " : "") << " "
         << dep.first->dumpAsStr();
      llvm::errs() << ss.str() << "\n";
    }
  }
  void dumpAliases() const {
    llvm::errs() << "Aliases for instruction: " << instructionString << "\n";
    for (const auto &alias : curAliases) {
      std::stringstream ss;
      ss << alias.first->dumpAsStr() << " : " << alias.second->dumpAsStr();
      llvm::errs() << ss.str() << "\n";
    }
  }
  void dump() const {
    dumpDep();
    dumpAliases();
  };
};
