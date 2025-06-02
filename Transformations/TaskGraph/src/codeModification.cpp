#include "codeModification.hpp"

void addInitApacPart(Rewriter &TheRewriter, const SourceLocation &beginCodeLoc,
                     FunctionDecl *firstFunction) {
  std::string includeText = "#include <omp.h>\n#include <cstring>\n";
  bool printedHeader = false;
  if (firstFunction != nullptr) {
    std::stringstream SSprint;
    if (firstFunction->getBeginLoc() == beginCodeLoc) {
      printedHeader = true;
      SSprint << includeText;
    }
    SSprint << "const int nb_cores = omp_get_max_threads();\n"
            << "const int parallel_depth = ffs(nb_cores);// log2(nb_cores);\n"
            << "int __apac_depth = 0;\n"
            << "#pragma omp threadprivate(__apac_depth)\n"
            << "const static int __apac_depth_max = parallel_depth;\n\n";
    TheRewriter.InsertTextBefore(firstFunction->getBeginLoc(), SSprint.str());
  }
  if (!printedHeader) {
    TheRewriter.InsertTextBefore(beginCodeLoc, includeText);
  }
}

void addFunctionDepth(Rewriter &TheRewriter,
                      std::vector<FunctionDecl *> &functions) {
  for (auto &f : functions) {
    std::stringstream SSprintBefore, SSprintAfter;
    SSprintBefore << "int __apac_depth_local = __apac_depth;\n";

    SSprintBefore
        << "int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);\n"
        << "if(__apac_depth_ok) {\n";
    SSprintAfter << "}\nelse {\n";
    if (!f->getReturnType()->isVoidType())
      SSprintAfter << "return ";
    SSprintAfter << f->getNameAsString() + "_apacSeq(";
    for (auto &param : f->parameters()) {
      SSprintAfter << param->getNameAsString();
      if (param != f->parameters().back()) {
        SSprintAfter << ", ";
      }
    }
    SSprintAfter << ");\n}\n";
    TheRewriter.InsertTextAfterToken(f->getBody()->getBeginLoc(),
                                     SSprintBefore.str());
    TheRewriter.InsertTextBefore(f->getBody()->getEndLoc(), SSprintAfter.str());
  }
}
void handleTaskGroups(
    Rewriter &TheRewriter, std::vector<FunctionDecl *> &functions,
    std::vector<std::pair<ReturnStmt *, FunctionDecl *>> &returnStmts) {

  std::unordered_map<FunctionDecl *, ReturnStmt *> returnMap;
  for (auto &r : returnStmts) {
    returnMap[r.second] = r.first;
  }
  for (auto &f : functions) {
    if (returnMap.count(f))
      TheRewriter.InsertTextBefore(returnMap.at(f)->getBeginLoc(), "}\n");
    else
      TheRewriter.InsertTextBefore(f->getBody()->getEndLoc(), "}\n");
  }
  for (auto &f : functions) {
    if (f->hasBody()) {
      bool placedTaskGroup = false;
      Stmt *firstStmt = *(f->getBody()->child_begin());
      if (isa<DeclStmt>(firstStmt)) {
        DeclStmt *d = cast<DeclStmt>(firstStmt);
        if (d->isSingleDecl() && isa<VarDecl>(d->getSingleDecl())) {
          VarDecl *var = cast<VarDecl>(d->getSingleDecl());
          if (var->getNameAsString().find("__result") != std::string::npos) {
            TheRewriter.InsertTextBefore(firstStmt->getEndLoc(),
                                         "#pragma omp taskgroup\n{\n");
            placedTaskGroup = true;
          }
        }
      } else if (!placedTaskGroup)
        TheRewriter.InsertTextBefore(firstStmt->getBeginLoc(),
                                     "#pragma omp taskgroup\n{\n");
    }
  }
}
