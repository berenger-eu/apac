#include "Node.hpp"
void Node::dump() {
  llvm::errs() << "Instructions: " << instruction << "\n";
  for (auto &instr : instructionPtr) {
    instr->dump();
  }
  for (const auto &dep : dependencies) {
    llvm::errs() << "Dep: " << dep.first->varAsString()
                 << " Read: " << dep.second.isRead
                 << " Write: " << dep.second.isWrite << "\n";
  }
  for (const auto &dep : inOutDep) {
    llvm::errs() << "InOutDep: " << dep->varAsString() << "\n";
  }
  for (const auto &dep : inDep) {
    llvm::errs() << "InDep: " << dep->varAsString() << "\n";
  }
  llvm::errs() << "\n";
}

void Node::addLink(std::shared_ptr<Node> curN, std::shared_ptr<Node> n,
                   bool isRead, bool isWrite, std::shared_ptr<aliasArg> arg) {
  if (arg == nullptr)
    return;
  if (this->next.count(n) == 0)
    this->next.insert({n, NextAliasMap()});
  if (this->next.at(n).count(arg) == 0) {
    this->next.at(n).insert({arg, {isRead, isWrite}});
    n->prev.insert(curN);
  } else {
    this->next.at(n).at(arg).isRead = this->next.at(n).at(arg).isRead || isRead;
    this->next.at(n).at(arg).isWrite =
        this->next.at(n).at(arg).isWrite || isWrite;
  }
}

void Node::computeInOutDep() {
  for (const auto &dep : dependencies) {
    if (!dep.second.isWrite) {
      continue;
    }
    bool isInOut = false;
    if (isLooped)
      isInOut = true;
    auto itNext = next.begin(), endNext = next.end();
    while (itNext != endNext && !isInOut) {
      if (itNext->second.count(dep.first) > 0) {
        isInOut = true;
        break;
      } else if (dep.first->indexes.size() > 0) {
        for (const auto &nextAlias : itNext->second) {
          if (indexesMatch(nextAlias.first->indexes, dep.first->indexes)) {
            isInOut = true;
            break;
          }
        }
      }
      ++itNext;
    }
    if (isInOut) {
      inOutDep.insert(dep.first);
    }
  }
}
void Node::computeInDep() {
  for (const auto &dep : dependencies) {
    bool isIn = false;
    auto itPrev = prev.begin(), endPrev = prev.end();
    while (itPrev != endPrev && !isIn) {
      if ((*itPrev)->dependencies.count(dep.first) > 0) {
        isIn = true;
      } else if (dep.first->indexes.size() > 0) {
        for (const auto &prevAlias : (*itPrev)->dependencies) {
          if (indexesMatch(prevAlias.first->indexes, dep.first->indexes)) {
            isIn = true;
            break;
          }
        }
      }
      ++itPrev;
    }
    if (isIn && inOutDep.count(dep.first) == 0)
      inDep.insert(dep.first);
  }
}