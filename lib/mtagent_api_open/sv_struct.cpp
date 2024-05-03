#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sv_cfg.h>

#include "sv_struct.h"

int SetWTlv(TWTlv *ptlv, uint16_t wType, uint16_t wLen, const char *pValue)
{
	ptlv->wType = htons(wType);
	ptlv->wLen = htons(wLen);
	memcpy(ptlv->sValue, pValue, wLen);
	return sizeof(TWTlv)+wLen;
}

void* GetWTlvValueByType2(uint16_t wType, TPkgBody *pstTlv) 
{
	int i=0;
	TWTlv *ptlv = pstTlv->stTlv;
	for(i=0; i < pstTlv->bTlvNum; i++)
	{
		if(ntohs(ptlv->wType) == wType)
			break;
		ptlv = (TWTlv*)((char*)ptlv+sizeof(TWTlv)+ntohs(ptlv->wLen));
	}
	if(i >= pstTlv->bTlvNum)
		return NULL;
	return pstTlv->stTlv[i].sValue; 
}

TWTlv * GetWTlvType2_list(TPkgBody *pstTlv, int *piStartNum) 
{
	int i=0;
	TWTlv *ptlv = pstTlv->stTlv;
	if(*piStartNum >= pstTlv->bTlvNum || *piStartNum < 0)
		return NULL;
	for(i=0; i < pstTlv->bTlvNum; i++)
	{
		if(i >= *piStartNum) {
			*piStartNum = i+1;
			return ptlv;
		}
		ptlv = (TWTlv*)((char*)ptlv+sizeof(TWTlv)+ntohs(ptlv->wLen));
	}
	return NULL;
}

TWTlv * GetWTlvByType2_list(uint16_t wType, TPkgBody *pstTlv, int *piStartNum) 
{
	int i=0;
	TWTlv *ptlv = pstTlv->stTlv;
	if(*piStartNum >= pstTlv->bTlvNum || *piStartNum < 0)
		return NULL;
	for(i=0; i < pstTlv->bTlvNum; i++)
	{
		if(i >= *piStartNum && ntohs(ptlv->wType) == wType) {
			*piStartNum = i+1;
			return ptlv;
		}
		ptlv = (TWTlv*)((char*)ptlv+sizeof(TWTlv)+ntohs(ptlv->wLen));
	}
	return NULL;
}

TWTlv * GetWTlvByType2(uint16_t wType, TPkgBody *pstTlv) 
{
	int i=0;
	TWTlv *ptlv = pstTlv->stTlv;
	for(i=0; i < pstTlv->bTlvNum; i++)
	{
		if(ntohs(ptlv->wType) == wType)
			return ptlv;
		ptlv = (TWTlv*)((char*)ptlv+sizeof(TWTlv)+ntohs(ptlv->wLen));
	}
	return NULL;
}

void* GetWTlvValue(uint16_t wType, TWTlv *ptlv, int iTlvLen)
{
	int i=0;
	for(i=0; i < iTlvLen-1;)
	{
		if(ntohs(ptlv->wType) == wType)
			return ptlv->sValue;
		ptlv = (TWTlv*)((char*)ptlv+sizeof(TWTlv)+ntohs(ptlv->wLen));
		i += sizeof(TWTlv)+ntohs(ptlv->wLen);
	}
	return NULL; 
}

void* GetWTlvValueByType(uint16_t wType, uint16_t wTlvNum, TWTlv *ptlv)
{
	int i=0;
	for(i=0; i < wTlvNum; i++)
	{
		if(ntohs(ptlv->wType) == wType)
			break;
		ptlv = (TWTlv*)((char*)ptlv+sizeof(TWTlv)+ntohs(ptlv->wLen));
	}
	if(i >= wTlvNum)
		return NULL;
	return ptlv->sValue;
}

TWTlv * GetWTlvByType(uint16_t wType, uint16_t wTlvNum, TWTlv *ptlv)
{
	int i=0;
	for(i=0; i < wTlvNum; i++)
	{
		if(ntohs(ptlv->wType) == wType)
			return ptlv;
		ptlv = (TWTlv*)((char*)ptlv+sizeof(TWTlv)+ntohs(ptlv->wLen));
	}
	return NULL;
}

void LogInfoNtoH(LogInfo *pLog)
{
	pLog->dwLogSeq = ntohl(pLog->dwLogSeq);
	pLog->iAppId = ntohl(pLog->iAppId);
	pLog->iModuleId = ntohl(pLog->iModuleId);
	pLog->dwLogConfigId = ntohl(pLog->dwLogConfigId);
	pLog->wLogType = ntohs(pLog->wLogType);
	pLog->qwLogTime = ntohll(pLog->qwLogTime);
	pLog->wCustDataLen = ntohs(pLog->wCustDataLen);
	pLog->wLogDataLen = ntohs(pLog->wLogDataLen);
}

void TlvLogInfoNtoH(TlvLogInfo *pLog)
{
	pLog->dwCust_1 = ntohl(pLog->dwCust_1);
	pLog->dwCust_2 = ntohl(pLog->dwCust_2);
	pLog->iCust_3 = ntohl(pLog->iCust_3);
	pLog->iCust_4 = ntohl(pLog->iCust_4);
	pLog->dwLogConfigId = ntohl(pLog->dwLogConfigId);
	pLog->dwLogHostId = ntohl(pLog->dwLogHostId);
	pLog->iModuleId = ntohl(pLog->iModuleId);
	pLog->wLogType = ntohs(pLog->wLogType);
	pLog->qwLogTime = ntohll(pLog->qwLogTime);
}

void MonitorPkgLogInfoNtoH(MonitorPkgLogInfo *pinfo)
{
	pinfo->iAppId = ntohl(pinfo->iAppId);
	pinfo->dwLogHostId = ntohl(pinfo->dwLogHostId);
	pinfo->wCltReqLogCount = ntohs(pinfo->wCltReqLogCount);
	pinfo->wSrvWriteLogCount = ntohs(pinfo->wSrvWriteLogCount);
}

void MonitorPkgLogInfoHtoN(MonitorPkgLogInfo *pinfo)
{
	pinfo->iAppId = htonl(pinfo->iAppId);
	pinfo->dwLogHostId = htonl(pinfo->dwLogHostId);
	pinfo->wCltReqLogCount = htons(pinfo->wCltReqLogCount);
	pinfo->wSrvWriteLogCount = htons(pinfo->wSrvWriteLogCount);
}

// pkgbody is valid ret 0
// pkgbody is invalid ret -1 
int CheckPkgBody(TPkgBody *pstBody, uint16_t wBodyLen)
{
	int i = 0;
	uint8_t *pdata = (uint8_t*)pstBody;
	TWTlv *ptlv = NULL;
	if(wBodyLen < sizeof(TPkgBody))
		return -1;
	wBodyLen -= sizeof(TPkgBody);
	pdata += sizeof(TPkgBody);
	for(i=0; i < pstBody->bTlvNum; i++)
	{
		if(wBodyLen < sizeof(TWTlv))
			return -1;
		ptlv = (TWTlv *)pdata; 
		if(ntohs(ptlv->wLen)+sizeof(TWTlv) > wBodyLen)
			return -1;
		wBodyLen -= ntohs(ptlv->wLen)+sizeof(TWTlv);
		pdata += ntohs(ptlv->wLen)+sizeof(TWTlv);
	}
	return 0;
}

#define MAX_CONFIG_LINE_LEN (4*1024 - 1)
int LoalAllConfig(const char *pcfgFile, std::map<std::string, std::string> &mpCfg)
{
	char sLine[MAX_CONFIG_LINE_LEN+1], sTmp[MAX_CONFIG_LINE_LEN+1];
	FILE *pstFile;
	if ((pstFile = fopen(pcfgFile, "r")) == NULL)
		return -1;

    std::string name, value;
	while (1)
	{
		fgets(sLine, sizeof(sLine), pstFile);
		if (feof(pstFile))
			break; 
        if(sLine[0] == '#' || sLine[0] == '\0')
            continue;
        get_config_val(sTmp, sLine);
        name = sTmp;
        if(name.size() <= 0)
            continue;
        get_config_val(sTmp, sLine);
        value = sTmp;
        mpCfg.insert(std::pair<std::string, std::string>(name, value));
	}	
	fclose(pstFile);
    return 0;
}


