#include "AliasTableMaps.hpp"
#include "InstructionsOrderManager.hpp"
#include "ParamWriteAnalyzer.hpp"
#include "clang/Tooling/Tooling.h"
#include <iostream>
#include <map>

int main() {
  auto ast = clang::tooling::buildASTFromCodeWithArgs(R"cpp(
    void write(int *p) { *p = 1; }
    int read(int *p) { return *p; }
    void alias(int *p) { int *q = p; *q = 1; }
    void aliasAssign(int *p) { int *q; q = p; *q = 1; }
    void arithmetic(int *p) { write(p + 1); }
    void address(int *p) { write(&p[1]); }
    void indirect(int *p, void (*fn)(int *)) { fn(p); }
    void variadicSink(int, ...);
    void variadic(int *p) { variadicSink(0, p); }
    int *escape(int *p) { return p; }
    struct S { int x; void set() { x = 1; } };
    void member(S *p) { p->set(); }
    void lateWrite(int *);
    void forwardWrite(int *p) { lateWrite(p); }
    void lateWrite(int *p) { *p = 1; }
    int lateRead(int *);
    int forwardRead(int *p) { return lateRead(p); }
    int lateRead(int *p) { return *p; }
    int recRead(int *p, int n) { return n ? recRead(p, n-1) : *p; }
    void mutualB(int *, int);
    void mutualA(int *p, int n) { if (n) mutualB(p, n-1); }
    void mutualB(int *p, int n) { if (n) mutualA(p, n-1); else *p = 2; }
    void deleted(int *p) { delete p; }
    struct Assignable { Assignable &operator=(int) { return *this; } };
    void objectOperator(Assignable *p) { *p = 1; }
    void grouping() { int a; int b; int c; }
  )cpp",
                                                      {"-std=c++17"});
  if (!ast || ast->getDiagnostics().hasErrorOccurred())
    return 1;
  std::map<std::string, bool> expected = {
      {"write", false},         {"read", true},          {"alias", false},
      {"aliasAssign", false},   {"arithmetic", false},   {"address", false},
      {"indirect", false},      {"variadic", false},     {"escape", false},
      {"member", false},        {"forwardWrite", false}, {"lateWrite", false},
      {"forwardRead", true},    {"lateRead", true},      {"recRead", true},
      {"mutualA", false},       {"mutualB", false},      {"deleted", false},
      {"objectOperator", false}};
  ParamWriteAnalyzer analyzer;
  int failures = 0;
  for (int run = 0; run < 2; ++run) {
    analyzer.analyzeTranslationUnit(ast->getASTContext());
    unsigned checked = 0;
    for (const auto *decl :
         ast->getASTContext().getTranslationUnitDecl()->decls()) {
      const auto *f = dyn_cast<FunctionDecl>(decl);
      if (!f || !f->isThisDeclarationADefinition())
        continue;
      auto it = expected.find(f->getNameAsString());
      if (it == expected.end())
        continue;
      ++checked;
      if (analyzer.isParamReadOnly(f, 0) != it->second) {
        std::cerr << "Wrong read-only classification: " << it->first << '\n';
        ++failures;
      }
      if (analyzer.isParamReadOnly(f, f->getNumParams()))
        ++failures;
    }
    if (checked != expected.size())
      ++failures;
  }
  for (auto *decl : ast->getASTContext().getTranslationUnitDecl()->decls()) {
    auto *f = dyn_cast<FunctionDecl>(decl);
    if (!f || f->getNameAsString() != "grouping")
      continue;
    auto *body = cast<CompoundStmt>(f->getBody());
    auto it = body->body_begin();
    const auto *a = *it++, *b = *it++, *c = *it++;
    StmtOrder order;
    for (auto *stmt : {a, b, c})
      order.addInstructionToManager(stmt);
    order.moveInstruction(a, b);
    order.moveInstruction(a, b); // Same group must not erase its contents.
    order.moveInstruction(b, c);
    if (order.instructionGroups.size() != 1 ||
        order.instructionGroups.begin()->second.size() != 3)
      ++failures;
    auto &group = order.instructionGroups.begin()->second;
    group.insert(*group.begin()); // Comparator must be irreflexive.
    if (group.size() != 3)
      ++failures;
    auto *var = cast<VarDecl>(cast<DeclStmt>(a)->getSingleDecl());
    AliasTableMapStruct table;
    if (table.at(var) != nullptr)
      ++failures;
    IndexTableMapStruct indexes;
    std::vector<int> deep{0, 1}, shallow{0};
    indexes.insert({aliasArg(*var, Variable, deep), deep});
    indexes.insert({aliasArg(*var, Variable, shallow), shallow});
    auto entry = indexes.at(shallow);
    if (!entry || !getVariantSubArray(*entry)->aliasStruct ||
        !getVariantSubArray(*entry)->aliasStruct->alias)
      ++failures;
  }
  if (failures)
    std::cerr << failures << " regression checks failed\n";
  else
    std::cout << "Parameter analysis, instruction groups and alias maps: OK\n";
  return failures ? 1 : 0;
}
