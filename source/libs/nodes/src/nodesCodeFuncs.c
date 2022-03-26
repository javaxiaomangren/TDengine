/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "cmdnodes.h"
#include "nodesUtil.h"
#include "plannodes.h"
#include "querynodes.h"
#include "query.h"
#include "taoserror.h"
#include "tjson.h"

static int32_t nodeToJson(const void* pObj, SJson* pJson);
static int32_t jsonToNode(const SJson* pJson, void* pObj);
static int32_t jsonToNodeObject(const SJson* pJson, const char* pName, SNode** pNode);
static int32_t makeNodeByJson(const SJson* pJson, SNode** pNode);

const char* nodesNodeName(ENodeType type) {
  switch (type) {
    case QUERY_NODE_COLUMN:
      return "Column";    
    case QUERY_NODE_VALUE:
      return "Value";
    case QUERY_NODE_OPERATOR:
      return "Operator";
    case QUERY_NODE_LOGIC_CONDITION:
      return "LogicCondition";
    case QUERY_NODE_FUNCTION:
      return "Function";
    case QUERY_NODE_REAL_TABLE:
      return "RealTable";
    case QUERY_NODE_TEMP_TABLE:
      return "TempTable";
    case QUERY_NODE_JOIN_TABLE:
      return "JoinTable";
    case QUERY_NODE_GROUPING_SET:
      return "GroupingSet";
    case QUERY_NODE_ORDER_BY_EXPR:
      return "OrderByExpr";
    case QUERY_NODE_LIMIT:
      return "Limit";
    case QUERY_NODE_STATE_WINDOW:
      return "StateWindow";
    case QUERY_NODE_SESSION_WINDOW:
      return "SessionWinow";
    case QUERY_NODE_INTERVAL_WINDOW:
      return "IntervalWindow";
    case QUERY_NODE_NODE_LIST:
      return "NodeList";
    case QUERY_NODE_FILL:
      return "Fill";
    case QUERY_NODE_RAW_EXPR:
      return "RawExpr";
    case QUERY_NODE_TARGET:
      return "Target";
    case QUERY_NODE_DATABLOCK_DESC:
      return "TupleDesc";
    case QUERY_NODE_SLOT_DESC:
      return "SlotDesc";
    case QUERY_NODE_COLUMN_DEF:
      return "ColumnDef";
    case QUERY_NODE_DOWNSTREAM_SOURCE:
      return "DownstreamSource";
    case QUERY_NODE_DATABASE_OPTIONS:
      return "DatabaseOptions";
    case QUERY_NODE_TABLE_OPTIONS:
      return "TableOptions";
    case QUERY_NODE_INDEX_OPTIONS:
      return "IndexOptions";
    case QUERY_NODE_SET_OPERATOR:
      return "SetOperator";
    case QUERY_NODE_SELECT_STMT:
      return "SelectStmt";
    case QUERY_NODE_VNODE_MODIF_STMT:
      return "VnodeModifStmt";
    case QUERY_NODE_CREATE_DATABASE_STMT:
      return "CreateDatabaseStmt";
    case QUERY_NODE_DROP_DATABASE_STMT:
      return "DropDatabaseStmt";
    case QUERY_NODE_ALTER_DATABASE_STMT:
      return "AlterDatabaseStmt";
    case QUERY_NODE_CREATE_TABLE_STMT:
      return "CreateTableStmt";
    case QUERY_NODE_CREATE_SUBTABLE_CLAUSE:
      return "CreateSubtableClause";
    case QUERY_NODE_CREATE_MULTI_TABLE_STMT:
      return "CreateMultiTableStmt";
    case QUERY_NODE_DROP_TABLE_CLAUSE:
      return "DropTableClause";
    case QUERY_NODE_DROP_TABLE_STMT:
      return "DropTableStmt";
    case QUERY_NODE_DROP_SUPER_TABLE_STMT:
      return "DropSuperTableStmt";
    case QUERY_NODE_ALTER_TABLE_STMT:
      return "AlterTableStmt";
    case QUERY_NODE_CREATE_USER_STMT:
      return "CreateUserStmt";
    case QUERY_NODE_ALTER_USER_STMT:
      return "AlterUserStmt";
    case QUERY_NODE_DROP_USER_STMT:
      return "DropUserStmt";
    case QUERY_NODE_USE_DATABASE_STMT:
      return "UseDatabaseStmt";
    case QUERY_NODE_CREATE_DNODE_STMT:
      return "CreateDnodeStmt";
    case QUERY_NODE_DROP_DNODE_STMT:
      return "DropDnodeStmt";
    case QUERY_NODE_ALTER_DNODE_STMT:
      return "AlterDnodeStmt";
    case QUERY_NODE_CREATE_INDEX_STMT:
      return "CreateIndexStmt";
    case QUERY_NODE_DROP_INDEX_STMT:
      return "DropIndexStmt";
    case QUERY_NODE_CREATE_QNODE_STMT:
      return "CreateQnodeStmt";
    case QUERY_NODE_DROP_QNODE_STMT:
      return "DropQnodeStmt";
    case QUERY_NODE_CREATE_TOPIC_STMT:
      return "CreateTopicStmt";
    case QUERY_NODE_DROP_TOPIC_STMT:
      return "DropTopicStmt";
    case QUERY_NODE_ALTER_LOCAL_STMT:
      return "AlterLocalStmt";
    case QUERY_NODE_SHOW_DATABASES_STMT:
      return "ShowDatabaseStmt";
    case QUERY_NODE_SHOW_TABLES_STMT:
      return "ShowTablesStmt";
    case QUERY_NODE_SHOW_STABLES_STMT:
      return "ShowStablesStmt";
    case QUERY_NODE_SHOW_USERS_STMT:
      return "ShowUsersStmt";
    case QUERY_NODE_SHOW_DNODES_STMT:
      return "ShowDnodesStmt";
    case QUERY_NODE_SHOW_VGROUPS_STMT:
      return "ShowVgroupsStmt";
    case QUERY_NODE_SHOW_MNODES_STMT:
      return "ShowMnodesStmt";
    case QUERY_NODE_SHOW_MODULES_STMT:
      return "ShowModulesStmt";
    case QUERY_NODE_SHOW_QNODES_STMT:
      return "ShowQnodesStmt";
    case QUERY_NODE_SHOW_FUNCTIONS_STMT:
      return "ShowFunctionsStmt";
    case QUERY_NODE_SHOW_INDEXES_STMT:
      return "ShowIndexesStmt";
    case QUERY_NODE_SHOW_STREAMS_STMT:
      return "ShowStreamsStmt";
    case QUERY_NODE_LOGIC_PLAN_SCAN:
      return "LogicScan";
    case QUERY_NODE_LOGIC_PLAN_JOIN:
      return "LogicJoin";
    case QUERY_NODE_LOGIC_PLAN_AGG:
      return "LogicAgg";
    case QUERY_NODE_LOGIC_PLAN_PROJECT:
      return "LogicProject";
    case QUERY_NODE_LOGIC_PLAN_VNODE_MODIF:
      return "LogicVnodeModif";
    case QUERY_NODE_LOGIC_PLAN_EXCHANGE:
      return "LogicExchange";
    case QUERY_NODE_LOGIC_PLAN_WINDOW:
      return "LogicWindow";
    case QUERY_NODE_LOGIC_SUBPLAN:
      return "LogicSubplan";
    case QUERY_NODE_LOGIC_PLAN:
      return "LogicPlan";
    case QUERY_NODE_PHYSICAL_PLAN_TAG_SCAN:
      return "PhysiTagScan";
    case QUERY_NODE_PHYSICAL_PLAN_TABLE_SCAN:
      return "PhysiTableScan";
    case QUERY_NODE_PHYSICAL_PLAN_TABLE_SEQ_SCAN:
      return "PhysiTableSeqScan";
    case QUERY_NODE_PHYSICAL_PLAN_STREAM_SCAN:
      return "PhysiSreamScan";
    case QUERY_NODE_PHYSICAL_PLAN_SYSTABLE_SCAN:
      return "PhysiSystemTableScan";
    case QUERY_NODE_PHYSICAL_PLAN_PROJECT:
      return "PhysiProject";
    case QUERY_NODE_PHYSICAL_PLAN_JOIN:
      return "PhysiJoin";
    case QUERY_NODE_PHYSICAL_PLAN_AGG:
      return "PhysiAgg";
    case QUERY_NODE_PHYSICAL_PLAN_EXCHANGE:
      return "PhysiExchange";
    case QUERY_NODE_PHYSICAL_PLAN_SORT:
      return "PhysiSort";
    case QUERY_NODE_PHYSICAL_PLAN_INTERVAL:
      return "PhysiInterval";
    case QUERY_NODE_PHYSICAL_PLAN_SESSION_WINDOW:
      return "PhysiSessionWindow";
    case QUERY_NODE_PHYSICAL_PLAN_DISPATCH:
      return "PhysiDispatch";
    case QUERY_NODE_PHYSICAL_PLAN_INSERT:
      return "PhysiInsert";
    case QUERY_NODE_PHYSICAL_SUBPLAN:
      return "PhysiSubplan";
    case QUERY_NODE_PHYSICAL_PLAN:
      return "PhysiPlan";
    default:
      break;
  }
  nodesWarn("nodesNodeName unknown node = %d", type);
  return "UnknownNode";
}

static int32_t nodeListToJson(SJson* pJson, const char* pName, const SNodeList* pList) {
  if (LIST_LENGTH(pList) > 0) {
    SJson* jList = tjsonAddArrayToObject(pJson, pName);
    if (NULL == jList) {
      return TSDB_CODE_OUT_OF_MEMORY;
    }
    SNode* pNode;
    FOREACH(pNode, pList) {
      int32_t code = tjsonAddItem(jList, nodeToJson, pNode);
      if (TSDB_CODE_SUCCESS != code) {
        return code;
      }
    }
  }
  return TSDB_CODE_SUCCESS;
}

static int32_t jsonToNodeListImpl(const SJson* pJsonArray, SNodeList** pList) {
  int32_t size = (NULL == pJsonArray ? 0 : tjsonGetArraySize(pJsonArray));
  if (size > 0) {
    *pList = nodesMakeList();
    if (NULL == *pList) {
      return TSDB_CODE_OUT_OF_MEMORY;
    }
  }

  int32_t code = TSDB_CODE_SUCCESS;
  for (int32_t i = 0; i < size; ++i) {
    SJson* pJsonItem = tjsonGetArrayItem(pJsonArray, i);
    SNode* pNode = NULL;
    code = makeNodeByJson(pJsonItem, &pNode);
    if (TSDB_CODE_SUCCESS == code) {
      code = nodesListAppend(*pList, pNode);
    }
    if (TSDB_CODE_SUCCESS != code) {
      break;
    }
  }
  return code;
}

static int32_t jsonToNodeList(const SJson* pJson, const char* pName, SNodeList** pList) {
  return jsonToNodeListImpl(tjsonGetObjectItem(pJson, pName), pList);
}

static const char* jkTableComInfoNumOfTags = "NumOfTags";
static const char* jkTableComInfoPrecision = "Precision";
static const char* jkTableComInfoNumOfColumns = "NumOfColumns";
static const char* jkTableComInfoRowSize = "RowSize";

static int32_t tableComInfoToJson(const void* pObj, SJson* pJson) {
  const STableComInfo* pNode = (const STableComInfo*)pObj;

  int32_t code = tjsonAddIntegerToObject(pJson, jkTableComInfoNumOfTags, pNode->numOfTags);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkTableComInfoPrecision, pNode->precision);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkTableComInfoNumOfColumns, pNode->numOfColumns);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkTableComInfoRowSize, pNode->rowSize);
  }
  
  return code;
}

static int32_t jsonToTableComInfo(const SJson* pJson, void* pObj) {
  STableComInfo* pNode = (STableComInfo*)pObj;

  int32_t code = tjsonGetNumberValue(pJson, jkTableComInfoNumOfTags, pNode->numOfTags);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkTableComInfoPrecision, pNode->precision);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkTableComInfoNumOfColumns, pNode->numOfColumns);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkTableComInfoRowSize, pNode->rowSize);
  }
  
  return code;
}

static const char* jkSchemaType = "Type";
static const char* jkSchemaColId = "ColId";
static const char* jkSchemaBytes = "bytes";
static const char* jkSchemaName = "Name";

static int32_t schemaToJson(const void* pObj, SJson* pJson) {
  const SSchema* pNode = (const SSchema*)pObj;

  int32_t code = tjsonAddIntegerToObject(pJson, jkSchemaType, pNode->type);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkSchemaColId, pNode->colId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkSchemaBytes, pNode->bytes);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddStringToObject(pJson, jkSchemaName, pNode->name);
  }
  
  return code;
}

static int32_t jsonToSchema(const SJson* pJson, void* pObj) {
  SSchema* pNode = (SSchema*)pObj;

  int32_t code = tjsonGetNumberValue(pJson, jkSchemaType, pNode->type);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkSchemaColId, pNode->colId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkSchemaBytes, pNode->bytes);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetStringValue(pJson, jkSchemaName, pNode->name);
  }
  
  return code;
}

static const char* jkTableMetaVgId = "VgId";
static const char* jkTableMetaTableType = "TableType";
static const char* jkTableMetaUid = "Uid";
static const char* jkTableMetaSuid = "Suid";
static const char* jkTableMetaSversion = "Sversion";
static const char* jkTableMetaTversion = "Tversion";
static const char* jkTableMetaComInfo = "ComInfo";
static const char* jkTableMetaColSchemas = "ColSchemas";

static int32_t tableMetaToJson(const void* pObj, SJson* pJson) {
  const STableMeta* pNode = (const STableMeta*)pObj;

  int32_t code = tjsonAddIntegerToObject(pJson, jkTableMetaVgId, pNode->vgId);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkTableMetaTableType, pNode->tableType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkTableMetaUid, pNode->uid);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkTableMetaSuid, pNode->suid);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkTableMetaSversion, pNode->sversion);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkTableMetaTversion, pNode->tversion);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkTableMetaComInfo, tableComInfoToJson, &pNode->tableInfo);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddArray(pJson, jkTableMetaColSchemas, schemaToJson, pNode->schema, sizeof(SSchema), TABLE_TOTAL_COL_NUM(pNode));
  }

  return code;
}

static int32_t jsonToTableMeta(const SJson* pJson, void* pObj) {
  STableMeta* pNode = (STableMeta*)pObj;

  int32_t code = tjsonGetNumberValue(pJson, jkTableMetaVgId, pNode->vgId);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkTableMetaTableType, pNode->tableType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkTableMetaUid, pNode->uid);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkTableMetaSuid, pNode->suid);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkTableMetaSversion, pNode->sversion);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkTableMetaTversion, pNode->tversion);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonToObject(pJson, jkTableMetaComInfo, jsonToTableComInfo, &pNode->tableInfo);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonToArray(pJson, jkTableMetaColSchemas, jsonToSchema, pNode->schema, sizeof(SSchema));
  }

  return code;
}

static const char* jkLogicPlanTargets = "Targets";
static const char* jkLogicPlanConditions = "Conditions";
static const char* jkLogicPlanChildren = "Children";

static int32_t logicPlanNodeToJson(const void* pObj, SJson* pJson) {
  const SLogicNode* pNode = (const SLogicNode*)pObj;

  int32_t code = nodeListToJson(pJson, jkLogicPlanTargets, pNode->pTargets);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkLogicPlanConditions, nodeToJson, pNode->pConditions);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkLogicPlanChildren, pNode->pChildren);
  }

  return code;
}

static int32_t jsonToLogicPlanNode(const SJson* pJson, void* pObj) {
  SLogicNode* pNode = (SLogicNode*)pObj;

  int32_t code = jsonToNodeList(pJson, jkLogicPlanTargets, &pNode->pTargets);
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkLogicPlanConditions, &pNode->pConditions);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkLogicPlanChildren, &pNode->pChildren);
  }

  return code;
}

static const char* jkScanLogicPlanScanCols = "ScanCols";
static const char* jkScanLogicPlanTableMetaSize = "TableMetaSize";
static const char* jkScanLogicPlanTableMeta = "TableMeta";

static int32_t logicScanNodeToJson(const void* pObj, SJson* pJson) {
  const SScanLogicNode* pNode = (const SScanLogicNode*)pObj;

  int32_t code = logicPlanNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkScanLogicPlanScanCols, pNode->pScanCols);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkScanLogicPlanTableMetaSize, TABLE_META_SIZE(pNode->pMeta));
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkScanLogicPlanTableMeta, tableMetaToJson, pNode->pMeta);
  }

  return code;
}

static int32_t jsonToLogicScanNode(const SJson* pJson, void* pObj) {
  SScanLogicNode* pNode = (SScanLogicNode*)pObj;

  int32_t objSize = 0;
  int32_t code = jsonToLogicPlanNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkScanLogicPlanScanCols, &pNode->pScanCols);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkScanLogicPlanTableMetaSize, &objSize);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonMakeObject(pJson, jkScanLogicPlanTableMeta, jsonToTableMeta, (void**)&pNode->pMeta, objSize);
  }

  return code;
}

static const char* jkProjectLogicPlanProjections = "Projections";

static int32_t logicProjectNodeToJson(const void* pObj, SJson* pJson) {
  const SProjectLogicNode* pNode = (const SProjectLogicNode*)pObj;

  int32_t code = logicPlanNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkProjectLogicPlanProjections, pNode->pProjections);
  }

  return code;
}

static int32_t jsonToLogicProjectNode(const SJson* pJson, void* pObj) {
  SProjectLogicNode* pNode = (SProjectLogicNode*)pObj;

  int32_t code = jsonToLogicPlanNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkProjectLogicPlanProjections, &pNode->pProjections);
  }

  return code;
}

static const char* jkJoinLogicPlanJoinType = "JoinType";
static const char* jkJoinLogicPlanOnConditions = "OnConditions";

static int32_t logicJoinNodeToJson(const void* pObj, SJson* pJson) {
  const SJoinLogicNode* pNode = (const SJoinLogicNode*)pObj;

  int32_t code = logicPlanNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkJoinLogicPlanJoinType, pNode->joinType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkJoinLogicPlanOnConditions, nodeToJson, pNode->pOnConditions);
  }

  return code;
}

static const char* jkPhysiPlanOutputDataBlockDesc = "OutputDataBlockDesc";
static const char* jkPhysiPlanConditions = "Conditions";
static const char* jkPhysiPlanChildren = "Children";

static int32_t physicPlanNodeToJson(const void* pObj, SJson* pJson) {
  const SPhysiNode* pNode = (const SPhysiNode*)pObj;

  int32_t code = tjsonAddObject(pJson, jkPhysiPlanOutputDataBlockDesc, nodeToJson, pNode->pOutputDataBlockDesc);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkPhysiPlanConditions, nodeToJson, pNode->pConditions);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkPhysiPlanChildren, pNode->pChildren);
  }

  return code;
}

static int32_t jsonToPhysicPlanNode(const SJson* pJson, void* pObj) {
  SPhysiNode* pNode = (SPhysiNode*)pObj;

  int32_t code = jsonToNodeObject(pJson, jkPhysiPlanOutputDataBlockDesc, (SNode**)&pNode->pOutputDataBlockDesc);
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkPhysiPlanConditions, &pNode->pConditions);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkPhysiPlanChildren, &pNode->pChildren);
  }

  return code;
}

static const char* jkNameType = "NameType";
static const char* jkNameAcctId = "AcctId";
static const char* jkNameDbName = "DbName";
static const char* jkNameTableName = "TableName";

static int32_t nameToJson(const void* pObj, SJson* pJson) {
  const SName* pNode = (const SName*)pObj;

  int32_t code = tjsonAddIntegerToObject(pJson, jkNameType, pNode->type);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkNameAcctId, pNode->acctId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddStringToObject(pJson, jkNameDbName, pNode->dbname);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddStringToObject(pJson, jkNameTableName, pNode->tname);
  }

  return code;
}

static int32_t jsonToName(const SJson* pJson, void* pObj) {
  SName* pNode = (SName*)pObj;

  int32_t code = tjsonGetUTinyIntValue(pJson, jkNameType, &pNode->type);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkNameAcctId, &pNode->acctId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetStringValue(pJson, jkNameDbName, pNode->dbname);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetStringValue(pJson, jkNameTableName, pNode->tname);
  }

  return code;
}

static const char* jkScanPhysiPlanScanCols = "ScanCols";
static const char* jkScanPhysiPlanTableId = "TableId";
static const char* jkScanPhysiPlanTableType = "TableType";
static const char* jkScanPhysiPlanScanOrder = "ScanOrder";
static const char* jkScanPhysiPlanScanCount = "ScanCount";
static const char* jkScanPhysiPlanReverseScanCount = "ReverseScanCount";
static const char* jkScanPhysiPlanTableName = "TableName";

static int32_t physiScanNodeToJson(const void* pObj, SJson* pJson) {
  const STagScanPhysiNode* pNode = (const STagScanPhysiNode*)pObj;

  int32_t code = physicPlanNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkScanPhysiPlanScanCols, pNode->pScanCols);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkScanPhysiPlanTableId, pNode->uid);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkScanPhysiPlanTableType, pNode->tableType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkScanPhysiPlanScanOrder, pNode->order);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkScanPhysiPlanScanCount, pNode->count);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkScanPhysiPlanReverseScanCount, pNode->reverse);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkScanPhysiPlanTableName, nameToJson, &pNode->tableName);
  }

  return code;
}

static int32_t jsonToPhysiScanNode(const SJson* pJson, void* pObj) {
  STagScanPhysiNode* pNode = (STagScanPhysiNode*)pObj;

  int32_t code = jsonToPhysicPlanNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkScanPhysiPlanScanCols, &pNode->pScanCols);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetUBigIntValue(pJson, jkScanPhysiPlanTableId, &pNode->uid);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetTinyIntValue(pJson, jkScanPhysiPlanTableType, &pNode->tableType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkScanPhysiPlanScanOrder, &pNode->order);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkScanPhysiPlanScanCount, &pNode->count);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkScanPhysiPlanReverseScanCount, &pNode->reverse);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonToObject(pJson, jkScanPhysiPlanTableName, jsonToName, &pNode->tableName);
  }

  return code;
}

static int32_t physiTagScanNodeToJson(const void* pObj, SJson* pJson) {
  return physiScanNodeToJson(pObj, pJson);
}

static int32_t jsonToPhysiTagScanNode(const SJson* pJson, void* pObj) {
  return jsonToPhysiScanNode(pJson, pObj);
}

static const char* jkTableScanPhysiPlanScanFlag = "ScanFlag";
static const char* jkTableScanPhysiPlanStartKey = "StartKey";
static const char* jkTableScanPhysiPlanEndKey = "EndKey";

static int32_t physiTableScanNodeToJson(const void* pObj, SJson* pJson) {
  const STableScanPhysiNode* pNode = (const STableScanPhysiNode*)pObj;

  int32_t code = physiScanNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkTableScanPhysiPlanScanFlag, pNode->scanFlag);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkTableScanPhysiPlanStartKey, pNode->scanRange.skey);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkTableScanPhysiPlanEndKey, pNode->scanRange.ekey);
  }

  return code;
}

static int32_t jsonToPhysiTableScanNode(const SJson* pJson, void* pObj) {
  STableScanPhysiNode* pNode = (STableScanPhysiNode*)pObj;

  int32_t code = jsonToPhysiScanNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetUTinyIntValue(pJson, jkTableScanPhysiPlanScanFlag, &pNode->scanFlag);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetBigIntValue(pJson, jkTableScanPhysiPlanStartKey, &pNode->scanRange.skey);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetBigIntValue(pJson, jkTableScanPhysiPlanEndKey, &pNode->scanRange.ekey);
  }

  return code;
}

static int32_t physiStreamScanNodeToJson(const void* pObj, SJson* pJson) {
  return physiScanNodeToJson(pObj, pJson);
}

static int32_t jsonToPhysiStreamScanNode(const SJson* pJson, void* pObj) {
  return jsonToPhysiScanNode(pJson, pObj);
}

static const char* jkEndPointFqdn = "Fqdn";
static const char* jkEndPointPort = "Port";

static int32_t epToJson(const void* pObj, SJson* pJson) {
  const SEp* pNode = (const SEp*)pObj;

  int32_t code = tjsonAddStringToObject(pJson, jkEndPointFqdn, pNode->fqdn);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkEndPointPort, pNode->port);
  }

  return code;
}

static int32_t jsonToEp(const SJson* pJson, void* pObj) {
  SEp* pNode = (SEp*)pObj;

  int32_t code = tjsonGetStringValue(pJson, jkEndPointFqdn, pNode->fqdn);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetSmallIntValue(pJson, jkEndPointPort, &pNode->port);
  }

  return code;
}

static const char* jkEpSetInUse = "InUse";
static const char* jkEpSetNumOfEps = "NumOfEps";
static const char* jkEpSetEps = "Eps";

static int32_t epSetToJson(const void* pObj, SJson* pJson) {
  const SEpSet* pNode = (const SEpSet*)pObj;

  int32_t code = tjsonAddIntegerToObject(pJson, jkEpSetInUse, pNode->inUse);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkEpSetNumOfEps, pNode->numOfEps);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddArray(pJson, jkEpSetEps, epToJson, pNode->eps, sizeof(SEp), pNode->numOfEps);
  }

  return code;
}

static int32_t jsonToEpSet(const SJson* pJson, void* pObj) {
  SEpSet* pNode = (SEpSet*)pObj;

  int32_t code = tjsonGetTinyIntValue(pJson, jkEpSetInUse, &pNode->inUse);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetTinyIntValue(pJson, jkEpSetNumOfEps, &pNode->numOfEps);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonToArray(pJson, jkEpSetEps, jsonToEp, pNode->eps, sizeof(SEp));
  }

  return code;
}

static const char* jkSysTableScanPhysiPlanMnodeEpSet = "MnodeEpSet";
static const char* jkSysTableScanPhysiPlanShowRewrite = "ShowRewrite";
static const char* jkSysTableScanPhysiPlanAccountId = "AccountId";

static int32_t physiSysTableScanNodeToJson(const void* pObj, SJson* pJson) {
  const SSystemTableScanPhysiNode* pNode = (const SSystemTableScanPhysiNode*)pObj;

  int32_t code = physiScanNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkSysTableScanPhysiPlanMnodeEpSet, epSetToJson, &pNode->mgmtEpSet);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddBoolToObject(pJson, jkSysTableScanPhysiPlanShowRewrite, pNode->showRewrite);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkSysTableScanPhysiPlanAccountId, pNode->accountId);
  }

  return code;
}

static int32_t jsonToPhysiSysTableScanNode(const SJson* pJson, void* pObj) {
  SSystemTableScanPhysiNode* pNode = (SSystemTableScanPhysiNode*)pObj;

  int32_t code = jsonToPhysiScanNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonToObject(pJson, jkSysTableScanPhysiPlanMnodeEpSet, jsonToEpSet, &pNode->mgmtEpSet);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetBoolValue(pJson, jkSysTableScanPhysiPlanShowRewrite, &pNode->showRewrite);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkSysTableScanPhysiPlanAccountId, pNode->accountId);
  }

  return code;
}

static const char* jkProjectPhysiPlanProjections = "Projections";

static int32_t physiProjectNodeToJson(const void* pObj, SJson* pJson) {
  const SProjectPhysiNode* pNode = (const SProjectPhysiNode*)pObj;

  int32_t code = physicPlanNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkProjectPhysiPlanProjections, pNode->pProjections);
  }

  return code;
}

static int32_t jsonToPhysiProjectNode(const SJson* pJson, void* pObj) {
  SProjectPhysiNode* pNode = (SProjectPhysiNode*)pObj;

  int32_t code = jsonToPhysicPlanNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkProjectPhysiPlanProjections, &pNode->pProjections);
  }

  return code;
}

static const char* jkJoinPhysiPlanJoinType = "JoinType";
static const char* jkJoinPhysiPlanOnConditions = "OnConditions";
static const char* jkJoinPhysiPlanTargets = "Targets";

static int32_t physiJoinNodeToJson(const void* pObj, SJson* pJson) {
  const SJoinPhysiNode* pNode = (const SJoinPhysiNode*)pObj;

  int32_t code = physicPlanNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkJoinPhysiPlanJoinType, pNode->joinType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkJoinPhysiPlanOnConditions, nodeToJson, pNode->pOnConditions);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkJoinPhysiPlanTargets, pNode->pTargets);
  }

  return code;
}

static int32_t jsonToPhysiJoinNode(const SJson* pJson, void* pObj) {
  SJoinPhysiNode* pNode = (SJoinPhysiNode*)pObj;

  int32_t code = jsonToPhysicPlanNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkJoinPhysiPlanJoinType, pNode->joinType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkJoinPhysiPlanOnConditions, &pNode->pOnConditions);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkJoinPhysiPlanTargets, &pNode->pTargets);
  }

  return code;
}

static const char* jkAggPhysiPlanExprs = "Exprs";
static const char* jkAggPhysiPlanGroupKeys = "GroupKeys";
static const char* jkAggPhysiPlanAggFuncs = "AggFuncs";

static int32_t physiAggNodeToJson(const void* pObj, SJson* pJson) {
  const SAggPhysiNode* pNode = (const SAggPhysiNode*)pObj;

  int32_t code = physicPlanNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkAggPhysiPlanExprs, pNode->pExprs);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkAggPhysiPlanGroupKeys, pNode->pGroupKeys);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkAggPhysiPlanAggFuncs, pNode->pAggFuncs);
  }

  return code;
}

static int32_t jsonToPhysiAggNode(const SJson* pJson, void* pObj) {
  SAggPhysiNode* pNode = (SAggPhysiNode*)pObj;

  int32_t code = jsonToPhysicPlanNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkAggPhysiPlanExprs, &pNode->pExprs);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkAggPhysiPlanGroupKeys, &pNode->pGroupKeys);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkAggPhysiPlanAggFuncs, &pNode->pAggFuncs);
  }

  return code;
}

static const char* jkExchangePhysiPlanSrcGroupId = "SrcGroupId";
static const char* jkExchangePhysiPlanSrcEndPoints = "SrcEndPoints";

static int32_t physiExchangeNodeToJson(const void* pObj, SJson* pJson) {
  const SExchangePhysiNode* pNode = (const SExchangePhysiNode*)pObj;

  int32_t code = physicPlanNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkExchangePhysiPlanSrcGroupId, pNode->srcGroupId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkExchangePhysiPlanSrcEndPoints, pNode->pSrcEndPoints);
  }

  return code;
}

static int32_t jsonToPhysiExchangeNode(const SJson* pJson, void* pObj) {
  SExchangePhysiNode* pNode = (SExchangePhysiNode*)pObj;

  int32_t code = jsonToPhysicPlanNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkExchangePhysiPlanSrcGroupId, &pNode->srcGroupId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkExchangePhysiPlanSrcEndPoints, &pNode->pSrcEndPoints);
  }

  return code;
}

static const char* jkSortPhysiPlanExprs = "Exprs";
static const char* jkSortPhysiPlanSortKeys = "SortKeys";

static int32_t physiSortNodeToJson(const void* pObj, SJson* pJson) {
  const SSortPhysiNode* pNode = (const SSortPhysiNode*)pObj;

  int32_t code = physicPlanNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkSortPhysiPlanExprs, pNode->pExprs);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkSortPhysiPlanSortKeys, pNode->pSortKeys);
  }

  return code;
}

static int32_t jsonToPhysiSortNode(const SJson* pJson, void* pObj) {
  SSortPhysiNode* pNode = (SSortPhysiNode*)pObj;

  int32_t code = jsonToPhysicPlanNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkSortPhysiPlanExprs, &pNode->pExprs);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkSortPhysiPlanSortKeys, &pNode->pSortKeys);
  }

  return code;
}

static const char* jkWindowPhysiPlanExprs = "Exprs";
static const char* jkWindowPhysiPlanFuncs = "Funcs";

static int32_t physiWindowNodeToJson(const void* pObj, SJson* pJson) {
  const SWinodwPhysiNode* pNode = (const SWinodwPhysiNode*)pObj;

  int32_t code = physicPlanNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkWindowPhysiPlanExprs, pNode->pExprs);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkWindowPhysiPlanFuncs, pNode->pFuncs);
  }

  return code;
}

static int32_t jsonToPhysiWindowNode(const SJson* pJson, void* pObj) {
  SWinodwPhysiNode* pNode = (SWinodwPhysiNode*)pObj;

  int32_t code = jsonToPhysicPlanNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkWindowPhysiPlanExprs, &pNode->pExprs);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkWindowPhysiPlanFuncs, &pNode->pFuncs);
  }

  return code;
}

static const char* jkIntervalPhysiPlanInterval = "Interval";
static const char* jkIntervalPhysiPlanOffset = "Offset";
static const char* jkIntervalPhysiPlanSliding = "Sliding";
static const char* jkIntervalPhysiPlanIntervalUnit = "intervalUnit";
static const char* jkIntervalPhysiPlanSlidingUnit  = "slidingUnit";
static const char* jkIntervalPhysiPlanFill = "Fill";
static const char* jkIntervalPhysiPlanTsPk = "TsPk";

static int32_t physiIntervalNodeToJson(const void* pObj, SJson* pJson) {
  const SIntervalPhysiNode* pNode = (const SIntervalPhysiNode*)pObj;

  int32_t code = physiWindowNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkIntervalPhysiPlanInterval, pNode->interval);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkIntervalPhysiPlanOffset, pNode->offset);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkIntervalPhysiPlanSliding, pNode->sliding);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkIntervalPhysiPlanIntervalUnit, pNode->intervalUnit);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkIntervalPhysiPlanSlidingUnit, pNode->slidingUnit);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkIntervalPhysiPlanFill, nodeToJson, pNode->pFill);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkIntervalPhysiPlanTsPk, nodeToJson, pNode->pTspk);
  }

  return code;
}

static int32_t jsonToPhysiIntervalNode(const SJson* pJson, void* pObj) {
  SIntervalPhysiNode* pNode = (SIntervalPhysiNode*)pObj;

  int32_t code = jsonToPhysiWindowNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetBigIntValue(pJson, jkIntervalPhysiPlanInterval, &pNode->interval);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetBigIntValue(pJson, jkIntervalPhysiPlanOffset, &pNode->offset);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetBigIntValue(pJson, jkIntervalPhysiPlanSliding, &pNode->sliding);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetTinyIntValue(pJson, jkIntervalPhysiPlanIntervalUnit, &pNode->intervalUnit);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetTinyIntValue(pJson, jkIntervalPhysiPlanSlidingUnit, &pNode->slidingUnit);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkIntervalPhysiPlanFill, (SNode**)&pNode->pFill);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkIntervalPhysiPlanTsPk, (SNode**)&pNode->pTspk);
  }

  return code;
}

static const char* jkSessionWindowPhysiPlanGap = "Gap";

static int32_t physiSessionWindowNodeToJson(const void* pObj, SJson* pJson) {
  const SSessionWinodwPhysiNode* pNode = (const SSessionWinodwPhysiNode*)pObj;

  int32_t code = physiWindowNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkSessionWindowPhysiPlanGap, pNode->gap);
  }

  return code;
}

static int32_t jsonToPhysiSessionWindowNode(const SJson* pJson, void* pObj) {
  SSessionWinodwPhysiNode* pNode = (SSessionWinodwPhysiNode*)pObj;

  int32_t code = jsonToPhysiWindowNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkSessionWindowPhysiPlanGap, pNode->gap);
  }

  return code;
}

static const char* jkDataSinkInputDataBlockDesc = "InputDataBlockDesc";

static int32_t physicDataSinkNodeToJson(const void* pObj, SJson* pJson) {
  const SDataSinkNode* pNode = (const SDataSinkNode*)pObj;
  return tjsonAddObject(pJson, jkDataSinkInputDataBlockDesc, nodeToJson, pNode->pInputDataBlockDesc);
}

static int32_t jsonToPhysicDataSinkNode(const SJson* pJson, void* pObj) {
  SDataSinkNode* pNode = (SDataSinkNode*)pObj;
  return jsonToNodeObject(pJson, jkDataSinkInputDataBlockDesc, (SNode**)&pNode->pInputDataBlockDesc);
}

static int32_t physiDispatchNodeToJson(const void* pObj, SJson* pJson) {
  return physicDataSinkNodeToJson(pObj, pJson);
}

static int32_t jsonToPhysiDispatchNode(const SJson* pJson, void* pObj) {
  return jsonToPhysicDataSinkNode(pJson, pObj);
}

static const char* jkSubplanIdQueryId = "QueryId";
static const char* jkSubplanIdGroupId = "GroupId";
static const char* jkSubplanIdSubplanId = "SubplanId";

static int32_t subplanIdToJson(const void* pObj, SJson* pJson) {
  const SSubplanId* pNode = (const SSubplanId*)pObj;

  int32_t code = tjsonAddIntegerToObject(pJson, jkSubplanIdQueryId, pNode->queryId);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkSubplanIdGroupId, pNode->groupId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkSubplanIdSubplanId, pNode->subplanId);
  }

  return code;
}

static int32_t jsonToSubplanId(const SJson* pJson, void* pObj) {
  SSubplanId* pNode = (SSubplanId*)pObj;

  int32_t code = tjsonGetUBigIntValue(pJson, jkSubplanIdQueryId, &pNode->queryId);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkSubplanIdGroupId, &pNode->groupId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkSubplanIdSubplanId, &pNode->subplanId);
  }

  return code;
}

static const char* jkQueryNodeAddrId = "Id";
static const char* jkQueryNodeAddrInUse = "InUse";
static const char* jkQueryNodeAddrNumOfEps = "NumOfEps";
static const char* jkQueryNodeAddrEps = "Eps";

static int32_t queryNodeAddrToJson(const void* pObj, SJson* pJson) {
  const SQueryNodeAddr* pNode = (const SQueryNodeAddr*)pObj;

  int32_t code = tjsonAddIntegerToObject(pJson, jkQueryNodeAddrId, pNode->nodeId);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkQueryNodeAddrInUse, pNode->epSet.inUse);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkQueryNodeAddrNumOfEps, pNode->epSet.numOfEps);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddArray(pJson, jkQueryNodeAddrEps, epToJson, pNode->epSet.eps, sizeof(SEp), pNode->epSet.numOfEps);
  }

  return code;
}

static int32_t jsonToQueryNodeAddr(const SJson* pJson, void* pObj) {
  SQueryNodeAddr* pNode = (SQueryNodeAddr*)pObj;

  int32_t code = tjsonGetIntValue(pJson, jkQueryNodeAddrId, &pNode->nodeId);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetTinyIntValue(pJson, jkQueryNodeAddrInUse, &pNode->epSet.inUse);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetTinyIntValue(pJson, jkQueryNodeAddrNumOfEps, &pNode->epSet.numOfEps);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonToArray(pJson, jkQueryNodeAddrEps, jsonToEp, pNode->epSet.eps, sizeof(SEp));
  }

  return code;
}

static const char* jkSubplanId = "Id";
static const char* jkSubplanType = "SubplanType";
static const char* jkSubplanMsgType = "MsgType";
static const char* jkSubplanLevel = "Level";
static const char* jkSubplanNodeAddr = "NodeAddr";
static const char* jkSubplanRootNode = "RootNode";
static const char* jkSubplanDataSink = "DataSink";

static int32_t subplanToJson(const void* pObj, SJson* pJson) {
  const SSubplan* pNode = (const SSubplan*)pObj;

  int32_t code = tjsonAddObject(pJson, jkSubplanId, subplanIdToJson, &pNode->id);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkSubplanType, pNode->subplanType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkSubplanMsgType, pNode->msgType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkSubplanLevel, pNode->level);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkSubplanNodeAddr, queryNodeAddrToJson, &pNode->execNode);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkSubplanRootNode, nodeToJson, pNode->pNode);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkSubplanDataSink, nodeToJson, pNode->pDataSink);
  }

  return code;
}

static int32_t jsonToSubplan(const SJson* pJson, void* pObj) {
  SSubplan* pNode = (SSubplan*)pObj;

  int32_t code = tjsonToObject(pJson, jkSubplanId, jsonToSubplanId, &pNode->id);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkSubplanType, pNode->subplanType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkSubplanMsgType, &pNode->msgType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkSubplanLevel, &pNode->level);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonToObject(pJson, jkSubplanNodeAddr, jsonToQueryNodeAddr, &pNode->execNode);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkSubplanRootNode, (SNode**)&pNode->pNode);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkSubplanDataSink, (SNode**)&pNode->pDataSink);
  }

  return code;
}

static const char* jkPlanQueryId = "QueryId";
static const char* jkPlanNumOfSubplans = "NumOfSubplans";
static const char* jkPlanSubplans = "Subplans";

static int32_t planToJson(const void* pObj, SJson* pJson) {
  const SQueryPlan* pNode = (const SQueryPlan*)pObj;

  int32_t code = tjsonAddIntegerToObject(pJson, jkPlanQueryId, pNode->queryId);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkPlanNumOfSubplans, pNode->numOfSubplans);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkPlanSubplans, pNode->pSubplans);
  }

  return code;
}

static int32_t jsonToPlan(const SJson* pJson, void* pObj) {
  SQueryPlan* pNode = (SQueryPlan*)pObj;

  int32_t code = tjsonGetUBigIntValue(pJson, jkPlanQueryId, &pNode->queryId);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkPlanNumOfSubplans, &pNode->numOfSubplans);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkPlanSubplans, &pNode->pSubplans);
  }

  return code;
}

static const char* jkAggLogicPlanGroupKeys = "GroupKeys";
static const char* jkAggLogicPlanAggFuncs = "AggFuncs";

static int32_t logicAggNodeToJson(const void* pObj, SJson* pJson) {
  const SAggLogicNode* pNode = (const SAggLogicNode*)pObj;

  int32_t code = logicPlanNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkAggLogicPlanGroupKeys, pNode->pGroupKeys);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkAggLogicPlanAggFuncs, pNode->pAggFuncs);
  }

  return code;
}

static const char* jkDataTypeType = "Type";
static const char* jkDataTypePrecision = "Precision";
static const char* jkDataTypeScale = "Scale";
static const char* jkDataTypeDataBytes = "Bytes";

static int32_t dataTypeToJson(const void* pObj, SJson* pJson) {
  const SDataType* pNode = (const SDataType*)pObj;

  int32_t code = tjsonAddIntegerToObject(pJson, jkDataTypeType, pNode->type);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkDataTypePrecision, pNode->precision);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkDataTypeScale, pNode->scale);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkDataTypeDataBytes, pNode->bytes);
  }

  return code;
}

static int32_t jsonToDataType(const SJson* pJson, void* pObj) {
  SDataType* pNode = (SDataType*)pObj;

  int32_t code = tjsonGetUTinyIntValue(pJson, jkDataTypeType, &pNode->type);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetUTinyIntValue(pJson, jkDataTypePrecision, &pNode->precision);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetUTinyIntValue(pJson, jkDataTypeScale, &pNode->scale);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkDataTypeDataBytes, &pNode->bytes);
  }

  return TSDB_CODE_SUCCESS;
}

static const char* jkExprDataType = "DataType";
static const char* jkExprAliasName = "AliasName";

static int32_t exprNodeToJson(const void* pObj, SJson* pJson) {
  const SExprNode* pNode = (const SExprNode*)pObj;

  int32_t code = tjsonAddObject(pJson, jkExprDataType, dataTypeToJson, &pNode->resType);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddStringToObject(pJson, jkExprAliasName, pNode->aliasName);
  }

  return code;
}

static int32_t jsonToExprNode(const SJson* pJson, void* pObj) {
  SExprNode* pNode = (SExprNode*)pObj;

  int32_t code = tjsonToObject(pJson, jkExprDataType, jsonToDataType, &pNode->resType);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetStringValue(pJson, jkExprAliasName, pNode->aliasName);
  }

  return code;
}

static const char* jkColumnTableId = "TableId";
static const char* jkColumnColId = "ColId";
static const char* jkColumnColType = "ColType";
static const char* jkColumnDbName = "DbName";
static const char* jkColumnTableName = "TableName";
static const char* jkColumnTableAlias = "TableAlias";
static const char* jkColumnColName = "ColName";
static const char* jkColumnDataBlockId = "DataBlockId";
static const char* jkColumnSlotId = "SlotId";

static int32_t columnNodeToJson(const void* pObj, SJson* pJson) {
  const SColumnNode* pNode = (const SColumnNode*)pObj;

  int32_t code = exprNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkColumnTableId, pNode->tableId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkColumnColId, pNode->colId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkColumnColType, pNode->colType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddStringToObject(pJson, jkColumnDbName, pNode->dbName);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddStringToObject(pJson, jkColumnTableName, pNode->tableName);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddStringToObject(pJson, jkColumnTableAlias, pNode->tableAlias);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddStringToObject(pJson, jkColumnColName, pNode->colName);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkColumnDataBlockId, pNode->dataBlockId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkColumnSlotId, pNode->slotId);
  }

  return code;
}

static int32_t jsonToColumnNode(const SJson* pJson, void* pObj) {
  SColumnNode* pNode = (SColumnNode*)pObj;

  int32_t code = jsonToExprNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetUBigIntValue(pJson, jkColumnTableId, &pNode->tableId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetSmallIntValue(pJson, jkColumnColId, &pNode->colId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkColumnColType, pNode->colType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetStringValue(pJson, jkColumnDbName, pNode->dbName);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetStringValue(pJson, jkColumnTableName, pNode->tableName);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetStringValue(pJson, jkColumnTableAlias, pNode->tableAlias);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetStringValue(pJson, jkColumnColName, pNode->colName);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetSmallIntValue(pJson, jkColumnDataBlockId, &pNode->dataBlockId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetSmallIntValue(pJson, jkColumnSlotId, &pNode->slotId);
  }

  return code;
}

static const char* jkValueLiteral = "Literal";
static const char* jkValueDuration = "Duration";
static const char* jkValueTranslate = "Translate";
static const char* jkValueDatum = "Datum";

static int32_t datumToJson(const void* pObj, SJson* pJson) {
  const SValueNode* pNode = (const SValueNode*)pObj;

  int32_t code = TSDB_CODE_SUCCESS;
  switch (pNode->node.resType.type) {
    case TSDB_DATA_TYPE_NULL:
      break;
    case TSDB_DATA_TYPE_BOOL:
      code = tjsonAddIntegerToObject(pJson, jkValueDatum, pNode->datum.b);
      break;
    case TSDB_DATA_TYPE_TINYINT:
    case TSDB_DATA_TYPE_SMALLINT:
    case TSDB_DATA_TYPE_INT:
    case TSDB_DATA_TYPE_BIGINT:
    case TSDB_DATA_TYPE_TIMESTAMP:
      code = tjsonAddIntegerToObject(pJson, jkValueDatum, pNode->datum.i);
      break;
    case TSDB_DATA_TYPE_UTINYINT:
    case TSDB_DATA_TYPE_USMALLINT:
    case TSDB_DATA_TYPE_UINT:
    case TSDB_DATA_TYPE_UBIGINT:
      code = tjsonAddIntegerToObject(pJson, jkValueDatum, pNode->datum.u);
      break;
    case TSDB_DATA_TYPE_FLOAT:
    case TSDB_DATA_TYPE_DOUBLE:
      code = tjsonAddDoubleToObject(pJson, jkValueDatum, pNode->datum.d);
      break;
    case TSDB_DATA_TYPE_NCHAR:
    case TSDB_DATA_TYPE_VARCHAR:
    case TSDB_DATA_TYPE_VARBINARY:
      code = tjsonAddStringToObject(pJson, jkValueDatum, varDataVal(pNode->datum.p));
      break;
    case TSDB_DATA_TYPE_JSON:
    case TSDB_DATA_TYPE_DECIMAL:
    case TSDB_DATA_TYPE_BLOB:
      // todo
    default:
      break;
  }

  return code ;
}

static int32_t valueNodeToJson(const void* pObj, SJson* pJson) {
  const SValueNode* pNode = (const SValueNode*)pObj;

  int32_t code = exprNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddStringToObject(pJson, jkValueLiteral, pNode->literal);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddBoolToObject(pJson, jkValueDuration, pNode->isDuration);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddBoolToObject(pJson, jkValueTranslate, pNode->translate);
  }
  if (TSDB_CODE_SUCCESS == code && pNode->translate) {
    code = datumToJson(pNode, pJson);
  }

  return code;
}

static int32_t jsonToDatum(const SJson* pJson, void* pObj) {
  SValueNode* pNode = (SValueNode*)pObj;

  int32_t code = TSDB_CODE_SUCCESS;
  switch (pNode->node.resType.type) {
    case TSDB_DATA_TYPE_NULL:
      break;
    case TSDB_DATA_TYPE_BOOL:
      code = tjsonGetBoolValue(pJson, jkValueDatum, &pNode->datum.b);
      break;
    case TSDB_DATA_TYPE_TINYINT:
    case TSDB_DATA_TYPE_SMALLINT:
    case TSDB_DATA_TYPE_INT:
    case TSDB_DATA_TYPE_BIGINT:
    case TSDB_DATA_TYPE_TIMESTAMP:
      code = tjsonGetBigIntValue(pJson, jkValueDatum, &pNode->datum.i);
      break;
    case TSDB_DATA_TYPE_UTINYINT:
    case TSDB_DATA_TYPE_USMALLINT:
    case TSDB_DATA_TYPE_UINT:
    case TSDB_DATA_TYPE_UBIGINT:
      code = tjsonGetUBigIntValue(pJson, jkValueDatum, &pNode->datum.u);
      break;
    case TSDB_DATA_TYPE_FLOAT:
    case TSDB_DATA_TYPE_DOUBLE:
      code = tjsonGetDoubleValue(pJson, jkValueDatum, &pNode->datum.d);
      break;
    case TSDB_DATA_TYPE_NCHAR:
    case TSDB_DATA_TYPE_VARCHAR:
    case TSDB_DATA_TYPE_VARBINARY: {
      pNode->datum.p = taosMemoryCalloc(1, pNode->node.resType.bytes + VARSTR_HEADER_SIZE + 1);
      if (NULL == pNode->datum.p) {
        code = TSDB_CODE_OUT_OF_MEMORY;
        break;
      }
      varDataSetLen(pNode->datum.p, pNode->node.resType.bytes);
      code = tjsonGetStringValue(pJson, jkValueDatum, varDataVal(pNode->datum.p));
      break;
    }
    case TSDB_DATA_TYPE_JSON:
    case TSDB_DATA_TYPE_DECIMAL:
    case TSDB_DATA_TYPE_BLOB:
      // todo
    default:
      break;
  }

  return code;
}

static int32_t jsonToValueNode(const SJson* pJson, void* pObj) {
  SValueNode* pNode = (SValueNode*)pObj;

  int32_t code = jsonToExprNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonDupStringValue(pJson, jkValueLiteral, &pNode->literal);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetBoolValue(pJson, jkValueDuration, &pNode->isDuration);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetBoolValue(pJson, jkValueTranslate, &pNode->translate);
  }
  if (TSDB_CODE_SUCCESS == code && pNode->translate) {
    code = jsonToDatum(pJson, pNode);
  }

  return code;
}

static const char* jkOperatorType = "OpType";
static const char* jkOperatorLeft = "Left";
static const char* jkOperatorRight = "Right";

static int32_t operatorNodeToJson(const void* pObj, SJson* pJson) {
  const SOperatorNode* pNode = (const SOperatorNode*)pObj;

  int32_t code = exprNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkOperatorType, pNode->opType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkOperatorLeft, nodeToJson, pNode->pLeft);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkOperatorRight, nodeToJson, pNode->pRight);
  }

  return code;
}

static int32_t jsonToOperatorNode(const SJson* pJson, void* pObj) {
  SOperatorNode* pNode = (SOperatorNode*)pObj;

  int32_t code = jsonToExprNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkOperatorType, pNode->opType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkOperatorLeft, &pNode->pLeft);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkOperatorRight, &pNode->pRight);
  }

  return code;
}

static const char* jkLogicCondType = "CondType";
static const char* jkLogicCondParameters = "Parameters";

static int32_t logicConditionNodeToJson(const void* pObj, SJson* pJson) {
  const SLogicConditionNode* pNode = (const SLogicConditionNode*)pObj;

  int32_t code = exprNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkLogicCondType, pNode->condType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkLogicCondParameters, pNode->pParameterList);
  }

  return code;
}

static int32_t jsonToLogicConditionNode(const SJson* pJson, void* pObj) {
  SLogicConditionNode* pNode = (SLogicConditionNode*)pObj;

  int32_t code = jsonToExprNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkLogicCondType, pNode->condType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkLogicCondParameters, &pNode->pParameterList);
  }

  return code;
}

static const char* jkFunctionName = "Name";
static const char* jkFunctionId = "Id";
static const char* jkFunctionType = "Type";
static const char* jkFunctionParameter = "Parameters";

static int32_t functionNodeToJson(const void* pObj, SJson* pJson) {
  const SFunctionNode* pNode = (const SFunctionNode*)pObj;

  int32_t code = exprNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddStringToObject(pJson, jkFunctionName, pNode->functionName);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkFunctionId, pNode->funcId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkFunctionType, pNode->funcType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkFunctionParameter, pNode->pParameterList);
  }

  return code;
}

static int32_t jsonToFunctionNode(const SJson* pJson, void* pObj) {
  SFunctionNode* pNode = (SFunctionNode*)pObj;

  int32_t code = jsonToExprNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetStringValue(pJson, jkFunctionName, pNode->functionName);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkFunctionId, &pNode->funcId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkFunctionType, &pNode->funcType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkFunctionParameter, &pNode->pParameterList);
  }

  return code;
}

static const char* jkTableDbName = "DbName";
static const char* jkTableTableName = "tableName";
static const char* jkTableTableAlias = "tableAlias";

static int32_t tableNodeToJson(const void* pObj, SJson* pJson) {
  const STableNode* pNode = (const STableNode*)pObj;

  int32_t code = exprNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddStringToObject(pJson, jkTableDbName, pNode->dbName);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddStringToObject(pJson, jkTableTableName, pNode->tableName);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddStringToObject(pJson, jkTableTableAlias, pNode->tableAlias);
  }

  return code;
}

static int32_t jsonToTableNode(const SJson* pJson, void* pObj) {
  STableNode* pNode = (STableNode*)pObj;

  int32_t code = jsonToExprNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetStringValue(pJson, jkTableDbName, pNode->dbName);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetStringValue(pJson, jkTableTableName, pNode->tableName);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetStringValue(pJson, jkTableTableAlias, pNode->tableAlias);
  }

  return code;
}

static const char* jkVgroupInfoVgId = "VgId";
static const char* jkVgroupInfoHashBegin = "HashBegin";
static const char* jkVgroupInfoHashEnd = "HashEnd";
static const char* jkVgroupInfoEpSet = "EpSet";
static const char* jkVgroupInfoNumOfTable = "NumOfTable";

static int32_t vgroupInfoToJson(const void* pObj, SJson* pJson) {
  const SVgroupInfo* pNode = (const SVgroupInfo*)pObj;

  int32_t code = tjsonAddIntegerToObject(pJson, jkVgroupInfoVgId, pNode->vgId);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkVgroupInfoHashBegin, pNode->hashBegin);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkVgroupInfoHashEnd, pNode->hashEnd);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkVgroupInfoEpSet, epSetToJson, &pNode->epSet);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkVgroupInfoNumOfTable, pNode->numOfTable);
  }

  return code;
}

static int32_t jsonToVgroupInfo(const SJson* pJson, void* pObj) {
  SVgroupInfo* pNode = (SVgroupInfo*)pObj;

  int32_t code = tjsonGetIntValue(pJson, jkVgroupInfoVgId, &pNode->vgId);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetUIntValue(pJson, jkVgroupInfoHashBegin, &pNode->hashBegin);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetUIntValue(pJson, jkVgroupInfoHashEnd, &pNode->hashEnd);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonToObject(pJson, jkVgroupInfoEpSet, jsonToEpSet, &pNode->epSet);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkVgroupInfoNumOfTable, &pNode->numOfTable);
  }

  return code;
}

static const char* jkVgroupsInfoNum = "Num";
static const char* jkVgroupsInfoVgroups = "Vgroups";

static int32_t vgroupsInfoToJson(const void* pObj, SJson* pJson) {
  const SVgroupsInfo* pNode = (const SVgroupsInfo*)pObj;

  int32_t code = tjsonAddIntegerToObject(pJson, jkVgroupsInfoNum, pNode->numOfVgroups);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddArray(pJson, jkVgroupsInfoVgroups, vgroupInfoToJson, pNode->vgroups, sizeof(SVgroupInfo), pNode->numOfVgroups);
  }

  return code;
}

static int32_t jsonToVgroupsInfo(const SJson* pJson, void* pObj) {
  SVgroupsInfo* pNode = (SVgroupsInfo*)pObj;

  int32_t code = tjsonGetIntValue(pJson, jkVgroupsInfoNum, &pNode->numOfVgroups);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonToArray(pJson, jkVgroupsInfoVgroups, jsonToVgroupInfo, pNode->vgroups, sizeof(SVgroupInfo));
  }

  return code;
}

static const char* jkRealTableMetaSize = "MetaSize";
static const char* jkRealTableMeta = "Meta";
static const char* jkRealTableVgroupsInfoSize = "VgroupsInfoSize";
static const char* jkRealTableVgroupsInfo = "VgroupsInfo";

static int32_t realTableNodeToJson(const void* pObj, SJson* pJson) {
  const SRealTableNode* pNode = (const SRealTableNode*)pObj;

  int32_t code = tableNodeToJson(pObj, pJson);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkRealTableMetaSize, TABLE_META_SIZE(pNode->pMeta));
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkRealTableMeta, tableMetaToJson, pNode->pMeta);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkRealTableVgroupsInfoSize, VGROUPS_INFO_SIZE(pNode->pVgroupList));
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkRealTableVgroupsInfo, vgroupsInfoToJson, pNode->pVgroupList);
  }

  return code;
}

static int32_t jsonToRealTableNode(const SJson* pJson, void* pObj) {
  SRealTableNode* pNode = (SRealTableNode*)pObj;

  int32_t objSize = 0;
  int32_t code = jsonToTableNode(pJson, pObj);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkRealTableMetaSize, &objSize);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonMakeObject(pJson, jkRealTableMeta, jsonToTableMeta, (void**)&pNode->pMeta, objSize);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkRealTableVgroupsInfoSize, &objSize);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonMakeObject(pJson, jkRealTableVgroupsInfo, jsonToVgroupsInfo, (void**)&pNode->pVgroupList, objSize);
  }

  return code;
}

static const char* jkGroupingSetType = "GroupingSetType";
static const char* jkGroupingSetParameter = "Parameters";

static int32_t groupingSetNodeToJson(const void* pObj, SJson* pJson) {
  const SGroupingSetNode* pNode = (const SGroupingSetNode*)pObj;

  int32_t code = tjsonAddIntegerToObject(pJson, jkGroupingSetType, pNode->groupingSetType);
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkGroupingSetParameter, pNode->pParameterList);
  }

  return code;
}

static const char* jkOrderByExprExpr = "Expr";
static const char* jkOrderByExprOrder = "Order";
static const char* jkOrderByExprNullOrder = "NullOrder";

static int32_t orderByExprNodeToJson(const void* pObj, SJson* pJson) {
  const SOrderByExprNode* pNode = (const SOrderByExprNode*)pObj;

  int32_t code = tjsonAddObject(pJson, jkOrderByExprExpr, nodeToJson, pNode->pExpr);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkOrderByExprOrder, pNode->order);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkOrderByExprNullOrder, pNode->nullOrder);
  }

  return code;
}

static int32_t jsonToOrderByExprNode(const SJson* pJson, void* pObj) {
  SOrderByExprNode* pNode = (SOrderByExprNode*)pObj;

  int32_t code = jsonToNodeObject(pJson, jkOrderByExprExpr, &pNode->pExpr);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkOrderByExprOrder, pNode->order);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetNumberValue(pJson, jkOrderByExprNullOrder, pNode->nullOrder);
  }

  return code;
}

static const char* jkIntervalWindowInterval = "Interval";
static const char* jkIntervalWindowOffset = "Offset";
static const char* jkIntervalWindowSliding = "Sliding";
static const char* jkIntervalWindowFill = "Fill";
static const char* jkIntervalWindowTsPk = "TsPk";

static int32_t intervalWindowNodeToJson(const void* pObj, SJson* pJson) {
  const SIntervalWindowNode* pNode = (const SIntervalWindowNode*)pObj;

  int32_t code = tjsonAddObject(pJson, jkIntervalWindowInterval, nodeToJson, pNode->pInterval);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkIntervalWindowOffset, nodeToJson, pNode->pOffset);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkIntervalWindowSliding, nodeToJson, pNode->pSliding);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkIntervalWindowFill, nodeToJson, pNode->pFill);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkIntervalWindowTsPk, nodeToJson, pNode->pCol);
  }

  return code;
}

static int32_t jsonToIntervalWindowNode(const SJson* pJson, void* pObj) {
  SIntervalWindowNode* pNode = (SIntervalWindowNode*)pObj;

  int32_t code = jsonToNodeObject(pJson, jkIntervalWindowInterval, &pNode->pInterval);
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkIntervalWindowOffset, &pNode->pOffset);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkIntervalWindowSliding, &pNode->pSliding);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkIntervalWindowFill, &pNode->pFill);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkIntervalWindowTsPk, &pNode->pCol);
  }

  return code;
}

static const char* jkNodeListDataType = "DataType";
static const char* jkNodeListNodeList = "NodeList";

static int32_t nodeListNodeToJson(const void* pObj, SJson* pJson) {
  const SNodeListNode* pNode = (const SNodeListNode*)pObj;

  int32_t code = tjsonAddObject(pJson, jkNodeListDataType, dataTypeToJson, &pNode->dataType);
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkNodeListNodeList, pNode->pNodeList);
  }

  return code;
}

static int32_t jsonToNodeListNode(const SJson* pJson, void* pObj) {
  SNodeListNode* pNode = (SNodeListNode*)pObj;

  int32_t code = tjsonToObject(pJson, jkNodeListDataType, jsonToDataType, &pNode->dataType);
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkNodeListNodeList, &pNode->pNodeList);
  }

  return code;
}

static const char* jkTargetDataBlockId = "DataBlockId";
static const char* jkTargetSlotId = "SlotId";
static const char* jkTargetExpr = "Expr";

static int32_t targetNodeToJson(const void* pObj, SJson* pJson) {
  const STargetNode* pNode = (const STargetNode*)pObj;

  int32_t code = tjsonAddIntegerToObject(pJson, jkTargetDataBlockId, pNode->dataBlockId);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkTargetSlotId, pNode->slotId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkTargetExpr, nodeToJson, pNode->pExpr);
  }

  return code;
}

static int32_t jsonToTargetNode(const SJson* pJson, void* pObj) {
  STargetNode* pNode = (STargetNode*)pObj;

  int32_t code = tjsonGetSmallIntValue(pJson, jkTargetDataBlockId, &pNode->dataBlockId);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetSmallIntValue(pJson, jkTargetSlotId, &pNode->slotId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkTargetExpr, &pNode->pExpr);
  }

  return code;
}

static const char* jkSlotDescSlotId = "SlotId";
static const char* jkSlotDescDataType = "DataType";
static const char* jkSlotDescReserve = "Reserve";
static const char* jkSlotDescOutput = "Output";

static int32_t slotDescNodeToJson(const void* pObj, SJson* pJson) {
  const SSlotDescNode* pNode = (const SSlotDescNode*)pObj;

  int32_t code = tjsonAddIntegerToObject(pJson, jkSlotDescSlotId, pNode->slotId);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkSlotDescDataType, dataTypeToJson, &pNode->dataType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddBoolToObject(pJson, jkSlotDescReserve, pNode->reserve);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddBoolToObject(pJson, jkSlotDescOutput, pNode->output);
  }

  return code;
}

static int32_t jsonToSlotDescNode(const SJson* pJson, void* pObj) {
  SSlotDescNode* pNode = (SSlotDescNode*)pObj;

  int32_t code = tjsonGetSmallIntValue(pJson, jkSlotDescSlotId, &pNode->slotId);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonToObject(pJson, jkSlotDescDataType, jsonToDataType, &pNode->dataType);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetBoolValue(pJson, jkSlotDescReserve, &pNode->reserve);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetBoolValue(pJson, jkSlotDescOutput, &pNode->output);
  }

  return code;
}

static const char* jkDownstreamSourceAddr = "Addr";
static const char* jkDownstreamSourceTaskId = "TaskId";
static const char* jkDownstreamSourceSchedId = "SchedId";

static int32_t downstreamSourceNodeToJson(const void* pObj, SJson* pJson) {
  const SDownstreamSourceNode* pNode = (const SDownstreamSourceNode*)pObj;

  int32_t code = tjsonAddObject(pJson, jkDownstreamSourceAddr, queryNodeAddrToJson, &pNode->addr);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkDownstreamSourceTaskId, pNode->taskId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkDownstreamSourceSchedId, pNode->schedId);
  }

  return code;
}

static int32_t jsonToDownstreamSourceNode(const SJson* pJson, void* pObj) {
  SDownstreamSourceNode* pNode = (SDownstreamSourceNode*)pObj;

  int32_t code = tjsonToObject(pJson, jkDownstreamSourceAddr, jsonToQueryNodeAddr, &pNode->addr);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetUBigIntValue(pJson, jkDownstreamSourceTaskId, &pNode->taskId);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetUBigIntValue(pJson, jkDownstreamSourceSchedId, &pNode->schedId);
  }

  return code;
}

static const char* jkDataBlockDescDataBlockId = "DataBlockId";
static const char* jkDataBlockDescSlots = "Slots";
static const char* jkDataBlockResultRowSize = "ResultRowSize";

static int32_t dataBlockDescNodeToJson(const void* pObj, SJson* pJson) {
  const SDataBlockDescNode* pNode = (const SDataBlockDescNode*)pObj;

  int32_t code = tjsonAddIntegerToObject(pJson, jkDataBlockDescDataBlockId, pNode->dataBlockId);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddIntegerToObject(pJson, jkDataBlockResultRowSize, pNode->resultRowSize);
  }

  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkDataBlockDescSlots, pNode->pSlots);
  }

  return code;
}

static int32_t jsonToDataBlockDescNode(const SJson* pJson, void* pObj) {
  SDataBlockDescNode* pNode = (SDataBlockDescNode*)pObj;

  int32_t code = tjsonGetSmallIntValue(pJson, jkDataBlockDescDataBlockId, &pNode->dataBlockId);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetIntValue(pJson, jkDataBlockResultRowSize, &pNode->resultRowSize);
  }

  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkDataBlockDescSlots, &pNode->pSlots);
  }

  return code;
}

static const char* jkSelectStmtDistinct = "Distinct";
static const char* jkSelectStmtProjections = "Projections";
static const char* jkSelectStmtFrom = "From";
static const char* jkSelectStmtWhere = "Where";
static const char* jkSelectStmtPartitionBy = "PartitionBy";
static const char* jkSelectStmtWindow = "Window";
static const char* jkSelectStmtGroupBy = "GroupBy";
static const char* jkSelectStmtHaving = "Having";
static const char* jkSelectStmtOrderBy = "OrderBy";
static const char* jkSelectStmtLimit = "Limit";
static const char* jkSelectStmtSlimit = "Slimit";

static int32_t selectStmtTojson(const void* pObj, SJson* pJson) {
  const SSelectStmt* pNode = (const SSelectStmt*)pObj;

  int32_t code = tjsonAddBoolToObject(pJson, jkSelectStmtDistinct, pNode->isDistinct);
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkSelectStmtProjections, pNode->pProjectionList);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkSelectStmtFrom, nodeToJson, pNode->pFromTable);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkSelectStmtWhere, nodeToJson, pNode->pWhere);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkSelectStmtPartitionBy, pNode->pPartitionByList);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkSelectStmtWindow, nodeToJson, pNode->pWindow);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkSelectStmtGroupBy, pNode->pGroupByList);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkSelectStmtHaving, nodeToJson, pNode->pHaving);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = nodeListToJson(pJson, jkSelectStmtOrderBy, pNode->pOrderByList);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkSelectStmtLimit, nodeToJson, pNode->pLimit);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkSelectStmtSlimit, nodeToJson, pNode->pSlimit);
  }

  return code;
}

static int32_t jsonToSelectStmt(const SJson* pJson, void* pObj) {
  SSelectStmt* pNode = (SSelectStmt*)pObj;

  int32_t code = tjsonGetBoolValue(pJson, jkSelectStmtDistinct, &pNode->isDistinct);
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkSelectStmtProjections, &pNode->pProjectionList);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkSelectStmtFrom, &pNode->pFromTable);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkSelectStmtWhere, &pNode->pWhere);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkSelectStmtPartitionBy, &pNode->pPartitionByList);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkSelectStmtWindow, &pNode->pWindow);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkSelectStmtGroupBy, &pNode->pGroupByList);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkSelectStmtHaving, &pNode->pHaving);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeList(pJson, jkSelectStmtOrderBy, &pNode->pOrderByList);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkSelectStmtLimit, &pNode->pLimit);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkSelectStmtSlimit, &pNode->pSlimit);
  }

  return code;
}

static const char* jkCreateTopicStmtTopicName = "TopicName";
static const char* jkCreateTopicStmtSubscribeDbName = "SubscribeDbName";
static const char* jkCreateTopicStmtIgnoreExists = "IgnoreExists";
static const char* jkCreateTopicStmtQuery = "Query";

static int32_t createTopicStmtToJson(const void* pObj, SJson* pJson) {
  const SCreateTopicStmt* pNode = (const SCreateTopicStmt*)pObj;

  int32_t code = tjsonAddStringToObject(pJson, jkCreateTopicStmtTopicName, pNode->topicName);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddStringToObject(pJson, jkCreateTopicStmtSubscribeDbName, pNode->subscribeDbName);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddBoolToObject(pJson, jkCreateTopicStmtIgnoreExists, pNode->ignoreExists);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, jkCreateTopicStmtQuery, nodeToJson, pNode->pQuery);
  }

  return code;
}

static int32_t jsonToCreateTopicStmt(const SJson* pJson, void* pObj) {
  SCreateTopicStmt* pNode = (SCreateTopicStmt*)pObj;

  int32_t code = tjsonGetStringValue(pJson, jkCreateTopicStmtTopicName, pNode->topicName);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetStringValue(pJson, jkCreateTopicStmtSubscribeDbName, pNode->subscribeDbName);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonGetBoolValue(pJson, jkCreateTopicStmtIgnoreExists, &pNode->ignoreExists);
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = jsonToNodeObject(pJson, jkCreateTopicStmtQuery, &pNode->pQuery);
  }

  return code;
}

static int32_t specificNodeToJson(const void* pObj, SJson* pJson) {
  switch (nodeType(pObj)) {
    case QUERY_NODE_COLUMN:
      return columnNodeToJson(pObj, pJson);
    case QUERY_NODE_VALUE:
      return valueNodeToJson(pObj, pJson);
    case QUERY_NODE_OPERATOR:
      return operatorNodeToJson(pObj, pJson);
    case QUERY_NODE_LOGIC_CONDITION:
      return logicConditionNodeToJson(pObj, pJson);
    case QUERY_NODE_FUNCTION:
      return functionNodeToJson(pObj, pJson);
    case QUERY_NODE_REAL_TABLE:
      return realTableNodeToJson(pObj, pJson);
    case QUERY_NODE_TEMP_TABLE:
    case QUERY_NODE_JOIN_TABLE:
      break;
    case QUERY_NODE_GROUPING_SET:
      return groupingSetNodeToJson(pObj, pJson);
    case QUERY_NODE_ORDER_BY_EXPR:
      return orderByExprNodeToJson(pObj, pJson);
    case QUERY_NODE_LIMIT:
    case QUERY_NODE_STATE_WINDOW:
    case QUERY_NODE_SESSION_WINDOW:
      break;
    case QUERY_NODE_INTERVAL_WINDOW:
      return intervalWindowNodeToJson(pObj, pJson);
    case QUERY_NODE_NODE_LIST:
      return nodeListNodeToJson(pObj, pJson);
    case QUERY_NODE_FILL:
    case QUERY_NODE_RAW_EXPR:
      break;
    case QUERY_NODE_TARGET:
      return targetNodeToJson(pObj, pJson);
    case QUERY_NODE_DATABLOCK_DESC:
      return dataBlockDescNodeToJson(pObj, pJson);
    case QUERY_NODE_SLOT_DESC:
      return slotDescNodeToJson(pObj, pJson);
    case QUERY_NODE_COLUMN_DEF:
      break;
    case QUERY_NODE_DOWNSTREAM_SOURCE:
      return downstreamSourceNodeToJson(pObj, pJson);
    case QUERY_NODE_SET_OPERATOR:
      break;
    case QUERY_NODE_SELECT_STMT:
      return selectStmtTojson(pObj, pJson);
    case QUERY_NODE_VNODE_MODIF_STMT:
    case QUERY_NODE_CREATE_DATABASE_STMT:
    case QUERY_NODE_CREATE_TABLE_STMT:
    case QUERY_NODE_USE_DATABASE_STMT:
    case QUERY_NODE_SHOW_DATABASES_STMT:
    case QUERY_NODE_SHOW_TABLES_STMT:
      break;
    case QUERY_NODE_CREATE_TOPIC_STMT:
      return createTopicStmtToJson(pObj, pJson);
    case QUERY_NODE_LOGIC_PLAN_SCAN:
      return logicScanNodeToJson(pObj, pJson);
    case QUERY_NODE_LOGIC_PLAN_JOIN:
      return logicJoinNodeToJson(pObj, pJson);
    case QUERY_NODE_LOGIC_PLAN_AGG:
      return logicAggNodeToJson(pObj, pJson);
    case QUERY_NODE_LOGIC_PLAN_PROJECT:
      return logicProjectNodeToJson(pObj, pJson);
    case QUERY_NODE_LOGIC_PLAN_VNODE_MODIF:
    case QUERY_NODE_LOGIC_SUBPLAN:
    case QUERY_NODE_LOGIC_PLAN:
      break;
    case QUERY_NODE_PHYSICAL_PLAN_TAG_SCAN:
      return physiTagScanNodeToJson(pObj, pJson);
    case QUERY_NODE_PHYSICAL_PLAN_TABLE_SCAN:
      return physiTableScanNodeToJson(pObj, pJson);
    case QUERY_NODE_PHYSICAL_PLAN_STREAM_SCAN:
      return physiStreamScanNodeToJson(pObj, pJson);
    case QUERY_NODE_PHYSICAL_PLAN_SYSTABLE_SCAN:
      return physiSysTableScanNodeToJson(pObj, pJson);
    case QUERY_NODE_PHYSICAL_PLAN_PROJECT:
      return physiProjectNodeToJson(pObj, pJson);
    case QUERY_NODE_PHYSICAL_PLAN_JOIN:
      return physiJoinNodeToJson(pObj, pJson);
    case QUERY_NODE_PHYSICAL_PLAN_AGG:
      return physiAggNodeToJson(pObj, pJson);
    case QUERY_NODE_PHYSICAL_PLAN_EXCHANGE:
      return physiExchangeNodeToJson(pObj, pJson);
    case QUERY_NODE_PHYSICAL_PLAN_SORT:
      return physiSortNodeToJson(pObj, pJson);
    case QUERY_NODE_PHYSICAL_PLAN_INTERVAL:
      return physiIntervalNodeToJson(pObj, pJson);
    case QUERY_NODE_PHYSICAL_PLAN_SESSION_WINDOW:
      return physiSessionWindowNodeToJson(pObj, pJson);
    case QUERY_NODE_PHYSICAL_PLAN_DISPATCH:
      return physiDispatchNodeToJson(pObj, pJson);
    case QUERY_NODE_PHYSICAL_PLAN_INSERT:
      break;
    case QUERY_NODE_PHYSICAL_SUBPLAN:
      return subplanToJson(pObj, pJson);
    case QUERY_NODE_PHYSICAL_PLAN:
      return planToJson(pObj, pJson);
    default:
      // assert(0);
      break;
  }
  nodesWarn("specificNodeToJson unknown node = %s", nodesNodeName(nodeType(pObj)));
  return TSDB_CODE_SUCCESS;
}

static int32_t jsonToSpecificNode(const SJson* pJson, void* pObj) {
  switch (nodeType(pObj)) {
    case QUERY_NODE_COLUMN:
      return jsonToColumnNode(pJson, pObj);
    case QUERY_NODE_VALUE:
      return jsonToValueNode(pJson, pObj);
    case QUERY_NODE_OPERATOR:
      return jsonToOperatorNode(pJson, pObj);
    case QUERY_NODE_LOGIC_CONDITION:
      return jsonToLogicConditionNode(pJson, pObj);
    case QUERY_NODE_FUNCTION:
      return jsonToFunctionNode(pJson, pObj);
    case QUERY_NODE_REAL_TABLE:
      return jsonToRealTableNode(pJson, pObj);
    // case QUERY_NODE_TEMP_TABLE:
    // case QUERY_NODE_JOIN_TABLE:
    //   break;
    // case QUERY_NODE_GROUPING_SET:
    //   return jsonToGroupingSetNode(pJson, pObj);
    case QUERY_NODE_ORDER_BY_EXPR:
      return jsonToOrderByExprNode(pJson, pObj);
    // case QUERY_NODE_LIMIT:
    // case QUERY_NODE_STATE_WINDOW:
    // case QUERY_NODE_SESSION_WINDOW:
    case QUERY_NODE_INTERVAL_WINDOW:
      return jsonToIntervalWindowNode(pJson, pObj);
    case QUERY_NODE_NODE_LIST:
      return jsonToNodeListNode(pJson, pObj);
    // case QUERY_NODE_FILL:
    case QUERY_NODE_TARGET:
      return jsonToTargetNode(pJson, pObj);
    // case QUERY_NODE_RAW_EXPR:
    //   break;
    case QUERY_NODE_DATABLOCK_DESC:
      return jsonToDataBlockDescNode(pJson, pObj);
    case QUERY_NODE_SLOT_DESC:
      return jsonToSlotDescNode(pJson, pObj);
    case QUERY_NODE_DOWNSTREAM_SOURCE:
      return jsonToDownstreamSourceNode(pJson, pObj);
    // case QUERY_NODE_SET_OPERATOR:
    //   break;
    case QUERY_NODE_SELECT_STMT:
      return jsonToSelectStmt(pJson, pObj);
    case QUERY_NODE_CREATE_TOPIC_STMT:
      return jsonToCreateTopicStmt(pJson, pObj);
    case QUERY_NODE_LOGIC_PLAN_SCAN:
      return jsonToLogicScanNode(pJson, pObj);
    // case QUERY_NODE_LOGIC_PLAN_JOIN:
    //   return jsonToLogicJoinNode(pJson, pObj);
    // case QUERY_NODE_LOGIC_PLAN_AGG:
    //   return jsonToLogicAggNode(pJson, pObj);
    case QUERY_NODE_LOGIC_PLAN_PROJECT:
      return jsonToLogicProjectNode(pJson, pObj);
    case QUERY_NODE_PHYSICAL_PLAN_TAG_SCAN:
      return jsonToPhysiTagScanNode(pJson, pObj);
    case QUERY_NODE_PHYSICAL_PLAN_TABLE_SCAN:
      return jsonToPhysiTableScanNode(pJson, pObj);
   case QUERY_NODE_PHYSICAL_PLAN_STREAM_SCAN:
      return jsonToPhysiStreamScanNode(pJson, pObj);
    case QUERY_NODE_PHYSICAL_PLAN_SYSTABLE_SCAN:
      return jsonToPhysiSysTableScanNode(pJson, pObj);  
    case QUERY_NODE_PHYSICAL_PLAN_PROJECT:
      return jsonToPhysiProjectNode(pJson, pObj);
    case QUERY_NODE_PHYSICAL_PLAN_JOIN:
      return jsonToPhysiJoinNode(pJson, pObj);
    case QUERY_NODE_PHYSICAL_PLAN_AGG:
      return jsonToPhysiAggNode(pJson, pObj);
    case QUERY_NODE_PHYSICAL_PLAN_EXCHANGE:
      return jsonToPhysiExchangeNode(pJson, pObj);
    case QUERY_NODE_PHYSICAL_PLAN_SORT:
      return jsonToPhysiSortNode(pJson, pObj);
    case QUERY_NODE_PHYSICAL_PLAN_INTERVAL:
      return jsonToPhysiIntervalNode(pJson, pObj);
    case QUERY_NODE_PHYSICAL_PLAN_SESSION_WINDOW:
      return jsonToPhysiSessionWindowNode(pJson, pObj);
    case QUERY_NODE_PHYSICAL_PLAN_DISPATCH:
      return jsonToPhysiDispatchNode(pJson, pObj);
    case QUERY_NODE_PHYSICAL_SUBPLAN:
      return jsonToSubplan(pJson, pObj);
    case QUERY_NODE_PHYSICAL_PLAN:
      return jsonToPlan(pJson, pObj);
    default:
      assert(0);
      break;
  }
  nodesWarn("jsonToSpecificNode unknown node = %s", nodesNodeName(nodeType(pObj)));
  return TSDB_CODE_SUCCESS;
}

static const char* jkNodeType = "NodeType";
static const char* jkNodeName = "Name";

static int32_t nodeToJson(const void* pObj, SJson* pJson) {
  const SNode* pNode = (const SNode*)pObj;

  int32_t code = tjsonAddIntegerToObject(pJson, jkNodeType, pNode->type);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddStringToObject(pJson, jkNodeName, nodesNodeName(pNode->type));
  }
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonAddObject(pJson, nodesNodeName(pNode->type), specificNodeToJson, pNode);
    if (TSDB_CODE_SUCCESS != code) {
      nodesError("%s ToJson error", nodesNodeName(pNode->type));
    }
  }

  return code;
}

static int32_t jsonToNode(const SJson* pJson, void* pObj) {
  SNode* pNode = (SNode*)pObj;

  int32_t code = tjsonGetNumberValue(pJson, jkNodeType, pNode->type);
  if (TSDB_CODE_SUCCESS == code) {
    code = tjsonToObject(pJson, nodesNodeName(pNode->type), jsonToSpecificNode, pNode);
    if (TSDB_CODE_SUCCESS != code) {
      nodesError("%s toNode error", nodesNodeName(pNode->type));
    }
  }

  return code;
}

static int32_t makeNodeByJson(const SJson* pJson, SNode** pNode) {
  int32_t val = 0;
  int32_t code = tjsonGetIntValue(pJson, jkNodeType, &val);
  if (TSDB_CODE_SUCCESS == code) {
    *pNode = nodesMakeNode(val);
    if (NULL == *pNode) {
      return TSDB_CODE_FAILED;
    }
    code = jsonToNode(pJson, *pNode);
  }

  return code;
}

static int32_t jsonToNodeObject(const SJson* pJson, const char* pName, SNode** pNode) {
  SJson* pJsonNode = tjsonGetObjectItem(pJson, pName);
  if (NULL == pJsonNode) {
    return TSDB_CODE_SUCCESS;
  }
  return makeNodeByJson(pJsonNode, pNode);
}

int32_t nodesNodeToString(const SNodeptr pNode, bool format, char** pStr, int32_t* pLen) {
  if (NULL == pNode || NULL == pStr) {
    terrno = TSDB_CODE_FAILED;
    return TSDB_CODE_FAILED;
  }

  SJson* pJson = tjsonCreateObject();
  if (NULL == pJson) {
    terrno = TSDB_CODE_OUT_OF_MEMORY;
    return TSDB_CODE_OUT_OF_MEMORY;
  }

  int32_t code = nodeToJson(pNode, pJson);
  if (TSDB_CODE_SUCCESS != code) {
    terrno = code;
    return code;
  }

  *pStr = format ? tjsonToString(pJson) : tjsonToUnformattedString(pJson);
  tjsonDelete(pJson);

  if (NULL != pLen) {
    *pLen = strlen(*pStr) + 1;
  }

  return TSDB_CODE_SUCCESS;
}

int32_t nodesStringToNode(const char* pStr, SNode** pNode) {
  if (NULL == pStr || NULL == pNode) {
    return TSDB_CODE_SUCCESS;
  }
  SJson* pJson = tjsonParse(pStr);
  if (NULL == pJson) {
    return TSDB_CODE_FAILED;
  }
  int32_t code = makeNodeByJson(pJson, pNode);
  if (TSDB_CODE_SUCCESS != code) {
    nodesDestroyNode(*pNode);
    *pNode = NULL;
    terrno = code;
    return code;
  }
  return TSDB_CODE_SUCCESS;
}

int32_t nodesListToString(const SNodeList* pList, bool format, char** pStr, int32_t* pLen) {
  if (NULL == pList || NULL == pStr || NULL == pLen) {
    terrno = TSDB_CODE_FAILED;
    return TSDB_CODE_FAILED;
  }

  if (0 == LIST_LENGTH(pList)) {
    return TSDB_CODE_SUCCESS;
  }

  SJson* pJson = tjsonCreateArray();
  if (NULL == pJson) {
    terrno = TSDB_CODE_OUT_OF_MEMORY;
    return TSDB_CODE_OUT_OF_MEMORY;
  }

  SNode* pNode;
  FOREACH(pNode, pList) {
    int32_t code = tjsonAddItem(pJson, nodeToJson, pNode);
    if (TSDB_CODE_SUCCESS != code) {
      terrno = code;
      return code;
    }
  }

  *pStr = format ? tjsonToString(pJson) : tjsonToUnformattedString(pJson);
  tjsonDelete(pJson);

  *pLen = strlen(*pStr) + 1;
  return TSDB_CODE_SUCCESS;
}

int32_t nodesStringToList(const char* pStr, SNodeList** pList) {
  if (NULL == pStr || NULL == pList) {
    return TSDB_CODE_SUCCESS;
  }
  SJson* pJson = tjsonParse(pStr);
  if (NULL == pJson) {
    return TSDB_CODE_FAILED;
  }
  int32_t code = jsonToNodeListImpl(pJson, pList);
  if (TSDB_CODE_SUCCESS != code) {
    nodesDestroyList(*pList);
    terrno = code;
    return code;
  }
  return TSDB_CODE_SUCCESS;
}
