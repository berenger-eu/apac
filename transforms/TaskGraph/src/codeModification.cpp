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

void handleFunctions(
    Rewriter &TheRewriter, std::vector<FunctionDecl *> &functions,
    std::vector<std::pair<ReturnStmt *, FunctionDecl *>> &returnStmts) {
  std::unordered_map<FunctionDecl *, ReturnStmt *> returnMap;
  for (auto &r : returnStmts) {
    returnMap[r.second] = r.first;
  }
  for (auto &f : functions) {

    std::stringstream SSprintBefore, SSprintAfter;
    addFunctionDepth(TheRewriter, SSprintBefore, SSprintAfter, f);
    handleFunctionTaskGroup(TheRewriter, f,
                            returnMap.count(f) ? returnMap.at(f) : nullptr,
                            SSprintBefore, SSprintAfter);
    TheRewriter.InsertTextAfterToken(f->getBody()->getBeginLoc(),
                                     SSprintBefore.str());
    TheRewriter.InsertTextAfter(f->getBody()->getEndLoc(), SSprintAfter.str());
  }
}

void addFunctionDepth(Rewriter &TheRewriter, std::stringstream &SSprintBefore,
                      std::stringstream &SSprintAfter, FunctionDecl *f) {
  // Stringstream before :  ... if(__apac_depth_ok) {
  // Stringstream after :  } else { return f_apacSeq(...); }
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
}
void handleFunctionTaskGroup(Rewriter &TheRewriter, FunctionDecl *f,
                             ReturnStmt *returnStmt,
                             std::stringstream &SSprintBefore,
                             std::stringstream &SSprintAfter) {
  auto curAfter = SSprintAfter.str();
  SSprintAfter.str("");
  SSprintAfter.clear();

  if (f->hasBody()) {
    if (returnStmt == nullptr)
      SSprintAfter << "}\n";
    else
      TheRewriter.InsertText(returnStmt->getBeginLoc(), "}\n");
    bool placedTaskGroup = false;
    Stmt *firstStmt = *(f->getBody()->child_begin());
    if (isa<DeclStmt>(firstStmt)) {
      DeclStmt *d = cast<DeclStmt>(firstStmt);
      if (d->isSingleDecl() && isa<VarDecl>(d->getSingleDecl())) {
        VarDecl *var = cast<VarDecl>(d->getSingleDecl());
        if (var->getNameAsString().find("__result") != std::string::npos) {
          TheRewriter.InsertTextBefore(firstStmt->getEndLoc(),
                                       "\n#pragma omp taskgroup\n{\n");
          placedTaskGroup = true;
        }
      }
    }
    if (!placedTaskGroup)
      SSprintBefore << "\n#pragma omp taskgroup\n{\n";
  }
  SSprintAfter << curAfter;
}
