//--------------------------------------------------------------------------------------------------
// Copyright (c) YugaByte, Inc.
//
// Different results of processing a statement.
//--------------------------------------------------------------------------------------------------

#ifndef YB_SQL_UTIL_STATEMENT_RESULT_H_
#define YB_SQL_UTIL_STATEMENT_RESULT_H_

#include "yb/client/yb_op.h"
#include "yb/client/yb_table_name.h"
#include "yb/common/schema.h"
#include "yb/common/yql_protocol.pb.h"
#include "yb/common/yql_rowblock.h"
#include "yb/sql/ptree/pt_select.h"

namespace yb {
namespace sql {

//------------------------------------------------------------------------------------------------
// Result of preparing a statement. Only DML statement will return a prepared result that describes
// the schemas of the bind variables used and, for SELECT statement, the schemas of the columns
// selected.
class PreparedResult {
 public:
  // Public types.
  typedef std::unique_ptr<PreparedResult> UniPtr;
  typedef std::unique_ptr<const PreparedResult> UniPtrConst;

  // Constructors.
  explicit PreparedResult(const PTDmlStmt *tnode);
  virtual ~PreparedResult();

  // Accessors.
  const client::YBTableName& table_name() const { return table_name_; }
  const std::vector<ColumnSchema>& bind_variable_schemas() const { return bind_variable_schemas_; }
  const std::vector<ColumnSchema>& column_schemas() const { return column_schemas_; }

 private:
  const client::YBTableName table_name_;
  const std::vector<ColumnSchema> bind_variable_schemas_;
  const std::vector<ColumnSchema> column_schemas_;
};

//------------------------------------------------------------------------------------------------
// Result of executing a statement. Different possible types of results are listed below.
class ExecuteResult {
 public:
  // Public types.
  typedef std::unique_ptr<ExecuteResult> UniPtr;
  typedef std::unique_ptr<const ExecuteResult> UniPtrConst;

  // Constructors.
  ExecuteResult() { }
  virtual ~ExecuteResult() { }

  // Execution result types.
  enum class Type {
    SET_KEYSPACE = 1,
    ROWS         = 2
  };

  virtual const Type type() const = 0;
};

//------------------------------------------------------------------------------------------------
// Result of "USE <keyspace>" statement.
class SetKeyspaceResult : public ExecuteResult {
 public:
  // Public types.
  typedef std::unique_ptr<SetKeyspaceResult> UniPtr;
  typedef std::unique_ptr<const SetKeyspaceResult> UniPtrConst;

  // Constructors.
  explicit SetKeyspaceResult(const std::string& keyspace) : keyspace_(keyspace) { }
  virtual ~SetKeyspaceResult() override { };

  // Result type.
  virtual const Type type() const override { return Type::SET_KEYSPACE; }

  // Accessor function for keyspace.
  const std::string& keyspace() const { return keyspace_; }

 private:
  const std::string keyspace_;
};

//------------------------------------------------------------------------------------------------
// Result of rows returned from executing a DML statement.
class RowsResult : public ExecuteResult {
 public:
  // Public types.
  typedef std::unique_ptr<RowsResult> UniPtr;
  typedef std::unique_ptr<const RowsResult> UniPtrConst;

  // Constructors.
  explicit RowsResult(client::YBqlReadOp *op);
  explicit RowsResult(client::YBqlWriteOp *op);
  virtual ~RowsResult() override;

  // Result type.
  virtual const Type type() const override { return Type::ROWS; }

  // Accessor functions.
  const client::YBTableName& table_name() const { return table_name_; }
  const std::vector<ColumnSchema>& column_schemas() const { return column_schemas_; }
  const std::string& rows_data() const { return rows_data_; }
  const std::string& paging_state() const { return paging_state_; }
  void clear_paging_state() { paging_state_.clear(); }
  YQLClient client() const { return client_; }

  // Parse the rows data and return it as a row block. It is the caller's responsibility to free
  // the row block after use.
  YQLRowBlock *GetRowBlock() const;

 private:
  const client::YBTableName table_name_;
  const std::vector<ColumnSchema> column_schemas_;
  const std::string rows_data_;
  const YQLClient client_;
  std::string paging_state_;
};

} // namespace sql
} // namespace yb

#endif  // YB_SQL_UTIL_STATEMENT_RESULT_H_