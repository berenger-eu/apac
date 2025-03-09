#include "stringManipulation.hpp"
#include "utilitaryFunctions.hpp"
using namespace clang;
class ASTHeapifyVisitor : public RecursiveASTVisitor<ASTHeapifyVisitor> {
public:
  ASTHeapifyVisitor(Rewriter &R, struct item_found &funHeap,
                    struct item_found &varHeap)
      : TheRewriter(R), functionHeap(funHeap), variableHeap(varHeap) {};
  inline bool VisitStmt(Stmt *) { return true; }
  bool TraverseFunctionDecl(FunctionDecl *);
  inline bool TraverseFunctionTemplateDecl(FunctionTemplateDecl *fDecl) {
    if (fDecl->getNameAsString().find("invalid_ref") == std::string::npos) {
      return RecursiveASTVisitor::TraverseFunctionTemplateDecl(fDecl);
    }
    return true;
  }
  bool subVisitCompoundStmt(CompoundStmt *coSt);

private:
  // Like Visit functions, but called by VisitCompoundStmt and not by default
  // when encountering specific nodes
  std::string subVisitVarDecl(VarDecl &, std::vector<struct item_found> &);
  void subVisitIfStmt(IfStmt *);
  void subVisitForStmt(ForStmt *);
  void subVisitWhileStmt(WhileStmt *);
  bool deleteSegmentAtStmt(Stmt &st);
  void handleSubStmt(Stmt *);
  std::string stringDeclStmt(DeclStmt *, std::vector<struct item_found> &);
  void handleDeclStmt(DeclStmt *, std::vector<struct item_found> &);
  void initItem(struct item_found &, VarDecl &);
  void deleteSectionAfterCreatedScope(const SourceLocation &,
                                      const std::vector<struct item_found> &);

  Rewriter &TheRewriter;
  struct item_found functionHeap;
  struct item_found variableHeap;
  std::unordered_map<std::string, int> varCounter;
  std::vector<struct item_found>
      currentVarsEncountered; // TODO implement in cleaner manner
};