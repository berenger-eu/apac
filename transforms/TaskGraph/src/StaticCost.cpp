#include "StaticCost.hpp"
#include "common.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/ExprCXX.h"
#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <sstream>

using namespace clang;
namespace {
const std::string unknown = "__apac_cost_unknown";
using Bindings = std::map<const VarDecl *, std::string>;
std::string add(const std::string &a, const std::string &b) {
  if (a == "0ULL")
    return b;
  if (b == "0ULL")
    return a;
  return "__apac_cost_add(" + a + "," + b + ")";
}
std::string mul(const std::string &a, const std::string &b) {
  return "__apac_cost_mul(" + a + "," + b + ")";
}
const VarDecl *variable(const Expr *e) {
  const auto *r = e ? dyn_cast<DeclRefExpr>(e->IgnoreParenImpCasts()) : nullptr;
  return r ? dyn_cast<VarDecl>(r->getDecl()) : nullptr;
}
void walk(const Stmt *s, const std::function<void(const Stmt *)> &visit) {
  if (!s)
    return;
  visit(s);
  for (const auto *child : s->children())
    walk(child, visit);
}
std::string args(const std::vector<std::string> &values) {
  std::string result;
  for (const auto &v : values) {
    if (!result.empty())
      result += ",";
    result += v;
  }
  return result;
}
} // namespace

struct StaticCostModel::Impl {
  struct Function {
    const FunctionDecl *decl, *body;
    std::vector<unsigned> features;
    int size = -1, component = -1;
    bool recursive = false;
    std::string expression;
    std::vector<unsigned> branchingSteps;
  };
  ASTContext &ctx;
  std::vector<Function> functions;
  std::map<const FunctionDecl *, unsigned> ids;
  std::vector<std::vector<unsigned>> components;
  unsigned expressionBudget = 0;

  explicit Impl(ASTContext &context) : ctx(context) {
    std::vector<const FunctionDecl *> declarations;
    for (auto *d : ctx.getTranslationUnitDecl()->decls())
      if (auto *f = dyn_cast<FunctionDecl>(d))
        if (!isa<CXXMethodDecl>(f) && f->isThisDeclarationADefinition() &&
            f->hasBody() &&
            ctx.getSourceManager().isWrittenInMainFile(f->getLocation()))
          declarations.push_back(f);
    // Prefer the untouched sequential clone: earlier passes may have lowered
    // the original body into temporary variables and gotos.
    std::set<const FunctionDecl *> clones;
    std::map<const FunctionDecl *, const FunctionDecl *> bodies;
    for (auto *f : declarations) {
      std::vector<const FunctionDecl *> candidates;
      for (auto *g : declarations)
        if (g->getNameAsString() == f->getNameAsString() + "_apacSeq" &&
            g->getNumParams() == f->getNumParams())
          candidates.push_back(g);
      const FunctionDecl *clone = nullptr;
      for (auto *g : candidates)
        if (ctx.hasSameType(f->getType(), g->getType()))
          clone = g;
      if (!clone && candidates.size() == 1)
        clone = candidates.front();
      if (clone) {
        bodies[f] = clone;
        clones.insert(clone);
      }
    }
    for (auto *f : declarations) {
      if (clones.count(f))
        continue;
      Function model{f, bodies.count(f) ? bodies[f] : f, {}};
      for (unsigned i = 0; i < f->getNumParams(); ++i) {
        const auto *p = f->getParamDecl(i);
        auto type = p->getType();
        if (!p->getName().empty() && type->isIntegerType() &&
            !type->isBooleanType() && !type.isVolatileQualified() &&
            ctx.getTypeSize(type) <= 64) {
          model.features.push_back(i);
          if (model.size < 0)
            model.size = int(i);
          auto name = p->getNameAsString();
          if (name == "n" || name == "size" || name == "count" ||
              name == "bodies")
            model.size = int(i);
        }
      }
      ids[f->getCanonicalDecl()] = functions.size();
      ids[model.body->getCanonicalDecl()] = functions.size();
      functions.push_back(std::move(model));
    }
    std::vector<std::set<unsigned>> edges(functions.size());
    for (unsigned i = 0; i < functions.size(); ++i)
      walk(functions[i].body->getBody(), [&](const Stmt *s) {
        if (auto *c = dyn_cast<CallExpr>(s)) {
          int next = id(c->getDirectCallee());
          if (next >= 0)
            edges[i].insert(unsigned(next));
        }
      });
    // Tarjan's SCC decomposition removes both direct and mutual recursion from
    // the graph of generated estimators, independently of declaration order.
    std::vector<int> number(functions.size(), -1), low(functions.size());
    std::vector<unsigned> stack;
    std::vector<bool> active(functions.size());
    int counter = 0;
    std::function<void(unsigned)> visit = [&](unsigned v) {
      number[v] = low[v] = counter++;
      stack.push_back(v);
      active[v] = true;
      for (unsigned w : edges[v]) {
        if (number[w] < 0) {
          visit(w);
          low[v] = std::min(low[v], low[w]);
        } else if (active[w])
          low[v] = std::min(low[v], number[w]);
      }
      if (low[v] != number[v])
        return;
      components.emplace_back();
      while (true) {
        unsigned w = stack.back();
        stack.pop_back();
        active[w] = false;
        functions[w].component = int(components.size() - 1);
        components.back().push_back(w);
        if (w == v)
          break;
      }
    };
    for (unsigned i = 0; i < functions.size(); ++i)
      if (number[i] < 0)
        visit(i);
    for (unsigned i = 0; i < functions.size(); ++i) {
      auto &f = functions[i];
      f.recursive = components[f.component].size() > 1 || edges[i].count(i);
      // Recognize branching, constant-decrement self recursion by structure,
      // not function name. Its tree can be estimated with bounded integer DP.
      if (f.recursive && components[f.component].size() == 1 && f.size >= 0) {
        bool valid = true;
        auto *sizeParam = f.body->getParamDecl(unsigned(f.size));
        walk(f.body->getBody(), [&](const Stmt *node) {
          if (auto *b = dyn_cast<BinaryOperator>(node))
            if (b->isAssignmentOp() && variable(b->getLHS()) == sizeParam)
              valid = false;
          if (auto *u = dyn_cast<UnaryOperator>(node))
            if (u->isIncrementDecrementOp() &&
                variable(u->getSubExpr()) == sizeParam)
              valid = false;
          if (isa<ForStmt>(node) || isa<WhileStmt>(node) || isa<DoStmt>(node))
            walk(node, [&](const Stmt *inside) {
              if (auto *c = dyn_cast<CallExpr>(inside))
                if (id(c->getDirectCallee()) == int(i))
                  valid = false;
            });
          auto *call = dyn_cast<CallExpr>(node);
          if (!call || id(call->getDirectCallee()) != int(i))
            return;
          auto *arg =
              unsigned(f.size) < call->getNumArgs()
                  ? call->getArg(unsigned(f.size))->IgnoreParenImpCasts()
                  : nullptr;
          auto *decrement = dyn_cast_or_null<BinaryOperator>(arg);
          auto *step = decrement
                           ? dyn_cast<IntegerLiteral>(
                                 decrement->getRHS()->IgnoreParenImpCasts())
                           : nullptr;
          if (!decrement || decrement->getOpcode() != BO_Sub ||
              variable(decrement->getLHS()) != sizeParam || !step ||
              step->getValue().getActiveBits() > 6 ||
              step->getValue().isZero()) {
            valid = false;
            return;
          }
          f.branchingSteps.push_back(unsigned(step->getValue().getZExtValue()));
        });
        if (!valid || f.branchingSteps.size() < 2 ||
            f.branchingSteps.size() > 16)
          f.branchingSteps.clear();
      }
      Bindings values;
      for (unsigned p : f.features)
        values[f.body->getParamDecl(p)] = argument(p);
      expressionBudget = 2048;
      f.expression = sequence({f.body->getBody()}, values, i);
    }
  }
  int id(const FunctionDecl *f) const {
    auto it = f ? ids.find(f->getCanonicalDecl()) : ids.end();
    return it == ids.end() ? -1 : int(it->second);
  }
  static std::string argument(unsigned p) {
    return "__apac_arg_" + std::to_string(p);
  }
  static std::string name(unsigned i) {
    return "__apac_cost_" + std::to_string(i);
  }
  static std::string local(unsigned i) { return name(i) + "_local"; }
  std::string feature(const Function &f, const Bindings &values) const {
    if (f.size < 0)
      return unknown;
    auto it = values.find(f.body->getParamDecl(unsigned(f.size)));
    return it == values.end() ? unknown : it->second;
  }
  // Only scalar expressions are reproduced. Never inspect buffers or call the
  // program to discover a value. Every arithmetic helper is bounded/saturating.
  std::string value(const Expr *e, const Bindings &values) const {
    if (!e)
      return unknown;
    e = e->IgnoreParenImpCasts();
    if (auto *literal = dyn_cast<IntegerLiteral>(e))
      return literal->getValue().getActiveBits() <= 64
                 ? std::to_string(literal->getValue().getZExtValue()) + "ULL"
                 : unknown;
    if (auto *ref = dyn_cast<DeclRefExpr>(e)) {
      auto *v = dyn_cast<VarDecl>(ref->getDecl());
      auto it = values.find(v);
      return it == values.end() ? unknown : it->second;
    }
    if (auto *u = dyn_cast<UnaryOperator>(e)) {
      if (u->getOpcode() == UO_Plus)
        return value(u->getSubExpr(), values);
      if (u->getOpcode() == UO_Minus)
        return "0ULL";
      if (u->getOpcode() == UO_LNot)
        return "__apac_cost_cmp(" + value(u->getSubExpr(), values) + ",0ULL,0)";
    }
    if (auto *b = dyn_cast<BinaryOperator>(e)) {
      auto a = value(b->getLHS(), values), c = value(b->getRHS(), values);
      if (a.size() + c.size() > 4096)
        return unknown;
      switch (b->getOpcode()) {
      case BO_Add:
        return add(a, c);
      case BO_Sub:
        return "__apac_cost_sub(" + a + "," + c + ")";
      case BO_Mul:
        return mul(a, c);
      case BO_Div:
        return "__apac_cost_div(" + a + "," + c + ")";
      case BO_Rem:
        return "__apac_cost_rem(" + a + "," + c + ")";
      case BO_EQ:
      case BO_NE:
      case BO_LT:
      case BO_LE:
      case BO_GT:
      case BO_GE: {
        int op = b->getOpcode() == BO_EQ   ? 0
                 : b->getOpcode() == BO_NE ? 1
                 : b->getOpcode() == BO_LT ? 2
                 : b->getOpcode() == BO_LE ? 3
                 : b->getOpcode() == BO_GT ? 4
                                           : 5;
        return "__apac_cost_cmp(" + a + "," + c + "," + std::to_string(op) +
               ")";
      }
      default:
        return unknown;
      }
    }
    if (auto *call = dyn_cast<CallExpr>(e)) {
      auto *f = call->getDirectCallee();
      if (f && call->getNumArgs() == 2 &&
          f->getQualifiedNameAsString().rfind("std::", 0) == 0 &&
          (f->getNameAsString() == "min" || f->getNameAsString() == "max"))
        return "__apac_cost_" + f->getNameAsString() + "(" +
               value(call->getArg(0), values) + "," +
               value(call->getArg(1), values) + ")";
    }
    return unknown;
  }
  std::string expression(const Stmt *s, Bindings &values, unsigned owner) {
    if (!s)
      return "0ULL";
    std::string result = "1ULL";
    if (auto *conditional = dyn_cast<ConditionalOperator>(s)) {
      auto condition = value(conditional->getCond(), values);
      auto conditionCost = expression(conditional->getCond(), values, owner);
      auto yes = values, no = values;
      auto a = expression(conditional->getTrueExpr(), yes, owner);
      auto b = expression(conditional->getFalseExpr(), no, owner);
      for (auto &binding : values)
        if (yes[binding.first] != no[binding.first])
          binding.second = unknown;
        else
          binding.second = yes[binding.first];
      return add(conditionCost, "__apac_cost_choose(" + condition +
                                    ",[&](){return " + a + ";},[&](){return " +
                                    b + ";})");
    }
    if (auto *logical = dyn_cast<BinaryOperator>(s)) {
      if (logical->getOpcode() == BO_LAnd || logical->getOpcode() == BO_LOr) {
        auto condition = value(logical->getLHS(), values);
        auto lhsCost = expression(logical->getLHS(), values, owner);
        auto rhsValues = values;
        auto rhsCost = expression(logical->getRHS(), rhsValues, owner);
        for (auto &binding : values)
          if (binding.second != rhsValues[binding.first])
            binding.second = unknown;
        auto yes = logical->getOpcode() == BO_LAnd ? rhsCost : "0ULL";
        auto no = logical->getOpcode() == BO_LOr ? rhsCost : "0ULL";
        return add(lhsCost, "__apac_cost_choose(" + condition +
                                ",[&](){return " + yes + ";},[&](){return " +
                                no + ";})");
      }
    }
    if (auto *call = dyn_cast<CallExpr>(s)) {
      int next = id(call->getDirectCallee());
      if (next >= 0) {
        if (functions[next].component != functions[owner].component) {
          std::vector<std::string> actual;
          for (unsigned p : functions[next].features)
            actual.push_back(p < call->getNumArgs()
                                 ? value(call->getArg(p), values)
                                 : unknown);
          actual.push_back("__apac_budget");
          result =
              add(result, name(unsigned(next)) + "_impl(" + args(actual) + ")");
        } else {
          // Mark only recursive edges reached along the selected cost path.
          // Terminal paths retain their local cost, without an n multiplier.
          result = "(__apac_recursive=true,1ULL)";
        } // Intra-SCC edges never call another estimator.
      } else {
        auto *callee = call->getDirectCallee();
        auto n = callee ? callee->getQualifiedNameAsString() : "";
        auto shortName = callee ? callee->getNameAsString() : "";
        bool arithmetic = true;
        for (auto *arg : call->arguments())
          arithmetic &= arg->getType()->isArithmeticType();
        if ((shortName == "min" || shortName == "max") &&
            n.rfind("std::", 0) == 0 && arithmetic) {
        } else if (shortName == "sqrt" || shortName == "sqrtf" ||
                   shortName == "fabs")
          result = "16ULL";
        else if (n.rfind("std::swap", 0) == 0)
          result = "3ULL";
        else
          result = unknown;
      }
      // Arguments themselves may contain work (including unknown calls).
      for (auto *arg : call->arguments())
        result = add(result, expression(arg, values, owner));
      return result;
    }
    for (auto *child : s->children())
      result = add(result, expression(child, values, owner));
    if (auto *b = dyn_cast<BinaryOperator>(s)) {
      if (b->isAssignmentOp()) {
        auto *target = variable(b->getLHS());
        if (target)
          values[target] = b->getOpcode() == BO_Assign
                               ? value(b->getRHS(), values)
                               : unknown;
      }
    }
    if (auto *u = dyn_cast<UnaryOperator>(s))
      if (u->isIncrementDecrementOp())
        if (auto *target = variable(u->getSubExpr()))
          values[target] = unknown;
    if (isa<CXXConstructExpr>(s) || isa<CXXNewExpr>(s) ||
        isa<CXXDeleteExpr>(s) || isa<AsmStmt>(s))
      return unknown;
    return result;
  }
  std::string sequence(std::vector<const Stmt *> statements, Bindings values,
                       unsigned owner) {
    if (statements.empty())
      return "0ULL";
    if (!expressionBudget)
      return unknown;
    --expressionBudget;
    const Stmt *s = statements.front();
    statements.erase(statements.begin());
    if (!s)
      return sequence(statements, values, owner);
    if (auto *block = dyn_cast<CompoundStmt>(s)) {
      statements.insert(statements.begin(), block->body_begin(),
                        block->body_end());
      return sequence(statements, values, owner);
    }
    if (auto *r = dyn_cast<ReturnStmt>(s))
      return add("1ULL", expression(r->getRetValue(), values, owner));
    if (isa<ContinueStmt>(s) || isa<BreakStmt>(s))
      return "1ULL";
    if (auto *branch = dyn_cast<IfStmt>(s)) {
      auto condition = value(branch->getCond(), values);
      Bindings conditionValues = values;
      auto conditionCost =
          expression(branch->getCond(), conditionValues, owner);
      auto yes = statements, no = statements;
      if (branch->getThen())
        yes.insert(yes.begin(), branch->getThen());
      if (branch->getElse())
        no.insert(no.begin(), branch->getElse());
      auto a = sequence(yes, conditionValues, owner),
           b = sequence(no, conditionValues, owner);
      return add(conditionCost, "__apac_cost_choose(" + condition +
                                    ",[&](){return " + a + ";},[&](){return " +
                                    b + ";})");
    }
    const Stmt *body = nullptr;
    const Expr *condition = nullptr, *increment = nullptr;
    std::string init = "0ULL";
    if (auto *loop = dyn_cast<ForStmt>(s)) {
      body = loop->getBody();
      condition = loop->getCond();
      increment = loop->getInc();
      if (loop->getInit())
        init = sequence({loop->getInit()}, values, owner);
      // Bind declaration/assignment initializers without executing them.
      if (auto *decl = dyn_cast_or_null<DeclStmt>(loop->getInit())) {
        for (auto *d : decl->decls())
          if (auto *v = dyn_cast<VarDecl>(d))
            values[v] = value(v->getInit(), values);
      } else if (auto *e = dyn_cast_or_null<Expr>(loop->getInit()))
        expression(e, values, owner);
    } else if (auto *loop = dyn_cast<WhileStmt>(s)) {
      body = loop->getBody();
      condition = loop->getCond();
    }
    if (body) {
      std::string trips = feature(functions[owner], values);
      if (auto *comparison = dyn_cast_or_null<BinaryOperator>(
              condition ? condition->IgnoreParenImpCasts() : nullptr)) {
        if (comparison->getOpcode() == BO_LT ||
            comparison->getOpcode() == BO_LE) {
          auto start = value(comparison->getLHS(), values),
               end = value(comparison->getRHS(), values);
          if (comparison->getOpcode() == BO_LE)
            end = add(end, "1ULL");
          trips = "__apac_cost_sub(" + end + "," + start + ")";
          if (auto *step = dyn_cast_or_null<CompoundAssignOperator>(increment))
            if (step->getOpcode() == BO_AddAssign)
              trips = "__apac_cost_ceil_div(" + trips + "," +
                      value(step->getRHS(), values) + ")";
        }
      }
      // A partially known loop uses the function's selected size as its coarse
      // multiplicity. This is an estimate, not a proven iteration bound.
      trips = "__apac_cost_trip(" + trips + "," +
              feature(functions[owner], values) + ")";
      Bindings loopValues = values;
      // The body runs at many induction values: do not substitute its initial
      // value into predicates/callee sizes and pretend every iteration is
      // first.
      if (auto *comparison = dyn_cast_or_null<BinaryOperator>(
              condition ? condition->IgnoreParenImpCasts() : nullptr))
        if (auto *index = variable(comparison->getLHS()))
          loopValues[index] = unknown;
      auto work = sequence({body}, loopValues, owner);
      walk(s, [&](const Stmt *node) {
        if (auto *b = dyn_cast<BinaryOperator>(node))
          if (b->isAssignmentOp())
            if (auto *v = variable(b->getLHS()))
              values[v] = unknown;
        if (auto *u = dyn_cast<UnaryOperator>(node))
          if (u->isIncrementDecrementOp())
            if (auto *v = variable(u->getSubExpr()))
              values[v] = unknown;
      });
      // A zero-trip loop must not mark a recursive edge in its body as reached.
      auto loopWork = "__apac_cost_choose(__apac_cost_cmp(" + trips +
                      ",0ULL,4),[&](){return " + mul(trips, add("2ULL", work)) +
                      ";},[&](){return 0ULL;})";
      return add(add(init, loopWork), sequence(statements, values, owner));
    }
    std::string work;
    if (auto *decl = dyn_cast<DeclStmt>(s)) {
      work = "0ULL";
      for (auto *d : decl->decls()) {
        auto *v = dyn_cast<VarDecl>(d);
        if (!v) {
          work = unknown;
          continue;
        }
        work = add(work, expression(v->getInit(), values, owner));
        values[v] = value(v->getInit(), values);
      }
    } else if (isa<Expr>(s) || isa<NullStmt>(s))
      work = expression(s, values, owner);
    else
      work = unknown;
    return add(work, sequence(statements, values, owner));
  }
  std::string signature(unsigned i, bool budget) const {
    std::vector<std::string> result;
    for (unsigned p : functions[i].features)
      result.push_back("__apac_cost_type " + argument(p));
    if (budget)
      result.push_back("unsigned &__apac_budget");
    return args(result);
  }
};

StaticCostModel::StaticCostModel(ASTContext &ctx) : impl_(new Impl(ctx)) {}
StaticCostModel::~StaticCostModel() = default;

std::string StaticCostModel::definitions() const {
  std::string result = R"cost(
using __apac_cost_type = unsigned long long;
static constexpr __apac_cost_type __apac_cost_unknown = ~0ULL;
static constexpr __apac_cost_type __apac_cost_limit = __apac_cost_unknown - 1;
template<class T> static inline __apac_cost_type __apac_cost_size(T value) {
  return value > 0 ? static_cast<__apac_cost_type>(value) : 0ULL;
}
static inline __apac_cost_type __apac_cost_add(__apac_cost_type a, __apac_cost_type b) {
  if(a==__apac_cost_unknown || b==__apac_cost_unknown) return __apac_cost_unknown;
  return a > __apac_cost_limit-b ? __apac_cost_limit : a+b;
}
static inline __apac_cost_type __apac_cost_mul(__apac_cost_type a, __apac_cost_type b) {
  if(!a || !b) return 0;
  if(a==__apac_cost_unknown || b==__apac_cost_unknown) return __apac_cost_unknown;
  return a > __apac_cost_limit/b ? __apac_cost_limit : a*b;
}
template<unsigned N> static inline __apac_cost_type __apac_cost_tree(__apac_cost_type size, const unsigned (&steps)[N]) {
  if(size==__apac_cost_unknown) return __apac_cost_unknown;
  if(size>=128) return __apac_cost_limit;
  unsigned base=0;
  for(unsigned step: steps) if(step>base) base=step;
  __apac_cost_type nodes[128] = {};
  for(unsigned i=0;i<=size;++i) {
    nodes[i]=1;
    if(i>=base) for(unsigned step: steps) nodes[i]=__apac_cost_add(nodes[i],nodes[i-step]);
  }
  return nodes[size];
}
static inline __apac_cost_type __apac_cost_trip(__apac_cost_type value, __apac_cost_type fallback) {
  return value==__apac_cost_unknown ? fallback : value;
}
static inline __apac_cost_type __apac_cost_sub(__apac_cost_type a, __apac_cost_type b) {
  if(a==__apac_cost_unknown || b==__apac_cost_unknown) return __apac_cost_unknown;
  return a>b ? a-b : 0;
}
static inline __apac_cost_type __apac_cost_div(__apac_cost_type a, __apac_cost_type b) {
  if(a==__apac_cost_unknown || b==__apac_cost_unknown || !b) return __apac_cost_unknown;
  return a/b;
}
static inline __apac_cost_type __apac_cost_ceil_div(__apac_cost_type a, __apac_cost_type b) {
  if(a==__apac_cost_unknown || b==__apac_cost_unknown || !b) return __apac_cost_unknown;
  return a/b+(a%b!=0);
}
static inline __apac_cost_type __apac_cost_rem(__apac_cost_type a, __apac_cost_type b) {
  if(a==__apac_cost_unknown || b==__apac_cost_unknown || !b) return __apac_cost_unknown;
  return a%b;
}
static inline __apac_cost_type __apac_cost_max(__apac_cost_type a, __apac_cost_type b) {
  return a>b ? a:b;
}
static inline __apac_cost_type __apac_cost_min(__apac_cost_type a, __apac_cost_type b) {
  if(a==__apac_cost_unknown || b==__apac_cost_unknown) return __apac_cost_unknown;
  return a<b ? a:b;
}
static inline __apac_cost_type __apac_cost_cmp(__apac_cost_type a, __apac_cost_type b, int op) {
  if(a==__apac_cost_unknown || b==__apac_cost_unknown) return __apac_cost_unknown;
  return op==0 ? a==b : op==1 ? a!=b : op==2 ? a<b : op==3 ? a<=b : op==4 ? a>b : a>=b;
}
template<class A,class B> static inline __apac_cost_type __apac_cost_choose(__apac_cost_type c,A a,B b) {
  if(c==__apac_cost_unknown) { auto x=a(); auto y=b(); return __apac_cost_max(x,y); }
  return c ? a() : b();
}
static const __apac_cost_type __apac_cost_threshold = [] {
  const char *text=std::getenv("APAC_COST_THRESHOLD");
  if(!text) return 0ULL;
  bool valid=*text!=0;
  for(const char *p=text;*p;++p) valid &= *p>='0' && *p<='9';
  errno=0; char *end=nullptr;
  auto value=std::strtoull(text,&end,10);
  if(!valid || errno==ERANGE || *end || value>__apac_cost_limit) {
    std::fprintf(stderr,"APAC_COST_THRESHOLD must be a nonnegative integer below ULLONG_MAX\n");
    std::exit(2);
  }
  return value;
}();
)cost";
  auto &m = *impl_;
  for (unsigned i = 0; i < m.functions.size(); ++i) {
    result += "static inline __apac_cost_type " + Impl::name(i) + "_impl(" +
              m.signature(i, true) + ");\n";
    result += "static inline __apac_cost_type " + Impl::local(i) + "(" +
              m.signature(i, true) + ",bool &__apac_recursive);\n";
  }
  for (unsigned i = 0; i < m.functions.size(); ++i) {
    const auto &f = m.functions[i];
    result +=
        "// APAC cost model: " + f.decl->getNameAsString() + " => " +
        Impl::name(i) + "; SCC " + std::to_string(f.component) +
        "; recursive scale " +
        (f.recursive
             ? (f.size >= 0 ? f.decl->getParamDecl(f.size)->getNameAsString()
                            : "unknown")
             : "none") +
        "\n";
    result += "static inline __apac_cost_type " + Impl::local(i) + "(" +
              m.signature(i, true) +
              ",bool &__apac_recursive) {\n"
              "if(!__apac_budget) return __apac_cost_unknown; "
              "--__apac_budget;\nreturn " +
              f.expression + ";\n}\n";
    std::vector<std::string> ownArgs;
    for (unsigned p : f.features)
      ownArgs.push_back(Impl::argument(p));
    ownArgs.push_back("__apac_budget");
    ownArgs.push_back("__apac_recursive");
    std::string code =
        "bool __apac_recursive=false;\nauto __apac_local=" + Impl::local(i) +
        "(" + args(ownArgs) + ");\n";
    if (!f.recursive) {
      code += "return __apac_local;\n";
    } else {
      code += "if(!__apac_recursive) return __apac_local;\n";
      if (f.size < 0) {
        code += "return __apac_cost_unknown;\n";
      } else {
        std::string work = "__apac_local";
        for (unsigned member : m.components[f.component]) {
          if (member == i)
            continue;
          const auto &g = m.functions[member];
          std::vector<std::string> actual;
          for (unsigned p : g.features) {
            std::string mapped = unknown;
            if (int(p) == g.size)
              mapped = Impl::argument(f.size);
            else
              for (unsigned q : f.features)
                if (g.decl->getParamDecl(p)->getName() ==
                    f.decl->getParamDecl(q)->getName())
                  mapped = Impl::argument(q);
            actual.push_back(mapped);
          }
          actual.push_back("__apac_budget");
          actual.push_back("__apac_recursive");
          work = add(work, Impl::local(member) + "(" + args(actual) + ")");
        }
        std::string scale =
            "__apac_cost_max(1ULL," + Impl::argument(f.size) + ")";
        if (!f.branchingSteps.empty()) {
          code += "static constexpr unsigned __apac_steps[]={";
          for (unsigned step : f.branchingSteps)
            code += std::to_string(step) + ",";
          code += "};\n";
          scale =
              "__apac_cost_tree(" + Impl::argument(f.size) + ",__apac_steps)";
        }
        code += "return " + mul(scale, work) + ";\n";
      }
    }
    result += "static inline __apac_cost_type " + Impl::name(i) + "_impl(" +
              m.signature(i, true) + ") {\n" + code + "}\n";
    std::vector<std::string> actual;
    for (unsigned p : f.features)
      actual.push_back(Impl::argument(p));
    actual.push_back("__apac_budget");
    result += "static inline __apac_cost_type " + Impl::name(i) + "(" +
              m.signature(i, false) +
              ") {\nunsigned __apac_budget=64;\nreturn " + Impl::name(i) +
              "_impl(" + args(actual) + ");\n}\n";
  }
  return result;
}
std::string StaticCostModel::entryCost(const FunctionDecl *function) const {
  int i = impl_->id(function);
  if (i < 0)
    return unknown;
  std::vector<std::string> actual;
  for (unsigned p : impl_->functions[i].features)
    actual.push_back("__apac_cost_size(" +
                     function->getParamDecl(p)->getNameAsString() + ")");
  return Impl::name(i) + "(" + args(actual) + ")";
}
std::string StaticCostModel::callCost(const CallExpr *call) const {
  int i = impl_->id(call->getDirectCallee());
  if (i < 0)
    return unknown;
  std::vector<std::string> actual;
  for (unsigned p : impl_->functions[i].features) {
    if (p >= call->getNumArgs())
      return unknown;
    actual.push_back(
        "__apac_cost_size(" +
        getExprAsString(call->getArg(p), impl_->ctx.getLangOpts(), true) + ")");
  }
  return Impl::name(i) + "(" + args(actual) + ")";
}

std::string StaticCostModel::sequentialCall(const CallExpr *call) const {
  int i = impl_->id(call->getDirectCallee());
  if (i < 0 || impl_->functions[i].decl == impl_->functions[i].body)
    return getStmtAsStringFull(call, impl_->ctx.getLangOpts());
  std::vector<std::string> actual;
  for (auto *arg : call->arguments())
    actual.push_back(getExprAsString(arg, impl_->ctx.getLangOpts(), true));
  return impl_->functions[i].body->getQualifiedNameAsString() + "(" +
         args(actual) + ");\n";
}
