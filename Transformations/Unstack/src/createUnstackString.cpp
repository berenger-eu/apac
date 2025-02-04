#include "createUnstackString.hpp"

// Transform a CallExpr into a varTemp and adds the unstacked call before
// instructionBegin
void UnstackTransformer::unstackTransformCallExpr(
    CallExpr *calExp, const SourceLocation &instructionBegin,
    int &tempVarsCounter) {
  std::vector<CallExpr *> vectCallExpr;
  findAllCallExpr(calExp, vectCallExpr);
  std::stringstream SScall;
  std::queue<int> tempVarQueue;
  for (auto b = vectCallExpr.begin(), e = vectCallExpr.end(); b != e; b++) {
    tempVarQueue.push(tempVarsCounter);
    SScall << createTempVarString(*b, tempVarsCounter, tempVarQueue);
    tempVarsCounter++;
  }
  std::stringstream SSvar;
  SSvar << "__tempVar_" << tempVarsCounter - 1;
  TheRewriter.ReplaceText(
      SourceRange(calExp->getBeginLoc(), calExp->getEndLoc()), SSvar.str());
  TheRewriter.InsertTextBefore(instructionBegin, SScall.str());
}

std::string
UnstackTransformer::createCallArgString(Expr *argExpr,
                                        std::queue<int> &tempVarQueue) {
  std::stringstream res;
  if (argExpr == NULL)
    ;
  argExpr = argExpr->IgnoreImpCasts();
  // If it's a call, we can replace it by the corresponding variable
  if (isa<CallExpr>(argExpr)) {
    res << " __tempVar_" << tempVarQueue.front() << " ";
    tempVarQueue.pop();
  }
  // If it's a BinaryOperator, then we have to look at both expressions
  else if (isa<BinaryOperator>(argExpr)) {
    BinaryOperator *bop = cast<BinaryOperator>(argExpr);
    res << createCallArgString(bop->getLHS(), tempVarQueue)
        << bop->getOpcodeStr().str()
        << createCallArgString(bop->getRHS(), tempVarQueue);
  } else if (isa<UnaryOperator>(argExpr)) {
    UnaryOperator *uop = cast<UnaryOperator>(argExpr);
    std::string opcodeStr;
    if (uop->isDecrementOp()) {
      opcodeStr = "--";
    } else {
      opcodeStr = "++";
    }
    if (uop->isPostfix()) {
      res << createCallArgString(uop->getSubExpr(), tempVarQueue) << opcodeStr;
    } else {
      res << opcodeStr << createCallArgString(uop->getSubExpr(), tempVarQueue);
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
std::string UnstackTransformer::createTempVarString(
    CallExpr *calExp, int currentCounterNumber, std::queue<int> &tempVarQueue) {
  std::stringstream SSresult;
  // Get the return type of the function
  ASTContext &aCons = calExp->getDirectCallee()->getASTContext();
  std::string functionType =
      calExp->getCallReturnType(aCons).getAsString(TheRewriter.getLangOpts());
  // Type __tempVar_x;
  SSresult << functionType << " __tempVar_"
           << currentCounterNumber
           //= functionName (
           << " = " << calExp->getDirectCallee()->getNameAsString() << "(";
  bool firstArg = true;
  // Prints all of its arguments
  for (auto b = calExp->arg_begin(), e = calExp->arg_end(); b != e; b++) {
    if (!firstArg) {
      SSresult << "," << createCallArgString(*b, tempVarQueue);
    } else {
      SSresult << createCallArgString(*b, tempVarQueue);
      firstArg = false;
    }
  }
  SSresult << ");\n";
  return SSresult.str();
}
