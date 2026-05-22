#pragma once

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <clang/AST/ASTConsumer.h>

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace constify {

/// Global verbose flag.
/// Keep it simple for now. Later, replace by llvm::cl::opt<bool>.
inline bool Verbose = false;


/// A stable node id inside one translation unit.
struct NodeId {
    std::size_t value = static_cast<std::size_t>(-1);

    bool isValid() const {
        return value != static_cast<std::size_t>(-1);
    }

    friend bool operator==(NodeId a, NodeId b) {
        return a.value == b.value;
    }

    friend bool operator!=(NodeId a, NodeId b) {
        return !(a == b);
    }
};

/// What kind of source entity a node represents.
///
/// Important:
/// - A variable declaration and a reference declaration are different.
/// - A function declaration and a method declaration are different.
/// - A parameter is modeled as a VarDecl, but tagged separately because its
///   constness affects function signatures.
enum class NodeKind {
    Variable,
    Reference,
    Pointer,
    Parameter,
    Field,
    Function,
    Method,
    Unknown,
    Constructor
};

/// Current known const state.
///
/// AlreadyConst:
///   The source already says const. We should preserve that.
///   In practice this node should not be rewritten, but it may still constrain
///   other nodes.
///
/// Candidate:
///   We would like to make this const unless evidence says otherwise.
///
/// CannotBeConst:
///   Some write, non-const call, escaping mutable reference, etc. forbids const.
///
/// ForcedConst:
///   Optional stronger state if later you want to distinguish "already const"
///   from "must be const due to external API". Not used yet.
enum class ConstState {
    AlreadyConst,
    Candidate,
    CannotBeConst,
    ForcedConst
};

/// Directional dependency between two nodes.
///
/// Example:
///
///     int x;
///     int& y = x;
///     y++;
///
/// We add:
///
///     y --ReferenceAlias--> x
///
/// Meaning:
///
///     if y cannot be const, then x cannot be const.
///
/// Another example:
///
///     Foo f;
///     f.op();
///
/// If op is non-const and mutates `this`, then:
///
///     call proves f cannot be const
///
/// But if op can itself become const, then:
///
///     method op constrains object f less strictly
///
/// This distinction is where your tool will either become useful or produce
/// nonsense.
enum class EdgeKind {
    /// Reference aliases the target object.
    /// CannotBeConst propagates from reference to referred object.
    ReferenceAlias,

    /// Pointer may mutate the pointee.
    /// This is conservative and should be refined later.
    PointerMayAlias,

    /// A non-const reference parameter aliases an argument.
    /// CannotBeConst propagates from parameter to argument.
    RefParamAlias,

    /// A pointer parameter may alias an argument.
    PointerParamAlias,

    /// A method's constness affects whether the receiver object can be const.
    MethodReceiver,

    /// Generic assignment/copy dependency.
    /// Usually this should NOT propagate CannotBeConst.
    ValueFlow,

    /// Informational: object calls method.
    /// Does not directly propagate CannotBeConst.
    MethodCallOnObject,

    /// Unknown conservative dependency.
    Unknown
};

/// Why a node cannot be const.
enum class RejectReasonKind {
    IncrementOrDecrement,
    AssignedTo,
    PassedToNonConstRef,
    PassedToMutablePointer,
    NonConstMethodCall,
    AddressEscapes,
    UnknownWrite
};


inline const char* toString(NodeKind k) {
    switch (k) {
    case NodeKind::Variable:  return "Variable";
    case NodeKind::Reference: return "Reference";
    case NodeKind::Pointer:   return "Pointer";
    case NodeKind::Parameter: return "Parameter";
    case NodeKind::Function:  return "Function";
    case NodeKind::Method:    return "Method";
    case NodeKind::Field:     return "Field";
    case NodeKind::Unknown:   return "Unknown";
    case NodeKind::Constructor: return "Constructor";
    }
    return "Unknown";
}

inline const char* toString(ConstState s) {
    switch (s) {
    case ConstState::AlreadyConst:  return "AlreadyConst";
    case ConstState::Candidate:     return "Candidate";
    case ConstState::CannotBeConst: return "CannotBeConst";
    case ConstState::ForcedConst:   return "ForcedConst";
    }
    return "Unknown";
}

inline const char* toString(EdgeKind k) {
    switch (k) {
    case EdgeKind::ReferenceAlias:    return "ReferenceAlias";
    case EdgeKind::PointerMayAlias:   return "PointerMayAlias";
    case EdgeKind::RefParamAlias:     return "RefParamAlias";
    case EdgeKind::PointerParamAlias: return "PointerParamAlias";
    case EdgeKind::MethodReceiver:    return "MethodReceiver";
    case EdgeKind::ValueFlow:         return "ValueFlow";
    case EdgeKind::Unknown:           return "Unknown";
    case EdgeKind::MethodCallOnObject: return "MethodCallOnObject";
    }
    return "Unknown";
}

inline const char* toString(RejectReasonKind k) {
    switch (k) {
    case RejectReasonKind::IncrementOrDecrement: return "IncrementOrDecrement";
    case RejectReasonKind::AssignedTo:           return "AssignedTo";
    case RejectReasonKind::PassedToNonConstRef:  return "PassedToNonConstRef";
    case RejectReasonKind::PassedToMutablePointer:return "PassedToMutablePointer";
    case RejectReasonKind::NonConstMethodCall:   return "NonConstMethodCall";
    case RejectReasonKind::AddressEscapes:       return "AddressEscapes";
    case RejectReasonKind::UnknownWrite:         return "UnknownWrite";
    }
    return "Unknown";
}

/// Source insertion/rewrite information.
///
/// For now this stores where the declaration begins.
/// Later you will want something more precise:
///
///     int *p;          -> const int *p ?
///     int * const p;   -> pointer itself const
///     Foo const& r;    -> already const ref target
///     auto x = ...;    -> adding const before auto
///
/// Do not underestimate this: inserting const in C++ declarators is ugly.
struct SourceAnchor {
    clang::SourceLocation spellingLoc;
    clang::SourceRange sourceRange;
};

struct RejectReason {
    RejectReasonKind kind = RejectReasonKind::UnknownWrite;
    clang::SourceLocation loc;
    const clang::Stmt* stmt = nullptr;
};

struct Node {
    NodeId id;
    NodeKind kind = NodeKind::Unknown;
    ConstState state = ConstState::Candidate;
    bool shouldTrack = false;

    const clang::Decl* decl = nullptr;

    std::string name;
    std::string qualifiedName;
    std::string typeAsString;

    SourceAnchor anchor;

    std::vector<RejectReason> rejectReasons;
};

struct Edge {
    NodeId from;
    NodeId to;
    EdgeKind kind = EdgeKind::Unknown;

    /// AST object explaining why this edge exists.
    /// Could be DeclRefExpr, CallExpr, CXXMemberCallExpr, etc.
    const void* cause = nullptr;

    clang::SourceLocation loc;
};

/// Main graph container.
///
/// This is intentionally dumb:
/// - create nodes
/// - create edges
/// - mark nodes as impossible
/// - propagate later
///
/// Keep analysis and rewriting separate.
class ConstGraph {
public:
    explicit ConstGraph(clang::ASTContext& ctx)
        : Ctx(ctx), SM(ctx.getSourceManager()) {}


    bool nodeExistsForDecl(const clang::Decl* D) const {
        if(!D) return false;

        D = D->getCanonicalDecl();

        auto It = DeclToNode.find(D);

        return It != DeclToNode.end();
    }

    NodeId getOrCreateDeclNode(const clang::Decl* D, const bool shouldTrack, const bool isAlreadyConst) {
        if (!D) {
            return {};
        }

        D = D->getCanonicalDecl();

        auto It = DeclToNode.find(D);
        if (It != DeclToNode.end()) {
            assert(Nodes[It->second.value].decl == D);
            assert(Nodes[It->second.value].shouldTrack == shouldTrack);
            return It->second;
        }

        Node N;
        N.id = NodeId{Nodes.size()};
        N.decl = D;
        N.kind = classifyDecl(D);
        N.name = getShortName(D);
        N.qualifiedName = getQualifiedName(D);
        N.typeAsString = getTypeString(D);
        N.anchor = getSourceAnchor(D);
        N.shouldTrack = shouldTrack;

        N.state = (shouldTrack ? initialConstState(D) : ConstState::CannotBeConst);

        Nodes.push_back(std::move(N));
        DeclToNode[D] = Nodes.back().id;

        if (Verbose) {
            llvm::errs()
                << "[constify] create node #"
                << Nodes.back().id.value
                << " kind=" << toString(Nodes.back().kind)
                << " state=" << toString(Nodes.back().state)
                << " name=" << Nodes.back().qualifiedName
                << " type=" << Nodes.back().typeAsString
                << " shouldTrack=" << Nodes.back().shouldTrack
                << "\n";
        }

        return Nodes.back().id;
    }

    void addEdge(NodeId From,
                 NodeId To,
                 EdgeKind Kind,
                 const void* Cause,
                 clang::SourceLocation Loc) {
        if (!From.isValid() || !To.isValid() || From == To) {
            return;
        }

        Edge E;
        E.from = From;
        E.to = To;
        E.kind = Kind;
        E.cause = Cause;
        E.loc = Loc;
        Edges.push_back(E);

        if (Verbose) {
            llvm::errs()
                << "[constify] edge #"
                << From.value << " -> #" << To.value
                << " kind=" << toString(Kind);

            if (Cause) {
                llvm::errs() << " cause@" << Cause;
            }

            llvm::errs() << "\n";
        }
    }

    void reject(NodeId Id,
                RejectReasonKind Reason,
                clang::SourceLocation Loc,
                const clang::Stmt* Stmt = nullptr) {
        if (!Id.isValid()) {
            return;
        }

        Node& N = Nodes[Id.value];

        if (N.state == ConstState::AlreadyConst) {
            // Original code compiled. If an already-const node appears written,
            // either we are looking at something else, or the AST interpretation
            // is wrong. Do not downgrade it silently.
            if (Verbose) {
                llvm::errs()
                    << "[constify] warning: rejecting already-const node #"
                    << Id.value << " name=" << N.qualifiedName
                    << " reason=" << toString(Reason)
                    << "\n";
            }
            return;
        }

        N.state = ConstState::CannotBeConst;
        N.rejectReasons.push_back(RejectReason{Reason, Loc, Stmt});

        if (Verbose) {
            llvm::errs()
                << "[constify] reject node #"
                << Id.value
                << " name=" << N.qualifiedName
                << " reason=" << toString(Reason)
                << "\n";
        }
    }

    /// Propagate CannotBeConst through edges that semantically imply aliasing.
    ///
    /// This is intentionally conservative and simple.
    ///
    /// Direction convention:
    ///
    ///     From cannot be const => To cannot be const
    ///
    /// Example:
    ///
    ///     int x;
    ///     int& y = x;
    ///     y++;
    ///
    /// Edge:
    ///
    ///     y -> x
    ///
    /// So once y is rejected, x is rejected too.
    void propagateCannotBeConst() {
        bool Changed = true;

        while (Changed) {
            Changed = false;

            for (const Edge& E : Edges) {
                if (!propagatesCannot(E.kind)) {
                    continue;
                }

                Node& From = Nodes[E.from.value];
                Node& To = Nodes[E.to.value];

                if (From.state == ConstState::CannotBeConst &&
                    To.state == ConstState::Candidate) {
                    To.state = ConstState::CannotBeConst;
                    To.rejectReasons.push_back(
                        RejectReason{RejectReasonKind::UnknownWrite,
                                     E.loc,
                                     nullptr});

                    Changed = true;

                    if (Verbose) {
                        llvm::errs()
                            << "[constify] propagate reject #"
                            << E.from.value << " (" << From.qualifiedName << ")"
                            << " -> #"
                            << E.to.value << " (" << To.qualifiedName << ")"
                            << " via " << toString(E.kind)
                            << "\n";
                    }
                }
            }
        }
    }

    template <class T>
    void dump(T& OS) const {
        OS << "\n[CONSTIFY NODES]\n";

        for (const Node& N : Nodes) {
            OS << "  #" << N.id.value
               << " " << toString(N.kind)
               << " " << toString(N.state)
               << " name=" << N.qualifiedName
               << " type=\"" << N.typeAsString << "\"";

            if (!N.rejectReasons.empty()) {
                OS << " rejects=[";
                for (std::size_t I = 0; I < N.rejectReasons.size(); ++I) {
                    if (I) {
                        OS << ", ";
                    }
                    OS << toString(N.rejectReasons[I].kind);
                }
                OS << "]";
            }

            OS << "\n";
        }

        OS << "\n[CONSTIFY EDGES]\n";

        for (const Edge& E : Edges) {
            OS << "  #"
               << E.from.value
               << " (" << Nodes[E.from.value].qualifiedName << ")"
               << " -> #"
               << E.to.value
               << " (" << Nodes[E.to.value].qualifiedName << ")"
               << " [" << toString(E.kind) << "]";

            if (E.cause) {
                OS << " cause@" << E.cause;
            }

            OS << "\n";
        }

        OS << "\n[ADJACENCY]\n";

        for (std::size_t I = 0; I < Nodes.size(); ++I) {
            OS << "  #" << I
               << " (" << Nodes[I].qualifiedName << ")"
               << " ->";

            bool HasEdge = false;

            for (const Edge& E : Edges) {
                if (E.from.value == I) {
                    OS << " #"
                       << E.to.value
                       << "(" << Nodes[E.to.value].qualifiedName
                       << ", " << toString(E.kind)
                       << ")";
                    HasEdge = true;
                }
            }

            if (!HasEdge) {
                OS << " <none>";
            }

            OS << "\n";
        }
    }

    const std::vector<Node>& nodes() const {
        return Nodes;
    }

    const std::vector<Edge>& edges() const {
        return Edges;
    }

private:
    clang::ASTContext& Ctx;
    clang::SourceManager& SM;

    std::vector<Node> Nodes;
    std::vector<Edge> Edges;

    llvm::DenseMap<const clang::Decl*, NodeId> DeclToNode;

private:
    static bool propagatesCannot(EdgeKind K) {
        switch (K) {
        case EdgeKind::ReferenceAlias:
        case EdgeKind::PointerMayAlias:
        case EdgeKind::RefParamAlias:
        case EdgeKind::PointerParamAlias:
        case EdgeKind::MethodReceiver:
        case EdgeKind::Unknown:
            return true;

        case EdgeKind::ValueFlow:
        case EdgeKind::MethodCallOnObject:
            return false;
        }

        return true;
    }

    static NodeKind classifyDecl(const clang::Decl* D) {
        if (llvm::isa<clang::CXXConstructorDecl>(D)) {
            return NodeKind::Constructor;
        }

        if (const auto* MD = llvm::dyn_cast<clang::CXXMethodDecl>(D)) {
            (void)MD;
            return NodeKind::Method;
        }

        if (const auto* FD = llvm::dyn_cast<clang::FunctionDecl>(D)) {
            (void)FD;
            return NodeKind::Function;
        }

        if (const auto* PVD = llvm::dyn_cast<clang::ParmVarDecl>(D)) {
            (void)PVD;
            return NodeKind::Parameter;
        }

        if (const auto* FD = llvm::dyn_cast<clang::FieldDecl>(D)) {
            (void)FD;
            return NodeKind::Field;
        }

        if (const auto* VD = llvm::dyn_cast<clang::VarDecl>(D)) {
            clang::QualType QT = VD->getType();

            if (QT->isReferenceType()) {
                return NodeKind::Reference;
            }

            if (QT->isPointerType()) {
                return NodeKind::Pointer;
            }

            return NodeKind::Variable;
        }

        return NodeKind::Unknown;
    }

    static std::string getShortName(const clang::Decl* D) {
        if (const auto* ND = llvm::dyn_cast<clang::NamedDecl>(D)) {
            return ND->getNameAsString();
        }
        return "<unnamed>";
    }

    static std::string getQualifiedName(const clang::Decl* D) {
        if (const auto* ND = llvm::dyn_cast<clang::NamedDecl>(D)) {
            return ND->getQualifiedNameAsString();
        }
        return "<unnamed>";
    }

    static std::string getTypeString(const clang::Decl* D) {
        if (const auto* VD = llvm::dyn_cast<clang::ValueDecl>(D)) {
            return VD->getType().getAsString();
        }

        if (const auto* FD = llvm::dyn_cast<clang::FunctionDecl>(D)) {
            return FD->getType().getAsString();
        }

        return "<no-type>";
    }


    SourceAnchor getSourceAnchor(const clang::Decl* D) const {
        SourceAnchor A;
        A.spellingLoc = SM.getSpellingLoc(D->getBeginLoc());
        A.sourceRange = D->getSourceRange();
        return A;
    }

    static ConstState initialConstState(const clang::Decl* D) {
        if (const auto* VD = llvm::dyn_cast<clang::ValueDecl>(D)) {
            clang::QualType QT = VD->getType();

            // Reference declarations themselves are not const in C++.
            // What matters is whether the referred-to type is const.
            if (QT->isReferenceType()) {
                clang::QualType Pointee = QT.getNonReferenceType();
                if (Pointee.isConstQualified()) {
                    return ConstState::AlreadyConst;
                }
                return ConstState::Candidate;
            }

            if (QT.isConstQualified()) {
                return ConstState::AlreadyConst;
            }

            return ConstState::Candidate;
        }

        if (const auto* MD = llvm::dyn_cast<clang::CXXMethodDecl>(D)) {
            if (MD->isConst()) {
                return ConstState::AlreadyConst;
            }
            return ConstState::Candidate;
        }

        return ConstState::Candidate;
    }
};

/// AST visitor that builds the const graph.
///
/// This pass does not rewrite.
/// It only records:
/// - declaration nodes
/// - alias edges
/// - call edges
/// - obvious writes
///
/// The rewrite pass should run after propagation.
class ConstGraphBuilder final
    : public clang::RecursiveASTVisitor<ConstGraphBuilder> {
public:
    explicit ConstGraphBuilder(clang::ASTContext& Ctx)
        : Ctx(Ctx), Graph(Ctx) {}

    ConstGraph& graph() {
        return Graph;
    }

    bool VisitVarDecl(clang::VarDecl* VD) {
        if (!shouldTrackVarDecl(VD)) {
            return true;
        }

        const clang::QualType QT = VD->getType();
        const bool IsConst = QT.isConstQualified();
        NodeId VarNode = Graph.getOrCreateDeclNode(VD, true, IsConst);
        // int& y = x;
        //
        // If y is mutated, x is mutated.
        //
        // Add:
        //     y -> x
        if (VD->getType()->isReferenceType()) {
            if (const clang::Expr* Init = VD->getInit()) {
                if (const clang::ValueDecl* Target = findReferencedValueDecl(Init)) {
                    const bool targetIsConst = Target->getType().isConstQualified();
                    assert(!IsConst || targetIsConst);
                    NodeId TargetNode = Graph.getOrCreateDeclNode(Target, shouldTrackVarDecl(Target), targetIsConst);
                    Graph.addEdge(VarNode,
                                TargetNode,
                                EdgeKind::ReferenceAlias,
                                Init,
                                Init->getExprLoc());
                }
            }
        }

        // int* p = &x;
        //
        // Conservative first version:
        //     p -> x
        //
        // This is not always correct enough. Later distinguish:
        //     int* const p
        //     const int* p
        //     int const* p
        if (VD->getType()->isPointerType()) {
            if (const clang::Expr* Init = VD->getInit()) {
                if (const clang::ValueDecl* Target = findAddressOfValueDecl(Init)) {
                    const bool targetIsConst = Target->getType().isConstQualified();
                    assert(!IsConst || targetIsConst);
                    NodeId TargetNode = Graph.getOrCreateDeclNode(Target, shouldTrackVarDecl(Target), targetIsConst);
                    Graph.addEdge(VarNode,
                                TargetNode,
                                EdgeKind::PointerMayAlias,
                                Init,
                                Init->getExprLoc());
                }
            }
        }

        return true;
    }

    bool VisitFunctionDecl(clang::FunctionDecl* FD) {
        if (!shouldTrackAndAnalyzeBody(FD)) {
            return true;
        }

        Graph.getOrCreateDeclNode(FD, true, false);

        for (clang::ParmVarDecl* P : FD->parameters()) {
            assert(shouldTrackVarDecl(P));
            const bool IsConst = P->getType().isConstQualified();
            Graph.getOrCreateDeclNode(P, true, IsConst);
        }

        return true;
    }

    bool VisitCXXMethodDecl(clang::CXXMethodDecl* MD) {
        if (!shouldTrackAndAnalyzeBody(MD)) {
            return true;
        }

        const bool methodIsConst = MD->isConst();
        Graph.getOrCreateDeclNode(MD, true, methodIsConst);

        for (clang::ParmVarDecl* P : MD->parameters()) {
            assert(shouldTrackVarDecl(P));
            const bool IsConst = P->getType().isConstQualified();
            Graph.getOrCreateDeclNode(P, true, IsConst);
        }

        return true;
    }

    bool VisitUnaryOperator(clang::UnaryOperator* UO) {
        if (!UO || !UO->isIncrementDecrementOp()) {
            return true;
        }

        const clang::Expr* Sub = UO->getSubExpr()->IgnoreParenImpCasts();

        // Case:
        //
        //     field++;
        //     ++field;
        //     this->field++;
        //
        // This rejects both:
        //   - the field
        //   - the enclosing non-static non-constructor method
        if (const auto* ME = llvm::dyn_cast<clang::MemberExpr>(Sub)) {
            if (const auto* FD = llvm::dyn_cast<clang::FieldDecl>(ME->getMemberDecl())) {
                if (shouldTrackVarDecl(FD)) {
                    const bool fieldIsConst = FD->getType().isConstQualified();
                    assert(!fieldIsConst);
                    NodeId FieldNode = Graph.getOrCreateDeclNode(FD, true, fieldIsConst);

                    Graph.reject(FieldNode,
                                RejectReasonKind::IncrementOrDecrement,
                                UO->getOperatorLoc(),
                                UO);


                    const clang::CXXMethodDecl* MD = currentMethod();

                    const bool InConstructor =
                        MD && llvm::isa<clang::CXXConstructorDecl>(MD);

                    if (!InConstructor && isThisAccess(ME)) {
                        rejectCurrentMethodForMutation(UO->getOperatorLoc(),
                                                UO,
                                                RejectReasonKind::IncrementOrDecrement);
                    }
                }

                return true;
            }
        }

        // Case:
        //
        //     local++;
        //     ++param;
        //
        // This rejects the local/parameter itself.
        //
        // It should NOT reject the current method, because mutating a local variable
        // does not prevent the method from being const.
        if (const clang::ValueDecl* VD = findReferencedValueDecl(Sub)) {
            if (shouldTrackVarDecl(VD)) {
                const bool isConst = VD->getType().isConstQualified();
                assert(!isConst);
                NodeId N = Graph.getOrCreateDeclNode(VD, true, isConst);

                Graph.reject(N,
                            RejectReasonKind::IncrementOrDecrement,
                            UO->getOperatorLoc(),
                            UO);
            }
        }

        return true;
    }

 bool VisitBinaryOperator(clang::BinaryOperator* BO) {
    if (!BO || !BO->isAssignmentOp()) {
        return true;
    }

    const clang::Expr* LHS = BO->getLHS()->IgnoreParenImpCasts();

    if (const auto* ME = llvm::dyn_cast<clang::MemberExpr>(LHS)) {
        if (const auto* FD = llvm::dyn_cast<clang::FieldDecl>(ME->getMemberDecl())) {
            if (shouldTrackVarDecl(FD)) {
                const bool fieldIsConst = FD->getType().isConstQualified();
                assert(!fieldIsConst);
                NodeId FieldNode = Graph.getOrCreateDeclNode(FD, true, fieldIsConst);

                Graph.reject(FieldNode,
                            RejectReasonKind::AssignedTo,
                            BO->getOperatorLoc(),
                            BO);

                const clang::CXXMethodDecl* MD = currentMethod();

                const bool InConstructor =
                    MD && llvm::isa<clang::CXXConstructorDecl>(MD);

                if (!InConstructor && isThisAccess(ME)) {
                    rejectCurrentMethodForMutation(BO->getOperatorLoc(),
                                                BO,
                                                RejectReasonKind::AssignedTo);
                }

            }

            return true;
        }
    }

    if (const clang::ValueDecl* VD = findReferencedValueDecl(LHS)) {
        if (shouldTrackVarDecl(VD)) {
            const bool isConst = VD->getType().isConstQualified();
            assert(!isConst);
            NodeId N = Graph.getOrCreateDeclNode(VD, true, isConst);

            Graph.reject(N,
                         RejectReasonKind::AssignedTo,
                         BO->getOperatorLoc(),
                         BO);
        }
    }

    return true;
}

    bool VisitCallExpr(clang::CallExpr* CE) {
        if (!CE) {
            return true;
        }
        if(!shouldTrackStmt(CE)) {
            return true;
        }

        const clang::FunctionDecl* Callee = CE->getDirectCallee();
        if (!Callee) {
            // Function pointer / unresolved call.
            // Be conservative later if needed.
            // TODO Consider all args as const
            return true;
        }

        NodeId CalleeNode = Graph.getOrCreateDeclNode(Callee, shouldTrackAndAnalyzeBody(Callee), false);

        const unsigned NArgs =
            std::min<unsigned>(CE->getNumArgs(), Callee->getNumParams());

        for (unsigned I = 0; I < NArgs; ++I) {
            const clang::Expr* Arg = CE->getArg(I);

            if (!Arg) {
                continue;
            }

            if (const clang::ValueDecl* VD = findReferencedValueDecl(Arg)) {
                if (shouldTrackVarDecl(VD)) {
                    const bool ArgIsConst = VD->getType().isConstQualified();
                    NodeId N = Graph.getOrCreateDeclNode(VD, true, ArgIsConst);
                    (void)N;

                    if(ArgIsConst){
                        // If the argument is already const, the parameter cannot prevent constness.
                        continue;
                    }


                    const clang::ParmVarDecl* Param = Callee->getParamDecl(I);
                    if (!Param) {
                        // TODO Handle variadic arguments if needed.
                        continue;
                    }

                    clang::QualType ParamType = Param->getType();
                    const bool paramIsConst = ParamType.isConstQualified();
                    NodeId ParamNode = Graph.getOrCreateDeclNode(Param, shouldTrackVarDecl(Param), paramIsConst);

                
                    // void f(int& x);
                    // f(m);
                    //
                    // If parameter x cannot be const, then m cannot be const.
                    //
                    // Add:
                    //     param -> argument
                    if (ParamType->isLValueReferenceType()) {
                        clang::QualType Referred = ParamType.getNonReferenceType();

                        if (!Referred.isConstQualified()) {
                            if (const clang::ValueDecl* ArgDecl = findReferencedValueDecl(Arg)) {
                                assert(shouldTrackVarDecl(ArgDecl));
                                const bool ArgDeclIsConst = ArgDecl->getType().isConstQualified();
                                NodeId ArgNode = Graph.getOrCreateDeclNode(ArgDecl, true, ArgDeclIsConst);

                                Graph.addEdge(ParamNode,
                                            ArgNode,
                                            EdgeKind::RefParamAlias,
                                            CE,
                                            Arg->getExprLoc());

                                // For a first conservative version, reject the argument
                                // immediately when passed to a non-const reference of an
                                // external or not-yet-analyzed function.
                                //
                                // If Callee has a body and your graph analysis can prove
                                // ParamNode remains const, then remove this immediate
                                // rejection and rely on propagation instead.
                                if (!shouldTrackAndAnalyzeBody(Callee)) {
                                    Graph.reject(ArgNode,
                                                RejectReasonKind::PassedToNonConstRef,
                                                Arg->getExprLoc(),
                                                CE);
                                }
                            }
                        }
                    }

                    // void f(int* p);
                    // f(&m);
                    //
                    // Conservative.
                    if (ParamType->isPointerType()) {
                        clang::QualType Pointee = ParamType->getPointeeType();

                        if (!Pointee.isConstQualified()) {
                            if (const clang::ValueDecl* ArgDecl = findAddressOfValueDecl(Arg)) {
                                assert(shouldTrackVarDecl(ArgDecl));
                                const bool ArgDeclIsConst = ArgDecl->getType().isConstQualified();
                                NodeId ArgNode = Graph.getOrCreateDeclNode(ArgDecl, true, ArgDeclIsConst);

                                Graph.addEdge(ParamNode,
                                            ArgNode,
                                            EdgeKind::PointerParamAlias,
                                            CE,
                                            Arg->getExprLoc());

                                if (!shouldTrackAndAnalyzeBody(Callee)) {
                                    Graph.reject(ArgNode,
                                                RejectReasonKind::PassedToMutablePointer,
                                                Arg->getExprLoc(),
                                                CE);
                                }
                            }
                        }
                    }
                }
            }
        }        

        (void)CalleeNode;
        return true;
    }

    bool VisitCXXMemberCallExpr(clang::CXXMemberCallExpr* MCE) {
        if (!MCE) {
            return true;
        }

        const clang::CXXMethodDecl* Method = MCE->getMethodDecl();
        if (!Method) {
            // TODO if this a pointer or a virtual method
            return true;
        }

        const bool methodIsConst = Method->isConst();
        NodeId MethodNode = Graph.getOrCreateDeclNode(Method, shouldTrackAndAnalyzeBody(Method), methodIsConst);

        // The object called on.
        const clang::Expr* ObjExpr = MCE->getImplicitObjectArgument();
        if (!ObjExpr) {
            // TODO Handle operator calls if needed.
            return true;
        }

        if (const clang::ValueDecl* ObjDecl = findReferencedValueDecl(ObjExpr)) {
            const bool objIsConst = ObjDecl->getType().isConstQualified();
            NodeId ObjNode = Graph.getOrCreateDeclNode(ObjDecl, shouldTrackVarDecl(ObjDecl), objIsConst);

            Graph.addEdge(ObjNode,
                          MethodNode,                          
                          EdgeKind::CallsMethod,
                          MCE,
                          MCE->getExprLoc());

            // Critical rule:
            //
            // If the method is already const, the receiver is not rejected.
            //
            // If the method is non-const but has a body, do NOT immediately
            // reject the receiver. Try to prove the method can become const.
            //
            // If the method has no body, be conservative.
            if (!Method->isConst() && !shouldTrackAndAnalyzeBody(Method)) {
                Graph.reject(ObjNode,
                             RejectReasonKind::NonConstMethodCall,
                             MCE->getExprLoc(),
                             MCE);
            }
        }

        // Arguments are handled in VisitCallExpr.

        return true;
    }

bool TraverseFunctionDecl(clang::FunctionDecl* FD) {
    if (!FD) {
        return true;
    }

    // Do not handle CXX methods here. They have their own traversal path.
    if (llvm::isa<clang::CXXMethodDecl>(FD)) {
        return clang::RecursiveASTVisitor<ConstGraphBuilder>
            ::TraverseFunctionDecl(FD);
    }

    if (!shouldTrackAndAnalyzeBody(FD)) {
        return true;
    }

    Graph.getOrCreateDeclNode(FD, true, false);

    FunctionStack.push_back(FD);

    bool Result =
        clang::RecursiveASTVisitor<ConstGraphBuilder>
            ::TraverseFunctionDecl(FD);

    FunctionStack.pop_back();

    return Result;
}

bool TraverseCXXMethodDecl(clang::CXXMethodDecl* MD) {
    if (!MD) {
        return true;
    }

    if (!shouldTrackAndAnalyzeBody(MD)) {
        return true;
    }

    Graph.getOrCreateDeclNode(MD, true, MD->isConst());

    FunctionStack.push_back(MD);

    bool Result =
        clang::RecursiveASTVisitor<ConstGraphBuilder>
            ::TraverseCXXMethodDecl(MD);

    FunctionStack.pop_back();

    return Result;
}

bool TraverseCXXConstructorDecl(clang::CXXConstructorDecl* CD) {
    if (!CD) {
        return true;
    }

    if (!shouldTrackAndAnalyzeBody(CD)) {
        return true;
    }

    Graph.getOrCreateDeclNode(CD, true, false);

    FunctionStack.push_back(CD);

    bool Result =
        clang::RecursiveASTVisitor<ConstGraphBuilder>
            ::TraverseCXXConstructorDecl(CD);

    FunctionStack.pop_back();

    return Result;
}

private:
    clang::ASTContext& Ctx;
    ConstGraph Graph;
    std::vector<const clang::FunctionDecl*> FunctionStack;

private:

    bool isThisAccess(const clang::MemberExpr* ME) {
        const clang::Expr* Base = ME->getBase();
        if (!Base) return false;

        Base = Base->IgnoreParenImpCasts();

        if (llvm::isa<clang::CXXThisExpr>(Base))
            return true;

        if (const auto* UO = llvm::dyn_cast<clang::UnaryOperator>(Base)) {
            if (UO->getOpcode() == clang::UO_Deref) {
                const clang::Expr* Sub = UO->getSubExpr()->IgnoreParenImpCasts();
                return llvm::isa<clang::CXXThisExpr>(Sub);
            }
        }

        return false;
    }

    void rejectCurrentMethodForMutation(clang::SourceLocation Loc,
                                    const clang::Stmt* Stmt,
                                    RejectReasonKind Reason) {
        const clang::CXXMethodDecl* MD = currentMethod();

        if (!MD) {
            return;
        }

        // Constructors are not const methods.
        // Writes during construction should not prove that the object is mutable
        // after construction.
        if (llvm::isa<clang::CXXConstructorDecl>(MD)) {
            return;
        }

        // Destructors are also not const methods.
        if (llvm::isa<clang::CXXDestructorDecl>(MD)) {
            return;
        }

        if (MD->isStatic()) {
            return;
        }

        assert(shouldTrackAndAnalyzeBody(MD));
        NodeId MethodNode = Graph.getOrCreateDeclNode(MD, true, MD->isConst());

        Graph.reject(MethodNode, Reason, Loc, Stmt);
    }


    const clang::FunctionDecl* currentFunction() const {
        if (FunctionStack.empty()) {
            return nullptr;
        }
        return FunctionStack.back();
    }

    const clang::CXXMethodDecl* currentMethod() const {
        return llvm::dyn_cast_or_null<clang::CXXMethodDecl>(currentFunction());
    }

    // bool isInMainFile(clang::SourceLocation Loc) const {
    //     if (Loc.isInvalid()) {
    //         return false;
    //     }

    //     const clang::SourceManager& SM = Ctx.getSourceManager();

    //     // Important: use spelling location for declarations.
    //     Loc = SM.getSpellingLoc(Loc);

    //     if (Loc.isInvalid()) {
    //         return false;
    //     }

    //     if (SM.isInSystemHeader(Loc) || SM.isInExternCSystemHeader(Loc)) {
    //         return false;
    //     }

    //     return SM.isWrittenInMainFile(Loc);
    // }

    bool shouldTrackVarDecl(const clang::Decl* D) const {
        if (!D) {
            return false;
        }

        if (D->isImplicit()) {
            return false;
        }

        clang::SourceLocation Loc = D->getLocation();

        if (Loc.isInvalid()) {
            return false;
        }

        const clang::SourceManager& SM = Ctx.getSourceManager();
        clang::SourceLocation SpellingLoc = SM.getSpellingLoc(Loc);

        if (!SM.isWrittenInMainFile(SpellingLoc)) {
            return false;
        }

        if (SM.isInSystemHeader(SpellingLoc) || SM.isInExternCSystemHeader(SpellingLoc)) {
            return false;
        }

        return llvm::isa<clang::ParmVarDecl>(D) ||
            llvm::isa<clang::VarDecl>(D) ||
            llvm::isa<clang::FieldDecl>(D) ||
            llvm::isa<clang::FunctionDecl>(D);
    }

    bool shouldTrackStmt(const clang::Stmt* S) const {
        if (!S) {
            return false;
        }

        clang::SourceLocation Loc = S->getBeginLoc();

        if (Loc.isInvalid()) {
            return false;
        }

        const clang::SourceManager& SM = Ctx.getSourceManager();
        clang::SourceLocation SpellingLoc = SM.getSpellingLoc(Loc);

        if (!SM.isWrittenInMainFile(SpellingLoc)) {
            return false;
        }

        if (SM.isInSystemHeader(SpellingLoc) || SM.isInExternCSystemHeader(SpellingLoc)) {
            return false;
        }

        return true;
    }

    bool shouldTrackAndAnalyzeBody(const clang::FunctionDecl* FD) const {
        if (!FD) {
            return false;
        }

        if (FD->isImplicit()) {
            return false;
        }

        if(!FD->hasBody()) {
            return false;
        }

        clang::SourceLocation Loc = FD->getLocation();

        if (Loc.isInvalid()) {
            return false;
        }

        const clang::SourceManager& SM = Ctx.getSourceManager();
        clang::SourceLocation SpellingLoc = SM.getSpellingLoc(Loc);

        if (!SM.isWrittenInMainFile(SpellingLoc)) {
            return false;
        }

        if (SM.isInSystemHeader(SpellingLoc) || SM.isInExternCSystemHeader(SpellingLoc)) {
            return false;
        }

        return true;
    }

    static const clang::ValueDecl*
    findReferencedValueDecl(const clang::Expr* E) {
        if (!E) {
            return nullptr;
        }
        E = E->IgnoreParenImpCasts();
        if (!E) {
            return nullptr;
        }

        if (const auto* DRE = llvm::dyn_cast<clang::DeclRefExpr>(E)) {
            return llvm::dyn_cast<clang::ValueDecl>(DRE->getDecl());
        }

        if (const auto* ME = llvm::dyn_cast<clang::MemberExpr>(E)) {
            return llvm::dyn_cast<clang::ValueDecl>(ME->getMemberDecl());
        }

        return nullptr;
    }

    static const clang::ValueDecl*
    findAddressOfValueDecl(const clang::Expr* E) {
        if (!E) {
            return nullptr;
        }
        E = E->IgnoreParenImpCasts();
        if (!E) {
            return nullptr;
        }

        if (const auto* UO = llvm::dyn_cast<clang::UnaryOperator>(E)) {
            if (UO->getOpcode() == clang::UO_AddrOf) {
                return findReferencedValueDecl(UO->getSubExpr());
            }
        }

        return nullptr;
    }
};

/// Optional ASTConsumer wrapper.
///
/// You can use this from your ASTFrontendAction.
///
/// Example:
///
///     std::unique_ptr<ASTConsumer>
///     CreateASTConsumer(CompilerInstance& CI, StringRef InFile) override {
///         return std::make_unique<constify::ConstifyASTConsumer>();
///     }
class ConstifyASTConsumer final : public clang::ASTConsumer {
public:
    void HandleTranslationUnit(clang::ASTContext& Ctx) override {
        ConstGraphBuilder Builder(Ctx);

        Builder.TraverseDecl(Ctx.getTranslationUnitDecl());

        Builder.graph().propagateCannotBeConst();

        if (Verbose) {
            Builder.graph().dump(llvm::errs());
        }

        // Later:
        //
        //   ConstRewriter Rewriter(Ctx, Builder.graph());
        //   Rewriter.apply();
        //
        // Do not rewrite here until the graph is trustworthy.
    }
};

} // namespace constify
