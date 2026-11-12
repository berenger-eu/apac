#pragma once
#include <memory>
#include <string>
namespace clang {
class ASTContext;
class FunctionDecl;
class CallExpr;
} // namespace clang
// Pure scalar models. Recursive SCCs are collapsed, never recursively
// evaluated.
class StaticCostModel {
public:
  explicit StaticCostModel(clang::ASTContext &);
  ~StaticCostModel();
  std::string definitions() const;
  std::string entryCost(const clang::FunctionDecl *) const;
  std::string callCost(const clang::CallExpr *) const;
  std::string sequentialCall(const clang::CallExpr *) const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
