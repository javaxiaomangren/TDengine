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

#define _DEFAULT_SOURCE
#include "dndNode.h"

static int32_t dndInitVars(SDnode *pDnode, const SDnodeOpt *pOption) {
  pDnode->data.supportVnodes = pOption->numOfSupportVnodes;
  pDnode->data.serverPort = pOption->serverPort;
  pDnode->data.dataDir = strdup(pOption->dataDir);
  pDnode->data.localEp = strdup(pOption->localEp);
  pDnode->data.localFqdn = strdup(pOption->localFqdn);
  pDnode->data.firstEp = strdup(pOption->firstEp);
  pDnode->data.secondEp = strdup(pOption->secondEp);
  pDnode->data.disks = pOption->disks;
  pDnode->data.numOfDisks = pOption->numOfDisks;
  pDnode->ntype = pOption->ntype;
  pDnode->data.rebootTime = taosGetTimestampMs();

  if (pDnode->data.dataDir == NULL || pDnode->data.localEp == NULL || pDnode->data.localFqdn == NULL || pDnode->data.firstEp == NULL ||
      pDnode->data.secondEp == NULL) {
    terrno = TSDB_CODE_OUT_OF_MEMORY;
    return -1;
  }

  if (!tsMultiProcess || pDnode->ntype == NODE_BEGIN || pDnode->ntype == NODE_END) {
    pDnode->data.lockfile = dndCheckRunning(pDnode->data.dataDir);
    if (pDnode->data.lockfile == NULL) {
      return -1;
    }
  }

  return 0;
}

static void dndClearVars(SDnode *pDnode) {
  for (EDndNodeType n = 0; n < NODE_END; ++n) {
    SMgmtWrapper *pMgmt = &pDnode->wrappers[n];
    taosMemoryFreeClear(pMgmt->path);
  }
  if (pDnode->data.lockfile != NULL) {
    taosUnLockFile(pDnode->data.lockfile);
    taosCloseFile(&pDnode->data.lockfile);
    pDnode->data.lockfile = NULL;
  }
  taosMemoryFreeClear(pDnode->data.localEp);
  taosMemoryFreeClear(pDnode->data.localFqdn);
  taosMemoryFreeClear(pDnode->data.firstEp);
  taosMemoryFreeClear(pDnode->data.secondEp);
  taosMemoryFreeClear(pDnode->data.dataDir);
  taosMemoryFree(pDnode);
  dDebug("dnode memory is cleared, data:%p", pDnode);
}

SDnode *dndCreate(const SDnodeOpt *pOption) {
  dDebug("start to create dnode object");
  int32_t code = -1;
  char    path[PATH_MAX] = {0};
  SDnode *pDnode = NULL;

  pDnode = taosMemoryCalloc(1, sizeof(SDnode));
  if (pDnode == NULL) {
    terrno = TSDB_CODE_OUT_OF_MEMORY;
    goto _OVER;
  }

  if (dndInitVars(pDnode, pOption) != 0) {
    dError("failed to init variables since %s", terrstr());
    goto _OVER;
  }

  dndSetStatus(pDnode, DND_STAT_INIT);
  dmSetMgmtFp(&pDnode->wrappers[NODE_BEGIN]);
  mmSetMgmtFp(&pDnode->wrappers[MNODE]);
  vmSetMgmtFp(&pDnode->wrappers[VNODE]);
  qmSetMgmtFp(&pDnode->wrappers[QNODE]);
  smSetMgmtFp(&pDnode->wrappers[SNODE]);
  bmSetMgmtFp(&pDnode->wrappers[BNODE]);

  for (EDndNodeType n = 0; n < NODE_END; ++n) {
    SMgmtWrapper *pWrapper = &pDnode->wrappers[n];
    snprintf(path, sizeof(path), "%s%s%s", pDnode->data.dataDir, TD_DIRSEP, pWrapper->name);
    pWrapper->path = strdup(path);
    pWrapper->procShm.id = -1;
    pWrapper->pDnode = pDnode;
    pWrapper->ntype = n;
    if (pWrapper->path == NULL) {
      terrno = TSDB_CODE_OUT_OF_MEMORY;
      goto _OVER;
    }

    pWrapper->procType = DND_PROC_SINGLE;
    taosInitRWLatch(&pWrapper->latch);
  }

  if (dndInitMsgHandle(pDnode) != 0) {
    dError("failed to init msg handles since %s", terrstr());
    goto _OVER;
  }

  if (dndReadShmFile(pDnode) != 0) {
    dError("failed to read shm file since %s", terrstr());
    goto _OVER;
  }

  SMsgCb msgCb = dndCreateMsgcb(&pDnode->wrappers[0]);
  tmsgSetDefaultMsgCb(&msgCb);

  dInfo("dnode is created, data:%p", pDnode);
  code = 0;

_OVER:
  if (code != 0 && pDnode) {
    dndClearVars(pDnode);
    pDnode = NULL;
    dError("failed to create dnode since %s", terrstr());
  }

  return pDnode;
}

void dndClose(SDnode *pDnode) {
  if (pDnode == NULL) return;

  for (EDndNodeType n = 0; n < NODE_END; ++n) {
    SMgmtWrapper *pWrapper = &pDnode->wrappers[n];
    dndCloseNode(pWrapper);
  }

  dndClearVars(pDnode);
  dInfo("dnode is closed, data:%p", pDnode);
}

void dndHandleEvent(SDnode *pDnode, EDndEvent event) {
  if (event == DND_EVENT_STOP) {
    pDnode->event = event;
  }
}


