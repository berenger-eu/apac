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
