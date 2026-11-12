#include "ASTUnstackVisitor.hpp"
#include "clang/AST/ExprCXX.h"
#include "clang/Lex/Lexer.h"

using namespace clang;

namespace {
bool hasGoto(Stmt *statement) {
  if (!statement)
    return false;
  if (isa<GotoStmt>(statement) || isa<IndirectGotoStmt>(statement))
    return true;
  for (Stmt *child : statement->children())
    if (hasGoto(child))
      return true;
  return false;
}
bool mentions(Stmt *statement, const VarDecl *variable) {
  if (!statement)
    return false;
  if (auto *reference = dyn_cast<DeclRefExpr>(statement))
    if (reference->getDecl() == variable)
      return true;
  for (Stmt *child : statement->children())
    if (mentions(child, variable))
      return true;
  return false;
}
// Calls delimit argument evaluations. Residual reads outside them must not be
// delayed until after a different argument's call (f(g()+x, h()) for example).
bool isolatedArgument(Expr *expression) {
  if (auto *call = dyn_cast<CallExpr>(expression))
    return call->isPRValue();
  if (isa<DeclRefExpr>(expression) || isa<MemberExpr>(expression) ||
      isa<AbstractConditionalOperator>(expression) ||
      isa<ArraySubscriptExpr>(expression) ||
      isa<UnaryExprOrTypeTraitExpr>(expression) ||
      isa<CXXNoexceptExpr>(expression) || isa<CXXTypeidExpr>(expression) ||
      isa<CXXDefaultArgExpr>(expression))
    return false;
  if (auto *binary = dyn_cast<BinaryOperator>(expression))
    if (binary->isLogicalOp() || binary->isAssignmentOp() ||
        binary->getOpcode() == BO_Comma || binary->getOpcode() == BO_Shl ||
        binary->getOpcode() == BO_Shr || binary->getOpcode() == BO_PtrMemD ||
        binary->getOpcode() == BO_PtrMemI)
      return false;
  for (Stmt *child : expression->children())
    if (auto *operand = dyn_cast_or_null<Expr>(child))
      if (!isolatedArgument(operand))
        return false;
  return true;
}
} // namespace

bool ASTUnstackVisitor::TraverseFunctionDecl(FunctionDecl *function) {
  // New initialized temporaries must not make an existing goto jump across an
  // initialization. GotoRet introduces its own scoped exits only after this
  // pass.
  if (!functionsConditions(function) || function->isConstexpr() ||
      hasGoto(function->getBody()))
    return true;
  source = TheRewriter.getSourceMgr()
               .getBufferData(TheRewriter.getSourceMgr().getMainFileID())
               .str();
  statement(function->getBody());
  return true;
}

std::string ASTUnstackVisitor::temporary() {
  std::string name;
  do {
    name = "__tempVar_" + std::to_string(counter++);
  } while (source.find(name) != std::string::npos);
  return name;
}

bool ASTUnstackVisitor::safe(Expr *expression) const {
  if (!expression)
    return true;
  if (expression->getBeginLoc().isMacroID() ||
      expression->getEndLoc().isMacroID() || expression->isTypeDependent() ||
      expression->isValueDependent() ||
      expression->getType().isVolatileQualified())
    return false;
  // Naming a class prvalue can lose guaranteed copy elision and changes the
  // destruction point. Keep its entire full expression intact, even if movable.
  if ((expression->getType()->isRecordType() && expression->isPRValue()) ||
      isa<ExprWithCleanups>(expression) || isa<CXXConstructExpr>(expression) ||
      isa<CXXOperatorCallExpr>(expression) || isa<CXXNewExpr>(expression) ||
      isa<CXXDeleteExpr>(expression) || isa<CXXThrowExpr>(expression) ||
      isa<LambdaExpr>(expression) || isa<StmtExpr>(expression) ||
      isa<InitListExpr>(expression))
    return false;
  if (auto *unary = dyn_cast<UnaryOperator>(expression))
    if (unary->isIncrementDecrementOp())
      return false;
  for (auto *child : expression->children())
    if (auto *operand = dyn_cast_or_null<Expr>(child))
      if (!safe(operand))
        return false;
  return true;
}

std::string ASTUnstackVisitor::render(
    Expr *expression, const std::map<const Stmt *, std::string> &names) const {
  class Printer : public PrinterHelper {
    const std::map<const Stmt *, std::string> &names;

  public:
    explicit Printer(const std::map<const Stmt *, std::string> &names)
        : names(names) {}
    bool handledStmt(Stmt *statement, llvm::raw_ostream &out) override {
      auto it = names.find(statement);
      if (it == names.end())
        return false;
      out << it->second;
      return true;
    }
  } printer(names);
  std::string result;
  llvm::raw_string_ostream stream(result);
  expression->printPretty(stream, &printer,
                          PrintingPolicy(TheRewriter.getLangOpts()));
  return result;
}

void ASTUnstackVisitor::extract(Expr *expression, bool root,
                                std::map<const Stmt *, std::string> &names,
                                std::string &prefix) {
  if (!expression)
    return;
  // Preserve lazy evaluation, sequencing and unevaluated operands. In
  // particular never lift a call from a conditional arm or a sizeof operand.
  if (isa<AbstractConditionalOperator>(expression) ||
      isa<UnaryExprOrTypeTraitExpr>(expression) ||
      isa<CXXNoexceptExpr>(expression) || isa<CXXTypeidExpr>(expression) ||
      isa<CXXDefaultArgExpr>(expression) || isa<ArraySubscriptExpr>(expression))
    return;
  if (auto *binary = dyn_cast<BinaryOperator>(expression)) {
    if (binary->isLogicalOp() || binary->getOpcode() == BO_Comma ||
        binary->getOpcode() == BO_Shl || binary->getOpcode() == BO_Shr ||
        binary->getOpcode() == BO_PtrMemD || binary->getOpcode() == BO_PtrMemI)
      return;
    if (binary->isAssignmentOp()) {
      // C++17 evaluates the RHS before the LHS. Do not lift address/index
      // computations from the LHS past the remaining RHS expression.
      extract(binary->getRHS(), true, names, prefix);
      return;
    }
  }
  auto *call = dyn_cast<CallExpr>(expression);
  if (call) {
    // Choose one permitted order of argument evaluations, without interleaving
    // observable residual reads from separate arguments.
    bool isolated = true;
    if (call->getNumArgs() > 1)
      for (Expr *arg : call->arguments())
        isolated &= isolatedArgument(arg);
    if (call->getDirectCallee() && !isa<CXXMemberCallExpr>(call) && isolated)
      for (Expr *arg : call->arguments())
        extract(arg, false, names, prefix);
  } else {
    for (Stmt *child : expression->children())
      if (auto *operand = dyn_cast_or_null<Expr>(child))
        extract(operand, root && operand == expression->IgnoreImplicit(), names,
                prefix);
  }
  // Keep reference-returning calls in their consumer. DeclarationSplitter
  // lowers named references to reference_wrapper, which cannot preserve the
  // original category/overload resolution of these extracted temporaries.
  if (!call || root || !call->isPRValue() || call->getType()->isVoidType())
    return;
  auto name = temporary();
  prefix += "auto " + name + " = " + render(call, names) + ";\n";
  // Naming a scalar prvalue makes it an lvalue; restore the original category.
  names[call] = "static_cast<decltype(" + name + ")>(" + name + ")";
}

void ASTUnstackVisitor::normalize(Stmt *original, Expr *expression,
                                  bool inCompound) {
  if (!expression || !safe(expression))
    return;
  std::map<const Stmt *, std::string> names;
  std::string prefix;
  // Preserve a top-level call; extract calls used by its arguments or by
  // operators. This leaves one assignment/call per statement for TaskGraph.
  extract(expression, true, names, prefix);
  if (prefix.empty())
    return;
  auto &sm = TheRewriter.getSourceMgr();
  auto end = Lexer::findLocationAfterToken(original->getEndLoc(), tok::semi, sm,
                                           TheRewriter.getLangOpts(), false);
  if (end.isInvalid() && !inCompound)
    return;
  TheRewriter.ReplaceText(expression->getSourceRange(),
                          render(expression, names));
  // A naked if/loop/label body needs a block so the prefix remains conditional.
  if (inCompound) {
    TheRewriter.InsertTextBefore(original->getBeginLoc(), prefix);
  } else {
    TheRewriter.InsertTextBefore(original->getBeginLoc(), "{\n" + prefix);
    TheRewriter.InsertTextBefore(end, "\n}");
  }
}

void ASTUnstackVisitor::statement(Stmt *current, bool inCompound) {
  if (!current || !elementsConditions(current))
    return;
  if (auto *compound = dyn_cast<CompoundStmt>(current)) {
    for (Stmt *child : compound->body())
      statement(child, true);
  } else if (auto *decl = dyn_cast<DeclStmt>(current)) {
    if (!decl->isSingleDecl())
      return;
    if (auto *var = dyn_cast<VarDecl>(decl->getSingleDecl())) {
      if (var->hasLocalStorage() && !var->isConstexpr() &&
          !var->getType()->isArrayType() && !mentions(var->getInit(), var))
        normalize(decl, var->getInit(), inCompound);
    }
  } else if (auto *ret = dyn_cast<ReturnStmt>(current)) {
    normalize(ret, ret->getRetValue(), inCompound);
  } else if (auto *expr = dyn_cast<Expr>(current)) {
    normalize(expr, expr, inCompound);
  } else if (auto *branch = dyn_cast<IfStmt>(current)) {
    if (branch->isConstexpr())
      return;
    statement(branch->getThen());
    statement(branch->getElse());
  } else if (auto *loop = dyn_cast<ForStmt>(current)) {
    statement(loop->getBody());
  } else if (auto *loop = dyn_cast<WhileStmt>(current)) {
    statement(loop->getBody());
  } else if (auto *loop = dyn_cast<DoStmt>(current)) {
    statement(loop->getBody());
  } else if (auto *loop = dyn_cast<CXXForRangeStmt>(current)) {
    statement(loop->getBody());
  } else if (isa<SwitchStmt>(current)) {
    // Later case labels can otherwise jump past a newly initialized temporary.
    return;
  } else if (auto *label = dyn_cast<SwitchCase>(current)) {
    statement(label->getSubStmt());
  } else if (auto *label = dyn_cast<LabelStmt>(current)) {
    statement(label->getSubStmt());
  }
}
