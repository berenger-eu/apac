#include "codeModification.hpp"
#include "DepthRuntime.hpp"
#include "StaticCost.hpp"

void addInitApacPart(Rewriter &TheRewriter, const SourceLocation &beginCodeLoc,
                     FunctionDecl *firstFunction, const StaticCostModel *costs) {
  std::string includeText = apacDepthIncludes();
  bool printedHeader = false;
  if (firstFunction != nullptr) {
    std::stringstream SSprint;
    if (firstFunction->getBeginLoc() == beginCodeLoc) {
      printedHeader = true;
      SSprint << includeText;
    }
    SSprint << apacDepthRuntime();
    if (costs) SSprint << costs->definitions();
    TheRewriter.InsertTextBefore(firstFunction->getBeginLoc(), SSprint.str());
  }
  if (!printedHeader) {
    TheRewriter.InsertTextBefore(beginCodeLoc, includeText);
  }
}

void handleFunctions(
    Rewriter &TheRewriter, std::vector<FunctionDecl *> &functions,
    std::vector<std::pair<ReturnStmt *, FunctionDecl *>> &returnStmts,
    const std::map<FunctionDecl *, std::string> &phaseTokens, const StaticCostModel *costs) {
  std::unordered_map<FunctionDecl *, ReturnStmt *> returnMap;
  for (auto &r : returnStmts) {
    returnMap[r.second] = r.first;
  }
  for (auto &f : functions) {

    std::stringstream SSprintBefore, SSprintAfter;
    auto phase = phaseTokens.find(f);
    if (phase != phaseTokens.end())
      SSprintBefore << "char " << phase->second << ";\n";
    addFunctionDepth(TheRewriter, SSprintBefore, SSprintAfter, f, costs);
    handleFunctionTaskGroup(TheRewriter, f,
                            returnMap.count(f) ? returnMap.at(f) : nullptr,
                            SSprintBefore, SSprintAfter);
    TheRewriter.InsertTextAfterToken(f->getBody()->getBeginLoc(),
                                     SSprintBefore.str());
    TheRewriter.InsertTextAfter(f->getBody()->getEndLoc(), SSprintAfter.str());
  }
}

void addFunctionDepth(Rewriter &TheRewriter, std::stringstream &SSprintBefore,
                      std::stringstream &SSprintAfter, FunctionDecl *f, const StaticCostModel *costs) {
  // Stringstream before :  ... if(__apac_depth_ok) {
  // Stringstream after :  } else { return f_apacSeq(...); }
  SSprintBefore << "int __apac_depth_local = __apac_depth;\n";

  SSprintBefore << "int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);\n";
  if (costs)
    SSprintBefore << "if(__apac_depth_ok && (__apac_cost_threshold == 0 || "
                  << costs->entryCost(f) << " >= __apac_cost_threshold)) {\n";
  else SSprintBefore << "if(__apac_depth_ok) {\n";
  SSprintAfter << "}\nelse {\n";
  if (costs) SSprintAfter << "__apac_depth_guard __apac_sequential_depth(__apac_depth_max);\n";
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
    // Standalone TaskGraph inputs may not have GotoRet's outer __result
    // declaration. A taskgroup scope would hide locals used by the return.
    // In that case keep the function scope and wait for its child tasks.
    if (returnStmt && returnStmt->getRetValue()) {
      bool needsFunctionScope = false;
      auto children = f->getBody()->children();
      const Stmt *first = children.empty() ? nullptr : *children.begin();
      for (const auto *ref : getAllDeclRefExprInsideExpr(returnStmt->getRetValue())) {
        const auto *var = dyn_cast<VarDecl>(ref->getDecl());
        if (!var || !var->isLocalVarDecl() || var->getDeclContext() != f)
          continue;
        bool outerResult = false;
        if (const auto *decl = dyn_cast_or_null<DeclStmt>(first))
          outerResult = decl->isSingleDecl() && decl->getSingleDecl() == var &&
                        var->getNameAsString().find("__result") != std::string::npos;
        needsFunctionScope |= !outerResult;
      }
      if (needsFunctionScope) {
        TheRewriter.InsertText(returnStmt->getBeginLoc(), "\n#pragma omp taskwait\n");
        SSprintAfter << curAfter;
        return;
      }
    }
    if (returnStmt == nullptr)
      SSprintAfter << "}\n";
    else
      TheRewriter.InsertText(returnStmt->getBeginLoc(), "}\n");
    bool placedTaskGroup = false;
    auto children = f->getBody()->children();
    Stmt *firstStmt = children.empty() ? nullptr : *children.begin();
    if (firstStmt && isa<DeclStmt>(firstStmt)) {
      DeclStmt *d = cast<DeclStmt>(firstStmt);
      if (d->isSingleDecl() && isa<VarDecl>(d->getSingleDecl())) {
        VarDecl *var = cast<VarDecl>(d->getSingleDecl());
        if (var->getNameAsString().find("__result") != std::string::npos) {
          TheRewriter.InsertTextAfterToken(firstStmt->getEndLoc(),
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
