#include "codeModification.hpp"

void addInitApacPart(Rewriter &TheRewriter, const SourceLocation &beginCodeLoc,
                     FunctionDecl *firstFunction) {
  if (firstFunction != nullptr) {
    std::stringstream SSprint;

    SSprint << "const int nb_cores = omp_get_max_threads();\n"
            << "const int parallel_depth = ffs(nb_cores);// log2(nb_cores);\n"
            << "int __apac_depth = 0;\n"
            << "#pragma omp threadprivate(__apac_depth)\n"
            << "const static int __apac_depth_max = parallel_depth;\n\n";
    TheRewriter.InsertTextBefore(firstFunction->getBeginLoc(), SSprint.str());
  }
  TheRewriter.InsertTextBefore(beginCodeLoc,
                               "#include <omp.h>\n#include <cstring>\n");
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
  for (auto &st : placesToPlaceIncrement) {
    std::stringstream SSprint;
    SSprint << "__apac_depth = __apac_depth_local + 1;\n";
    TheRewriter.InsertTextBefore(st->getBeginLoc(), SSprint.str());
  }
}