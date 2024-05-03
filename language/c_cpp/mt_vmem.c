/*** DES-PWE license ***

   Copyright (c) 2019 by itc

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


   DES-PWE migration (c) 2023 by itc
   current version：v1.0
   License Agreement： apache license 2.0

   

   云版本为开源版提供永久免费提示通道支持，提示通道支持短信、邮件、
   微信等多种方式，欢迎使用

   开发库  mtreport_api 说明:
         用户使用迁移云管系统的c/c++ 开发库，本库使用 标准 c 开发无任何第
		 三方库依赖，用户可以在 c或者 c++ 项目中使用

****/

#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "mt_log.h"
#include "mt_vmem.h"
#include "mt_shm.h"
#include "mt_report.h"

extern MtReport g_mtReport;

inline time_t sv_GetCurTime() {
	return time(NULL);
}

static inline int _MtReport_Flag_IsTimeout(VmemBufNodeFirst *pFirst)
{
	if(pFirst->dwUseFlagStartTime+2 < sv_GetCurTime())
		return 1;
	return 0;
}

int32_t MtReport_InitVmem()
{
	if(g_mtReport.cIsVmemInit)
		return 1;

	if(g_mtReport.pMtShm == NULL)
		return -1;

	int j = 0, iCreate = 0;
	VmemBufNodeFirst *pfirst = NULL;
#define VMEM_CREATE_INIT_SHM(node, n_count, pshm) do { \
	node *pnode = (node*)pshm; \
	for(iCreate=0; iCreate < n_count; iCreate++) \
		pnode[iCreate].wNextNodeIndex = iCreate+1; \
	pfirst = (VmemBufNodeFirst*)pnode; \
	pfirst->wNodeCount = n_count; \
	pfirst->wNodeUsed = 1; \
	pfirst->wFreeNodeIndex = 1; \
}while(0)

#define VMEM_INIT_SHM(node, n_count, check_str, shm) \
	for(j=0; j < VMEM_SHM_COUNT; j++) { \
		g_mtReport.shm[j] = g_mtReport.pMtShm->shm+j*n_count;  \
		if(!g_mtReport.pMtShm->cIsVmemInit) \
			VMEM_CREATE_INIT_SHM(node, n_count, g_mtReport.shm[j]); \
	}
	VMEM_INIT_SHM(VmemBufNode16, VMEM_16_NODE_COUNT, VMEM_SHM_CHECK_STR_16, pV16Shm)
	VMEM_INIT_SHM(VmemBufNode32, VMEM_32_NODE_COUNT, VMEM_SHM_CHECK_STR_32, pV32Shm)
	VMEM_INIT_SHM(VmemBufNode64, VMEM_64_NODE_COUNT, VMEM_SHM_CHECK_STR_64, pV64Shm)
	VMEM_INIT_SHM(VmemBufNode128, VMEM_128_NODE_COUNT, VMEM_SHM_CHECK_STR_128, pV128Shm)
	VMEM_INIT_SHM(VmemBufNode255, VMEM_255_NODE_COUNT, VMEM_SHM_CHECK_STR_255, pV255Shm)
#undef VMEM_INIT_SHM

	g_mtReport.pMtShm->cIsVmemInit = 1;
	g_mtReport.cIsVmemInit = 1;
	return 0;
}

#define VMEM_MAKE_IDEX32(iRet, len, t_idx, n_idx) do { \
	iRet = 0; \
	iRet |= t_idx; iRet <<= 8; \
	iRet |= len; iRet <<= 16; \
	iRet |= n_idx; \
}while(0)

#define SCAN_FREE_NODE do  { \
	for(iScan = 0; iScan < 50; iScan++) { \
		if(pFirst->wScanFreeIndex >= pFirst->wNodeCount) \
			pFirst->wScanFreeIndex = 1; \
		if(pNode[pFirst->wScanFreeIndex].wNextNodeIndex == pFirst->wNodeCount+1) { \
			pNode[pFirst->wScanFreeIndex].wNextNodeIndex = pFirst->wFreeNodeIndex; \
			pFirst->wFreeNodeIndex = pFirst->wScanFreeIndex; \
			pFirst->wNodeUsed--; \
		}\
	    pFirst->wScanFreeIndex++; \
	} \
}while(0)

#define VMEM_GET_FREE_NODE do { \
	wFreeIndex = pFirst->wFreeNodeIndex; \
	pFirst->wFreeNodeIndex = pNode[wFreeIndex].wNextNodeIndex; \
}while(0)

#define __SAVE_DATA_TO_VMEM(fun, len, ntype, pshm) \
static int32_t fun(const char *pdata, int32_t iDataLen) \
{ \
	uint16_t wNeedCount = iDataLen/len + ((iDataLen%len) ? 1 : 0); \
	uint16_t wPreIndex = 0, wFreeIndex = 0, wFirstIndex = 0;\
	int32_t iReadBuf = 0, i = 0, iRet = -101, iScan = 0; \
\
	VmemBufNodeFirst *pFirst = NULL; \
	ntype *pNode = NULL; \
\
	for(i=0; i < VMEM_SHM_COUNT; i++) { \
		pNode = g_mtReport.pshm[i]; \
		pFirst = (VmemBufNodeFirst*)pNode; \
		if(!VARMEM_CAS_GET(&(pFirst->bUseFlag)) && !_MtReport_Flag_IsTimeout(pFirst)) \
			continue; \
		pFirst->dwUseFlagStartTime = sv_GetCurTime(); \
		if(pFirst->wNodeCount < pFirst->wNodeUsed+wNeedCount) { \
		    SCAN_FREE_NODE; \
            if(pFirst->wNodeCount < pFirst->wNodeUsed+wNeedCount) { \
			    VARMEM_CAS_FREE(pFirst->bUseFlag); \
			    continue; \
            } \
		} \
\
		SCAN_FREE_NODE; \
		VMEM_GET_FREE_NODE; \
		wFirstIndex = wFreeIndex;  \
		pFirst->wNodeUsed += wNeedCount; \
		while(wNeedCount > 1) { \
			memcpy(pNode[wFreeIndex].sDataBuf, pdata+iReadBuf, len); \
			pNode[wFreeIndex].bDataLen = len; \
			wPreIndex = wFreeIndex; \
			iReadBuf += len; \
			VMEM_GET_FREE_NODE; \
			pNode[wPreIndex].wNextNodeIndex = wFreeIndex; \
			wNeedCount--; \
		} \
		pNode[wFreeIndex].wNextNodeIndex = pFirst->wNodeCount; \
		memcpy(pNode[wFreeIndex].sDataBuf, pdata+iReadBuf, iDataLen-iReadBuf); \
		pNode[wFreeIndex].bDataLen = iDataLen-iReadBuf; \
		VARMEM_CAS_FREE(pFirst->bUseFlag); \
		VMEM_MAKE_IDEX32(iRet, len, i, wFirstIndex); \
		break; \
	} \
	return iRet; \
}

__SAVE_DATA_TO_VMEM(_SaveToVmem16, 16, VmemBufNode16, pV16Shm)
__SAVE_DATA_TO_VMEM(_SaveToVmem32, 32, VmemBufNode32, pV32Shm)
__SAVE_DATA_TO_VMEM(_SaveToVmem64, 64, VmemBufNode64, pV64Shm)
__SAVE_DATA_TO_VMEM(_SaveToVmem128, 128, VmemBufNode128, pV128Shm)
__SAVE_DATA_TO_VMEM(_SaveToVmem255, 255, VmemBufNode255, pV255Shm)

#undef __SAVE_DATA_TO_VMEM
#undef SCAN_FREE_NODE
#undef VMEM_GET_FREE_NODE
#undef VMEM_MAKE_IDEX32

int32_t MtReport_SaveToVmem(const char *pdata, int32_t iDataLen)
{
	if(!g_mtReport.cIsInit)
		return -101;

	if(iDataLen <= 0 || iDataLen > VMEM_MAX_DATA_LEN)
		return -1;

#define VMEM_SAVE_CHECK_255 1
#define VMEM_SAVE_CHECK_128 2
#define VMEM_SAVE_CHECK_64 4
#define VMEM_SAVE_CHECK_32 8
#define VMEM_SAVE_CHECK_16 16 

	int iRet = -1, iCheckFlag = 0;

	// first try to best type
	if(iRet < 0 && iDataLen >= 255) {
		iRet = _SaveToVmem255(pdata, iDataLen);
		iCheckFlag |= VMEM_SAVE_CHECK_255;
	}
	if(iRet < 0 && iDataLen >= 128) {
		iRet = _SaveToVmem128(pdata, iDataLen);
		iCheckFlag |= VMEM_SAVE_CHECK_128;
	}
	if(iRet < 0 && iDataLen >= 64) {
		iRet = _SaveToVmem64(pdata, iDataLen);
		iCheckFlag |= VMEM_SAVE_CHECK_64;
	}
	if(iRet < 0 && iDataLen >= 32) {
		iRet = _SaveToVmem32(pdata, iDataLen);
		iCheckFlag |= VMEM_SAVE_CHECK_32;
	}
	if(iRet < 0 && iDataLen >= 16) {
		iRet = _SaveToVmem16(pdata, iDataLen);
		iCheckFlag |= VMEM_SAVE_CHECK_16;
	}

	// second try to save all type
	if(iRet < 0 && !(iCheckFlag&VMEM_SAVE_CHECK_16)) {
		iRet = _SaveToVmem16(pdata, iDataLen);
		iCheckFlag |= VMEM_SAVE_CHECK_16;
	}
	if(iRet < 0 && !(iCheckFlag&VMEM_SAVE_CHECK_32)) {
		iRet = _SaveToVmem32(pdata, iDataLen);
		iCheckFlag |= VMEM_SAVE_CHECK_32;
	}
	if(iRet < 0 && !(iCheckFlag&VMEM_SAVE_CHECK_64)) {
		iRet = _SaveToVmem64(pdata, iDataLen);
		iCheckFlag |= VMEM_SAVE_CHECK_64;
	}
	if(iRet < 0 && !(iCheckFlag&VMEM_SAVE_CHECK_128)) {
		iRet = _SaveToVmem128(pdata, iDataLen);
		iCheckFlag |= VMEM_SAVE_CHECK_128;
	}
	if(iRet < 0 && !(iCheckFlag&VMEM_SAVE_CHECK_255)) {
		iRet = _SaveToVmem255(pdata, iDataLen);
		iCheckFlag |= VMEM_SAVE_CHECK_255;
	}
#undef VMEM_SAVE_CHECK_16
#undef VMEM_SAVE_CHECK_32
#undef VMEM_SAVE_CHECK_64
#undef VMEM_SAVE_CHECK_128
#undef VMEM_SAVE_CHECK_255

	return iRet;
}

#define VMEM_PARSE_IDEX32(idx, len, t_idx, n_idx) do { \
	n_idx = (idx & 0xffff); \
	t_idx = ((idx>>24) & 0xff); \
	len = ((idx>>16) & 0xff); \
}while(0)

int32_t MtReport_GetFromVmem(int32_t index, char *pbuf, int32_t *piBufLen)
{
	if(!g_mtReport.cIsInit)
		return -101;

	uint16_t nIdx = 0;
	uint8_t tIdx = 0, len = 0;
	int32_t iBufLen = *piBufLen, iDataLen = 0;
	VMEM_PARSE_IDEX32(index, len, tIdx, nIdx);
	if(tIdx > VMEM_SHM_COUNT)
		return -1;

	VmemBufNodeFirst *pFirst = NULL;

#define VMEM_GET_DATA(type, pshm) do { \
	type *pNode = g_mtReport.pshm[tIdx]; \
	pFirst = (VmemBufNodeFirst*)pNode; \
	if(nIdx >= pFirst->wNodeCount) \
		return -2; \
	do { \
		if(pNode[nIdx].bDataLen > iBufLen) \
			return -4; \
		memcpy(pbuf+iDataLen, pNode[nIdx].sDataBuf, pNode[nIdx].bDataLen); \
		iDataLen += pNode[nIdx].bDataLen; \
		iBufLen -= pNode[nIdx].bDataLen; \
		nIdx = pNode[nIdx].wNextNodeIndex; \
	}while(nIdx < pFirst->wNodeCount); \
	*piBufLen = iDataLen; \
}while(0)

	switch(len) {
		case 16:
			VMEM_GET_DATA(VmemBufNode16, pV16Shm);
			break;
		case 32:
			VMEM_GET_DATA(VmemBufNode32, pV32Shm);
			break;
		case 64:
			VMEM_GET_DATA(VmemBufNode64, pV64Shm);
			break;
		case 128:
			VMEM_GET_DATA(VmemBufNode128, pV128Shm);
			break;
		case 255:
			VMEM_GET_DATA(VmemBufNode255, pV255Shm);
			break;
		default:
			return -3;
	}
#undef VMEM_GET_DATA
	return 0;
}

const char * MtReport_GetFromVmem_Local(int32_t index)
{
	static char sLocalBuf[VMEM_MAX_DATA_LEN+1];
	int32_t iBufLen = VMEM_MAX_DATA_LEN;
	if(MtReport_GetFromVmem(index, sLocalBuf, &iBufLen) == 0) {
		sLocalBuf[iBufLen] = '\0';
		return sLocalBuf;
	}
	return NULL;
}

int32_t MtReport_FreeVmem(int32_t index)
{
	if(!g_mtReport.cIsInit)
		return -101;

	uint16_t nIdx = 0;
	uint8_t tIdx = 0, len = 0, bUseFlagGet = 0;
	int32_t iNextSave = 0;
	VMEM_PARSE_IDEX32(index, len, tIdx, nIdx);
	if(tIdx >= VMEM_SHM_COUNT)
		return -1;

	VmemBufNodeFirst *pFirst = NULL;
/*
   * 释放算法：如果能获取到 useFlag 则释放并加入到空闲链表节点中
   *           否则将 wNextNodeIndex 设置为 wNodeCount+1，此后由
   *           mtreport_agent 根据 wNextNodeIndex 扫描回收放入空闲链表
   */
#define VMEM_FREE_NODE(type, pshm) do { \
	type *pNode = g_mtReport.pshm[tIdx]; \
	pFirst = (VmemBufNodeFirst*)pNode; \
	if(nIdx >= pFirst->wNodeCount) \
		return -2; \
	if(VARMEM_CAS_GET(&(pFirst->bUseFlag)) || _MtReport_Flag_IsTimeout(pFirst)) { \
		bUseFlagGet = 1; \
		pFirst->dwUseFlagStartTime = sv_GetCurTime(); \
	} \
	do { \
		iNextSave = pNode[nIdx].wNextNodeIndex; \
		pNode[nIdx].bDataLen = 0; \
		if(bUseFlagGet) { \
			pNode[nIdx].wNextNodeIndex = pFirst->wFreeNodeIndex; \
			pFirst->wFreeNodeIndex = nIdx; \
			pFirst->wNodeUsed--; \
			if(pFirst->wNodeUsed <= 0) \
				COMM_LIB_BUG; \
		} \
		else \
			pNode[nIdx].wNextNodeIndex = pFirst->wNodeCount+1; \
		nIdx = iNextSave; \
	}while(nIdx < pFirst->wNodeCount); \
	if(bUseFlagGet) \
		VARMEM_CAS_FREE(pFirst->bUseFlag); \
}while(0)

	switch(len) {
		case 16:
			VMEM_FREE_NODE(VmemBufNode16, pV16Shm);
			break;
		case 32:
			VMEM_FREE_NODE(VmemBufNode32, pV32Shm);
			break;
		case 64:
			VMEM_FREE_NODE(VmemBufNode64, pV64Shm);
			break;
		case 128:
			VMEM_FREE_NODE(VmemBufNode128, pV128Shm);
			break;
		case 255:
			VMEM_FREE_NODE(VmemBufNode255, pV255Shm);
			break;
		default:
			return -3;
	}
#undef VMEM_FREE_NODE
	return 0;
}

int32_t MtReport_CheckVmem()
{
	if(!g_mtReport.cIsInit || 0 == g_mtReport.cIsVmemInit || g_mtReport.pMtShm == NULL)
		return -101;

	VmemBufNodeFirst *pFirst = NULL;
	int i = 0, j = 0, iCheckFreeCount = 0;
/*
   * 释放算法：如果能获取到 useFlag 则释放并加入到空闲链表节点中
   *           否则将 wNextNodeIndex 设置为 wNodeCount+1，此后由
   *           mtreport_agent 根据 wNextNodeIndex 扫描回收放入空闲链表
   */
#define CHECK_VMEM_FREE_NODE(type, pshm) do { \
	for(j=0; j < VMEM_SHM_COUNT; j++) { \
		type *pNode = g_mtReport.pshm[j]; \
		pFirst = (VmemBufNodeFirst*)pNode; \
		for(i=1; i < pFirst->wNodeCount; i++) { \
			if(pNode[i].bDataLen == 0 && pNode[i].wNextNodeIndex == pFirst->wNodeCount+1) { \
				if(VARMEM_CAS_GET(&(pFirst->bUseFlag))) { \
					pNode[i].wNextNodeIndex = pFirst->wFreeNodeIndex; \
					pFirst->wFreeNodeIndex = i; \
					pFirst->wNodeUsed--; \
					if(pFirst->wNodeUsed <= 0) \
						COMM_LIB_BUG; \
					VARMEM_CAS_FREE(pFirst->bUseFlag); \
					iCheckFreeCount++; \
				} \
				else { \
					break; \
				} \
			} \
		} \
	} \
}while(0)

	CHECK_VMEM_FREE_NODE(VmemBufNode16, pV16Shm);
	CHECK_VMEM_FREE_NODE(VmemBufNode32, pV32Shm);
	CHECK_VMEM_FREE_NODE(VmemBufNode64, pV64Shm);
	CHECK_VMEM_FREE_NODE(VmemBufNode128, pV128Shm);
	CHECK_VMEM_FREE_NODE(VmemBufNode255, pV255Shm);
#undef CHECK_VMEM_FREE_NODE
	return iCheckFreeCount;
}

int32_t MtReport_GetAndFreeVmem(int32_t index, char *pbuf, int32_t *piBufLen)
{
	if(MtReport_GetFromVmem(index, pbuf, piBufLen) < 0)
		return -1;
	MtReport_FreeVmem(index);
	return 0;
}

// 节点延迟回收函数
// 扫描指定小组中的节点，将其中的空闲节点加入到小组空闲链表中
// iNodeLen 节点长度 - 16, 32 ... 255
// iArrIdx  节点小组索引 0 - VMEM_SHM_COUNT
// iNodeIdx  小组内节点索引 0 - xx_NODE_COUNT
// iScanCount 要扫描的节点数目 
// 返回码：-1 -- 未获取到竞争锁，-2 -- 扫描完成，>= 0 表示下次继续扫描的索引
static int _MtReport_ScanNode_Inner(int iNodeLen, int iArrIdx, int iNodeIdx, int iScanCount)
{
#define MA_SCAN_FREE_NODE(type, pshm) do { \
	if(iArrIdx >= VMEM_SHM_COUNT) \
		return -2; \
	type *pNode = g_mtReport.pshm[iArrIdx]; \
	pFirst = (VmemBufNodeFirst*)pNode; \
	if(!VARMEM_CAS_GET(&(pFirst->bUseFlag)) && !_MtReport_Flag_IsTimeout(pFirst)) \
		return -1; \
	pFirst->dwUseFlagStartTime = sv_GetCurTime(); \
	for(iScan = 0; iNodeIdx < pFirst->wNodeCount && iScan < iScanCount; iScan++) { \
		if(pNode[iNodeIdx].wNextNodeIndex == pFirst->wNodeCount+1) { \
			pNode[iNodeIdx].wNextNodeIndex = pFirst->wFreeNodeIndex; \
			pFirst->wFreeNodeIndex = iNodeIdx; \
			pFirst->wNodeUsed--; \
			if(pFirst->wNodeUsed <= 0) \
                COMM_LIB_BUG; \
		}\
		iNodeIdx++; \
	} \
	VARMEM_CAS_FREE(pFirst->bUseFlag); \
	if(iNodeIdx >= pFirst->wNodeCount) \
		return -2; \
	return iNodeIdx; \
}while(0)

	int iScan = 0;
	VmemBufNodeFirst *pFirst = NULL;
	switch(iNodeLen) {
		case 16:
			MA_SCAN_FREE_NODE(VmemBufNode16, pV16Shm);
			break;
		case 32:
			MA_SCAN_FREE_NODE(VmemBufNode32, pV32Shm);
			break;
		case 64:
			MA_SCAN_FREE_NODE(VmemBufNode64, pV64Shm);
			break;
		case 128:
			MA_SCAN_FREE_NODE(VmemBufNode128, pV128Shm);
			break;
		case 255:
			MA_SCAN_FREE_NODE(VmemBufNode255, pV255Shm);
			break;
		default:
			break;
	}
#undef MA_SCAN_FREE_NODE
	return iNodeIdx;
}

static int _MtReport_GetNextNodeLen(int iCurNodeLen)
{
	if(iCurNodeLen == 16)
		return 32;
	else if(iCurNodeLen == 32)
		return 64;
	else if(iCurNodeLen == 64)
		return 128;
	else if(iCurNodeLen == 128)
		return 255;
	return 16;
}

int MtReport_ScanNode(int iScanCount)
{
	// scan vmem
	static int iCurScanLen = 16;
	static int iCurScanArrIdx = 0;
	static int iNodeIdx = 0;

	int iRet = 0;
	if(iScanCount < 1000)
		iScanCount = 1000;
	iRet = _MtReport_ScanNode_Inner(iCurScanLen, iCurScanArrIdx, iNodeIdx, iScanCount);
	if(iRet == -2) {
		// 换下一阶数组
		iNodeIdx = 0;
		iCurScanArrIdx++;
		if(iCurScanArrIdx >= VMEM_SHM_COUNT) {
			// 换下一个长度的 vmem
			iCurScanArrIdx = 0;
			iCurScanLen = _MtReport_GetNextNodeLen(iCurScanLen);
			if(iCurScanLen == 16) {
				// 全部节点已扫描完, 下次开始新一轮扫描
				return 0;
			}
		}
	}
	else if(iRet >= 0) {
		// 下次从该处继续扫描
		iNodeIdx = iRet;
	}
	// 该轮扫描还有节点未扫描完
	return 1;
}


void MtReport_show_shm()
{
	VmemBufNodeFirst *pfirst = NULL;
	VmemBufNode16 *pNode16 = g_mtReport.pV16Shm[0];
	pfirst = (VmemBufNodeFirst*)pNode16;
	printf("node16\n");
	printf("node count:%d node use:%d free:%d uflag:%d\n", pfirst->wNodeCount,
		pfirst->wNodeUsed, pfirst->wFreeNodeIndex, pfirst->bUseFlag);
	printf("next-1:%d 2:%d 3:%d ... %d:%d\n", pNode16[1].wNextNodeIndex, pNode16[2].wNextNodeIndex,
		pNode16[3].wNextNodeIndex, VMEM_16_NODE_COUNT-1, pNode16[VMEM_16_NODE_COUNT-1].wNextNodeIndex);
	printf("\n\n");

	VmemBufNode32 *pNode = g_mtReport.pV32Shm[0];
	pfirst = (VmemBufNodeFirst*)pNode;
	printf("node32\n");
	printf("node count:%d node use:%d free:%d uflag:%d\n", pfirst->wNodeCount,
		pfirst->wNodeUsed, pfirst->wFreeNodeIndex, pfirst->bUseFlag);
	printf("next-1:%d 2:%d 3:%d ... %d:%d\n", pNode[1].wNextNodeIndex, pNode[2].wNextNodeIndex,
		pNode[3].wNextNodeIndex, VMEM_32_NODE_COUNT-1, pNode[VMEM_32_NODE_COUNT-1].wNextNodeIndex);
	printf("\n\n");

	VmemBufNode64 *pNode64 = g_mtReport.pV64Shm[0];
	pfirst = (VmemBufNodeFirst*)pNode64;
	printf("node64\n");
	printf("node count:%d node use:%d free:%d uflag:%d\n", pfirst->wNodeCount,
		pfirst->wNodeUsed, pfirst->wFreeNodeIndex, pfirst->bUseFlag);
	printf("next-1:%d 2:%d 3:%d ... %d:%d\n", pNode64[1].wNextNodeIndex, pNode64[2].wNextNodeIndex,
		pNode64[3].wNextNodeIndex, VMEM_64_NODE_COUNT-1, pNode64[VMEM_64_NODE_COUNT-1].wNextNodeIndex);
	printf("\n\n");

	VmemBufNode128 *pNode128 = g_mtReport.pV128Shm[0];
	pfirst = (VmemBufNodeFirst*)pNode128;
	printf("node128\n");
	printf("node count:%d node use:%d free:%d uflag:%d\n", pfirst->wNodeCount,
		pfirst->wNodeUsed, pfirst->wFreeNodeIndex, pfirst->bUseFlag);
	printf("next-1:%d 2:%d 3:%d ... %d:%d\n", pNode128[1].wNextNodeIndex, pNode128[2].wNextNodeIndex,
		pNode128[3].wNextNodeIndex, VMEM_128_NODE_COUNT-1, pNode128[VMEM_128_NODE_COUNT-1].wNextNodeIndex);
	printf("\n\n");

	VmemBufNode255 *pNode255 = g_mtReport.pV255Shm[0];
	pfirst = (VmemBufNodeFirst*)pNode255;
	printf("node255\n");
	printf("node count:%d node use:%d free:%d uflag:%d\n", pfirst->wNodeCount,
		pfirst->wNodeUsed, pfirst->wFreeNodeIndex, pfirst->bUseFlag);
	printf("next-1:%d 2:%d 3:%d ... %d:%d\n", pNode255[1].wNextNodeIndex, pNode255[2].wNextNodeIndex,
		pNode255[3].wNextNodeIndex, VMEM_255_NODE_COUNT-1, pNode255[VMEM_255_NODE_COUNT-1].wNextNodeIndex);
	printf("\n\n");
}

