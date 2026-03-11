#include "ASTSplitterVisitor.hpp"
using namespace clang;

bool ASTSplitterVisitor::isValidSeparation(VarDecl *v1) {
  bool result = true;
  if (v1->getType()->isStructureOrClassType()) {
    const CXXRecordDecl *cxxRecordDecl = v1->getType()->getAsCXXRecordDecl();
    auto it = cxxRecordDecl->ctor_begin();
    while (it != cxxRecordDecl->ctor_end() && result) {
      if (it->isDefaultConstructor() && it->isDeleted()) {
        result = false;
      }
      it++;
    }
  }
  return result;
}

bool ASTSplitterVisitor::VisitDeclStmt(DeclStmt *declSt) {
  if (isInHeaders(TheRewriter.getSourceMgr(), declSt->getBeginLoc())) {
    return true;
  }

  std::stringstream SSprintDecl;
  std::stringstream SSprintInit;
  if (declSt->isSingleDecl() && isa<VarDecl>(declSt->getSingleDecl())) {
    VarDecl *v = cast<VarDecl>(declSt->getSingleDecl());
    if (v->getInit()) {
      stringVarDecl(v, SSprintDecl, SSprintInit);
      TheRewriter.RemoveText(
          SourceRange(v->getInit()->getBeginLoc(), v->getInit()->getEndLoc()));
    }
  } else {
    const DeclGroupRef &dgr = declSt->getDeclGroup();
    for (DeclGroupRef::const_iterator b = dgr.begin(), e = dgr.end(); b != e;
         b++) {
      if (isa<VarDecl>(*b)) {
        VarDecl *v = cast<VarDecl>(*b);
        stringVarDecl(v, SSprintDecl, SSprintInit);
        /*
        if(b==dgr.begin())
            SSprintDecl<<v->getType().getAsString()<<" "<<v->getNameAsString();
        else
            SSprintDecl<<", "<<v->getNameAsString();
        if(v->getInit()){
            SSprintInit<<getVarDeclDefStr(*v);
            TheRewriter.RemoveText(SourceRange(v->getInit()->getBeginLoc(),v->getInit()->getEndLoc()));
        }
        */
      }
    }
    // SSprintDecl<<";\n";
  }
  std::stringstream SSprintFinal;
  SSprintFinal << SSprintDecl.str() << SSprintInit.str();
  if (!SSprintFinal.str().empty()) {
    TheRewriter.ReplaceText(
        SourceRange(declSt->getBeginLoc(), declSt->getEndLoc()),
        SSprintFinal.str());
  }

  return true;
}
void ASTSplitterVisitor::stringVarDecl(VarDecl *v,
                                       std::stringstream &SSprintDecl,
                                       std::stringstream &SSprintInit) {
  if (!isValidSeparation(v)) {
    SSprintDecl << getCompleteVarDeclStr(v);
  } else if (isReferenceQualType(v->getType())) {
    const QualType &qType =
        getUnreferencedQType(v->getType(), v->getASTContext());
    const Expr *init = (v->getInit()->IgnoreCasts());
    // Or use somehow MaterializeTemporaryExpr
    int childrenSize = 0;
    for (auto it = init->child_begin(); it != init->child_end(); ++it) {
      childrenSize++;
    }
    // When the initialization is a value and not a variable or reference for a
    // const reference A temporary value is created to store the value, so the
    // transformation is different
    if ((qType.isConstQualified()
         // If it's a call to a function returning a value
         && (childrenSize == 1 && isa<CallExpr>(init) &&
             !isReferenceQualType(init->getType())))
        // Or if there is more than one child, then it must be a value and not a
        // variable
        || childrenSize > 1
        // Or if it's not a variable
        || !isa<DeclRefExpr>(init)) {
      // Creates std::reference_wrapper<type>
      SSprintDecl << "std::reference_wrapper<" << qType.getAsString() << "> "
                  << v->getNameAsString()
                  //= invalid_ref<unqualified type>();
                  << " = invalid_ref<"
                  << qType.getUnqualifiedType().getAsString() << ">();\n";
      SSprintDecl << v->getType().getAsString() << " __refT_"
                  << v->getNameAsString() << "=" << getInitString(v) << ";\n";
      SSprintInit << v->getNameAsString() << "= __refT_" << v->getNameAsString()
                  << ";\n";
    } else {
      // Creates std::reference_wrapper<type>
      SSprintDecl << "std::reference_wrapper<" << qType.getAsString() << "> "
                  << v->getNameAsString()
                  //= invalid_ref<type>();
                  << " = invalid_ref<" << qType.getAsString() << ">();\n";
      SSprintInit << getVarDeclDefStr(v);
    }
  } else if (v->getType().isConstQualified() && v->getInit()) {
    // SSprintDecl<<getCompleteVarDeclStr(v);

    const QualType &qType =
        getReferenceToQType(v->getType(), v->getASTContext());
    // Creates std::reference_wrapper<type>
    SSprintDecl << "std::reference_wrapper<" << v->getType().getAsString()
                << "> "
                << v->getNameAsString()
                //= invalid_ref<unqualified type>();
                << " = invalid_ref<"
                << v->getType().getUnqualifiedType().getAsString() << ">();\n";
    SSprintDecl << qType.getAsString() << " __refT_" << v->getNameAsString()
                << "=" << getInitString(v) << ";\n";
    SSprintInit << v->getNameAsString() << "= __refT_" << v->getNameAsString()
                << ";\n";

  } else {
    SSprintDecl << getVarDeclDeclStr(v);
    if (v->getInit()) {
      SSprintInit << getVarDeclDefStr(v);
    }
  }
}
