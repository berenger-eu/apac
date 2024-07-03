#include "InstructionsOrderManager.hpp"

void StmtOrder::moveInstruction(const Stmt *key1, const Stmt *key2) {
  const Stmt *key, *newKey;

  if (key1 == nullptr || key2 == nullptr)
    return;
  // Choose the key with the biggest location (so further in the code), to avoid
  // moving an instruction behind a needed declaration
  if (key1->getBeginLoc() > key2->getBeginLoc()) {
    key = key2;
    newKey = key1;
  } else {
    key = key1;
    newKey = key2;
  }

  if (instructionLinks.count(key) == 0)
    return;
  if (instructionLinks.count(newKey) == 0)
    return;
  auto groupNumberOld = instructionLinks.at(key);
  if (instructionGroups.count(groupNumberOld) == 0)
    return;
  auto groupNumberNew = instructionLinks.at(newKey);
  if (instructionGroups.count(groupNumberNew) == 0)
    return;

  auto &instructionGroup = instructionGroups.at(groupNumberOld);
  auto &instructionGroupNew = instructionGroups.at(groupNumberNew);
  for (auto instr : instructionGroup) {
    instructionGroupNew.insert(instr);
    instructionLinks.at(instr.first) = groupNumberNew;
  }
  instructionGroups.erase(groupNumberOld);
}
void fuseInstructions(const std::vector<const Stmt *> vect,
                      StmtOrder &instructionsOrderManager) {
  if (vect.size() < 2)
    return;
  const Stmt *key = vect.at(0);
  for (auto instr : vect) {
    if (instr == key)
      continue;
    if (key->getBeginLoc() < instr->getBeginLoc()) {
      key = instr;
    }
  }
  for (auto instr : vect) {
    if (instr == key)
      continue;
    instructionsOrderManager.moveInstruction(key, instr);
  }
}

std::string
modifiedStringForInstruction(Rewriter &TheRewriter,
                             const StmtOrder &instructionsOrderManager,
                             const Stmt *instr) {
  std::stringstream ssPrint;
  // If the instruction contains a group of instructions (for,if,...)
  StmtOrder *subOrder = instructionsOrderManager.getSubStmtOrder(instr).get();
  if (subOrder != nullptr) {
    ssPrint << getStmtAsString(instr, TheRewriter.getLangOpts()) << "{\n";
    for (auto instrSubGroups : subOrder->instructionGroups) {
      bool addPragma = isPragmaValid(*subOrder, instrSubGroups.second);
      if (addPragma)
        ssPrint << "#pragma \n{\n";
      for (auto instrPair : instrSubGroups.second) {
        auto instr = instrPair.first;
        ssPrint << modifiedStringForInstruction(TheRewriter, *subOrder, instr)
                << "\n";
      }
      if (addPragma)
        ssPrint << "}\n";
    }
    ssPrint << "}\n";
  } else
    ssPrint << getStmtAsStringFull(instr, TheRewriter.getLangOpts()) << "\n";
  llvm::errs() << "Modified instruction: " << ssPrint.str();
  return ssPrint.str();
}

void modifyFile(Rewriter &TheRewriter,
                const StmtOrder &instructionsOrderManager) {
  for (const auto &instructionGroup :
       instructionsOrderManager.instructionGroups) {
    // Remove old text
    auto instructionGroupSet = instructionGroup.second;
    if (instructionGroupSet.size() == 0)
      continue;

    const Stmt *st = (*instructionGroupSet.rbegin()).first;
    TheRewriter.RemoveText(SourceRange(
        st->getBeginLoc(), Lexer::getLocForEndOfToken(
                               st->getEndLoc(), 0, TheRewriter.getSourceMgr(),
                               TheRewriter.getLangOpts())));
    std::stringstream ssPrint;
    bool addPragma =
        isPragmaValid(instructionsOrderManager, instructionGroupSet);
    if (addPragma)
      ssPrint << "#pragma \n{\n";
    llvm::errs() << ssPrint.str();
    for (auto instrPair : instructionGroupSet) {
      auto instr = instrPair.first;
      TheRewriter.RemoveText(
          SourceRange(instr->getBeginLoc(),
                      Lexer::getLocForEndOfToken(instr->getEndLoc(), 0,
                                                 TheRewriter.getSourceMgr(),
                                                 TheRewriter.getLangOpts())));
      ssPrint << modifiedStringForInstruction(TheRewriter,
                                              instructionsOrderManager, instr);
    }
    if (addPragma)
      ssPrint << "}\n";
    llvm::errs() << "Modified instruction2: " << ssPrint.str();

    TheRewriter.InsertText(st->getBeginLoc(), ssPrint.str());
  }
}

bool isPragmaValid(const StmtOrder &instructionsOrderManager,
                   const auto &instructionGroup) {

  // Group of instructions is empty
  if (instructionGroup.size() == 0)
    return false;
  // Group of instructions has only one instruction
  if (instructionGroup.size() == 1) {

    auto instrPair = instructionGroup.begin();
    auto instr = instrPair->first;
    // Instruction is a declaration, so no task can be created
    if (isa<DeclStmt>(instr))
      return false;
    // Instruction is a complex instruction, so no task should be created
    std::shared_ptr<Node> node;
    if (instrPair->second != nullptr)
      return false;
  }
  return true;
}