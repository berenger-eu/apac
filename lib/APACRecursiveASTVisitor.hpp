#include "common.hpp"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

inline bool isToParseFunction(const std::string &function,
                              const std::vector<std::string> &functionsToParse,
                              const std::vector<std::string> &functionsToIgnore,
                              const std::string &main) {
  // If it's one of our internal functions, we don't parse it
  if (function.find("_apacSeq") != std::string::npos)
    return false;
  // Check if the function is in the ignore list
  for (const auto &f : functionsToIgnore) {
    if (function == f) {
      return false;
    }
  }
  // By default, we parse all functions
  if (functionsToParse.empty())
    return true;
  for (const auto &f : functionsToParse) {
    if (function == f) {
      return true;
    }
  }
  // We always parse the main function unless it is in the ignore list
  if (function == main) {
    return true;
  }
  return false;
}

template <typename Derived>
class APACRecursiveASTVisitor : public RecursiveASTVisitor<Derived> {
public:
  APACRecursiveASTVisitor(Rewriter &R, std::string &mainRef,
                          const std::vector<std::string> &functionsRef,
                          const std::vector<std::string> &functionsToIgnoreRef)
      : TheRewriter(R), mainName(mainRef), functions(functionsRef),
        functionsToIgnore(functionsToIgnoreRef) {}

  inline bool VisitStmt(Stmt *) { return true; }
  bool TraverseNamespaceDecl(NamespaceDecl *nDecl) {
    if (elementsConditions(nDecl)) {
      return RecursiveASTVisitor<Derived>::TraverseNamespaceDecl(nDecl);
    }
    return true;
  }
  bool TraverseCXXMethodDecl(CXXMethodDecl *mDecl) {
    if (functionsConditions(mDecl)) {
      return RecursiveASTVisitor<Derived>::TraverseCXXMethodDecl(mDecl);
    }
    return true;
  }
  bool TraverseFunctionDecl(FunctionDecl *fDecl) {
    if (functionsConditions(fDecl)) {
      return RecursiveASTVisitor<Derived>::TraverseFunctionDecl(fDecl);
    }
    return true;
  }
  bool TraverseDecl(Decl *d) {
    if (elementsConditions(d)) {
      return RecursiveASTVisitor<Derived>::TraverseDecl(d);
    }
    return true;
  }
  bool TraverseCXXOperatorCallExpr(CXXOperatorCallExpr *opCall) {
    if (elementsConditions(opCall)) {
      auto firstArg = opCall->getArg(0);
      if (isa<DeclRefExpr>(firstArg->IgnoreParenCasts())) {
        auto declRef = cast<DeclRefExpr>(firstArg->IgnoreParenCasts());
        auto decl = declRef->getDecl();
        if (decl->getName() == "__result") {
          return true;
        }
      }
      return RecursiveASTVisitor<Derived>::TraverseCXXOperatorCallExpr(opCall);
    }
    return true;
  }
  bool TraverseFunctionTemplateDecl(FunctionTemplateDecl *fDecl) {
    if (elementsConditions(fDecl) && templatesConditions(fDecl)) {
      return RecursiveASTVisitor<Derived>::TraverseFunctionTemplateDecl(fDecl);
    }
    return true;
  }
  bool TraverseIfStmt(IfStmt *ifSt) {
    if (elementsConditions(ifSt)) {
      return RecursiveASTVisitor<Derived>::TraverseIfStmt(ifSt);
    }
    return true;
  }

public:
  inline bool functionsConditions(FunctionDecl *fDecl) {
    return isToParseFunction(fDecl->getNameAsString(), functions,
                             functionsToIgnore, mainName) &&
           elementsConditions(fDecl);
  }
  inline bool elementsConditions(Stmt *s) {
    return !isInHeaders(TheRewriter.getSourceMgr(), s->getBeginLoc());
  }
  inline bool elementsConditions(Decl *d) {
    return !isInHeaders(TheRewriter.getSourceMgr(), d->getBeginLoc());
  }
  inline bool templatesConditions(FunctionTemplateDecl *fDecl) {
    return fDecl->getNameAsString().find("invalid_ref") == std::string::npos;
  }

protected:
  Rewriter &TheRewriter;
  const std::string &mainName;
  const std::vector<std::string> &functions;
  const std::vector<std::string> &functionsToIgnore;
};