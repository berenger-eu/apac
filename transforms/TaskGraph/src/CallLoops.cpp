#include "ArrayPhases.hpp"
#include "StaticCost.hpp"
#include "clang/AST/DeclCXX.h"
#include <functional>

namespace {
bool valueVariable(const VarDecl *v) {
  return v && v->isLocalVarDeclOrParm() &&
         (v->getType()->isArithmeticType() || v->getType()->isEnumeralType()) &&
         !v->getType().isVolatileQualified() &&
         v->getTLSKind() == VarDecl::TLS_None && !v->hasAttrs();
}
bool scalarVariable(const VarDecl *v) {
  return valueVariable(v) && v->getType()->isIntegerType();
}
// Loop control and scalar arguments must be evaluated without reading task
// outputs or invoking code. Buffer contents are accessed only by the call.
bool pureScalar(const Expr *e, std::set<std::string> *captures = nullptr) {
  if (!e)
    return false;
  e = e->IgnoreParenImpCasts();
  if (isa<IntegerLiteral>(e) || isa<FloatingLiteral>(e) ||
      isa<CXXBoolLiteralExpr>(e))
    return true;
  if (const auto *ref = dyn_cast<DeclRefExpr>(e)) {
    const auto *v = dyn_cast<VarDecl>(ref->getDecl());
    if (!valueVariable(v))
      return false;
    if (captures)
      captures->insert(v->getNameAsString());
    return true;
  }
  if (const auto *binary = dyn_cast<BinaryOperator>(e))
    return !binary->isAssignmentOp() && binary->getOpcode() != BO_Comma &&
           pureScalar(binary->getLHS(), captures) &&
           pureScalar(binary->getRHS(), captures);
  if (const auto *unary = dyn_cast<UnaryOperator>(e))
    return (unary->getOpcode() == UO_Plus || unary->getOpcode() == UO_Minus ||
            unary->getOpcode() == UO_Not || unary->getOpcode() == UO_LNot) &&
           pureScalar(unary->getSubExpr(), captures);
  if (const auto *conditional = dyn_cast<ConditionalOperator>(e))
    return pureScalar(conditional->getCond(), captures) &&
           pureScalar(conditional->getTrueExpr(), captures) &&
           pureScalar(conditional->getFalseExpr(), captures);
  return false;
}
const VarDecl *variable(const Expr *e) {
  const auto *ref =
      e ? dyn_cast<DeclRefExpr>(e->IgnoreParenImpCasts()) : nullptr;
  return ref ? dyn_cast<VarDecl>(ref->getDecl()) : nullptr;
}
std::string joined(const std::set<std::string> &items) {
  std::string result;
  for (const auto &item : items) {
    if (!result.empty())
      result += ",";
    result += item;
  }
  return result;
}
} // namespace

bool lowerPartitionCallLoop(ForStmt *loop, AliasTable &aliases,
                            const ParamWriteAnalyzer *parameters,
                            std::string &replacement, const StaticCostModel *costs) {
  std::set<const VarDecl *> controls;
  unsigned calls = 0;
  // The pipeline can hoist induction-variable declarations outside the loop.
  // Only declarations and assignments to control locals are allowed in its
  // body; arbitrary sequential reads/writes would need another synchronization.
  std::function<void(const Stmt *)> collect = [&](const Stmt *s) {
    if (!s)
      return;
    if (const auto *decl = dyn_cast<DeclStmt>(s))
      for (const auto *item : decl->decls())
        if (const auto *v = dyn_cast<VarDecl>(item))
          controls.insert(v);
    for (const auto *child : s->children())
      collect(child);
  };
  collect(loop);
  std::function<bool(const Stmt *, std::string &)> render;
  render = [&](const Stmt *s, std::string &out) -> bool {
    if (!s)
      return false;
    if (const auto *block = dyn_cast<CompoundStmt>(s)) {
      out += "{\n";
      for (const auto *child : block->body())
        if (!render(child, out))
          return false;
      out += "}\n";
      return true;
    }
    if (const auto *decl = dyn_cast<DeclStmt>(s)) {
      for (const auto *item : decl->decls()) {
        const auto *v = dyn_cast<VarDecl>(item);
        if (!scalarVariable(v) || (v->hasInit() && !pureScalar(v->getInit())))
          return false;
      }
      const auto *first = cast<VarDecl>(*decl->decl_begin());
      out += getStmtAsStringFull(s, first->getASTContext().getLangOpts());
      return true;
    }
    if (const auto *nested = dyn_cast<ForStmt>(s)) {
      const Expr *inc = nested->getInc();
      const VarDecl *index = nullptr;
      if (const auto *unary = dyn_cast_or_null<UnaryOperator>(inc)) {
        if (unary->isIncrementDecrementOp())
          index = variable(unary->getSubExpr());
      } else if (const auto *binary =
                     dyn_cast_or_null<CompoundAssignOperator>(inc)) {
        if ((binary->getOpcode() == BO_AddAssign ||
             binary->getOpcode() == BO_SubAssign) &&
            pureScalar(binary->getRHS()))
          index = variable(binary->getLHS());
      }
      if (!scalarVariable(index) || !pureScalar(nested->getCond()) ||
          nested->getConditionVariable())
        return false;
      controls.insert(index);
      std::string init;
      if (nested->getInit() && !render(nested->getInit(), init))
        return false;
      const auto &lang = index->getASTContext().getLangOpts();
      out += "for(" + (init.empty() ? ";" : init) +
             getExprAsString(nested->getCond(), lang, true) + ";" +
             getExprAsString(inc, lang, true) + ")\n{\n";
      if (!render(nested->getBody(), out))
        return false;
      out += "}\n";
      return true;
    }
    if (const auto *assign = dyn_cast<BinaryOperator>(s)) {
      const auto *target = variable(assign->getLHS());
      if (assign->getOpcode() != BO_Assign || !controls.count(target) ||
          !scalarVariable(target) || !pureScalar(assign->getRHS()))
        return false;
      out += getStmtAsStringFull(s, target->getASTContext().getLangOpts());
      return true;
    }
    const auto *call = dyn_cast<CallExpr>(s);
    const auto *callee = call ? call->getDirectCallee() : nullptr;
    if (!callee || isa<CXXMethodDecl>(callee) ||
        !callee->getReturnType()->isVoidType() || callee->isVariadic() ||
        call->getNumArgs() != callee->getNumParams())
      return false;
    std::set<std::string> captures;
    for (unsigned i = 0; i < call->getNumArgs(); ++i) {
      QualType type = callee->getParamDecl(i)->getType();
      if (type->isArithmeticType() || type->isEnumeralType()) {
        if (!pureScalar(call->getArg(i), &captures))
          return false;
        continue;
      }
      // Only explicit partition addresses are supported in this first subset.
      // Whole/element modes, references and pointer rebindings keep the safe
      // synchronous fallback until loop-carried phase transitions are handled.
      if (!type->isPointerType() || !type->getPointeeType()->isArithmeticType())
        return false;
      const auto *address =
          dyn_cast<UnaryOperator>(call->getArg(i)->IgnoreParenImpCasts());
      if (!address || address->getOpcode() != UO_AddrOf)
        return false;
      const auto *array = dyn_cast<ArraySubscriptExpr>(
          address->getSubExpr()->IgnoreParenImpCasts());
      if (!array || !pureScalar(array->getIdx(), &captures))
        return false;
      const auto *base = variable(array->getBase());
      if (!base || !base->isLocalVarDeclOrParm() ||
          base->getType()->isReferenceType() ||
          base->getType().isVolatileQualified() ||
          base->getTLSKind() != VarDecl::TLS_None ||
          !(base->getType()->isPointerType() || base->getType()->isArrayType()))
        return false;
      auto element =
          base->getType()->isPointerType()
              ? base->getType()->getPointeeType()
              : cast<ArrayType>(base->getType().getTypePtr())->getElementType();
      if (element.isVolatileQualified())
        return false;
      // Arrays stay shared; copying them would lose writes to the original.
      if (base->getType()->isPointerType())
        captures.insert(base->getNameAsString());
      if (auto pointer = aliases.getAliasArg(base))
        for (const auto &target : aliases.getAliased(pointer))
          if (target != pointer &&
              target->declaration.getType()->isArithmeticType())
            return false; // A buffer may alias scalar loop control.
    }
    Instruction instruction(const_cast<CallExpr *>(call));
    analyzeArrayPhases(instruction, aliases, parameters, "");
    if (instruction.arrayPhaseFallback || instruction.arrayAccesses.empty())
      return false;
    std::set<std::string> reads, writes;
    for (const auto &access : instruction.arrayAccesses) {
      if (access.mode != ArrayAccessMode::Partition)
        return false;
      (access.write ? writes : reads).insert(access.representative);
    }
    for (const auto &write : writes)
      reads.erase(write);
    captures.insert("__apac_depth_local");
    if (costs)
      out += "\nif(__apac_cost_threshold == 0 || " + costs->callCost(call) +
             " >= __apac_cost_threshold) {\n";
    out += "\n#pragma omp task default(shared) firstprivate(" +
           joined(captures) + ")";
    if (!reads.empty())
      out += " depend(in: " + joined(reads) + ")";
    if (!writes.empty())
      out += " depend(inout: " + joined(writes) + ")";
    out +=
        "\n{\n__apac_depth_guard __apac_loop_depth(__apac_depth_local + 1);\n";
    out += getStmtAsStringFull(call, callee->getASTContext().getLangOpts());
    out += "\n}\n";
    if (costs) {
      // Inline writes have no task dependency record. Complete prior siblings
      // before evaluating/executing the sequential call; future siblings are
      // then created after its writes. This intentionally favors coarse work.
      out += "} else {\n#pragma omp taskwait\n"
             "__apac_depth_guard __apac_sequential_depth(__apac_depth_max);\n";
      out += costs->sequentialCall(call);
      out += "}\n";
    }
    ++calls;
    return true;
  };
  std::string body;
  if (!render(loop, body) || calls == 0)
    return false;
  // Deferred calls are siblings and join before leaving the region. With the
  // cost option, a direct sequential call additionally joins prior siblings.
  // Both paths protect control-local lifetimes and the following code.
  replacement = body + "\n#pragma omp taskwait\n";
  return true;
}
