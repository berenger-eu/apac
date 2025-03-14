#include "ASTConditionUnstackVisitor.hpp"

bool ASTConditionUnstackVisitor::VisitWhileStmt(WhileStmt *whileSt) {
  if (whileSt->hasVarStorage()) {
    std::stringstream SSprint;
    SSprint << "{\n";
    DeclStmt *whileStCond = whileSt->getConditionVariableDeclStmt();
    for (auto decl : whileStCond->decls()) {
      if (isa<VarDecl>(decl)) {
        auto varDecl = cast<VarDecl>(decl);
        SSprint << getCompleteVarDeclStr(varDecl);
        TheRewriter.ReplaceText(
            SourceRange(whileStCond->getBeginLoc(), whileStCond->getEndLoc()),
            (cast<VarDecl>(whileStCond->getSingleDecl()))->getNameAsString());
      }
    }
    // We remove variables seen in the scope at the end of it
    TheRewriter.InsertTextAfter(whileSt->getBeginLoc(), SSprint.str());
    TheRewriter.InsertTextAfterToken(whileSt->getEndLoc(), "}\n");
  }
  return true;
}
bool ASTConditionUnstackVisitor::VisitIfStmt(IfStmt *ifSt) {
  std::stringstream SSprint;
  if (ifSt->hasVarStorage() || ifSt->hasInitStorage()) {

    std::stringstream SSprint;
    SSprint << "{\n";
    if (ifSt->getInit() != NULL && isa<DeclStmt>(ifSt->getInit())) {
      DeclStmt *ifStInit = cast<DeclStmt>(ifSt->getInit());
      for (auto decl : ifStInit->decls()) {
        if (isa<VarDecl>(decl)) {
          auto varDecl = cast<VarDecl>(decl);
          SSprint << getCompleteVarDeclStr(varDecl);
        }
      }
      TheRewriter.RemoveText(
          SourceRange(ifStInit->getBeginLoc(), ifStInit->getEndLoc()));
    }
    if (ifSt->getConditionVariableDeclStmt() != NULL) {
      DeclStmt *ifStCond = ifSt->getConditionVariableDeclStmt();
      for (auto decl : ifStCond->decls()) {
        if (isa<VarDecl>(decl)) {
          auto varDecl = cast<VarDecl>(decl);
          SSprint << getCompleteVarDeclStr(varDecl);
          TheRewriter.ReplaceText(
              SourceRange(ifStCond->getBeginLoc(), ifStCond->getEndLoc()),
              (cast<VarDecl>(ifStCond->getSingleDecl()))->getNameAsString());
        }
      }
    }
    // Adds the built text section before the If statement
    TheRewriter.InsertTextAfter(ifSt->getBeginLoc(), SSprint.str());
    // We remove variables seen in the scope at the end of it
    // First declStmt contains variables,
    // add them to curVarEncoutered and curVarsInScope
    // UNHANDLED, either do it in the if BUT c++17 will be required
    // Or outside of if, but then the same has to be done for else if
    TheRewriter.InsertTextAfterToken(ifSt->getEndLoc(), "}\n");
  }
  return true;
}

bool ASTConditionUnstackVisitor::VisitForStmt(ForStmt *forSt) {
  std::stringstream SSprint;
  if (forSt->getInit() && isa<DeclStmt>(forSt->getInit())) {
    std::stringstream SSprint;
    SSprint << "{\n";
    DeclStmt *forStCond = cast<DeclStmt>(forSt->getInit());
    for (auto decl : forStCond->decls()) {
      if (isa<VarDecl>(decl)) {
        auto varDecl = cast<VarDecl>(decl);
        SSprint << getCompleteVarDeclStr(varDecl);
        TheRewriter.ReplaceText(
            SourceRange(forStCond->getBeginLoc(), forStCond->getEndLoc()),
            (cast<VarDecl>(forStCond->getSingleDecl()))->getNameAsString());
      }
    }
    TheRewriter.ReplaceText(
        SourceRange(forStCond->getBeginLoc(), forStCond->getEndLoc()), ";");
    // We remove variables seen in the scope at the end of it
    TheRewriter.InsertTextAfter(forSt->getBeginLoc(), SSprint.str());
    TheRewriter.InsertTextAfterToken(forSt->getEndLoc(), "}\n");
  }
  return true;
}