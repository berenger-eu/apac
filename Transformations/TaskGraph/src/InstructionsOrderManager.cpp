#include "InstructionsOrderManager.hpp"

void StmtOrder::moveInstruction(const Stmt *key1, const Stmt *key2) {
  const Stmt *key, *newKey;

  if (key1 == nullptr || key2 == nullptr)
    return;
  // Choose the key with the biggest location (so further in the code), to avoid
  // moving an instruction behind a needed declaration
  // New key is for the group where we want to move the instruction
  if (key1->getBeginLoc() > key2->getBeginLoc()) {
    key = key2;
    newKey = key1;
  } else {
    key = key1;
    newKey = key2;
  }
  assert((instructionLinks.count(key) != 0 ||
          instructionLinks.count(newKey) != 0) &&
         "Instruction not found in the order manager\n");
  if (instructionLinks.count(key) == 0 || instructionLinks.count(newKey) == 0)
    return;

  auto groupNumberOld = instructionLinks.at(key);
  auto groupNumberNew = instructionLinks.at(newKey);
  assert(instructionGroups.count(groupNumberOld) != 0 &&
         instructionGroups.count(groupNumberNew) != 0 && "Group is empty\n");
  if (instructionGroups.count(groupNumberOld) == 0 ||
      instructionGroups.count(groupNumberNew) == 0)
    return;
  auto &instructionGroup = instructionGroups.at(groupNumberOld);
  auto &instructionGroupNew = instructionGroups.at(groupNumberNew);
  // Insert all the instructions from the old group to the new group
  for (auto instr : instructionGroup) {
    instructionGroupNew.insert(instr);
    instructionLinks.at(instr.first) = groupNumberNew;
  }
  // Remove the old group
  instructionGroups.erase(groupNumberOld);
}
void fuseInstructions(const std::vector<const Stmt *> vect,
                      StmtOrder &instructionsOrderManager) {
  if (vect.size() < 2)
    return;
  // We find the 'maximum' instruction, the one that is the furthest in the code
  const Stmt *key = vect.at(0);
  for (auto instr : vect) {
    if (instr == key)
      continue;
    if (key->getBeginLoc() < instr->getBeginLoc()) {
      key = instr;
    }
  }
  // We move all the instructions to the group of the 'maximum' instruction
  for (auto instr : vect) {
    if (instr == key)
      continue;
    instructionsOrderManager.moveInstruction(key, instr);
  }
}
