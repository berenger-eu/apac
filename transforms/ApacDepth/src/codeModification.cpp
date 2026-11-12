#include "codeModification.hpp"
#include "DepthRuntime.hpp"

void addInitApacPart(Rewriter &TheRewriter, const SourceLocation &beginCodeLoc,
                     FunctionDecl *firstFunction) {
  bool printedHeader = false;
  if (firstFunction != nullptr) {
    std::stringstream generated;
    if (firstFunction->getBeginLoc() == beginCodeLoc) {
      generated << apacDepthIncludes();
      printedHeader = true;
    }
    generated << apacDepthRuntime();
    TheRewriter.InsertTextBefore(firstFunction->getBeginLoc(), generated.str());
  }
  if (!printedHeader)
    TheRewriter.InsertTextBefore(beginCodeLoc, apacDepthIncludes());
}

void addFunctionDepth(Rewriter &TheRewriter,
                      std::vector<FunctionDecl *> &functions) {
  for (auto &f : functions) {
    std::stringstream SSprintBefore, SSprintAfter;
    SSprintBefore
        << "int __apac_depth_local = __apac_depth;\n"
        << "int __apac_depth_ok = (__apac_depth_local < __apac_depth_max);\n"
        << "if(__apac_depth_ok) {\n";
    SSprintAfter << "}\nelse {\n"
                 << "return " << f->getNameAsString() + "_apacSeq(";
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

void incrementFunctionDepth(Rewriter &TheRewriter,
                            std::vector<Stmt *> placesToPlaceIncrement) {
  for (auto *call : placesToPlaceIncrement) {
    // Calls are visited in postorder, so nested replacements are preserved.
    auto text = TheRewriter.getRewrittenText(call->getSourceRange());
    TheRewriter.ReplaceText(
        call->getSourceRange(),
        "([&]() -> decltype(auto) { __apac_depth_guard __apac_call_depth("
        "__apac_depth_local + 1); return " +
            text + "; }())");
  }
}
