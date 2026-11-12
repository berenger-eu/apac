#include "ArrayPhases.hpp"
#include "clang/AST/DeclTemplate.h"
#include <functional>

namespace {
QualType bufferType(QualType type) {
  type = type.getNonReferenceType();
  if (const auto *record = type->getAsCXXRecordDecl())
    if (const auto *specialization =
            dyn_cast<ClassTemplateSpecializationDecl>(record))
      if (specialization->getSpecializedTemplate()
              ->getQualifiedNameAsString() == "std::reference_wrapper")
        type = specialization->getTemplateArgs()[0]
                   .getAsType()
                   .getNonReferenceType();
  return type;
}
bool flatBuffer(QualType type) {
  type = bufferType(type);
  QualType element;
  if (type->isPointerType())
    element = type->getPointeeType();
  else if (const auto *array = dyn_cast<ArrayType>(type.getTypePtr()))
    element = array->getElementType();
  return !element.isNull() &&
         (element->isArithmeticType() || element->isEnumeralType());
}
const VarDecl *baseVariable(const Expr *expr) {
  const auto *array = dyn_cast<ArraySubscriptExpr>(expr->IgnoreParenImpCasts());
  const auto *ref = array ? getArrayBaseDeclRefExpr(array)
                          : getSingleDeclRefExprInsideExpr(expr);
  return ref ? dyn_cast<VarDecl>(ref->getDecl()) : nullptr;
}
bool scalarTarget(const VarDecl *base, AliasTable &table) {
  auto pointer = table.getAliasArg(base);
  if (!pointer || pointer->aliased.empty())
    return false;
  auto targets = table.getAliased(pointer);
  bool found = false;
  for (const auto &target : targets) {
    if (target == pointer || target->type == Pointer ||
        target->type == Reference)
      continue;
    if (target->declaration.getType()->isArrayType())
      return false;
    found = true;
  }
  return found;
}
std::string variableName(const VarDecl *variable) {
  aliasArg alias(*variable, getAliasType(variable));
  return alias.varAsString();
}
} // namespace

void analyzeArrayPhases(Instruction &instruction, AliasTable &table,
                        const ParamWriteAnalyzer *parameters,
                        const std::string &token) {
  std::set<const VarDecl *> buffers;
  std::set<const Stmt *> covered;
  auto inputs = [&](const Expr *expr) {
    std::vector<const Stmt *> pending{expr};
    while (!pending.empty()) {
      const Stmt *node = pending.back();
      pending.pop_back();
      if (const auto *ref = dyn_cast<DeclRefExpr>(node)) {
        if (const auto *variable = dyn_cast<VarDecl>(ref->getDecl())) {
          QualType type = bufferType(variable->getType());
          if (type->isPointerType() || type->isArithmeticType() ||
              type->isEnumeralType()) {
            if (variable->getType()->isRecordType() && !type->isPointerType())
              instruction.arrayPhaseFallback = true;
            auto name = variableName(variable);
            instruction.arrayAddressInputs.insert(name);
            // Plain pointer/scalar values are snapshots. A wrapper's binding is
            // instead protected by its ordinary dependencies.
            if (variable->isLocalVarDeclOrParm() &&
                !variable->getType()->isRecordType())
              instruction.arrayCaptures.insert(variable->getNameAsString());
          }
        }
      }
      if (isa<ArraySubscriptExpr>(node))
        instruction.arrayPhaseFallback = true;
      bool conversion = false;
      if (const auto *call = dyn_cast<CallExpr>(node))
        if (const auto *method =
                dyn_cast_or_null<CXXConversionDecl>(call->getDirectCallee()))
          conversion = method->getParent()->getQualifiedNameAsString() ==
                       "std::reference_wrapper";
      if (isa<ArraySubscriptExpr>(node) ||
          (isa<CallExpr>(node) && !conversion)) {
        instruction.arrayAddressNeedsWait = true;
        instruction.arrayPhaseFallback = true;
      }
      if (const auto *unary = dyn_cast<UnaryOperator>(node))
        if (unary->isIncrementDecrementOp())
          instruction.arrayPhaseFallback = true;
      if (const auto *binary = dyn_cast<BinaryOperator>(node))
        if (binary->isAssignmentOp())
          instruction.arrayPhaseFallback = true;
      for (const auto *child : node->children())
        if (child)
          pending.push_back(child);
    }
  };
  auto access = [&](const Expr *expr, ArrayAccessMode mode, bool write,
                    bool explicitlyIndexed) {
    const auto *base = baseVariable(expr);
    if (!base || !flatBuffer(base->getType())) {
      instruction.arrayPhaseFallback = true;
      return;
    }
    if (!explicitlyIndexed && scalarTarget(base, table)) {
      instruction.arrayPhaseFallback = true;
      return;
    }
    std::string representative;
    if (const auto *array =
            dyn_cast<ArraySubscriptExpr>(expr->IgnoreParenImpCasts())) {
      // Reuse the existing wrapper-aware locator printer.
      representative = table.getOrAddAliasArg(array)->dependencyAsString();
      if (array->getIdx()->HasSideEffects(base->getASTContext()))
        instruction.arrayPhaseFallback = true;
      inputs(array->getBase());
      inputs(array->getIdx());
    } else {
      inputs(expr);
      if (mode != ArrayAccessMode::Whole &&
          expr->HasSideEffects(base->getASTContext()))
        instruction.arrayPhaseFallback = true;
      if (mode != ArrayAccessMode::Whole)
        representative =
            "(" + getExprAsString(expr, base->getASTContext().getLangOpts()) +
            ")[0]";
    }
    auto element = bufferType(base->getType());
    if (element->isPointerType())
      element = element->getPointeeType();
    else
      element = cast<ArrayType>(element.getTypePtr())->getElementType();
    instruction.arrayAccesses.push_back(
        {mode, representative,
         element.getUnqualifiedType().getCanonicalType().getAsString(), write});
    buffers.insert(base);
    if (auto alias = table.getAliasArg(base))
      for (const auto &target : table.getAliased(alias))
        if (target->indexes.empty() && target->type == Pointer)
          instruction.arrayAddressInputs.insert(
              variableName(&target->declaration));
  };
  std::function<void(const Stmt *, bool)> scan = [&](const Stmt *node,
                                                     bool writing) {
    if (!node || covered.count(node))
      return;
    if (const auto *binary = dyn_cast<BinaryOperator>(node))
      if (binary->isAssignmentOp() && flatBuffer(binary->getLHS()->getType()))
        instruction.arrayPhaseFallback = true;
    if (const auto *unary = dyn_cast<UnaryOperator>(node))
      if (unary->isIncrementDecrementOp() &&
          flatBuffer(unary->getSubExpr()->getType()))
        instruction.arrayPhaseFallback = true;
    if (const auto *op = dyn_cast<CXXOperatorCallExpr>(node))
      if (op->isAssignmentOp() && op->getNumArgs() &&
          flatBuffer(op->getArg(0)->getType()))
        instruction.arrayPhaseFallback = true;
    if (const auto *binary = dyn_cast<BinaryOperator>(node)) {
      if (binary->isAssignmentOp()) {
        scan(binary->getLHS(), true);
        scan(binary->getRHS(), false);
        return;
      }
    }
    if (const auto *unary = dyn_cast<UnaryOperator>(node)) {
      if (unary->isIncrementDecrementOp()) {
        scan(unary->getSubExpr(), true);
        return;
      }
    }
    if (const auto *call = dyn_cast<CallExpr>(node)) {
      const auto *callee = call->getDirectCallee();
      // Implicit conversions/operator machinery are handled by the memory
      // expression that contains them, not as independent buffer calls.
      if (isa<CXXOperatorCallExpr>(call)) {
        if (!isa_and_nonnull<CXXConversionDecl>(callee))
          for (const auto *arg : call->arguments())
            if (arg->getType()->isPointerType() || flatBuffer(arg->getType()))
              instruction.arrayPhaseFallback = true;
      } else {
        for (unsigned i = 0; i < call->getNumArgs(); ++i) {
          const Expr *arg = call->getArg(i)->IgnoreParenImpCasts();
          const ParmVarDecl *param = callee && i < callee->getNumParams()
                                         ? callee->getParamDecl(i)
                                         : nullptr;
          bool indirect = !param || isPointerQualType(param->getType()) ||
                          isReferenceQualType(param->getType());
          bool write = indirect && (!parameters || !callee ||
                                    !parameters->isParamReadOnly(callee, i));
          if (param && param->getType()->isReferenceType() &&
              bufferType(param->getType())->isPointerType() && write) {
            instruction.arrayPhaseFallback = true;
            covered.insert(call->getArg(i));
            continue;
          }
          if (const auto *address = dyn_cast<UnaryOperator>(arg)) {
            if (address->getOpcode() == UO_AddrOf) {
              if (const auto *array = dyn_cast<ArraySubscriptExpr>(
                      address->getSubExpr()->IgnoreParenImpCasts())) {
                access(array, ArrayAccessMode::Partition, write, true);
                covered.insert(call->getArg(i));
                continue;
              }
            }
          }
          if (const auto *dereference = dyn_cast<UnaryOperator>(arg)) {
            if (dereference->getOpcode() == UO_Deref &&
                flatBuffer(dereference->getSubExpr()->getType())) {
              access(dereference->getSubExpr(), ArrayAccessMode::Element, write,
                     false);
              covered.insert(call->getArg(i));
              continue;
            }
          }
          if (const auto *array = dyn_cast<ArraySubscriptExpr>(arg)) {
            access(array, ArrayAccessMode::Element, write, true);
            covered.insert(call->getArg(i));
            continue;
          }
          if (flatBuffer(arg->getType())) {
            access(arg, ArrayAccessMode::Whole, write, false);
            covered.insert(call->getArg(i));
          } else if (arg->getType()->isPointerType() ||
                     arg->getType()->isArrayType()) {
            instruction.arrayPhaseFallback = true;
          }
        }
      }
    }
    if (const auto *array = dyn_cast<ArraySubscriptExpr>(node)) {
      auto alias = table.getOrAddAliasArg(array);
      auto dependency = instruction.dependencies.find(alias);
      access(array, ArrayAccessMode::Element,
             writing || (dependency != instruction.dependencies.end() &&
                         dependency->second.isWrite),
             true);
      // The index can itself read another array; its producer must finish
      // before the locator is evaluated at task creation.
      scan(array->getIdx(), false);
      return;
    }
    if (const auto *unary = dyn_cast<UnaryOperator>(node)) {
      if (unary->getOpcode() == UO_Deref &&
          flatBuffer(unary->getSubExpr()->getType())) {
        access(unary->getSubExpr(), ArrayAccessMode::Element, writing, false);
        return;
      }
    }
    for (const auto *child : node->children())
      if (child)
        scan(child, false);
  };
  scan(instruction.instruction, false);
  if (instruction.arrayAccesses.empty())
    return;
  for (const auto &dependency : instruction.dependencies) {
    const auto &alias = dependency.first;
    auto type = alias->declaration.getType().getNonReferenceType();
    if (alias->indexes.empty() && dependency.second.isWrite &&
        (type->isArithmeticType() || type->isEnumeralType()) &&
        instruction.arrayAddressInputs.count(alias->varAsString()))
      instruction.arrayPhaseFallback = true;
  }
  instruction.arrayPhaseToken = token;
  instruction.noFusion = true;
  // Legacy array expansion must not turn a representative into dependencies on
  // all children. Retain only pointer storage, indices and unrelated scalars.
  for (auto it = instruction.dependencies.begin();
       it != instruction.dependencies.end();) {
    const auto &alias = it->first;
    bool data = false;
    for (const auto *base : buffers) {
      if (&alias->declaration == base) {
        if (!alias->indexes.empty() ||
            bufferType(base->getType())->isArrayType())
          data = true;
        else {
          it->second.isWrite = false;
          it->second.isRead = true;
        }
      }
      auto main = table.getAliasArg(base);
      if (main && &alias->declaration != base)
        for (const auto &target : table.getAliased(main))
          if (target == alias) {
            if (target->declaration.getType()->isArrayType() ||
                !target->indexes.empty())
              data = true;
            else if (target->type == Pointer || target->type == Reference) {
              it->second.isWrite = false;
              it->second.isRead = true;
            }
          }
    }
    if (data)
      it = instruction.dependencies.erase(it);
    else
      ++it;
  }
}

bool finishArrayPhases(std::vector<Instruction> &instructions) {
  bool active = false;
  for (auto &instruction : instructions)
    active |= !instruction.arrayAccesses.empty() ||
              finishArrayPhases(instruction.scopedInstructions);
  if (active) {
    std::function<void(std::vector<Instruction> &)> fallback =
        [&](auto &scope) {
          for (auto &instruction : scope) {
            if (instruction.arrayPhaseFallback) {
              instruction.synchronous = true;
              instruction.noFusion = true;
              instruction.arrayAccesses.clear();
            }
            fallback(instruction.scopedInstructions);
          }
        };
    fallback(instructions);
  }
  return active;
}
