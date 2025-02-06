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
    rootNode->children.push_back(std::make_shared<Node>(call, tempVarsCounter));
    buildSubTree(rootNode->children.back(), tempVarsCounter);
  }
}

void UnstackTransformer::modifyCalls() {
  for (auto locRootsPair : callRoots) {
    std::stringstream SSresultLoc;
    for (auto root : locRootsPair.second) {
      SSresultLoc << createTempVarStringRoot(root);
    }
    TheRewriter.InsertText(locRootsPair.first, SSresultLoc.str());
  }
}
std::string
UnstackTransformer::createTempVarStringRoot(std::shared_ptr<Node> rootNode) {
  std::stringstream SSresult;
  for (auto child : rootNode->children) {
    SSresult << createTempVarStringRoot(child);
  }
  SSresult << createTempVarString(rootNode);
  return SSresult.str();
}

std::string UnstackTransformer::createCallArgString(std::shared_ptr<Node> node,
                                                    Expr *argExpr,
                                                    int &childCounter) {
  std::stringstream res;
  if (argExpr == nullptr) {
    return "";
  }
  argExpr = argExpr->IgnoreImpCasts();
  // If it's a call, we can replace it by the corresponding variable
  if (isa<CallExpr>(argExpr)) {
    if (childCounter >= node->children.size()) {
      return getExprAsString(argExpr, TheRewriter.getLangOpts());
    }
    res << " __tempVar_" << node->children.at(childCounter)->id << " ";
    childCounter++;
  }
  // If it's a BinaryOperator, then we have to look at both expressions
  else if (isa<BinaryOperator>(argExpr)) {
    BinaryOperator *bop = cast<BinaryOperator>(argExpr);
    res << createCallArgString(node, bop->getLHS(), childCounter)
        << bop->getOpcodeStr().str()
        << createCallArgString(node, bop->getRHS(), childCounter);
  } else if (isa<UnaryOperator>(argExpr)) {
    UnaryOperator *uop = cast<UnaryOperator>(argExpr);
    std::string opcodeStr;
    if (uop->isDecrementOp()) {
      opcodeStr = "--";
    } else {
      opcodeStr = "++";
    }
    if (uop->isPostfix()) {
      res << createCallArgString(node, uop->getSubExpr(), childCounter)
          << opcodeStr;
    } else {
      res << opcodeStr
          << createCallArgString(node, uop->getSubExpr(), childCounter);
    }
  }
  // Either the Expr does not contain a call, or we haven't found it
  // So we don't change anything either way
  else {
    res << getExprAsString(argExpr, TheRewriter.getLangOpts());
  }
  return res.str();
}

// Create string : type __tempVar_x; __tempVar_x = unstackedCall;
std::string
UnstackTransformer::createTempVarString(std::shared_ptr<Node> node) {
  std::stringstream SSresult;
  // Get the return type of the function
  auto &calExp = node->call;
  auto &nodeID = node->id;
  ASTContext &aCons = calExp->getDirectCallee()->getASTContext();
  std::string functionType =
      calExp->getCallReturnType(aCons).getAsString(TheRewriter.getLangOpts());
  // Type __tempVar_x;
  SSresult << functionType << " __tempVar_"
           << nodeID
           //= functionName (
           << " = " << calExp->getDirectCallee()->getNameAsString() << "(";
  bool firstArg = true;
  // Prints all of its arguments
  int childCounter = 0;
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
