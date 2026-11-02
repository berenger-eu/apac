#pragma once

#include <iterator>
#include <map>

// Private implementation header, included after ConstGraph is defined.

// Qualifier constraints are separate from the method-constness graph. A pointer
// has one slot for itself and one for each pointee; references use the slots of
// their referred-to type. A mutable destination constrains the source it
// aliases.
namespace constify {
class PointerConstAnalysis
    : public clang::RecursiveASTVisitor<PointerConstAnalysis> {
  struct Slot {
    bool originalConst;
    bool candidate;
    bool unsafeEscape = false;
  };
  struct Info {
    const clang::ValueDecl *decl;
    std::vector<Slot> slots;
    bool rewrite;
  };
  struct Access {
    const clang::ValueDecl *decl;
    unsigned depth;
  };
  struct Constraint {
    Access destination;
    Access source;
  };
  clang::ASTContext &Ctx;
  const ConstGraph &Graph;
  std::map<const clang::ValueDecl *, Info> Values;
  std::vector<Constraint> Constraints;
  std::set<const clang::FunctionDecl *> AddressTaken;
  const clang::FunctionDecl *CurrentFunction = nullptr;

  bool inMain(clang::SourceLocation loc) const {
    return loc.isValid() && !loc.isMacroID() &&
           Ctx.getSourceManager().isWrittenInMainFile(loc);
  }
  bool sequential(const clang::FunctionDecl *f) const {
    return f && f->getNameAsString().find("_apacSeq") != std::string::npos;
  }
  bool singleDeclaration(const clang::VarDecl *v) const {
    if (llvm::isa<clang::ParmVarDecl>(v))
      return true;
    for (const auto &parent : Ctx.getParents(*v))
      if (const auto *stmt = parent.get<clang::DeclStmt>())
        return stmt->isSingleDecl();
    return false;
  }
  bool canRewrite(const clang::ValueDecl *d) const {
    const auto *v = llvm::dyn_cast<clang::VarDecl>(d);
    if (!v || v->isImplicit() || !inMain(v->getBeginLoc()) ||
        !inMain(v->getLocation()) || !v->getTypeSourceInfo() || v->hasAttrs() ||
        v->isConstexpr() || v->getType()->isDependentType() ||
        v->getType()->isRValueReferenceType() ||
        sequential(llvm::dyn_cast<clang::FunctionDecl>(v->getDeclContext())) ||
        !singleDeclaration(v))
      return false;
    for (auto loc = v->getTypeSourceInfo()->getTypeLoc(); !loc.isNull();
         loc = loc.getNextTypeLoc())
      if (!loc.getAs<clang::DecltypeTypeLoc>().isNull())
        return false;
    if (const auto *owner =
            llvm::dyn_cast<clang::FunctionDecl>(v->getDeclContext()))
      if (owner->getTemplatedKind() != clang::FunctionDecl::TK_NonTemplate)
        return false;
    if (v->getType()->isRecordType() && v->getInit()) {
      if (const auto *construction =
              llvm::dyn_cast<clang::CXXConstructExpr>(v->getInit()))
        if (construction->getConstructor()->isDefaultConstructor() &&
            !construction->requiresZeroInitialization() &&
            !construction->getConstructor()
                 ->getParent()
                 ->allowConstDefaultInit())
          return false;
    }
    if (const auto *param = llvm::dyn_cast<clang::ParmVarDecl>(v)) {
      const auto *owner =
          llvm::dyn_cast<clang::FunctionDecl>(param->getDeclContext());
      if (!owner || !owner->doesThisDeclarationHaveABody())
        return false;
    } else if (!v->isLocalVarDecl() || !v->hasInit())
      return false;
    auto type = v->getType().getNonReferenceType();
    while (type->isPointerType())
      type = type->getPointeeType();
    return !type->isArrayType() && !type->isFunctionType() &&
           !type->isMemberPointerType();
  }
  Info &info(const clang::ValueDecl *decl) {
    decl = llvm::cast<clang::ValueDecl>(decl->getCanonicalDecl());
    auto found = Values.find(decl);
    if (found != Values.end())
      return found->second;
    if (const auto *method = llvm::dyn_cast<clang::CXXMethodDecl>(decl)) {
      bool candidate = method->hasBody() && !method->isStatic() &&
                       !method->isVirtual() &&
                       !llvm::isa<clang::CXXConstructorDecl>(method) &&
                       !llvm::isa<clang::CXXDestructorDecl>(method);
      bool graphCandidate = false;
      for (const auto &node : Graph.nodes())
        if (node.decl == method)
          graphCandidate = node.state == ConstState::Candidate;
      candidate &= graphCandidate;
      // Adding const can turn a previously unrelated derived method into an
      // override of a const virtual base method, changing dynamic dispatch.
      std::vector<const clang::CXXRecordDecl *> bases{method->getParent()};
      std::set<const clang::CXXRecordDecl *> visited;
      for (unsigned i = 0; i < bases.size(); ++i) {
        const auto *record = bases[i];
        if (!visited.insert(record).second)
          continue;
        if (i != 0) {
          for (const auto *baseMethod : record->methods()) {
            if (!baseMethod->isVirtual() || !baseMethod->isConst() ||
                baseMethod->getDeclName() != method->getDeclName() ||
                baseMethod->getNumParams() != method->getNumParams() ||
                baseMethod->getRefQualifier() != method->getRefQualifier())
              continue;
            bool same = true;
            for (unsigned p = 0; p < method->getNumParams(); ++p)
              same &= Ctx.hasSameType(
                  baseMethod->getParamDecl(p)->getType().getUnqualifiedType(),
                  method->getParamDecl(p)->getType().getUnqualifiedType());
            if (same)
              candidate = false;
          }
        }
        for (const auto &base : record->bases())
          if (const auto *declaration = base.getType()->getAsCXXRecordDecl())
            if (declaration->hasDefinition())
              bases.push_back(declaration->getDefinition());
      }

      for (const auto *declaration : method->redecls())
        candidate &= inMain(declaration->getBeginLoc()) &&
                     declaration->getTypeSourceInfo();
      for (const auto *other : method->getParent()->methods()) {
        if (other->getCanonicalDecl() == method ||
            other->getDeclName() != method->getDeclName() ||
            !other->isConst() ||
            other->getNumParams() != method->getNumParams() ||
            other->getRefQualifier() != method->getRefQualifier())
          continue;
        bool same = true;
        for (unsigned i = 0; i < method->getNumParams(); ++i)
          same &= Ctx.hasSameType(
              method->getParamDecl(i)->getType().getUnqualifiedType(),
              other->getParamDecl(i)->getType().getUnqualifiedType());
        if (same)
          candidate = false;
      }
      return Values
          .emplace(decl, Info{decl,
                              {{true, false}, {method->isConst(), candidate}},
                              false})
          .first->second;
    }
    bool rewrite = canRewrite(decl);
    Info value{decl, {}, rewrite};
    auto type = decl->getType().getNonReferenceType();
    while (!type.isNull()) {
      bool candidate = rewrite;
      value.slots.push_back({type.isConstQualified(), candidate});
      if (!type->isPointerType())
        break;
      type = type->getPointeeType();
    }
    return Values.emplace(decl, std::move(value)).first->second;
  }
  bool allowed(Access access) {
    auto &v = info(access.decl);
    return access.depth < v.slots.size() &&
           (v.slots[access.depth].originalConst ||
            v.slots[access.depth].candidate);
  }
  bool reject(Access access) {
    auto &v = info(access.decl);
    if (access.depth >= v.slots.size() || !v.slots[access.depth].candidate)
      return false;
    v.slots[access.depth].candidate = false;
    return !v.slots[access.depth].originalConst;
  }
  void rejectAll(const clang::Expr *expr, unsigned start = 0,
                 bool unsafe = false) {
    for (auto access : accesses(expr)) {
      auto size = info(access.decl).slots.size();
      for (unsigned i = access.depth + start; i < size; ++i) {
        reject({access.decl, i});
        if (unsafe)
          info(access.decl).slots[i].unsafeEscape = true;
      }
    }
  }
  std::vector<Access> accesses(const clang::Expr *expr) {
    if (!expr)
      return {};
    expr = expr->IgnoreParenImpCasts();
    if (llvm::isa<clang::CXXThisExpr>(expr) && CurrentFunction) {
      if (const auto *method =
              llvm::dyn_cast<clang::CXXMethodDecl>(CurrentFunction))
        return {{method, 0}};
    }
    if (const auto *ref = llvm::dyn_cast<clang::DeclRefExpr>(expr)) {
      if (const auto *value =
              llvm::dyn_cast<clang::ValueDecl>(ref->getDecl())) {
        if (llvm::isa<clang::VarDecl>(value))
          return {{value, 0}};
      }
    }
    if (const auto *member = llvm::dyn_cast<clang::MemberExpr>(expr)) {
      if (const auto *field =
              llvm::dyn_cast<clang::FieldDecl>(member->getMemberDecl())) {
        auto result = member->isArrow() ? pointees(member->getBase())
                                        : accesses(member->getBase());
        result.insert(result.begin(), {field, 0});
        return result;
      }
    }
    if (const auto *unary = llvm::dyn_cast<clang::UnaryOperator>(expr)) {
      auto result = accesses(unary->getSubExpr());
      if (unary->getOpcode() == clang::UO_Deref) {
        for (auto &a : result)
          ++a.depth;
        return result;
      }
      if (unary->isIncrementDecrementOp())
        return result;
    }
    if (const auto *array = llvm::dyn_cast<clang::ArraySubscriptExpr>(expr)) {
      auto result = accesses(array->getBase());
      if (!array->getBase()->IgnoreParenImpCasts()->getType()->isArrayType())
        for (auto &a : result)
          ++a.depth;
      return result;
    }
    if (const auto *binary = llvm::dyn_cast<clang::BinaryOperator>(expr)) {
      if (binary->isAdditiveOp()) {
        if (binary->getLHS()->getType()->isPointerType())
          return accesses(binary->getLHS());
        if (binary->getRHS()->getType()->isPointerType())
          return accesses(binary->getRHS());
      }
      if (binary->getOpcode() == clang::BO_Comma)
        return accesses(binary->getRHS());
      if (binary->isAssignmentOp())
        return accesses(binary->getLHS());
    }
    if (const auto *conditional =
            llvm::dyn_cast<clang::ConditionalOperator>(expr)) {
      auto result = accesses(conditional->getTrueExpr());
      auto other = accesses(conditional->getFalseExpr());
      result.insert(result.end(), other.begin(), other.end());
      return result;
    }
    if (const auto *cast = llvm::dyn_cast<clang::ExplicitCastExpr>(expr))
      return accesses(cast->getSubExpr());
    return {};
  }
  // The pointee of &object corresponds to the object itself, rather than a
  // negative depth in the pointer graph.
  std::vector<Access> pointees(const clang::Expr *expr) {
    if (!expr)
      return {};
    expr = expr->IgnoreParenImpCasts();
    if (const auto *unary = llvm::dyn_cast<clang::UnaryOperator>(expr))
      if (unary->getOpcode() == clang::UO_AddrOf)
        return accesses(unary->getSubExpr());
    auto result = accesses(expr);
    if (!expr->getType()->isArrayType())
      for (auto &a : result)
        ++a.depth;
    return result;
  }
  void connect(Access destination, Access source) {
    auto dn = info(destination.decl).slots.size();
    auto sn = info(source.decl).slots.size();
    while (destination.depth < dn && source.depth < sn) {
      Constraints.push_back({destination, source});
      ++destination.depth;
      ++source.depth;
    }
  }
  void fixed(clang::QualType type, const clang::Expr *argument,
             bool byReference) {
    if (type.isNull())
      return;
    auto values = byReference ? accesses(argument) : pointees(argument);
    if (!byReference) {
      if (!type->isPointerType())
        return;
      type = type->getPointeeType();
    }
    while (!type.isNull()) {
      if (!type.isConstQualified()) {
        for (auto value : values)
          reject(value);
        // A non-const reference to an ordinary field also requires a mutable
        // containing object, which is not represented by the field's slots.
        if (byReference)
          write(argument);
      }
      if (!type->isPointerType())
        break;
      type = type->getPointeeType();
      for (auto &a : values)
        ++a.depth;
      byReference = false;
    }
  }
  void bind(const clang::ValueDecl *destination, unsigned depth,
            const clang::Expr *source, bool reference) {
    if (!source)
      return;
    auto values = reference ? accesses(source) : pointees(source);
    for (auto value : values)
      connect({destination, depth}, value);
  }

  void write(const clang::Expr *expr) {
    if (!expr)
      return;
    expr = expr->IgnoreParenImpCasts();
    for (auto value : accesses(expr))
      reject(value);
    if (const auto *member = llvm::dyn_cast<clang::MemberExpr>(expr)) {
      auto values = member->isArrow() ? pointees(member->getBase())
                                      : accesses(member->getBase());
      for (auto value : values)
        reject(value);
    }
  }
  bool mayChangeSignature(const clang::FunctionDecl *f) {
    if (!f || sequential(f) || AddressTaken.count(f->getCanonicalDecl()) ||
        std::distance(f->redecls_begin(), f->redecls_end()) != 1)
      return false;
    if (const auto *method = llvm::dyn_cast<clang::CXXMethodDecl>(f))
      if (method->isVirtual())
        return false;
    for (const auto *decl : f->getDeclContext()->lookup(f->getDeclName()))
      if (const auto *other = llvm::dyn_cast<clang::FunctionDecl>(decl))
        if (other->getCanonicalDecl() != f->getCanonicalDecl())
          return false;
    return true;
  }
  void call(const clang::FunctionDecl *callee, unsigned count,
            llvm::function_ref<const clang::Expr *(unsigned)> argument) {
    for (unsigned i = 0; i < count; ++i) {
      const auto *arg = argument(i);
      if (!callee || i >= callee->getNumParams()) {
        rejectAll(arg);
        for (auto value : pointees(arg)) {
          auto size = info(value.decl).slots.size();
          for (unsigned level = value.depth; level < size; ++level)
            reject({value.decl, level});
        }
        continue;
      }
      const auto *parameter = callee->getParamDecl(i);
      auto type = parameter->getType();
      if (type->isReferenceType()) {
        if (info(parameter).rewrite)
          bind(parameter, 0, arg, true);
        else
          fixed(type.getNonReferenceType(), arg, true);
      } else if (type->isPointerType()) {
        if (info(parameter).rewrite)
          bind(parameter, 1, arg, false);
        else
          fixed(type, arg, false);
      }
    }
  }
  clang::QualType qualified(clang::QualType type, const Info &v,
                            unsigned depth) const {
    if (type->isPointerType()) {
      auto qualifiers = type.getQualifiers();
      type = Ctx.getQualifiedType(
          Ctx.getPointerType(qualified(type->getPointeeType(), v, depth + 1)),
          qualifiers);
    }
    if (depth < v.slots.size() && v.slots[depth].candidate)
      type = type.withConst();
    return type;
  }

public:
  PointerConstAnalysis(clang::ASTContext &ctx, const ConstGraph &graph)
      : Ctx(ctx), Graph(graph) {}
  bool TraverseDecl(clang::Decl *decl) {
    if (!decl)
      return true;
    auto loc = decl->getLocation();
    if (!llvm::isa<clang::TranslationUnitDecl>(decl) && loc.isValid() &&
        !Ctx.getSourceManager().isWrittenInMainFile(
            Ctx.getSourceManager().getExpansionLoc(loc)))
      return true;
    return clang::RecursiveASTVisitor<PointerConstAnalysis>::TraverseDecl(decl);
  }
  bool TraverseFunctionDecl(clang::FunctionDecl *f) {
    if (sequential(f))
      return true;
    auto *previous = CurrentFunction;
    CurrentFunction = f;
    bool result =
        clang::RecursiveASTVisitor<PointerConstAnalysis>::TraverseFunctionDecl(
            f);
    CurrentFunction = previous;
    return result;
  }
  bool TraverseCXXMethodDecl(clang::CXXMethodDecl *f) {
    auto *previous = CurrentFunction;
    CurrentFunction = f;
    bool result =
        clang::RecursiveASTVisitor<PointerConstAnalysis>::TraverseCXXMethodDecl(
            f);
    CurrentFunction = previous;
    return result;
  }
  bool VisitVarDecl(clang::VarDecl *v) {
    info(v);
    auto type = v->getType();
    if (type->isReferenceType())
      bind(v, 0, v->getInit(), true);
    else if (type->isPointerType())
      bind(v, 1, v->getInit(), false);
    return true;
  }
  bool VisitUnaryOperator(clang::UnaryOperator *op) {
    if (op->isIncrementDecrementOp())
      write(op->getSubExpr());
    return true;
  }
  bool VisitBinaryOperator(clang::BinaryOperator *op) {
    if (op->isAssignmentOp()) {
      write(op->getLHS());
      if (op->getLHS()->getType()->isPointerType()) {
        for (auto left : accesses(op->getLHS())) {
          auto rhs = pointees(op->getRHS());
          for (auto right : rhs)
            connect({left.decl, left.depth + 1}, right);
        }
      }
    }
    return true;
  }
  bool VisitReturnStmt(clang::ReturnStmt *stmt) {
    if (CurrentFunction && stmt->getRetValue()) {
      auto type = CurrentFunction->getReturnType();
      if (type->isReferenceType())
        fixed(type.getNonReferenceType(), stmt->getRetValue(), true);
      else if (type->isPointerType())
        fixed(type, stmt->getRetValue(), false);
    }
    return true;
  }
  bool VisitCallExpr(clang::CallExpr *expr) {
    unsigned offset = 0;
    if (const auto *op = llvm::dyn_cast<clang::CXXOperatorCallExpr>(expr)) {
      if (const auto *method = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(
              op->getDirectCallee())) {
        if (!method->isStatic() && op->getNumArgs()) {
          offset = 1;
          if (!method->isConst())
            for (auto value : accesses(op->getArg(0)))
              connect({method, 1}, value);
        }
      }
    }
    if (const auto *member = llvm::dyn_cast<clang::CXXMemberCallExpr>(expr)) {
      const auto *method = member->getMethodDecl();
      if (!method || !method->isConst()) {
        auto *object = member->getImplicitObjectArgument();
        auto values = object->getType()->isPointerType() ? pointees(object)
                                                         : accesses(object);
        for (auto value : values) {
          if (method)
            connect({method, 1}, value);
          else
            reject(value);
        }
      }
    }
    call(expr->getDirectCallee(), expr->getNumArgs() - offset,
         [&](unsigned i) { return expr->getArg(i + offset); });
    return true;
  }
  bool VisitCXXConstructExpr(clang::CXXConstructExpr *expr) {
    call(expr->getConstructor(), expr->getNumArgs(),
         [&](unsigned i) { return expr->getArg(i); });
    return true;
  }
  bool VisitDecltypeTypeLoc(clang::DecltypeTypeLoc loc) {
    rejectAll(loc.getTypePtr()->getUnderlyingExpr());
    return true;
  }
  bool VisitCXXTypeidExpr(clang::CXXTypeidExpr *expr) {
    if (!expr->isTypeOperand())
      rejectAll(expr->getExprOperand());
    return true;
  }
  bool VisitLambdaExpr(clang::LambdaExpr *expr) {
    for (const auto &capture : expr->captures()) {
      if (!capture.capturesVariable())
        continue;
      const auto *variable = capture.getCapturedVar();
      if (capture.getCaptureKind() == clang::LCK_ByRef ||
          variable->getType()->isPointerType()) {
        auto count = info(variable).slots.size();
        for (unsigned i = 0; i < count; ++i)
          reject({variable, i});
      }
    }
    return true;
  }
  bool VisitExplicitCastExpr(clang::ExplicitCastExpr *expr) {
    if (expr->getSubExpr()->getType()->isPointerType())
      rejectAll(expr->getSubExpr(), 0, true);
    return true;
  }
  bool VisitDeclRefExpr(clang::DeclRefExpr *expr) {
    const auto *function = llvm::dyn_cast<clang::FunctionDecl>(expr->getDecl());
    if (!function)
      return true;
    clang::DynTypedNode node = clang::DynTypedNode::create(*expr);
    while (true) {
      auto parents = Ctx.getParents(node);
      if (parents.size() != 1)
        break;
      if (const auto *callExpr = parents[0].get<clang::CallExpr>()) {
        if (callExpr->getCallee()->IgnoreParenImpCasts() == expr)
          return true;
        break;
      }
      if (!parents[0].get<clang::Expr>())
        break;
      node = parents[0];
    }
    AddressTaken.insert(function->getCanonicalDecl());
    return true;
  }
  void analyze() {
    TraverseDecl(Ctx.getTranslationUnitDecl());
    for (auto &entry : Values) {
      auto &v = entry.second;
      if (const auto *method = llvm::dyn_cast<clang::CXXMethodDecl>(v.decl))
        if (functionAddressTaken(method))
          reject({method, 1});
      if (const auto *parameter = llvm::dyn_cast<clang::ParmVarDecl>(v.decl)) {
        const auto *owner =
            llvm::dyn_cast<clang::FunctionDecl>(parameter->getDeclContext());
        unsigned first = v.decl->getType()->isReferenceType() ? 0 : 1;
        if (!mayChangeSignature(owner))
          for (unsigned i = first; i < v.slots.size(); ++i)
            reject({v.decl, i});
      }
    }
    bool changed;
    do {
      changed = false;
      for (auto constraint : Constraints) {
        const auto &destination = info(constraint.destination.decl)
                                      .slots[constraint.destination.depth];
        if (!allowed(constraint.destination) || destination.unsafeEscape)
          changed |= reject(constraint.source);
        if (destination.unsafeEscape) {
          auto &source =
              info(constraint.source.decl).slots[constraint.source.depth];
          if (!source.unsafeEscape) {
            source.unsafeEscape = true;
            changed = true;
          }
        }
      }
      for (auto &entry : Values) {
        auto &v = entry.second;
        // Qualification conversions at multiple pointer levels require const
        // on all intervening pointer types. A reference also requires const
        // on the referred pointer object for direct reference compatibility.
        unsigned first = v.decl->getType()->isReferenceType() ? 0 : 1;
        for (unsigned depth = 1; depth < v.slots.size(); ++depth) {
          if (v.slots[depth].originalConst)
            continue;
          for (unsigned middle = first; middle < depth; ++middle)
            if (!allowed({v.decl, middle}))
              changed |= reject({v.decl, depth});
        }
      }
    } while (changed);
  }
  bool canConst(const clang::ValueDecl *v, unsigned depth = 0) {
    return allowed({v, depth});
  }
  bool functionAddressTaken(const clang::FunctionDecl *f) const {
    return AddressTaken.count(f->getCanonicalDecl());
  }
  clang::QualType rewrittenType(const clang::ValueDecl *decl) {
    auto &v = info(decl);
    if (!v.rewrite)
      return decl->getType();
    auto result = qualified(decl->getType().getNonReferenceType(), v, 0);
    if (decl->getType()->isLValueReferenceType())
      result = Ctx.getLValueReferenceType(result);
    return result;
  }
};
} // namespace constify
