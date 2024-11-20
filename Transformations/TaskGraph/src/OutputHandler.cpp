#include "OutputHandler.hpp"
int invisibleNodeCounter = 0;

void OutputHandler::generateLinks(const Graph &inGraph, std::ofstream &file) {
  for (const auto &node : inGraph.nodes) {

    // Add links, with the dependencies
    for (const auto &nextNode : node->next) {
      std::stringstream ss;
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
    for (auto instr : node->instructionPtr)
      for (auto alias : instr->curAliases) {
        file << "\n";
        const int depth =
            getPtrDepthAccess(alias.first->declaration.getType(),
                              alias.second->declaration.getType(),
                              alias.first->declaration.getASTContext());
        for (int nbStar = 0;
             // TODO : getPtrDepthAccess, result of ptr stored when creating
             // alias?
             nbStar < depth;

             nbStar++)
          file << "*";
        file << alias.first->varAsString() << " : "
             << alias.second->varAsString();
      }
    file << "\"];\n";
    for (long unsigned int i = 0; i < node->graph.size(); i++) {
      auto subGraph = node->graph[i];
      long unsigned int curInstrIndex = 0, countGraph = -1;
      for (auto instr : node->instructionPtr) {
        if (countGraph != i) {
          if (instr->complexInstruction)
            countGraph++;
          if (countGraph != i)
            curInstrIndex++;
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
  for (auto graph : graphs)
    generateLinks(graph, file);
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
      ssPrint << "} else {";
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
  TheRewriter.RemoveText(
      SourceRange(instr->getBeginLoc(),
                  Lexer::getLocForEndOfToken(instr->getEndLoc(), 0,
                                             TheRewriter.getSourceMgr(),
                                             TheRewriter.getLangOpts())));
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
  } else
    ssPrint << getStmtAsStringFull(instr, TheRewriter.getLangOpts()) << "\n";
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

  ssPrint << pragmaStart;

  for (auto instrPair : instructionGroup)
    ssPrint << modifiedStringForInstruction(instructionsOrderManager,
                                            instrPair.first);
  ssPrint << pragmaEnd;

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

bool OutputHandler::isPragmaValid(
    const StmtOrder &instructionsOrderManager,
    const InstructionGroup &instructionGroup) const {
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
  if (isa<IfStmt>(instr)) {
    auto ifSt = cast<IfStmt>(instr);
    dependString = createDependsString(instructionsOrderManager.getNode(ifSt));
  }
  if (dependString.length() == 0)
    return "";
  ssPrint << dependString;
  return ssPrint.str();
}
std::string OutputHandler::createPragmaTaskString(
    const StmtOrder &instructionsOrderManager,
    const InstructionGroup &instructionGroup) const {
  std::stringstream ssPrint;
  ssPrint << "#pragma omp task ";
  auto instr = instructionGroup.begin()->first;
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
  ssPrint << createDependsString(node);
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
    ssPrint << " depend (inout:";
    for (auto it = inOutSet.begin(); it != inOutSet.end(); ++it) {
      if (it != inOutSet.begin())
        ssPrint << ",";
      ssPrint << (*it)->varAsString();
    }
    ssPrint << ") ";
  }
  if (inSet.size() > 0) {
    ssPrint << " depend (in:";
    for (auto it = inSet.begin(); it != inSet.end(); ++it) {
      if (it != inSet.begin())
        ssPrint << ",";
      ssPrint << (*it)->varAsString();
    }
    ssPrint << ") ";
  }
  return ssPrint.str();
}
void OutputHandler::createPragmaString(
    const StmtOrder &instructionsOrderManager,
    const InstructionGroup &instructionGroup, std::string &pragmaStart,
    std::string &pragmaEnd) {
  bool addPragma = isPragmaValid(instructionsOrderManager, instructionGroup);
  std::stringstream ssPragmaStart;
  if (addPragma)
    ssPragmaStart << createPragmaTaskString(instructionsOrderManager,
                                            instructionGroup)
                  << "\n{\n";
  else
    ssPragmaStart << createPragmaTaskWait(instructionsOrderManager,
                                          instructionGroup)
                  << "\n";
  pragmaStart = ssPragmaStart.str();
  if (addPragma)
    pragmaEnd = "}\n";
}