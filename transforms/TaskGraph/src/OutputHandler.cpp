#include "OutputHandler.hpp"
int invisibleNodeCounter = 0;
namespace {
std::pair<std::string, bool> phaseKind(const Instruction &instruction);
// Phase edges are emitted separately from scalar alias edges. Representative
// equality across different pointer names is resolved by OpenMP at runtime.
void generatePhaseLinks(const Graph &graph, std::ofstream &file) {
  std::string mode, anchor, writer;
  std::set<std::string> members, readers;
  std::map<std::string, std::pair<std::string, std::set<std::string>>> locations;
  auto link = [&](const std::string &from, const std::string &to, const std::string &label) {
    if (!from.empty() && from != to)
      file << "    " << from << " -> " << to << " [color=purple,label=\"" << label << "\"];\n";
  };
  for (const auto &node : graph.nodes) {
    const auto &instruction = *node->instructionPtr.front();
    auto id = std::to_string(node->id);
    if (instruction.synchronous || !node->graph.empty()) {
      for (const auto &previous : members) link(previous, id, "phase wait");
      if (!members.empty() || !anchor.empty()) anchor = id;
      mode.clear(); members.clear(); writer.clear(); readers.clear(); locations.clear();
    }
    if (!instruction.arrayAccesses.empty() && !instruction.synchronous) {
      auto kind = phaseKind(instruction);
      if (!mode.empty() && mode != kind.first) {
        auto transition = "array_phase_transition_" + id;
        file << "    " << transition << " [shape=diamond,label=\"" << mode << " -> " << kind.first << "\"];\n";
        for (const auto &previous : members) link(previous, transition, "phase join");
        anchor = transition;
        members.clear(); writer.clear(); readers.clear(); locations.clear();
      }
      mode = kind.first;
      link(anchor, id, "phase");
      members.insert(id);
      if (mode == "whole") {
        link(writer, id, "whole");
        if (kind.second) {
          for (const auto &previous : readers) link(previous, id, "whole");
          readers.clear(); writer = id;
        } else readers.insert(id);
      } else {
        for (const auto &access : instruction.arrayAccesses) {
          auto &location = locations[access.representative];
          link(location.first, id, access.representative);
          if (access.write) {
            for (const auto &previous : location.second) link(previous, id, access.representative);
            location.second.clear(); location.first = id;
          } else location.second.insert(id);
        }
      }
    }
    for (const auto &subgraph : node->graph) generatePhaseLinks(*subgraph, file);
  }
}
} // namespace

void OutputHandler::generateLinks(const Graph &inGraph, std::ofstream &file) {
  for (const auto &node : inGraph.nodes) {

    // Add links, with the dependencies
    std::map<int, std::pair<std::shared_ptr<Node>, NextAliasMap>>
        nodeOrderedMap;
    for (auto &nextNode : node->next) {
      nodeOrderedMap[nextNode.first->id] = {nextNode.first, nextNode.second};
    }
    for (const auto &nextNodeordered : nodeOrderedMap) {
      std::stringstream ss;
      auto &nextNode = nextNodeordered.second;
      for (auto iterBegin = nextNode.second.begin();
           iterBegin != nextNode.second.end(); iterBegin++) {
        if (iterBegin != nextNode.second.begin())
          ss << ",";
        ss << iterBegin->first->varAsString()
           << (iterBegin->second.isRead ? "R" : "")
           << (iterBegin->second.isWrite ? "W" : "");
      }
      file << "    " << node->id << " -> " << nextNode.first->id
           << "[label=\"  " << ss.str() << "\"];\n";
    }
    for (auto subGraph : node->graph)
      generateLinks(*subGraph, file);
  }
}
void OutputHandler::subGenerateDotGraph(const Graph &inGraph,
                                        std::ofstream &file) {
  file << "    invisibleNodeScope_" << invisibleNodeCounter++
       << " [style=invis];\n";
  for (const auto &node : inGraph.nodes) {
    file << "    " << node->id << " [label=\"" << node->instruction;
    for (auto instr : node->instructionPtr) {
      if (!instr->arrayAccesses.empty()) {
        file << "\nphase: " << phaseKind(*instr).first;
        for (const auto &access : instr->arrayAccesses)
          if (!access.representative.empty())
            file << "\n" << access.representative << (access.write ? " RW" : " R");
      }
      std::vector<std::string> aliasLines;
      for (auto alias : instr->curAliases) {
        std::string line;
        const int depth =
            getPtrDepthAccess(alias.first->declaration.getType(),
                              alias.second->declaration.getType(),
                              alias.first->declaration.getASTContext());
        for (int nbStar = 0;
             // TODO : getPtrDepthAccess, result of ptr stored when creating
             // alias?
             nbStar < depth;
             nbStar++)
          line += "*";
        line += alias.first->varAsString() + " : " + alias.second->varAsString();
        aliasLines.push_back(line);
      }
      std::sort(aliasLines.begin(), aliasLines.end());
      for (const auto &line : aliasLines)
        file << "\n" << line;
    }
    file << "\"];\n";
    for (long unsigned int i = 0; i < node->graph.size(); i++) {
      auto subGraph = node->graph[i];
      long unsigned int countGraph = -1;
      for (auto instr : node->instructionPtr) {
        if (countGraph != i) {
          if (instr->complexInstruction)
            countGraph++;
        }
      }
      file << "    " << node->id << " -> invisibleNodeScope_"
           << invisibleNodeCounter
           << "[label=\""
           //  << "instr " << curInstrIndex << " : "
           //  << node->instructionPtr.at(curInstrIndex)->instructionString
           << "\"];\n";
      file << "subgraph cluster_" << node->id << "_" << i << " {\n"
           << "label = \"subGraph" << node->id << "_" << i << "\";\n";
      subGenerateDotGraph(*subGraph, file);
      file << "}\n";
    }
  }
}
void OutputHandler::GenerateDotGraph(const std::vector<Graph> &graphs,
                                     const std::string &filename) {
  std::ofstream file(filename);
  if (file.fail())
    llvm::errs() << "Error generating file " << filename << "\n";
  file << "digraph G {\n";
  int i = 0;
  for (auto graph : graphs) {
    file << "subgraph cluster_f" << i << " {\n";
    subGenerateDotGraph(graph, file);
    file << "}\n";
    i++;
  }
  for (const auto &graph : graphs) {
    generateLinks(graph, file);
    generatePhaseLinks(graph, file);
  }
  file << "}\n";
  file.close();
}
std::string OutputHandler::modifiedStringForInstruction(
    const StmtOrder &instructionsOrderManager, const IfStmt *instr) {
  std::stringstream ssPrint;
  ssPrint << getStmtAsString(instr, TheRewriter.getLangOpts()) << "{\n";
  bool isSecond = false;
  for (auto instrSubGroups : instructionsOrderManager.instructionGroups) {
    if (isSecond && instr->getElse() != nullptr)
      ssPrint << "} else {\n";
    ssPrint << modifyGroup(instructionsOrderManager, instrSubGroups.second,
                           false);
    isSecond = true;
  }
  ssPrint << "}\n";
  return ssPrint.str();
}
std::string OutputHandler::modifiedStringForInstruction(
    const StmtOrder &instructionsOrderManager, const Stmt *instr) {
  std::stringstream ssPrint;
  // If the instruction contains a group of instructions (for,if,...)
  StmtOrder *subOrder = instructionsOrderManager.getSubStmtOrder(instr).get();
  std::string strTemp = std::string(
      TheRewriter.getRewrittenText(instr->getSourceRange()).size(), ' ');

  TheRewriter.ReplaceText(instr->getSourceRange(), strTemp);
  if (subOrder != nullptr) {
    // Special case for if statement, we need to add else
    if (isa<IfStmt>(instr))
      ssPrint << modifiedStringForInstruction(*subOrder, cast<IfStmt>(instr));
    else {
      if (!isa<CompoundStmt>(instr))
        ssPrint << getStmtAsString(instr, TheRewriter.getLangOpts()) << "{\n";
      for (auto instrSubGroups : subOrder->instructionGroups) {
        ssPrint << modifyGroup(*subOrder, instrSubGroups.second, false);
      }
      if (!isa<CompoundStmt>(instr))
        ssPrint << "}\n";
    }
  } else {
    auto node = instructionsOrderManager.getNode(instr);
    if (node && !node->instructionPtr.front()->loopTaskReplacement.empty())
      ssPrint << node->instructionPtr.front()->loopTaskReplacement;
    else
      ssPrint << getStmtAsStringFull(instr, TheRewriter.getLangOpts()) << "\n";
  }
  return ssPrint.str();
}
std::string
OutputHandler::modifyGroup(const StmtOrder &instructionsOrderManager,
                           const InstructionGroup &instructionGroup,
                           bool isWrite) {
  if (instructionGroup.size() == 0)
    return "";

  const Stmt *st = (*instructionGroup.rbegin()).first;
  std::stringstream ssPrint;
  std::string pragmaStart, pragmaEnd;
  createPragmaString(instructionsOrderManager, instructionGroup, pragmaStart,
                     pragmaEnd);

  auto node = instructionsOrderManager.getNode(instructionGroup.begin()->first);
  bool initializedDeclaration = false;
  if (const auto *decl = dyn_cast<DeclStmt>(instructionGroup.begin()->first))
    for (const auto *item : decl->decls())
      if (const auto *variable = dyn_cast<VarDecl>(item))
        initializedDeclaration |= variable->hasInit();
  bool scopeBoundary = instructionGroup.size() == 1 &&
      (instructionGroup.begin()->second != nullptr ||
       initializedDeclaration || isa<CXXDeleteExpr>(instructionGroup.begin()->first) ||
       isa<GotoStmt>(instructionGroup.begin()->first) ||
       (node && node->instructionPtr.front()->synchronous));
  if (scopeBoundary && !arrayPhaseModes_.empty()) {
    ssPrint << "\n#pragma omp taskwait\n";
    arrayPhaseModes_.clear();
  }
  if (node && !scopeBoundary) ssPrint << arrayPhasePrefix(node);
  ssPrint << pragmaStart;

  for (auto instrPair : instructionGroup)
    ssPrint << modifiedStringForInstruction(instructionsOrderManager,
                                            instrPair.first);
  ssPrint << pragmaEnd;
  if (scopeBoundary && !arrayPhaseModes_.empty()) {
    ssPrint << "\n#pragma omp taskwait\n";
    arrayPhaseModes_.clear();
  }

  if (isWrite) {
    for (auto instrPair : instructionGroup) {
      auto instr = instrPair.first;
      TheRewriter.RemoveText(
          SourceRange(instr->getBeginLoc(),
                      Lexer::getLocForEndOfToken(instr->getEndLoc(), 0,
                                                 TheRewriter.getSourceMgr(),
                                                 TheRewriter.getLangOpts())));
    }
    TheRewriter.InsertText(st->getBeginLoc(), ssPrint.str());
  }
  return ssPrint.str();
}

void OutputHandler::isPragmaValid(const StmtOrder &instructionsOrderManager,
                                  const InstructionGroup &instructionGroup,
                                  pragmaStatus &addPragma) const {
  // Group of instructions is empty
  if (instructionGroup.size() == 0)
    return;
  // Group of instructions has only one instruction
  if (instructionGroup.size() == 1) {

    auto instrPair = instructionGroup.begin();
    auto instr = instrPair->first;
    auto node = instructionsOrderManager.getNode(instr);
    if (node && node->instructionPtr.front()->synchronous) {
      addPragma.setTaskFalse();
      return; // An unconditional taskwait precedes the preserved statement.
    }
    // Instruction is a declaration, so no task can be created
    if (isa<DeclStmt>(instr)) {
      addPragma.setTaskFalse();
      addPragma.setTaskWaitFalse();
    }
    // Instruction is a delete, so no task can be created
    else if (isa<CXXDeleteExpr>(instr) || isa<GotoStmt>(instr))
      addPragma.setTaskFalse();
    // Instruction is a complex instruction, so no task should be created
    else if (instrPair->second != nullptr) {
      if (isa<IfStmt>(instr)) {
        addPragma.setTaskFalse();
        auto node = instructionsOrderManager.getNode(instr);
        if (node->inOutDep.size() == 0 && node->inDep.size() == 0)
          addPragma.setTaskWaitFalse();
      } else {
        addPragma.setTaskFalse();
        addPragma.setTaskWaitFalse();
      }
    } else {
      addPragma.setTaskWaitFalse();
    }
  }
  // Check if there is a function call, needed for firstprivate and depth
  // increment
  if (addPragma.isTaskValid) {
    bool hasFunctionCall = false;
    for (auto instrPair : instructionGroup) {
      auto instr = instrPair.first;
      std::stack<const Stmt *> stackStmt;
      stackStmt.push(instr);
      const Stmt *curInstr = nullptr;

      while (!stackStmt.empty() && !hasFunctionCall) {
        curInstr = stackStmt.top();
        stackStmt.pop();

        if (isa<CallExpr>(curInstr)) {
          llvm::errs() << "Function call in task\n";
          hasFunctionCall = true;
        } else {
          for (auto it = curInstr->child_begin(); it != curInstr->child_end();
               ++it) {
            if (isa<Stmt>(*it))
              stackStmt.push(cast<Stmt>(*it));
          }
        }
      }
    }
    if (hasFunctionCall)
      addPragma.setFunctionCallTrue();
  }
}
std::string OutputHandler::createPragmaTaskWait(
    const StmtOrder &instructionsOrderManager,
    const InstructionGroup &instructionGroup) const {
  std::stringstream ssPrint;
  ssPrint << "#pragma omp taskwait ";
  auto instr = instructionGroup.begin()->first;
  // TODO : Implement condition for taskwait as a node
  /*
  auto node = instructionsOrderManager.getNode(instr);

  if (node == nullptr) {
    llvm::errs() << "Node is null\n";
    instr->dump();
    llvm::errs() << instructionsOrderManager.instructionLinks.count(instr)
                 << "\n";
    llvm::errs() << instructionsOrderManager.nodesGroup.count(
                        instructionsOrderManager.instructionLinks.at(instr))
                 << "\n";
    return "";
  }
  node->dump();
  std::string dependString = createDependsString(node);
  */
  std::string dependString;
  auto node = instructionsOrderManager.getNode(instr);
  if (node && node->instructionPtr.front()->synchronous)
    return ssPrint.str();
  if (isa<IfStmt>(instr)) {
    auto ifSt = cast<IfStmt>(instr);
    // createDependsString already includes the depend(...) clauses.
    return ssPrint.str() + createDependsString(instructionsOrderManager.getNode(ifSt));
  } else if (isa<CXXDeleteExpr>(instr)) {
    auto deleteExpr = cast<CXXDeleteExpr>(instr);
    if (isa<DeclRefExpr>(deleteExpr->getArgument()->IgnoreImpCasts())) {

      dependString =
          cast<DeclRefExpr>(deleteExpr->getArgument()->IgnoreImpCasts())
              ->getDecl()
              ->getNameAsString();
    }
  }
  if (!dependString.empty())
    ssPrint << "depend (inout: " << dependString << ")";
  return ssPrint.str();
}
std::string
OutputHandler::createPragmaTaskString(const StmtOrder &instructionsOrderManager,
                                      const InstructionGroup &instructionGroup,
                                      bool hasFunctionCall) const {
  std::stringstream ssPrint;

  auto instr = instructionGroup.begin()->first;
  auto node = instructionsOrderManager.getNode(instr);
  ssPrint << createTaskwaitIndexes(node);
  ssPrint << "#pragma omp task default(shared) ";

  if (node == nullptr) {
    llvm::errs() << "Node is null\n";
    instr->dump();
    llvm::errs() << instructionsOrderManager.instructionLinks.count(instr)
                 << "\n";
    llvm::errs() << instructionsOrderManager.nodesGroup.count(
                        instructionsOrderManager.instructionLinks.at(instr))
                 << "\n";
    return "";
  }
  ssPrint << createDependsString(node);
  ssPrint << arrayPhaseDepends(node);
  ssPrint << createFirstPrivateString(node, hasFunctionCall);
  return ssPrint.str();
}
std::string
OutputHandler::createFirstPrivateString(const std::shared_ptr<Node> &node,
                                        bool hasFunctionCall) const {
  std::stringstream ssPrint;
  std::unordered_set<const DeclRefExpr *> firstPrivateSet;
  for (auto instr : node->instructionPtr) {
    for (auto alias : instr->dependencies) {

      if (alias.first->hasUnknownIndex) {

        assert(alias.first->expression != nullptr);
        alias.first->expression->getIdx()->dump();
        auto indexesExprVector =
            getArraySubscriptsIndexes(alias.first->expression);
        for (auto indexExpr : indexesExprVector) {
          auto variablesVector = getAllDeclRefExprInsideExpr(indexExpr);
          for (auto it = variablesVector.begin(); it != variablesVector.end();
               ++it) {
            firstPrivateSet.insert(*it);
          }
        }
      }
    }
  }
  std::set<std::string> phaseCaptures;
  for (const auto *instruction : node->instructionPtr)
    phaseCaptures.insert(instruction->arrayCaptures.begin(), instruction->arrayCaptures.end());
  for (const auto *ref : firstPrivateSet) {
    const auto *variable = dyn_cast<VarDecl>(ref->getDecl());
    if (variable && variable->isLocalVarDeclOrParm())
      phaseCaptures.insert(variable->getNameAsString());
  }
  if (hasFunctionCall) phaseCaptures.insert("__apac_depth_local");
  if (!phaseCaptures.empty()) {
    ssPrint << " firstprivate(";
    bool first = true;
    for (const auto &name : phaseCaptures) {
      if (!first) ssPrint << ",";
      first = false;
      ssPrint << name;
    }
    ssPrint << ") ";
  }
  return ssPrint.str();
}
std::string
OutputHandler::createDependsString(const std::shared_ptr<Node> &node) const {
  std::stringstream ssPrint;
  const auto &inOutSet = node->inOutDep;
  const auto &inSet = node->inDep;
  // We add the first dependency to a next node to the task as inout
  // And then we add the first dependency from a previous node to this one to
  // the task as in if the dependency is not in the inout set We don't have to
  // add the dependencies from one node to another as one is sufficient As
  // long as we chose the same single one for both of them (which here is
  // always the first one)

  if (inOutSet.size() > 0) {
    ssPrint << " depend (inout: ";
    for (auto it = inOutSet.begin(); it != inOutSet.end(); ++it) {
      if (it != inOutSet.begin())
        ssPrint << ",";
      ssPrint << (*it)->dependencyAsString();
    }
    ssPrint << ") ";
  }
  if (inSet.size() > 0) {
    ssPrint << " depend (in: ";
    for (auto it = inSet.begin(); it != inSet.end(); ++it) {
      if (it != inSet.begin())
        ssPrint << ",";
      ssPrint << (*it)->dependencyAsString();
    }
    ssPrint << ") ";
  }
  return ssPrint.str();
}
void OutputHandler::createPragmaString(
    const StmtOrder &instructionsOrderManager,
    const InstructionGroup &instructionGroup, std::string &pragmaStart,
    std::string &pragmaEnd) {
  struct pragmaStatus addPragma;
  isPragmaValid(instructionsOrderManager, instructionGroup, addPragma);
  std::stringstream ssPragmaStart;
  if (addPragma.isTaskValid) {
    ssPragmaStart << createPragmaTaskString(instructionsOrderManager,
                                            instructionGroup,
                                            addPragma.hasFunctionCall)
                  << "\n{\n";
    if (addPragma.hasFunctionCall)
      ssPragmaStart << "__apac_depth_guard __apac_task_depth(__apac_depth_local + 1);\n";
  } else if (addPragma.isTaskWaitValid)
    ssPragmaStart << createPragmaTaskWait(instructionsOrderManager,
                                          instructionGroup)
                  << "\n";
  pragmaStart = ssPragmaStart.str();
  if (addPragma.isTaskValid)
    pragmaEnd = "}\n";
}

std::string
OutputHandler::createTaskwaitIndexes(const std::shared_ptr<Node> &node) const {
  std::stringstream ssPrint;
  std::unordered_set<const DeclRefExpr *> firstPrivateSet;
  for (auto &inDep : node->inDep) {
    if (inDep->hasUnknownIndex) {
      assert(inDep->expression != nullptr);
      auto indexesExprVector = getArraySubscriptsIndexes(inDep->expression);
      for (auto indexExpr : indexesExprVector) {
        auto variablesVector = getAllDeclRefExprInsideExpr(indexExpr);
        for (auto it = variablesVector.begin(); it != variablesVector.end();
             ++it) {
          firstPrivateSet.insert(*it);
        }
      }
    }
  }
  for (auto &inOutDep : node->inOutDep) {
    if (inOutDep->hasUnknownIndex) {
      assert(inOutDep->expression != nullptr);
      auto indexesExprVector = getArraySubscriptsIndexes(inOutDep->expression);
      for (auto indexExpr : indexesExprVector) {
        auto variablesVector = getAllDeclRefExprInsideExpr(indexExpr);
        for (auto it = variablesVector.begin(); it != variablesVector.end();
             ++it) {
          firstPrivateSet.insert(*it);
        }
      }
    }
  }
  if (!firstPrivateSet.empty()) {
    ssPrint << "#pragma omp taskwait depend (in:  ";
    for (auto &firstPrivate : firstPrivateSet) {
      if (firstPrivate != *firstPrivateSet.begin())
        ssPrint << ",";
      ssPrint << firstPrivate->getDecl()->getNameAsString();
    }
    ssPrint << ")\n";
  }
  return ssPrint.str();
}
namespace {
std::pair<std::string, bool> phaseKind(const Instruction &instruction) {
  auto mode = instruction.arrayAccesses.front().mode;
  auto precision = instruction.arrayAccesses.front().precision;
  bool write = false;
  for (const auto &access : instruction.arrayAccesses) {
    write |= access.write;
    if (access.mode != mode || access.precision != precision) mode = ArrayAccessMode::Whole;
  }
  std::string key = mode == ArrayAccessMode::Whole ? "whole" :
                    mode == ArrayAccessMode::Element ? "element:" + precision : "partition:" + precision;
  return {key, write};
}
}
std::string OutputHandler::arrayPhasePrefix(const std::shared_ptr<Node> &node) {
  if (!node || node->instructionPtr.front()->arrayAccesses.empty()) return "";
  const auto &instruction = *node->instructionPtr.front();
  std::stringstream prefix;
  std::set<std::string> ready;
  for (const auto &name : instruction.arrayAddressInputs) {
    for (const auto &alias : node->inDep)
      if (alias->varAsString() == name) ready.insert(name);
    for (const auto &alias : node->inOutDep)
      if (alias->varAsString() == name) ready.insert(name);
  }
  if (instruction.arrayAddressNeedsWait) prefix << "\n#pragma omp taskwait\n";
  else if (!ready.empty()) {
    prefix << "\n#pragma omp taskwait depend(in: ";
    bool first = true;
    for (const auto &name : ready) {
      if (!first) prefix << ",";
      first = false; prefix << name;
    }
    prefix << ")\n";
  }
  auto kind = phaseKind(instruction).first;
  auto previous = arrayPhaseModes_.find(instruction.arrayPhaseToken);
  if (previous != arrayPhaseModes_.end() && previous->second != kind)
    prefix << "\n#pragma omp task depend(inout: " << instruction.arrayPhaseToken
           << ")\n{}\n";
  arrayPhaseModes_[instruction.arrayPhaseToken] = kind;
  return prefix.str();
}
std::string OutputHandler::arrayPhaseDepends(const std::shared_ptr<Node> &node) const {
  if (!node || node->instructionPtr.front()->arrayAccesses.empty()) return "";
  const auto &instruction = *node->instructionPtr.front();
  auto kind = phaseKind(instruction);
  std::stringstream clauses;
  bool whole = kind.first == "whole";
  clauses << " depend(" << (whole && kind.second ? "inout" : "in") << ": "
          << instruction.arrayPhaseToken << ") ";
  if (!whole) {
    std::map<std::string, bool> representatives;
    for (const auto &access : instruction.arrayAccesses)
      representatives[access.representative] |= access.write;
    for (const auto &entry : representatives)
      clauses << " depend(" << (entry.second ? "inout" : "in") << ": " << entry.first << ") ";
  }
  return clauses.str();
}
