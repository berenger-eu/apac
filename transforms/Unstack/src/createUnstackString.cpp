#include "createUnstackString.hpp"

// Transform a CallExpr into a varTemp and adds the unstacked call before
// instructionBegin
using namespace unstack;
void UnstackTransformer::unstackTransformCallExpr(
    CallExpr *calExp, const SourceLocation &instructionBegin,
    int &tempVarsCounter) {

  callRoots[instructionBegin].push_back(
      std::make_shared<Node>(calExp, tempVarsCounter));
  auto rootNode = callRoots[instructionBegin].back();
  buildSubTree(rootNode, tempVarsCounter);
}
void UnstackTransformer::buildSubTree(std::shared_ptr<Node> rootNode,
                                      int &tempVarsCounter) {
  std::vector<CallExpr *> subTopCalls;
  auto &calExp = rootNode->call;
  for (auto arg : calExp->arguments()) {
    findTopCallsInExpr(arg, subTopCalls);
  }
  for (auto call : subTopCalls) {
    llvm::errs() << getExprAsString(call, TheRewriter.getLangOpts()) << "\n";
    rootNode->children.insert(
        {call, std::make_shared<Node>(call, tempVarsCounter)});
    buildSubTree(rootNode->children[call], tempVarsCounter);
  }
}

void UnstackTransformer::modifyCalls() {
  for (auto locRootsPair : callRoots) {
    std::stringstream SSresultLoc;
    for (auto root : locRootsPair.second) {
      SSresultLoc << createTempVarStringHelper(root, true);
    }
    TheRewriter.InsertText(locRootsPair.first, SSresultLoc.str());
    std::vector<std::shared_ptr<Node>> nodes;
    for (auto root : locRootsPair.second) {
      auto &callExp = root->call;
      std::vector<CallExpr *> subTopCalls;
      for (auto arg : callExp->arguments()) {
        findTopCallsInExpr(arg, subTopCalls);
      }
      for (auto call : subTopCalls) {
        nodes.push_back(root->children[call]);
      }
    }
    for (auto node : nodes) {
      auto &callExp = node->call;
      std::stringstream SSresult;
      SSresult << " __tempVar_" << node->id << " ";
      llvm::errs() << SSresult.str() << "\n";
      TheRewriter.ReplaceText(callExp->getSourceRange(), SSresult.str());
    }
  }
}
std::string
UnstackTransformer::createTempVarStringHelper(std::shared_ptr<Node> node,
                                              bool isRoot = false) {
  std::stringstream SSresult;
  for (auto child : node->children) {
    SSresult << createTempVarStringHelper(child.second);
  }
  if (!isRoot)
    SSresult << createTempVarString(node);

  return SSresult.str();
}

std::string
UnstackTransformer::createCallArgString(std::shared_ptr<Node> node,
                                        Expr *argExpr,
                                        long unsigned int &childCounter) {
  if (!argExpr)
    return "";
  class TempPrinter : public PrinterHelper {
  public:
    explicit TempPrinter(const std::shared_ptr<Node> &node) : node(node) {}
    bool handledStmt(Stmt *stmt, llvm::raw_ostream &out) override {
      auto *call = dyn_cast<CallExpr>(stmt);
      auto it = node->children.find(call);
      if (it == node->children.end())
        return false;
      out << " __tempVar_" << it->second->id << " ";
      return true;
    }

  private:
    const std::shared_ptr<Node> &node;
  } printer(node);
  std::string result;
  llvm::raw_string_ostream stream(result);
  argExpr->printPretty(stream, &printer,
                       PrintingPolicy(TheRewriter.getLangOpts()));
  return result;
}

// Create string : type __tempVar_x; __tempVar_x = unstackedCall;
std::string
UnstackTransformer::createTempVarString(std::shared_ptr<Node> node) {
  std::stringstream SSresult;
  // Get the return type of the function
  auto &calExp = node->call;
  auto &nodeID = node->id;
  std::string functionType =
      calExp->getType().getAsString(TheRewriter.getLangOpts());
  if (calExp->isLValue())
    functionType += " &";
  else if (calExp->isXValue())
    functionType += " &&";
  // Type __tempVar_x;
  SSresult << functionType << " __tempVar_"
           << nodeID
           //= functionName (
           << " = "
           << getExprAsString(calExp->getCallee(), TheRewriter.getLangOpts())
           << "(";
  bool firstArg = true;
  // Prints all of its arguments
  long unsigned int childCounter = 0;
  for (auto b = calExp->arg_begin(), e = calExp->arg_end(); b != e; b++) {
    if (!firstArg) {
      SSresult << "," << createCallArgString(node, *b, childCounter);
    } else {
      SSresult << createCallArgString(node, *b, childCounter);
      firstArg = false;
    }
  }
  SSresult << ");\n";
  return SSresult.str();
}
