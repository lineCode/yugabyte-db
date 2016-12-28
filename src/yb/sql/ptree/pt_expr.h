//--------------------------------------------------------------------------------------------------
// Copyright (c) YugaByte, Inc.
//
// Tree node definitions for expression.
//--------------------------------------------------------------------------------------------------

#ifndef YB_SQL_PTREE_PT_EXPR_H_
#define YB_SQL_PTREE_PT_EXPR_H_

#include "yb/sql/ptree/tree_node.h"
#include "yb/sql/ptree/pt_type.h"
#include "yb/sql/ptree/pt_name.h"

namespace yb {
namespace sql {

//--------------------------------------------------------------------------------------------------

enum class BuiltinOperator : int {
  kNoOp = 0,

  // Operators that take one operand.
  kNot,
  kIsNull,
  kIsNotNull,
  kIsTrue,
  kIsFalse,

  // Operators that take two operands.
  kEQ,
  kLT,
  kGT,
  kLE,
  kGE,
  kNE,
  kAND,
  kOR,
  kLike,
  kNotLike,
  kIn,
  kNotIn,

  // Operators that take three operands.
  kBetween,
  kNotBetween,
};

//--------------------------------------------------------------------------------------------------

// Base class for all expressions.
class PTExpr : public TreeNode {
 public:
  //------------------------------------------------------------------------------------------------
  // Public types.
  typedef MCSharedPtr<PTExpr> SharedPtr;
  typedef MCSharedPtr<const PTExpr> SharedPtrConst;

  //------------------------------------------------------------------------------------------------
  // Constructor and destructor.
  explicit PTExpr(MemoryContext *memctx = nullptr, YBLocation::SharedPtr loc = nullptr)
      : TreeNode(memctx, loc) {
  }
  virtual ~PTExpr() {
  }

  // Expression return type in Cassandra format.
  virtual PTTypeId type_id() const = 0;

  // Expression return type in DocDB format.
  virtual client::YBColumnSchema::DataType yb_data_type() const = 0;

  // Node type.
  virtual TreeNodeOpcode opcode() const OVERRIDE {
    return TreeNodeOpcode::kPTExpr;
  }
};

using PTExprListNode = TreeListNode<PTExpr>;

//--------------------------------------------------------------------------------------------------

// Template for all expressions.
template<PTTypeId type_id_,
         client::YBColumnSchema::DataType yb_data_type_,
         typename ReturnType>
class PTExprOperator : public PTExpr {
 public:
  //------------------------------------------------------------------------------------------------
  // Public types.
  typedef MCSharedPtr<PTExprOperator> SharedPtr;
  typedef MCSharedPtr<const PTExprOperator> SharedPtrConst;

  //------------------------------------------------------------------------------------------------
  // Constructor and destructor.
  explicit PTExprOperator(MemoryContext *memctx = nullptr,
                          YBLocation::SharedPtr loc = nullptr,
                          BuiltinOperator op = BuiltinOperator::kNoOp)
      : PTExpr(memctx, loc),
        op_(op) {
  }
  virtual ~PTExprOperator() {
  }

  // Expresion return type in Cassandra format.
  virtual PTTypeId type_id() const OVERRIDE {
    return type_id_;
  }

  // Expresion return type in DocDB format.
  virtual client::YBColumnSchema::DataType yb_data_type() const OVERRIDE {
    return yb_data_type_;
  }

  // Access to op_.
  BuiltinOperator op() {
    return op_;
  }

 protected:
  BuiltinOperator op_;
};

//--------------------------------------------------------------------------------------------------
// Template for expression with no operand (0 input).
template<PTTypeId type_id_,
         client::YBColumnSchema::DataType yb_data_type_,
         typename ReturnType>
class PTExprConst : public PTExprOperator<type_id_, yb_data_type_, ReturnType> {
 public:
  //------------------------------------------------------------------------------------------------
  // Public types.
  typedef MCSharedPtr<PTExprConst> SharedPtr;
  typedef MCSharedPtr<const PTExprConst> SharedPtrConst;

  //------------------------------------------------------------------------------------------------
  // Constructor and destructor.
  PTExprConst(MemoryContext *memctx,
              YBLocation::SharedPtr loc,
              ReturnType value)
      : PTExprOperator<type_id_, yb_data_type_, ReturnType>(memctx, loc, BuiltinOperator::kNoOp),
        value_(value) {
  }
  virtual ~PTExprConst() {
  }

  // Shared pointer support.
  template<typename... TypeArgs>
  inline static PTExprConst::SharedPtr MakeShared(MemoryContext *memctx, TypeArgs&&... args) {
    return MCMakeShared<PTExprConst>(memctx, std::forward<TypeArgs>(args)...);
  }

  // Evaluate this expression and its operand.
  virtual ReturnType Eval() {
    return value_;
  }

 private:
  //------------------------------------------------------------------------------------------------
  // Constant value.
  ReturnType value_;
};

//--------------------------------------------------------------------------------------------------
// Template for expression with no operand (0 input).
template<PTTypeId type_id_,
         client::YBColumnSchema::DataType yb_data_type_,
         typename ReturnType>
class PTExpr0 : public PTExprOperator<type_id_, yb_data_type_, ReturnType> {
 public:
  //------------------------------------------------------------------------------------------------
  // Public types.
  typedef MCSharedPtr<PTExpr0> SharedPtr;
  typedef MCSharedPtr<const PTExpr0> SharedPtrConst;

  //------------------------------------------------------------------------------------------------
  // Constructor and destructor.
  PTExpr0(MemoryContext *memctx,
          YBLocation::SharedPtr loc,
          BuiltinOperator op)
      : PTExprOperator<type_id_, yb_data_type_, ReturnType>(memctx, loc, op) {
  }
  virtual ~PTExpr0() {
  }

  // Shared pointer support.
  template<typename... TypeArgs>
  inline static PTExpr0::SharedPtr MakeShared(MemoryContext *memctx, TypeArgs&&... args) {
    return MCMakeShared<PTExpr0>(memctx, std::forward<TypeArgs>(args)...);
  }
};

//--------------------------------------------------------------------------------------------------
// Template for expression with one operand (1 input).
template<PTTypeId type_id_,
         client::YBColumnSchema::DataType yb_data_type_,
         typename ReturnType>
class PTExpr1 : public PTExprOperator<type_id_, yb_data_type_, ReturnType> {
 public:
  //------------------------------------------------------------------------------------------------
  // Public types.
  typedef MCSharedPtr<PTExpr1> SharedPtr;
  typedef MCSharedPtr<const PTExpr1> SharedPtrConst;

  //------------------------------------------------------------------------------------------------
  // Constructor and destructor.
  PTExpr1(MemoryContext *memctx,
          YBLocation::SharedPtr loc,
          BuiltinOperator op,
          PTExpr::SharedPtr op1)
      : PTExprOperator<type_id_, yb_data_type_, ReturnType>(memctx, loc, op),
        op1_(op1) {
  }
  virtual ~PTExpr1() {
  }

  // Shared pointer support.
  template<typename... TypeArgs>
  inline static PTExpr1::SharedPtr MakeShared(MemoryContext *memctx, TypeArgs&&... args) {
    return MCMakeShared<PTExpr1>(memctx, std::forward<TypeArgs>(args)...);
  }

 private:
  //------------------------------------------------------------------------------------------------
  // Operand.
  PTExpr::SharedPtr op1_;
};

//--------------------------------------------------------------------------------------------------
// Template for expression with two operands (2 inputs).
template<PTTypeId type_id_,
         client::YBColumnSchema::DataType yb_data_type_,
         typename ReturnType>
class PTExpr2 : public PTExprOperator<type_id_, yb_data_type_, ReturnType> {
 public:
  //------------------------------------------------------------------------------------------------
  // Public types.
  typedef MCSharedPtr<PTExpr2> SharedPtr;
  typedef MCSharedPtr<const PTExpr2> SharedPtrConst;

  //------------------------------------------------------------------------------------------------
  // Constructor and destructor.
  PTExpr2(MemoryContext *memctx,
          YBLocation::SharedPtr loc,
          BuiltinOperator op,
          const PTExpr::SharedPtr& op1,
          const PTExpr::SharedPtr& op2)
      : PTExprOperator<type_id_, yb_data_type_, ReturnType>(memctx, loc, op),
        op1_(op1),
        op2_(op2) {
  }
  virtual ~PTExpr2() {
  }

  // Shared pointer support.
  template<typename... TypeArgs>
  inline static PTExpr2::SharedPtr MakeShared(MemoryContext *memctx, TypeArgs&&... args) {
    return MCMakeShared<PTExpr2>(memctx, std::forward<TypeArgs>(args)...);
  }

 private:
  //------------------------------------------------------------------------------------------------
  // Operand 1 and 2.
  PTExpr::SharedPtr op1_;
  PTExpr::SharedPtr op2_;
};

//--------------------------------------------------------------------------------------------------
// Template for expression with two operands (3 inputs).
template<PTTypeId type_id_,
         client::YBColumnSchema::DataType yb_data_type_,
         typename ReturnType>
class PTExpr3 : public PTExprOperator<type_id_, yb_data_type_, ReturnType> {
 public:
  //------------------------------------------------------------------------------------------------
  // Public types.
  typedef MCSharedPtr<PTExpr3> SharedPtr;
  typedef MCSharedPtr<const PTExpr3> SharedPtrConst;

  //------------------------------------------------------------------------------------------------
  // Constructor and destructor.
  PTExpr3(MemoryContext *memctx,
          YBLocation::SharedPtr loc,
          BuiltinOperator op,
          const PTExpr::SharedPtr& op1,
          const PTExpr::SharedPtr& op2,
          const PTExpr::SharedPtr& op3)
      : PTExprOperator<type_id_, yb_data_type_, ReturnType>(memctx, loc, op),
        op1_(op1),
        op2_(op2),
        op3_(op3) {
  }
  virtual ~PTExpr3() {
  }

  // Shared pointer support.
  template<typename... TypeArgs>
  inline static PTExpr3::SharedPtr MakeShared(MemoryContext *memctx, TypeArgs&&... args) {
    return MCMakeShared<PTExpr3>(memctx, std::forward<TypeArgs>(args)...);
  }

 private:
  //------------------------------------------------------------------------------------------------
  // Operand 1 and 2.
  PTExpr::SharedPtr op1_;
  PTExpr::SharedPtr op2_;
  PTExpr::SharedPtr op3_;
};

//--------------------------------------------------------------------------------------------------
// Tree node for constants
using PTConstInt = PTExprConst<PTTypeId::kBigInt, client::YBColumnSchema::INT64, int64_t>;
using PTConstDouble = PTExprConst<PTTypeId::kDouble, client::YBColumnSchema::DOUBLE, long double>;
using PTConstText = PTExprConst<PTTypeId::kCharBaseType,
                                client::YBColumnSchema::STRING,
                                MCString::SharedPtr>;
using PTConstBool = PTExprConst<PTTypeId::kBoolean, client::YBColumnSchema::BOOL, bool>;

// Tree node for comparisons.
using PTPredicate1 = PTExpr1<PTTypeId::kBoolean, client::YBColumnSchema::BOOL, bool>;
using PTPredicate2 = PTExpr2<PTTypeId::kBoolean, client::YBColumnSchema::BOOL, bool>;
using PTPredicate3 = PTExpr3<PTTypeId::kBoolean, client::YBColumnSchema::BOOL, bool>;

//--------------------------------------------------------------------------------------------------
// Column Reference. The datatype of this expression would need to be resolved by the analyzer.
class PTRef : public PTExpr {
 public:
  //------------------------------------------------------------------------------------------------
  // Public types.
  typedef MCSharedPtr<PTRef> SharedPtr;
  typedef MCSharedPtr<const PTRef> SharedPtrConst;

  //------------------------------------------------------------------------------------------------
  // Constructor and destructor.
  PTRef(MemoryContext *memctx,
        YBLocation::SharedPtr loc,
        const PTQualifiedName::SharedPtr& name);
  virtual ~PTRef();

  // Support for shared_ptr.
  template<typename... TypeArgs>
  inline static PTRef::SharedPtr MakeShared(MemoryContext *memctx, TypeArgs&&... args) {
    return MCMakeShared<PTRef>(memctx, std::forward<TypeArgs>(args)...);
  }

  // Node semantics analysis.
  virtual ErrorCode Analyze(SemContext *sem_context) OVERRIDE;
  void PrintSemanticAnalysisResult(SemContext *sem_context);

  // Expression return type in Cassandra format.
  virtual PTTypeId type_id() const OVERRIDE {
    return type_id_;
  }

  // Expression return type in DocDB format.
  virtual client::YBColumnSchema::DataType yb_data_type() const OVERRIDE {
    return yb_data_type_;
  }

  // Node type.
  virtual TreeNodeOpcode opcode() const OVERRIDE {
    return TreeNodeOpcode::kPTRef;
  }

 private:
  PTQualifiedName::SharedPtr name_;
  PTTypeId type_id_;
  client::YBColumnSchema::DataType yb_data_type_;
};

//--------------------------------------------------------------------------------------------------
// Expression alias - Name of an expression including reference to column.
class PTExprAlias : public PTExpr {
 public:
  //------------------------------------------------------------------------------------------------
  // Public types.
  typedef MCSharedPtr<PTExprAlias> SharedPtr;
  typedef MCSharedPtr<const PTExprAlias> SharedPtrConst;

  //------------------------------------------------------------------------------------------------
  // Constructor and destructor.
  PTExprAlias(MemoryContext *memctx,
              YBLocation::SharedPtr loc,
              const PTExpr::SharedPtr& expr,
              const MCString::SharedPtr& alias);
  virtual ~PTExprAlias();

  // Support for shared_ptr.
  template<typename... TypeArgs>
  inline static PTExprAlias::SharedPtr MakeShared(MemoryContext *memctx, TypeArgs&&... args) {
    return MCMakeShared<PTExprAlias>(memctx, std::forward<TypeArgs>(args)...);
  }

  // Expresion return type in Cassandra format.
  virtual PTTypeId type_id() const {
    return expr_->type_id();
  }

  // Expresion return type in DocDB format.
  virtual client::YBColumnSchema::DataType yb_data_type() const {
    return expr_->yb_data_type();
  }

 private:
  PTExpr::SharedPtr expr_;
  MCString::SharedPtr alias_;
};

}  // namespace sql
}  // namespace yb

#endif  // YB_SQL_PTREE_PT_EXPR_H_