#include "function.h"
#include "functionMgt.h"
#include "nodes.h"
#include "querynodes.h"
#include "sclInt.h"
#include "sclvector.h"
#include "tcommon.h"
#include "tdatablock.h"
#include "scalar.h"
#include "tudf.h"
#include "ttime.h"

int32_t scalarGetOperatorParamNum(EOperatorType type) {
  if (OP_TYPE_IS_NULL == type || OP_TYPE_IS_NOT_NULL == type || OP_TYPE_IS_TRUE == type || OP_TYPE_IS_NOT_TRUE == type
   || OP_TYPE_IS_FALSE == type || OP_TYPE_IS_NOT_FALSE == type || OP_TYPE_IS_UNKNOWN == type || OP_TYPE_IS_NOT_UNKNOWN == type
   || OP_TYPE_MINUS == type) {
    return 1;
  }

  return 2;
}

int32_t sclConvertToTsValueNode(int8_t precision, SValueNode* valueNode) {
  char *timeStr = valueNode->datum.p;
  int32_t code = convertStringToTimestamp(valueNode->node.resType.type, valueNode->datum.p, precision, &valueNode->datum.i);
  if (code != TSDB_CODE_SUCCESS) {
    return code;
  }
  taosMemoryFree(timeStr);
  valueNode->typeData = valueNode->datum.i;

  valueNode->node.resType.type = TSDB_DATA_TYPE_TIMESTAMP;
  valueNode->node.resType.bytes = tDataTypes[TSDB_DATA_TYPE_TIMESTAMP].bytes;

  return TSDB_CODE_SUCCESS;
}

int32_t sclCreateColumnInfoData(SDataType* pType, int32_t numOfRows, SScalarParam* pParam) {
  SColumnInfoData* pColumnData = taosMemoryCalloc(1, sizeof(SColumnInfoData));
  if (pColumnData == NULL) {
    terrno = TSDB_CODE_OUT_OF_MEMORY;
    return terrno;
  }

  pColumnData->info.type      = pType->type;
  pColumnData->info.bytes     = pType->bytes;
  pColumnData->info.scale     = pType->scale;
  pColumnData->info.precision = pType->precision;

  int32_t code = colInfoDataEnsureCapacity(pColumnData, numOfRows);
  if (code != TSDB_CODE_SUCCESS) {
    terrno = TSDB_CODE_OUT_OF_MEMORY;
    taosMemoryFree(pColumnData);
    return terrno;
  }

  pParam->columnData = pColumnData;
  pParam->colAlloced = true;
  return TSDB_CODE_SUCCESS;
}

int32_t doConvertDataType(SValueNode* pValueNode, SScalarParam* out, int32_t* overflow) {
  SScalarParam in = {.numOfRows = 1};
  int32_t code = sclCreateColumnInfoData(&pValueNode->node.resType, 1, &in);
  if (code != TSDB_CODE_SUCCESS) {
    return code;
  }

  colDataAppend(in.columnData, 0, nodesGetValueFromNode(pValueNode), false);

  colInfoDataEnsureCapacity(out->columnData, 1);
  code = vectorConvertImpl(&in, out, overflow);
  sclFreeParam(&in);

  return code;
}

int32_t scalarGenerateSetFromList(void **data, void *pNode, uint32_t type) {
  SHashObj *pObj = taosHashInit(256, taosGetDefaultHashFunction(type), true, false);
  if (NULL == pObj) {
    sclError("taosHashInit failed, size:%d", 256);
    SCL_ERR_RET(TSDB_CODE_QRY_OUT_OF_MEMORY);
  }

  taosHashSetEqualFp(pObj, taosGetDefaultEqualFunction(type));

  int32_t code = 0;
  SNodeListNode *nodeList = (SNodeListNode *)pNode;
  SListCell *cell = nodeList->pNodeList->pHead;
  SScalarParam out = {.columnData = taosMemoryCalloc(1, sizeof(SColumnInfoData))};

  int32_t len = 0;
  void *buf = NULL;

  for (int32_t i = 0; i < nodeList->pNodeList->length; ++i) {
    SValueNode *valueNode = (SValueNode *)cell->pNode;

    if (valueNode->node.resType.type != type) {
      out.columnData->info.type = type;
      if (IS_VAR_DATA_TYPE(type)) {
        if (IS_VAR_DATA_TYPE(valueNode->node.resType.type)) {
          out.columnData->info.bytes = valueNode->node.resType.bytes * TSDB_NCHAR_SIZE;
        } else {
          out.columnData->info.bytes = 64 * TSDB_NCHAR_SIZE;
        }
      } else {
        out.columnData->info.bytes = tDataTypes[type].bytes;
      }

      int32_t overflow = 0;
      code = doConvertDataType(valueNode, &out, &overflow);
      if (code != TSDB_CODE_SUCCESS) {
//        sclError("convert data from %d to %d failed", in.type, out.type);
        SCL_ERR_JRET(code);
      }

      if (overflow) {
        cell = cell->pNext;
        continue;
      }

      if (IS_VAR_DATA_TYPE(type)) {
        buf = colDataGetVarData(out.columnData, 0);
        len = varDataTLen(buf);
      } else {
        len = tDataTypes[type].bytes;
        buf = out.columnData->pData;
      }
    } else {
      buf = nodesGetValueFromNode(valueNode);
      if (IS_VAR_DATA_TYPE(type)) {
        len = varDataTLen(buf);
      } else {
        len = valueNode->node.resType.bytes;
      }
    }

    if (taosHashPut(pObj, buf, (size_t)len, NULL, 0)) {
      sclError("taosHashPut to set failed");
      SCL_ERR_JRET(TSDB_CODE_QRY_OUT_OF_MEMORY);
    }

    colDataDestroy(out.columnData);
    taosMemoryFreeClear(out.columnData);
    out.columnData = taosMemoryCalloc(1, sizeof(SColumnInfoData));

    cell = cell->pNext;
  }

  *data = pObj;

  colDataDestroy(out.columnData);
  taosMemoryFreeClear(out.columnData);
  return TSDB_CODE_SUCCESS;

_return:

  colDataDestroy(out.columnData);
  taosMemoryFreeClear(out.columnData);
  taosHashCleanup(pObj);
  SCL_RET(code);
}

void sclFreeRes(SHashObj *res) {
  SScalarParam *p = NULL;
  void *pIter = taosHashIterate(res, NULL);
  while (pIter) {
    p = (SScalarParam *)pIter;

    if (p) {
      sclFreeParam(p);
    }
    pIter = taosHashIterate(res, pIter);
  }
  taosHashCleanup(res);
}

void sclFreeParam(SScalarParam *param) {
  if (!param->colAlloced) {
    return;
  }
  
  if (param->columnData != NULL) {
    colDataDestroy(param->columnData);
    taosMemoryFreeClear(param->columnData);
  }

  if (param->pHashFilter != NULL) {
    taosHashCleanup(param->pHashFilter);
    param->pHashFilter = NULL;
  }
}

int32_t sclCopyValueNodeValue(SValueNode *pNode, void **res) {
  if (TSDB_DATA_TYPE_NULL == pNode->node.resType.type) {
    return TSDB_CODE_SUCCESS;
  }

  *res = taosMemoryMalloc(pNode->node.resType.bytes);
  if (NULL == (*res)) {
    sclError("malloc %d failed", pNode->node.resType.bytes);
    SCL_ERR_RET(TSDB_CODE_QRY_OUT_OF_MEMORY);
  }

  memcpy(*res, nodesGetValueFromNode(pNode), pNode->node.resType.bytes);
  return TSDB_CODE_SUCCESS;
}

void sclFreeParamList(SScalarParam *param, int32_t paramNum) {
  if (NULL == param) {
    return;
  }

  for (int32_t i = 0; i < paramNum; ++i) {
    SScalarParam* p = param + i;
    sclFreeParam(p);
  }

  taosMemoryFree(param);
}

int32_t sclInitParam(SNode* node, SScalarParam *param, SScalarCtx *ctx, int32_t *rowNum) {
  switch (nodeType(node)) {
    case QUERY_NODE_LEFT_VALUE: {
      SSDataBlock* pb = taosArrayGetP(ctx->pBlockList, 0);
      param->numOfRows = pb->info.rows;
      break;
    }
    case QUERY_NODE_VALUE: {
      SValueNode *valueNode = (SValueNode *)node;

      ASSERT(param->columnData == NULL);
      param->numOfRows = 1;
      /*int32_t code = */sclCreateColumnInfoData(&valueNode->node.resType, 1, param);
      if (TSDB_DATA_TYPE_NULL == valueNode->node.resType.type || valueNode->isNull) {
        colDataAppendNULL(param->columnData, 0);
      } else {
        colDataAppend(param->columnData, 0, nodesGetValueFromNode(valueNode), false);
      }
      break;
    }
    case QUERY_NODE_NODE_LIST: {
      SNodeListNode *nodeList = (SNodeListNode *)node;
      if (LIST_LENGTH(nodeList->pNodeList) <= 0) {
        sclError("invalid length in nodeList, length:%d", LIST_LENGTH(nodeList->pNodeList));
        SCL_RET(TSDB_CODE_QRY_INVALID_INPUT);
      }

      int32_t type = vectorGetConvertType(ctx->type.selfType, ctx->type.peerType);
      if (type == 0) {
        type = nodeList->dataType.type;
      }

      SCL_ERR_RET(scalarGenerateSetFromList((void **)&param->pHashFilter, node, type));
      param->hashValueType = type;
      param->colAlloced = true;
      if (taosHashPut(ctx->pRes, &node, POINTER_BYTES, param, sizeof(*param))) {
        taosHashCleanup(param->pHashFilter);
        param->pHashFilter = NULL;
        sclError("taosHashPut nodeList failed, size:%d", (int32_t)sizeof(*param));
        return TSDB_CODE_QRY_OUT_OF_MEMORY;
      }
      param->colAlloced = false;
      break;
    }
    case QUERY_NODE_COLUMN: {
      if (NULL == ctx->pBlockList) {
        sclError("invalid node type for constant calculating, type:%d, src:%p", nodeType(node), ctx->pBlockList);
        SCL_ERR_RET(TSDB_CODE_QRY_APP_ERROR);
      }

      SColumnNode *ref = (SColumnNode *)node;

      int32_t index = -1;
      for(int32_t i = 0; i < taosArrayGetSize(ctx->pBlockList); ++i) {
        SSDataBlock* pb = taosArrayGetP(ctx->pBlockList, i);
        if (pb->info.blockId == ref->dataBlockId) {
          index = i;
          break;
        }
      }

      if (index == -1) {
        sclError("column tupleId is too big, tupleId:%d, dataBlockNum:%d", ref->dataBlockId, (int32_t)taosArrayGetSize(ctx->pBlockList));
        SCL_ERR_RET(TSDB_CODE_QRY_INVALID_INPUT);
      }

      SSDataBlock *block = *(SSDataBlock **)taosArrayGet(ctx->pBlockList, index);
      if (NULL == block || ref->slotId >= taosArrayGetSize(block->pDataBlock)) {
        sclError("column slotId is too big, slodId:%d, dataBlockSize:%d", ref->slotId, (int32_t)taosArrayGetSize(block->pDataBlock));
        SCL_ERR_RET(TSDB_CODE_QRY_INVALID_INPUT);
      }

      SColumnInfoData *columnData = (SColumnInfoData *)taosArrayGet(block->pDataBlock, ref->slotId);
      param->numOfRows = block->info.rows;
      param->columnData = columnData;
      break;
    }
    case QUERY_NODE_FUNCTION:
    case QUERY_NODE_OPERATOR:
    case QUERY_NODE_LOGIC_CONDITION: {
      SScalarParam *res = (SScalarParam *)taosHashGet(ctx->pRes, &node, POINTER_BYTES);
      if (NULL == res) {
        sclError("no result for node, type:%d, node:%p", nodeType(node), node);
        SCL_ERR_RET(TSDB_CODE_QRY_APP_ERROR);
      }
      *param = *res;
      param->colAlloced = false;
      break;
    }
    default:
      break;
  }

  if (param->numOfRows > *rowNum) {
    if ((1 != param->numOfRows) && (1 < *rowNum)) {
      sclError("different row nums, rowNum:%d, newRowNum:%d", *rowNum, param->numOfRows);
      SCL_ERR_RET(TSDB_CODE_QRY_INVALID_INPUT);
    }

    *rowNum = param->numOfRows;
  }

  param->param = ctx->param;
  return TSDB_CODE_SUCCESS;
}

int32_t sclInitParamList(SScalarParam **pParams, SNodeList* pParamList, SScalarCtx *ctx, int32_t *paramNum, int32_t *rowNum) {
  int32_t code = 0;
  if (NULL == pParamList) {
    if (ctx->pBlockList) {
      SSDataBlock *pBlock = taosArrayGetP(ctx->pBlockList, 0);
      *rowNum = pBlock->info.rows;
    } else {
      *rowNum = 1;
    }

    *paramNum = 1;
  } else {
    *paramNum = pParamList->length;
  }

  SScalarParam *paramList = taosMemoryCalloc(*paramNum, sizeof(SScalarParam));
  if (NULL == paramList) {
    sclError("calloc %d failed", (int32_t)((*paramNum) * sizeof(SScalarParam)));
    SCL_ERR_RET(TSDB_CODE_QRY_OUT_OF_MEMORY);
  }

  if (pParamList) {
    SNode *tnode = NULL;
    int32_t i = 0;
    if (SCL_IS_CONST_CALC(ctx)) {
      WHERE_EACH (tnode, pParamList) {
        if (!SCL_IS_CONST_NODE(tnode)) {
          WHERE_NEXT;
        } else {
          SCL_ERR_JRET(sclInitParam(tnode, &paramList[i], ctx, rowNum));
          ERASE_NODE(pParamList);
        }

        ++i;
      }
    } else {
      FOREACH(tnode, pParamList) {
        SCL_ERR_JRET(sclInitParam(tnode, &paramList[i], ctx, rowNum));
        ++i;
      }
    }
  } else {
    paramList[0].numOfRows = *rowNum;
  }

  if (0 == *rowNum) {
    taosMemoryFreeClear(paramList);
  }

  *pParams = paramList;
  return TSDB_CODE_SUCCESS;

_return:
  taosMemoryFreeClear(paramList);
  SCL_RET(code);
}

int32_t sclGetNodeType(SNode *pNode, SScalarCtx *ctx) {
  if (NULL == pNode) {
    return -1;
  }

  switch ((int)nodeType(pNode)) {
    case QUERY_NODE_VALUE: {
      SValueNode *valueNode = (SValueNode *)pNode;
      return valueNode->node.resType.type;
    }
    case QUERY_NODE_NODE_LIST: {
      SNodeListNode *nodeList = (SNodeListNode *)pNode;
      return nodeList->dataType.type;
    }
    case QUERY_NODE_COLUMN: {
      SColumnNode *colNode = (SColumnNode *)pNode;
      return colNode->node.resType.type;
    }
    case QUERY_NODE_FUNCTION:
    case QUERY_NODE_OPERATOR:
    case QUERY_NODE_LOGIC_CONDITION: {
      SScalarParam *res = (SScalarParam *)taosHashGet(ctx->pRes, &pNode, POINTER_BYTES);
      if (NULL == res) {
        sclError("no result for node, type:%d, node:%p", nodeType(pNode), pNode);
        return -1;
      }
      return res->columnData->info.type;
    }
  }

  return -1;
}


void sclSetOperatorValueType(SOperatorNode *node, SScalarCtx *ctx) {
  ctx->type.opResType = node->node.resType.type;
  ctx->type.selfType = sclGetNodeType(node->pLeft, ctx);
  ctx->type.peerType = sclGetNodeType(node->pRight, ctx);
}

int32_t sclInitOperatorParams(SScalarParam **pParams, SOperatorNode *node, SScalarCtx *ctx, int32_t *rowNum) {
  int32_t code = 0;
  int32_t paramNum = scalarGetOperatorParamNum(node->opType);
  if (NULL == node->pLeft || (paramNum == 2 && NULL == node->pRight)) {
    sclError("invalid operation node, left:%p, right:%p", node->pLeft, node->pRight);
    SCL_ERR_RET(TSDB_CODE_QRY_INVALID_INPUT);
  }

  SScalarParam *paramList = taosMemoryCalloc(paramNum, sizeof(SScalarParam));
  if (NULL == paramList) {
    sclError("calloc %d failed", (int32_t)(paramNum * sizeof(SScalarParam)));
    SCL_ERR_RET(TSDB_CODE_QRY_OUT_OF_MEMORY);
  }

  sclSetOperatorValueType(node, ctx);

  SCL_ERR_JRET(sclInitParam(node->pLeft, &paramList[0], ctx, rowNum));
  if (paramNum > 1) {
    TSWAP(ctx->type.selfType, ctx->type.peerType);
    SCL_ERR_JRET(sclInitParam(node->pRight, &paramList[1], ctx, rowNum));
  }

  *pParams = paramList;
  return TSDB_CODE_SUCCESS;

_return:
  taosMemoryFreeClear(paramList);
  SCL_RET(code);
}

int32_t sclExecFunction(SFunctionNode *node, SScalarCtx *ctx, SScalarParam *output) {
  SScalarParam *params = NULL;
  int32_t rowNum = 0;
  int32_t paramNum = 0;
  int32_t code = 0;
  SCL_ERR_RET(sclInitParamList(&params, node->pParameterList, ctx, &paramNum, &rowNum));

  if (fmIsUserDefinedFunc(node->funcId)) {
    code = callUdfScalarFunc(node->functionName, params, paramNum, output);
    if (code != 0) {
      sclError("fmExecFunction error. callUdfScalarFunc. function name: %s, udf code:%d", node->functionName, code);
      goto _return;
    }
  } else {
    SScalarFuncExecFuncs ffpSet = {0};
    code = fmGetScalarFuncExecFuncs(node->funcId, &ffpSet);
    if (code) {
      sclError("fmGetFuncExecFuncs failed, funcId:%d, code:%s", node->funcId, tstrerror(code));
      SCL_ERR_JRET(code);
    }

    code = sclCreateColumnInfoData(&node->node.resType, rowNum, output);
    if (code != TSDB_CODE_SUCCESS) {
      SCL_ERR_JRET(code);
    }

    code = (*ffpSet.process)(params, paramNum, output);
    if (code) {
      sclError("scalar function exec failed, funcId:%d, code:%s", node->funcId, tstrerror(code));
      SCL_ERR_JRET(code);
    }
  }

_return:

  sclFreeParamList(params, paramNum);
  SCL_RET(code);
}

int32_t sclExecLogic(SLogicConditionNode *node, SScalarCtx *ctx, SScalarParam *output) {
  if (NULL == node->pParameterList || node->pParameterList->length <= 0) {
    sclError("invalid logic parameter list, list:%p, paramNum:%d", node->pParameterList, node->pParameterList ? node->pParameterList->length : 0);
    SCL_ERR_RET(TSDB_CODE_QRY_INVALID_INPUT);
  }

  if (TSDB_DATA_TYPE_BOOL != node->node.resType.type) {
    sclError("invalid logic resType, type:%d", node->node.resType.type);
    SCL_ERR_RET(TSDB_CODE_QRY_INVALID_INPUT);
  }

  if (LOGIC_COND_TYPE_NOT == node->condType && node->pParameterList->length > 1) {
    sclError("invalid NOT operation parameter number, paramNum:%d", node->pParameterList->length);
    SCL_ERR_RET(TSDB_CODE_QRY_INVALID_INPUT);
  }

  SScalarParam *params = NULL;
  int32_t rowNum = 0;
  int32_t paramNum = 0;
  int32_t code = 0;
  SCL_ERR_RET(sclInitParamList(&params, node->pParameterList, ctx, &paramNum, &rowNum));
  if (NULL == params) {
    output->numOfRows = 0;
    return TSDB_CODE_SUCCESS;
  }

  int32_t type = node->node.resType.type;
  output->numOfRows = rowNum;

  SDataType t = {.type = type, .bytes = tDataTypes[type].bytes};
  code = sclCreateColumnInfoData(&t, rowNum, output);
  if (code != TSDB_CODE_SUCCESS) {
    SCL_ERR_JRET(code);
  }

  bool value = false;
  bool complete = true;
  for (int32_t i = 0; i < rowNum; ++i) {
    complete = true;
    for (int32_t m = 0; m < paramNum; ++m) {
      if (NULL == params[m].columnData) {
        complete = false;
        continue;
      }
      char* p = colDataGetData(params[m].columnData, i);
      GET_TYPED_DATA(value, bool, params[m].columnData->info.type, p);

      if (LOGIC_COND_TYPE_AND == node->condType && (false == value)) {
        complete = true;
        break;
      } else if (LOGIC_COND_TYPE_OR == node->condType && value) {
        complete = true;
        break;
      } else if (LOGIC_COND_TYPE_NOT == node->condType) {
        value = !value;
      }
    }

    if (complete) {
      colDataAppend(output->columnData, i, (char*) &value, false);
    }
  }

  if (SCL_IS_CONST_CALC(ctx) && (false == complete)) {
    sclFreeParam(output);
    output->numOfRows = 0;
  }

_return:

  sclFreeParamList(params, paramNum);
  SCL_RET(code);
}

int32_t sclExecOperator(SOperatorNode *node, SScalarCtx *ctx, SScalarParam *output) {
  SScalarParam *params = NULL;
  int32_t rowNum = 0;
  int32_t code = 0;

  // json not support in in operator
  if (nodeType(node->pLeft) == QUERY_NODE_VALUE) {
    SValueNode *valueNode = (SValueNode *)node->pLeft;
    if (valueNode->node.resType.type == TSDB_DATA_TYPE_JSON && (node->opType == OP_TYPE_IN || node->opType == OP_TYPE_NOT_IN)) {
      SCL_RET(TSDB_CODE_QRY_JSON_IN_ERROR);
    }
  }

  SCL_ERR_RET(sclInitOperatorParams(&params, node, ctx, &rowNum));
  if (output->columnData == NULL) {
    code = sclCreateColumnInfoData(&node->node.resType, rowNum, output);
    if (code != TSDB_CODE_SUCCESS) {
      SCL_ERR_JRET(code);
    }
  }

  _bin_scalar_fn_t OperatorFn = getBinScalarOperatorFn(node->opType);

  int32_t paramNum = scalarGetOperatorParamNum(node->opType);
  SScalarParam* pLeft = &params[0];
  SScalarParam* pRight = paramNum > 1 ? &params[1] : NULL;

  terrno = TSDB_CODE_SUCCESS;
  OperatorFn(pLeft, pRight, output, TSDB_ORDER_ASC);
  code = terrno;

_return:

  sclFreeParamList(params, paramNum);
  SCL_RET(code);
}

EDealRes sclRewriteNullInOptr(SNode** pNode, SScalarCtx *ctx, EOperatorType opType) {
  if (opType <= OP_TYPE_CALC_MAX) {
    SValueNode *res = (SValueNode *)nodesMakeNode(QUERY_NODE_VALUE);
    if (NULL == res) {
      sclError("make value node failed");
      ctx->code = TSDB_CODE_QRY_OUT_OF_MEMORY;
      return DEAL_RES_ERROR;
    }

    res->node.resType.type = TSDB_DATA_TYPE_NULL;

    nodesDestroyNode(*pNode);
    *pNode = (SNode*)res;
  } else {
    SValueNode *res = (SValueNode *)nodesMakeNode(QUERY_NODE_VALUE);
    if (NULL == res) {
      sclError("make value node failed");
      ctx->code = TSDB_CODE_QRY_OUT_OF_MEMORY;
      return DEAL_RES_ERROR;
    }

    res->node.resType.type = TSDB_DATA_TYPE_BOOL;
    res->node.resType.bytes = tDataTypes[TSDB_DATA_TYPE_BOOL].bytes;
    res->datum.b = false;

    nodesDestroyNode(*pNode);
    *pNode = (SNode*)res;
  }

  return DEAL_RES_CONTINUE;
}

EDealRes sclAggFuncWalker(SNode* pNode, void* pContext) {
  if (QUERY_NODE_FUNCTION == nodeType(pNode)) {
    SFunctionNode* pFunc = (SFunctionNode*)pNode;
    *(bool*)pContext = fmIsAggFunc(pFunc->funcId);
    if (*(bool*)pContext) {
      return DEAL_RES_END;
    }
  }

  return DEAL_RES_CONTINUE;
}


bool sclContainsAggFuncNode(SNode* pNode) {
  bool aggFunc = false;
  nodesWalkExpr(pNode, sclAggFuncWalker, (void *)&aggFunc);
  return aggFunc;
}

EDealRes sclRewriteNonConstOperator(SNode** pNode, SScalarCtx *ctx) {
  SOperatorNode *node = (SOperatorNode *)*pNode;
  int32_t code = 0;

  if (node->pLeft && (QUERY_NODE_VALUE == nodeType(node->pLeft))) {
    SValueNode *valueNode = (SValueNode *)node->pLeft;
    if (SCL_IS_NULL_VALUE_NODE(valueNode) && (node->opType != OP_TYPE_IS_NULL && node->opType != OP_TYPE_IS_NOT_NULL)
        && (!sclContainsAggFuncNode(node->pRight))) {
      return sclRewriteNullInOptr(pNode, ctx, node->opType);
    }

    if (IS_STR_DATA_TYPE(valueNode->node.resType.type) && node->pRight && nodesIsExprNode(node->pRight)
      && ((SExprNode*)node->pRight)->resType.type == TSDB_DATA_TYPE_TIMESTAMP) {
      code = sclConvertToTsValueNode(((SExprNode*)node->pRight)->resType.precision, valueNode);
      if (code) {
        ctx->code = code;
        return DEAL_RES_ERROR;
      }
    }
  }

  if (node->pRight && (QUERY_NODE_VALUE == nodeType(node->pRight))) {
    SValueNode *valueNode = (SValueNode *)node->pRight;
    if (SCL_IS_NULL_VALUE_NODE(valueNode) && (node->opType != OP_TYPE_IS_NULL && node->opType != OP_TYPE_IS_NOT_NULL)
       && (!sclContainsAggFuncNode(node->pLeft))) {
      return sclRewriteNullInOptr(pNode, ctx, node->opType);
    }

    if (IS_STR_DATA_TYPE(valueNode->node.resType.type) && node->pLeft && nodesIsExprNode(node->pLeft)
      && ((SExprNode*)node->pLeft)->resType.type == TSDB_DATA_TYPE_TIMESTAMP) {
      code = sclConvertToTsValueNode(((SExprNode*)node->pLeft)->resType.precision, valueNode);
      if (code) {
        ctx->code = code;
        return DEAL_RES_ERROR;
      }
    }
  }

  if (node->pRight && (QUERY_NODE_NODE_LIST == nodeType(node->pRight))) {
    SNodeListNode *listNode = (SNodeListNode *)node->pRight;
    SNode* tnode = NULL;
    WHERE_EACH(tnode, listNode->pNodeList) {
      if (SCL_IS_NULL_VALUE_NODE(tnode)) {
        if (node->opType == OP_TYPE_IN) {
          ERASE_NODE(listNode->pNodeList);
          continue;
        } else { //OP_TYPE_NOT_IN
          return sclRewriteNullInOptr(pNode, ctx, node->opType);
        }
      }

      WHERE_NEXT;
    }

    if (listNode->pNodeList->length <= 0) {
      return sclRewriteNullInOptr(pNode, ctx, node->opType);
    }
  }

  return DEAL_RES_CONTINUE;
}

EDealRes sclRewriteFunction(SNode** pNode, SScalarCtx *ctx) {
  SFunctionNode *node = (SFunctionNode *)*pNode;
  SNode* tnode = NULL;
  if (!fmIsScalarFunc(node->funcId) && (!ctx->dual)) {
    return DEAL_RES_CONTINUE;
  }

  FOREACH(tnode, node->pParameterList) {
    if (!SCL_IS_CONST_NODE(tnode)) {
      return DEAL_RES_CONTINUE;
    }
  }

  SScalarParam output = {0};

  ctx->code = sclExecFunction(node, ctx, &output);
  if (ctx->code) {
    return DEAL_RES_ERROR;
  }

  SValueNode *res = (SValueNode *)nodesMakeNode(QUERY_NODE_VALUE);
  if (NULL == res) {
    sclError("make value node failed");
    sclFreeParam(&output);
    ctx->code = TSDB_CODE_QRY_OUT_OF_MEMORY;
    return DEAL_RES_ERROR;
  }

  res->translate = true;

  res->node.resType.type = output.columnData->info.type;
  res->node.resType.bytes = output.columnData->info.bytes;
  res->node.resType.scale = output.columnData->info.scale;
  res->node.resType.precision = output.columnData->info.precision;
  if (colDataIsNull_s(output.columnData, 0)) {
    res->isNull = true;
  } else {
    int32_t type = output.columnData->info.type;
    if (type == TSDB_DATA_TYPE_JSON){
      int32_t len = getJsonValueLen(output.columnData->pData);
      res->datum.p = taosMemoryCalloc(len, 1);
      memcpy(res->datum.p, output.columnData->pData, len);
    } else if (IS_VAR_DATA_TYPE(type)) {
      res->datum.p = taosMemoryCalloc(res->node.resType.bytes + VARSTR_HEADER_SIZE + 1, 1);
      memcpy(res->datum.p, output.columnData->pData, varDataTLen(output.columnData->pData));
    } else {
      nodesSetValueNodeValue(res, output.columnData->pData);
    }
  }

  nodesDestroyNode(*pNode);
  *pNode = (SNode*)res;

  sclFreeParam(&output);
  return DEAL_RES_CONTINUE;
}

EDealRes sclRewriteLogic(SNode** pNode, SScalarCtx *ctx) {
  SLogicConditionNode *node = (SLogicConditionNode *)*pNode;

  SScalarParam output = {0};
  ctx->code = sclExecLogic(node, ctx, &output);
  if (ctx->code) {
    return DEAL_RES_ERROR;
  }

  if (0 == output.numOfRows) {
    return DEAL_RES_CONTINUE;
  }

  SValueNode *res = (SValueNode *)nodesMakeNode(QUERY_NODE_VALUE);
  if (NULL == res) {
    sclError("make value node failed");
    sclFreeParam(&output);
    ctx->code = TSDB_CODE_QRY_OUT_OF_MEMORY;
    return DEAL_RES_ERROR;
  }

  res->node.resType = node->node.resType;
  res->translate = true;

  int32_t type = output.columnData->info.type;
  if (IS_VAR_DATA_TYPE(type)) {
    res->datum.p = output.columnData->pData;
    output.columnData->pData = NULL;
  } else {
    nodesSetValueNodeValue(res, output.columnData->pData);
  }

  nodesDestroyNode(*pNode);
  *pNode = (SNode*)res;

  sclFreeParam(&output);
  return DEAL_RES_CONTINUE;
}

EDealRes sclRewriteOperator(SNode** pNode, SScalarCtx *ctx) {
  SOperatorNode *node = (SOperatorNode *)*pNode;

  if ((!SCL_IS_CONST_NODE(node->pLeft)) || (!SCL_IS_CONST_NODE(node->pRight))) {
    return sclRewriteNonConstOperator(pNode, ctx);
  }

  SScalarParam output = {0};
  ctx->code = sclExecOperator(node, ctx, &output);
  if (ctx->code) {
    return DEAL_RES_ERROR;
  }

  SValueNode *res = (SValueNode *)nodesMakeNode(QUERY_NODE_VALUE);
  if (NULL == res) {
    sclError("make value node failed");
    sclFreeParam(&output);
    ctx->code = TSDB_CODE_QRY_OUT_OF_MEMORY;
    return DEAL_RES_ERROR;
  }

  res->translate = true;

  res->node.resType = node->node.resType;
  if (colDataIsNull_s(output.columnData, 0)) {
    res->isNull = true;
    res->node.resType = node->node.resType;
  } else {
    int32_t type = output.columnData->info.type;
    if (IS_VAR_DATA_TYPE(type)) {  // todo refactor
      res->datum.p = output.columnData->pData;
      output.columnData->pData = NULL;
    } else {
      nodesSetValueNodeValue(res, output.columnData->pData);
    }
  }

  nodesDestroyNode(*pNode);
  *pNode = (SNode*)res;

  sclFreeParam(&output);
  return DEAL_RES_CONTINUE;
}

EDealRes sclConstantsRewriter(SNode** pNode, void* pContext) {
  SScalarCtx *ctx = (SScalarCtx *)pContext;

  if (QUERY_NODE_FUNCTION == nodeType(*pNode)) {
    return sclRewriteFunction(pNode, ctx);
  }

  if (QUERY_NODE_LOGIC_CONDITION == nodeType(*pNode)) {
    return sclRewriteLogic(pNode, ctx);
  }

  if (QUERY_NODE_OPERATOR == nodeType(*pNode)) {
    return sclRewriteOperator(pNode, ctx);
  }

  return DEAL_RES_CONTINUE;
}

EDealRes sclWalkFunction(SNode* pNode, SScalarCtx *ctx) {
  SFunctionNode *node = (SFunctionNode *)pNode;
  SScalarParam output = {0};

  ctx->code = sclExecFunction(node, ctx, &output);
  if (ctx->code) {
    return DEAL_RES_ERROR;
  }

  if (taosHashPut(ctx->pRes, &pNode, POINTER_BYTES, &output, sizeof(output))) {
    ctx->code = TSDB_CODE_QRY_OUT_OF_MEMORY;
    return DEAL_RES_ERROR;
  }

  return DEAL_RES_CONTINUE;
}

EDealRes sclWalkLogic(SNode* pNode, SScalarCtx *ctx) {
  SLogicConditionNode *node = (SLogicConditionNode *)pNode;
  SScalarParam output = {0};

  ctx->code = sclExecLogic(node, ctx, &output);
  if (ctx->code) {
    return DEAL_RES_ERROR;
  }

  if (taosHashPut(ctx->pRes, &pNode, POINTER_BYTES, &output, sizeof(output))) {
    ctx->code = TSDB_CODE_QRY_OUT_OF_MEMORY;
    return DEAL_RES_ERROR;
  }

  return DEAL_RES_CONTINUE;
}

EDealRes sclWalkOperator(SNode* pNode, SScalarCtx *ctx) {
  SOperatorNode *node = (SOperatorNode *)pNode;
  SScalarParam output = {0};

  ctx->code = sclExecOperator(node, ctx, &output);
  if (ctx->code) {
    return DEAL_RES_ERROR;
  }

  if (taosHashPut(ctx->pRes, &pNode, POINTER_BYTES, &output, sizeof(output))) {
    ctx->code = TSDB_CODE_QRY_OUT_OF_MEMORY;
    return DEAL_RES_ERROR;
  }

  return DEAL_RES_CONTINUE;
}

EDealRes sclWalkTarget(SNode* pNode, SScalarCtx *ctx) {
  STargetNode *target = (STargetNode *)pNode;

  if (target->dataBlockId >= taosArrayGetSize(ctx->pBlockList)) {
    sclError("target tupleId is too big, tupleId:%d, dataBlockNum:%d", target->dataBlockId, (int32_t)taosArrayGetSize(ctx->pBlockList));
    ctx->code = TSDB_CODE_QRY_INVALID_INPUT;
    return DEAL_RES_ERROR;
  }

  int32_t index = -1;
  for(int32_t i = 0; i < taosArrayGetSize(ctx->pBlockList); ++i) {
    SSDataBlock* pb = taosArrayGetP(ctx->pBlockList, i);
    if (pb->info.blockId == target->dataBlockId) {
      index = i;
      break;
    }
  }

  if (index == -1) {
    sclError("column tupleId is too big, tupleId:%d, dataBlockNum:%d", target->dataBlockId, (int32_t)taosArrayGetSize(ctx->pBlockList));
    ctx->code = TSDB_CODE_QRY_INVALID_INPUT;
    return DEAL_RES_ERROR;
  }

  SSDataBlock *block = *(SSDataBlock **)taosArrayGet(ctx->pBlockList, index);

  if (target->slotId >= taosArrayGetSize(block->pDataBlock)) {
    sclError("target slot not exist, dataBlockId:%d, slotId:%d, dataBlockNum:%d", target->dataBlockId, target->slotId, (int32_t)taosArrayGetSize(block->pDataBlock));
    ctx->code = TSDB_CODE_QRY_INVALID_INPUT;
    return DEAL_RES_ERROR;
  }

  // if block->pDataBlock is not enough, there are problems if target->slotId bigger than the size of block->pDataBlock,
  SColumnInfoData *col = taosArrayGet(block->pDataBlock, target->slotId);

  SScalarParam *res = (SScalarParam *)taosHashGet(ctx->pRes, (void *)&target->pExpr, POINTER_BYTES);
  if (NULL == res) {
    sclError("no valid res in hash, node:%p, type:%d", target->pExpr, nodeType(target->pExpr));
    ctx->code = TSDB_CODE_QRY_APP_ERROR;
    return DEAL_RES_ERROR;
  }

  colDataAssign(col, res->columnData, res->numOfRows, NULL);
  block->info.rows = res->numOfRows;

  sclFreeParam(res);
  taosHashRemove(ctx->pRes, (void *)&target->pExpr, POINTER_BYTES);
  return DEAL_RES_CONTINUE;
}

EDealRes sclCalcWalker(SNode* pNode, void* pContext) {
  if (QUERY_NODE_VALUE == nodeType(pNode) || QUERY_NODE_NODE_LIST == nodeType(pNode) || QUERY_NODE_COLUMN == nodeType(pNode)|| QUERY_NODE_LEFT_VALUE == nodeType(pNode)) {
    return DEAL_RES_CONTINUE;
  }

  SScalarCtx *ctx = (SScalarCtx *)pContext;
  if (QUERY_NODE_FUNCTION == nodeType(pNode)) {
    return sclWalkFunction(pNode, ctx);
  }

  if (QUERY_NODE_LOGIC_CONDITION == nodeType(pNode)) {
    return sclWalkLogic(pNode, ctx);
  }

  if (QUERY_NODE_OPERATOR == nodeType(pNode)) {
    return sclWalkOperator(pNode, ctx);
  }

  if (QUERY_NODE_TARGET == nodeType(pNode)) {
    return sclWalkTarget(pNode, ctx);
  }

  sclError("invalid node type for scalar calculating, type:%d", nodeType(pNode));
  ctx->code = TSDB_CODE_QRY_INVALID_INPUT;
  return DEAL_RES_ERROR;
}

int32_t sclExtendResRows(SScalarParam *pDst, SScalarParam *pSrc, SArray *pBlockList) {
  SSDataBlock* pb = taosArrayGetP(pBlockList, 0);
  SScalarParam *pLeft = taosMemoryCalloc(1, sizeof(SScalarParam));
  if (NULL == pLeft) {
    sclError("calloc %d failed", (int32_t)sizeof(SScalarParam));
    SCL_ERR_RET(TSDB_CODE_QRY_OUT_OF_MEMORY);
  }

  pLeft->numOfRows = pb->info.rows;
  colInfoDataEnsureCapacity(pDst->columnData, pb->info.rows);

  _bin_scalar_fn_t OperatorFn = getBinScalarOperatorFn(OP_TYPE_ASSIGN);
  OperatorFn(pLeft, pSrc, pDst, TSDB_ORDER_ASC);

  taosMemoryFree(pLeft);

  return TSDB_CODE_SUCCESS;
}

int32_t sclCalcConstants(SNode *pNode, bool dual, SNode **pRes) {
  if (NULL == pNode) {
    SCL_ERR_RET(TSDB_CODE_QRY_INVALID_INPUT);
  }

  int32_t code = 0;
  SScalarCtx ctx = {0};
  ctx.dual = dual;
  ctx.pRes = taosHashInit(SCL_DEFAULT_OP_NUM, taosGetDefaultHashFunction(TSDB_DATA_TYPE_BIGINT), false, HASH_NO_LOCK);
  if (NULL == ctx.pRes) {
    sclError("taosHashInit failed, num:%d", SCL_DEFAULT_OP_NUM);
    SCL_ERR_RET(TSDB_CODE_QRY_OUT_OF_MEMORY);
  }

  nodesRewriteExprPostOrder(&pNode, sclConstantsRewriter, (void *)&ctx);
  SCL_ERR_JRET(ctx.code);
  *pRes = pNode;

_return:

  sclFreeRes(ctx.pRes);
  return code;
}

static int32_t sclGetMinusOperatorResType(SOperatorNode* pOp) {
  if (!IS_MATHABLE_TYPE(((SExprNode*)(pOp->pLeft))->resType.type)) {
    return TSDB_CODE_TSC_INVALID_OPERATION;
  }
  pOp->node.resType.type = TSDB_DATA_TYPE_DOUBLE;
  pOp->node.resType.bytes = tDataTypes[TSDB_DATA_TYPE_DOUBLE].bytes;
  return TSDB_CODE_SUCCESS;
}

static int32_t sclGetMathOperatorResType(SOperatorNode* pOp) {
  SDataType ldt = ((SExprNode*)(pOp->pLeft))->resType;
  SDataType rdt = ((SExprNode*)(pOp->pRight))->resType;
  if ((TSDB_DATA_TYPE_TIMESTAMP == ldt.type && TSDB_DATA_TYPE_TIMESTAMP == rdt.type) ||
      (TSDB_DATA_TYPE_TIMESTAMP == ldt.type && (IS_VAR_DATA_TYPE(rdt.type) || IS_FLOAT_TYPE(rdt.type))) ||
      (TSDB_DATA_TYPE_TIMESTAMP == rdt.type && (IS_VAR_DATA_TYPE(ldt.type) || IS_FLOAT_TYPE(ldt.type)))) {
    return TSDB_CODE_TSC_INVALID_OPERATION;
  }

  if ((TSDB_DATA_TYPE_TIMESTAMP == ldt.type && IS_INTEGER_TYPE(rdt.type)) ||
      (TSDB_DATA_TYPE_TIMESTAMP == rdt.type && IS_INTEGER_TYPE(ldt.type)) ||
      (TSDB_DATA_TYPE_TIMESTAMP == ldt.type && TSDB_DATA_TYPE_BOOL == rdt.type) ||
      (TSDB_DATA_TYPE_TIMESTAMP == rdt.type && TSDB_DATA_TYPE_BOOL == ldt.type)) {
    pOp->node.resType.type = TSDB_DATA_TYPE_TIMESTAMP;
    pOp->node.resType.bytes = tDataTypes[TSDB_DATA_TYPE_TIMESTAMP].bytes;
  } else {
    pOp->node.resType.type = TSDB_DATA_TYPE_DOUBLE;
    pOp->node.resType.bytes = tDataTypes[TSDB_DATA_TYPE_DOUBLE].bytes;
  }
  return TSDB_CODE_SUCCESS;
}

static int32_t sclGetCompOperatorResType(SOperatorNode* pOp) {
  SDataType ldt = ((SExprNode*)(pOp->pLeft))->resType;
  if (OP_TYPE_IN == pOp->opType || OP_TYPE_NOT_IN == pOp->opType) {
    ((SExprNode*)(pOp->pRight))->resType = ldt;
  } else if (nodesIsRegularOp(pOp)) {
    SDataType rdt = ((SExprNode*)(pOp->pRight))->resType;
    if (!IS_VAR_DATA_TYPE(ldt.type) || QUERY_NODE_VALUE != nodeType(pOp->pRight) ||
        (!IS_STR_DATA_TYPE(rdt.type) && (rdt.type != TSDB_DATA_TYPE_NULL))) {
      return TSDB_CODE_TSC_INVALID_OPERATION;
    }
  }
  pOp->node.resType.type = TSDB_DATA_TYPE_BOOL;
  pOp->node.resType.bytes = tDataTypes[TSDB_DATA_TYPE_BOOL].bytes;
  return TSDB_CODE_SUCCESS;
}

static int32_t sclGetJsonOperatorResType(SOperatorNode* pOp) {
  SDataType ldt = ((SExprNode*)(pOp->pLeft))->resType;
  SDataType rdt = ((SExprNode*)(pOp->pRight))->resType;
  if (TSDB_DATA_TYPE_JSON != ldt.type || !IS_STR_DATA_TYPE(rdt.type)) {
    return TSDB_CODE_TSC_INVALID_OPERATION;
  }
  if (pOp->opType == OP_TYPE_JSON_GET_VALUE) {
    pOp->node.resType.type = TSDB_DATA_TYPE_JSON;
  } else if (pOp->opType == OP_TYPE_JSON_CONTAINS) {
    pOp->node.resType.type = TSDB_DATA_TYPE_BOOL;
  }
  pOp->node.resType.bytes = tDataTypes[pOp->node.resType.type].bytes;
  return TSDB_CODE_SUCCESS;
}

static int32_t sclGetBitwiseOperatorResType(SOperatorNode* pOp) {
  pOp->node.resType.type = TSDB_DATA_TYPE_BIGINT;
  pOp->node.resType.bytes = tDataTypes[TSDB_DATA_TYPE_BIGINT].bytes;
  return TSDB_CODE_SUCCESS;
}


int32_t scalarCalculateConstants(SNode *pNode, SNode **pRes) {
  return sclCalcConstants(pNode, false, pRes);
}

int32_t scalarCalculateConstantsFromDual(SNode *pNode, SNode **pRes) {
  return sclCalcConstants(pNode, true, pRes);
}

int32_t scalarCalculate(SNode *pNode, SArray *pBlockList, SScalarParam *pDst) {
  if (NULL == pNode || NULL == pBlockList) {
    SCL_ERR_RET(TSDB_CODE_QRY_INVALID_INPUT);
  }

  int32_t code = 0;
  SScalarCtx ctx = {.code = 0, .pBlockList = pBlockList, .param = pDst ? pDst->param : NULL};

  // TODO: OPT performance
  ctx.pRes = taosHashInit(SCL_DEFAULT_OP_NUM, taosGetDefaultHashFunction(TSDB_DATA_TYPE_BIGINT), false, HASH_NO_LOCK);
  if (NULL == ctx.pRes) {
    sclError("taosHashInit failed, num:%d", SCL_DEFAULT_OP_NUM);
    SCL_ERR_RET(TSDB_CODE_QRY_OUT_OF_MEMORY);
  }

  nodesWalkExprPostOrder(pNode, sclCalcWalker, (void *)&ctx);
  SCL_ERR_JRET(ctx.code);

  if (pDst) {
    SScalarParam *res = (SScalarParam *)taosHashGet(ctx.pRes, (void *)&pNode, POINTER_BYTES);
    if (NULL == res) {
      sclError("no valid res in hash, node:%p, type:%d", pNode, nodeType(pNode));
      SCL_ERR_JRET(TSDB_CODE_QRY_APP_ERROR);
    }

    if (1 == res->numOfRows) {
      SCL_ERR_JRET(sclExtendResRows(pDst, res, pBlockList));
    } else {
      colInfoDataEnsureCapacity(pDst->columnData, res->numOfRows);
      colDataAssign(pDst->columnData, res->columnData, res->numOfRows, NULL);
      pDst->numOfRows = res->numOfRows;
    }

    sclFreeParam(res);
    taosHashRemove(ctx.pRes, (void *)&pNode, POINTER_BYTES);
  }

_return:
  //nodesDestroyNode(pNode);
  sclFreeRes(ctx.pRes);
  return code;
}

int32_t scalarGetOperatorResultType(SOperatorNode* pOp) {
  if (TSDB_DATA_TYPE_BLOB == ((SExprNode*)(pOp->pLeft))->resType.type ||
      (NULL != pOp->pRight && TSDB_DATA_TYPE_BLOB == ((SExprNode*)(pOp->pRight))->resType.type)) {
    return TSDB_CODE_TSC_INVALID_OPERATION;
  }

  switch (pOp->opType) {
    case OP_TYPE_ADD:
    case OP_TYPE_SUB:
    case OP_TYPE_MULTI:
    case OP_TYPE_DIV:
    case OP_TYPE_REM:
      return sclGetMathOperatorResType(pOp);
    case OP_TYPE_MINUS:
      return sclGetMinusOperatorResType(pOp);
    case OP_TYPE_ASSIGN:
      pOp->node.resType = ((SExprNode*)(pOp->pLeft))->resType;
      break;
    case OP_TYPE_BIT_AND:
    case OP_TYPE_BIT_OR:
      return sclGetBitwiseOperatorResType(pOp);
    case OP_TYPE_GREATER_THAN:
    case OP_TYPE_GREATER_EQUAL:
    case OP_TYPE_LOWER_THAN:
    case OP_TYPE_LOWER_EQUAL:
    case OP_TYPE_EQUAL:
    case OP_TYPE_NOT_EQUAL:
    case OP_TYPE_IS_NULL:
    case OP_TYPE_IS_NOT_NULL:
    case OP_TYPE_IS_TRUE:
    case OP_TYPE_IS_FALSE:
    case OP_TYPE_IS_UNKNOWN:
    case OP_TYPE_IS_NOT_TRUE:
    case OP_TYPE_IS_NOT_FALSE:
    case OP_TYPE_IS_NOT_UNKNOWN:
    case OP_TYPE_LIKE:
    case OP_TYPE_NOT_LIKE:
    case OP_TYPE_MATCH:
    case OP_TYPE_NMATCH:
    case OP_TYPE_IN:
    case OP_TYPE_NOT_IN:
      return sclGetCompOperatorResType(pOp);
    case OP_TYPE_JSON_GET_VALUE:
    case OP_TYPE_JSON_CONTAINS:
      return sclGetJsonOperatorResType(pOp);
    default:
      break;
  }

  return TSDB_CODE_SUCCESS;
}
