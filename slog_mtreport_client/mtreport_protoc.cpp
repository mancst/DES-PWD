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

   模块 slog_mtreport_client 功能:
        用于上报除迁移云管系统本身产生的监控点数据、日志，为减少部署上的依赖
		未引入任何第三方组件

****/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <set>
#include <sstream>
#include <string>
#include "sv_socket.h"
#include "mtreport_client.h"
#include "mtreport_protoc.h"
#include "mtreport_basic_pkg.h"
#include "sv_str.h"
#include "Json.h"
#include "Base64.h"
#include "aes.h"
#include "sv_struct.h"
#include "sv_md5.h"
#include "mt_shm.h"

// 心跳是否维持有效
int IsHelloValid()
{
	// 最近一次正确 hello 时间在2分钟之内
	return (stConfig.pReportShm->dwLastHelloOkTime+2*60 > stConfig.dwCurTime);
}

// str: DES-PWE_cfgs:time;cfg_item cfg_val;cfg_item cfg_val;...
static uint32_t GetModTimeFromReportStr(const std::string str)
{
    const char *pst = NULL, *pend = NULL;
    pst = strchr(str.c_str(), ':');
    pend = strchr(str.c_str(), ';');
    if(pst && pend && pend > pst) {
        std::string stm(pst, (size_t)(pend-pst));
        return strtoul(stm.c_str(), NULL, 10);
    }
    return 0;
}

static int ReadPluginCfg(TInnerPlusInfo *plugin, CmdSendPluginConfigContent *pcfg, int iBufLen)
{
    std::ostringstream tmp;
    std::ostringstream sPluginCfg;
    if(stConfig.szPlusPath[0] != '/') 
        sPluginCfg << stConfig.szCurPath << "/" << stConfig.szPlusPath << "/" << plugin->szPlusName;
    else 
        sPluginCfg << stConfig.szPlusPath << "/" << plugin->szPlusName;
    tmp << "export conf_file=" << sPluginCfg.str() << "/xrk_" << plugin->szPlusName << ".conf; ";
    tmp << stConfig.szCurPath << "/report_cfg.sh ";

    std::string strResult;
    get_cmd_result(tmp.str().c_str(), strResult);
    if(strResult.find("DES-PWE_cfgs:") != std::string::npos) {
        // 时间戳更新
		uint32_t dwTm = GetModTimeFromReportStr(strResult);
		if(dwTm != 0)
	        plugin->dwCfgFileLastModTime = dwTm;
        if((int)(strResult.size()+sizeof(CmdSendPluginConfigContent)) < iBufLen) {
            strcpy(pcfg->strCfgs, strResult.c_str());
            return (int)(strResult.size()+sizeof(CmdSendPluginConfigContent)+1);
        }
        else {
            strncpy(pcfg->strCfgs, strResult.c_str(), iBufLen-1);
            pcfg->strCfgs[iBufLen-sizeof(CmdSendPluginConfigContent)-1] = '\0';
            ERROR_LOG("need more space %lu < %d, plugin:%s(%d)", strResult.size(), iBufLen, plugin->szPlusName, plugin->iPluginId);
            return iBufLen;
        }
    }
    else {
        WARN_LOG("get plugin:%s(%d) config failed, cmd:%s, result:%s",
            plugin->szPlusName, plugin->iPluginId, tmp.str().c_str(), strResult.c_str());
    }
    return -1;
}

uint32_t MakePluginConfigReportPkg(TInnerPlusInfo *plugin)
{
	CBasicPacket pkg;

	// head
	ReqPkgHead stHead;
	pkg.InitReqPkgHead(&stHead, CMD_SEND_PLUGIN_CONFIG);

	// cmd content
    static char s_plugCfgBuf[4096] = {0};
	CmdSendPluginConfigContent *pstInfo = (CmdSendPluginConfigContent*)s_plugCfgBuf;
    int iContentLen = ReadPluginCfg(plugin, pstInfo, sizeof(s_plugCfgBuf));
    if(iContentLen < 0)
        return 0;

    pstInfo->iPluginId = ntohl(plugin->iPluginId);
    pstInfo->iConfigLen = ntohl(iContentLen);
	pkg.InitCmdContent((void*)pstInfo, (uint16_t)iContentLen);

	// 签名
	char sSig[MAX_SIGNATURE_LEN+MYSIZEOF(TSignature)]={0};
	TSignature *psig = (TSignature*)sSig;
	if(stConfig.iEnableEncryptData) {
		MonitorCommSig stSigInfo;
		stSigInfo.dwSeq = htonl(pkg.m_dwReqSeq);
		stSigInfo.dwCmd = htonl(pkg.m_dwReqCmd);
		if(InitSignature(psig, &stSigInfo, stConfig.pReportShm->sRandKey, MT_SIGNATURE_TYPE_COMMON) < 0)
			return 0;
	}
	pkg.InitSignature(psig);

	// tlv
	char sTlvBuf[128];
	TPkgBody *pbody = (TPkgBody*)sTlvBuf;
	TlvMoniCommInfo stTlvInfo;
	stTlvInfo.iMtClientIndex = htonl(stConfig.pReportShm->iMtClientIndex);
	stTlvInfo.iMachineId = htonl(stConfig.pReportShm->iMachineId);
	stTlvInfo.dwReserved_1 = htonl(stConfig.dwLocalIp);

	int iTlvBodyLen = MYSIZEOF(TPkgBody);
	iTlvBodyLen += SetWTlv(
		pbody->stTlv, TLV_MONI_COMM_INFO, MYSIZEOF(stTlvInfo), (const char*)&stTlvInfo);
	pbody->bTlvNum = 1;
	pkg.InitPkgBody(pbody, iTlvBodyLen);
	return pkg.MakeReqPkg(stConfig.pPkg, &stConfig.iPkgLen);
}

uint32_t MakePreInstallReportPkg(CmdS2cPreInstallContentReq *pct, int status)
{
	CBasicPacket pkg;

	// head
	ReqPkgHead stHead;
	pkg.InitReqPkgHead(&stHead, CMD_MONI_PREINSTALL_REPORT);

    if(stConfig.qwServerPacketSessId != 0) {
        stHead.qwSessionId = htonll(stConfig.qwServerPacketSessId);
        stConfig.qwServerPacketSessId = 0;
    }

	// cmd content
	CmdPreInstallReportContent stInfo;
    stInfo.iPluginId = htonl(pct->iPluginId);
    stInfo.iMachineId = htonl(pct->iMachineId);
    stInfo.iDbId = htonl(pct->iDbId);
    strncpy(stInfo.sCheckStr, pct->sCheckStr, sizeof(stInfo.sCheckStr));
	strncpy(stInfo.sDevLang, pct->sDevLang, sizeof(stInfo.sDevLang));
    stInfo.iStatus = htonl(status);
	pkg.InitCmdContent((void*)&stInfo, (uint16_t)MYSIZEOF(stInfo));

	// 签名
	char sSig[MAX_SIGNATURE_LEN+MYSIZEOF(TSignature)]={0};
	TSignature *psig = (TSignature*)sSig;
	if(stConfig.iEnableEncryptData) {
		MonitorCommSig stSigInfo;
		stSigInfo.dwSeq = htonl(pkg.m_dwReqSeq);
		stSigInfo.dwCmd = htonl(pkg.m_dwReqCmd);
		if(InitSignature(psig, &stSigInfo, stConfig.pReportShm->sRandKey, MT_SIGNATURE_TYPE_COMMON) < 0)
			return 0;
	}
	pkg.InitSignature(psig);

	// tlv
	char sTlvBuf[128];
	TPkgBody *pbody = (TPkgBody*)sTlvBuf;
	TlvMoniCommInfo stTlvInfo;
	stTlvInfo.iMtClientIndex = htonl(stConfig.pReportShm->iMtClientIndex);
	stTlvInfo.iMachineId = htonl(stConfig.pReportShm->iMachineId);
	stTlvInfo.dwReserved_1 = htonl(stConfig.dwLocalIp);

	int iTlvBodyLen = MYSIZEOF(TPkgBody);
	iTlvBodyLen += SetWTlv(
		pbody->stTlv, TLV_MONI_COMM_INFO, MYSIZEOF(stTlvInfo), (const char*)&stTlvInfo);
	pbody->bTlvNum = 1;
	pkg.InitPkgBody(pbody, iTlvBodyLen);
	return pkg.MakeReqPkg(stConfig.pPkg, &stConfig.iPkgLen);
}

uint32_t MakeServerRespPkg(char *pRespContent, int iRespContentLen, CBasicPacket &req_pkg)
{
	CBasicPacket pkg;

	// head
	ReqPkgHead stHead;
	pkg.InitReqPkgHead(&stHead, req_pkg.m_dwReqCmd);

    // 可靠 udp session id 一旦使用后需置为 0
    if(stConfig.qwServerPacketSessId != 0) {
        stHead.qwSessionId = htonll(stConfig.qwServerPacketSessId);
        stConfig.qwServerPacketSessId = 0;
    }

	// cmd content
	pkg.InitCmdContent((void*)pRespContent, (uint16_t)iRespContentLen);

	// 签名
	char sSig[MAX_SIGNATURE_LEN+MYSIZEOF(TSignature)]={0};
	TSignature *psig = (TSignature*)sSig;
	if(stConfig.iEnableEncryptData) {
		MonitorCommSig stSigInfo;
		stSigInfo.dwSeq = htonl(pkg.m_dwReqSeq);
		stSigInfo.dwCmd = htonl(pkg.m_dwReqCmd);
		if(InitSignature(psig, &stSigInfo, stConfig.pReportShm->sRandKey, MT_SIGNATURE_TYPE_COMMON) < 0)
			return 0;
	}
	pkg.InitSignature(psig);

	// tlv
	char sTlvBuf[128];
	TPkgBody *pbody = (TPkgBody*)sTlvBuf;
	TlvMoniCommInfo stTlvInfo;
	stTlvInfo.iMtClientIndex = htonl(stConfig.pReportShm->iMtClientIndex);
	stTlvInfo.iMachineId = htonl(stConfig.pReportShm->iMachineId);
	stTlvInfo.dwReserved_1 = htonl(stConfig.dwLocalIp);

	int iTlvBodyLen = MYSIZEOF(TPkgBody);
	iTlvBodyLen += SetWTlv(
		pbody->stTlv, TLV_MONI_COMM_INFO, MYSIZEOF(stTlvInfo), (const char*)&stTlvInfo);
	pbody->bTlvNum = 1;
	pkg.InitPkgBody(pbody, iTlvBodyLen);
	return pkg.MakeReqPkg(stConfig.pPkg, &stConfig.iPkgLen);
}

void SendPreInstallStatusToServer(CmdS2cPreInstallContentReq *pct, int status)
{
	stConfig.pPkgSess = (PKGSESSION*)stConfig.sSessBuf;
	stConfig.pPkg = stConfig.sSessBuf+MYSIZEOF(PKGSESSION);
	stConfig.iPkgLen = PKG_BUFF_LENGTH;

    PLUGIN_INST_LOG("report install status:%d to server:%s:%d", status, stConfig.szSrvIp_master, stConfig.iSrvPort);
    if(MakePreInstallReportPkg(pct, status) > 0) {
        struct sockaddr_in addr_server;
        addr_server.sin_family = PF_INET;
        addr_server.sin_port = htons(stConfig.iSrvPort);
        addr_server.sin_addr.s_addr = inet_addr(stConfig.szSrvIp_master);
        int iRet = SendPacket(stConfig.iConfigSocketIndex, &addr_server, stConfig.pPkg, stConfig.iPkgLen);
        if(iRet != stConfig.iPkgLen) {
		    ERROR_LOG("SendPacket(report preinstall status) failed, pkglen:%d, ret:%d", stConfig.iPkgLen, iRet);
        }
    }
    else {
        ERROR_LOG("MakePreInstallReportPkg failed !");
    }
}

// 首个 hello 包
uint32_t MakeFirstHelloPkg(uint32_t dwSrvIp)
{
	CBasicPacket pkg;

	// head
	ReqPkgHead stHead;
	pkg.InitReqPkgHead(&stHead, CMD_MONI_SEND_HELLO_FIRST);
	strncpy(pkg.m_pstReqHead->sEchoBuf, g_strCmpTime.c_str(), sizeof(pkg.m_pstReqHead->sEchoBuf)-1);

	// cmd content
	MonitorHelloFirstContent stInfo;
	memset(&stInfo, 0, MYSIZEOF(stInfo));
	stInfo.iMtClientIndex = htonl(stConfig.pReportShm->iMtClientIndex);
	stInfo.iMachineId = htonl(stConfig.pReportShm->iMachineId);
    stInfo.dwBindDES-PWESetTime = htonl(stConfig.pReportShm->dwBindDES-PWESetTime);
	strncpy(stInfo.sCmpTime, g_strCmpTime.c_str(), sizeof(stInfo.sCmpTime)-1);
	strncpy(stInfo.sVersion, g_strVersion.c_str(), sizeof(stInfo.sVersion)-1);
	strncpy(stInfo.sOsInfo, stConfig.szOs, sizeof(stInfo.sOsInfo)-1);
	strncpy(stInfo.sOsArc, stConfig.szOsArc, sizeof(stInfo.sOsArc)-1);
	strncpy(stInfo.sLibcVer, stConfig.szLibcVer, sizeof(stInfo.sLibcVer)-1);
	strncpy(stInfo.sLibcppVer, stConfig.szLibcppVer, sizeof(stInfo.sLibcppVer)-1);
    stInfo.iClientTTL = htonl(stConfig.ttl);

    std::string strHostName;
    if(get_cmd_result("hostname", strHostName) == 0 && strHostName.size() > 0)
        strncpy(stInfo.szHostName, strHostName.c_str(), sizeof(stInfo.szHostName)-1);
    else 
        WARN_LOG("get host name failed, msg:%s", strerror(errno));
    if(GetLocalIPAndMac(dwSrvIp, stConfig.szLocalIP, NULL) != 0) {
		ERROR_LOG("GetLocalIPAndMac failed, msg:%s", strerror(errno));
        return 0;
    }
    stInfo.dwClientIp = htonl(inet_addr(stConfig.szLocalIP));
    if(stConfig.szCustLocalIP[0] != '\0') {
        stConfig.dwLocalIp = inet_addr(stConfig.szCustLocalIP);
    }
    else {
        stConfig.dwLocalIp = inet_addr(stConfig.szLocalIP);
    }

    std::ostringstream ss;
    ss << "ip route get " << ipv4_addr_str(dwSrvIp) << " from " << stConfig.szLocalIP;
    std::string strRoute, strGwIp;
    if(get_cmd_result(ss.str().c_str(), strRoute) == 0 && strRoute.size() > 0) {
        std::istringstream ins(strRoute);
        std::string str;
        for(int i=0; ins >> str; i++) {
            if(i==0 && str=="local") {
                break;
            }
            else if(i==0 && str != ipv4_addr_str(dwSrvIp)) {
                WARN_LOG("get route check failed(%s != %s), cmd:%s",
                    str.c_str(), stConfig.szLocalIP, ss.str().c_str());
                break;
            }
            else if(i==2 && str != stConfig.szLocalIP) {
                WARN_LOG("get route check failed(%s != %s), cmd:%s",
                    str.c_str(), stConfig.szLocalIP, ss.str().c_str());
                break;
            }
            else if(i==3 && str != "via") {
                break;
            }
            else if(i==3 && str == "via") {
                if(ins >> strGwIp) {
                    break;
                }
                else {
                    WARN_LOG("get route failed !");
                    break;
                }
            }
        }
    }
    else 
        WARN_LOG("get route failed, msg:%s", strerror(errno));

    if(strGwIp.size() > 0) 
        stInfo.dwGwIp = htonl(inet_addr(strGwIp.c_str()));
    pkg.InitCmdContent((void*)&stInfo, (uint16_t)MYSIZEOF(stInfo));
    DEBUG_LOG("get local ip:%s, hostname:%s, gw ip:%s, seq:%u", 
        stConfig.szLocalIP, stInfo.szHostName, strGwIp.c_str(), pkg.m_dwReqSeq);

	// 签名
	char sSig[MAX_SIGNATURE_LEN+MYSIZEOF(TSignature)]={0};
	TSignature *psig = (TSignature*)sSig;
	MonitorHelloSig stSigInfo;
	stSigInfo.dwPkgSeq = htonl(pkg.m_dwReqSeq);
	stSigInfo.dwAgentClientIp = htonl(stConfig.dwLocalIp);
	OI_RandStrURandom(stSigInfo.sRespEncKey, 16);
	if(stConfig.iEnableEncryptData) 
		stSigInfo.bEnableEncryptData = 1;
	else
		stSigInfo.bEnableEncryptData = 0;
	if(InitSignature(psig, &stSigInfo, stConfig.szUserKey, (int)MT_SIGNATURE_TYPE_HELLO_FIRST) < 0)
		return -1;
	pkg.InitSignature(psig);

	memcpy(stConfig.pPkgSess->stCmdSessData.hello.sRespEncKey, stSigInfo.sRespEncKey, 16);
	stConfig.pPkgSess->stCmdSessData.hello.bHelloFlag |= HELLO_FLAG_USE_LAST_SRV;
	return pkg.MakeReqPkg(stConfig.pPkg, &stConfig.iPkgLen);
}

static int OnCmdExpire(TimerNode *pNodeShm, unsigned uiDataLen, char *pData)
{
	PKGSESSION *pPkgSess = (PKGSESSION*)pNodeShm->sSessData;
	ReqPkgHead *pHead = (ReqPkgHead*)(pData+1);

	if(pPkgSess->bSessStatus == SESS_FLAG_WAIT_RESPONSE) {
		DEBUG_LOG("cmd:%d - socket:%d last send time:%u:%u resend:%d", ntohl(pHead->dwCmd),
			pPkgSess->iSockIndex, pPkgSess->dwSendTimeSec, pPkgSess->dwSendTimeUsec, pHead->bResendTimes);
		pHead->bResendTimes++;

		// 超时时间设置为上一次的2倍
		pNodeShm->uiTimeOut *= 2;
		if(pNodeShm->uiTimeOut > PKG_TIMEOUT_MAX_MS)
			pNodeShm->uiTimeOut = PKG_TIMEOUT_MAX_MS;
	}
	else {
		pHead->bResendTimes = 0;
		pNodeShm->uiTimeOut = GetMaxResponseTime(pPkgSess->iSockIndex)+1000;
	}

	int iRet = SendPacket(pPkgSess->iSockIndex, NULL, pData, uiDataLen);
	if(iRet != (int)uiDataLen)
		ERROR_LOG("SendPacket failed ! ret:%d packet length:%d", iRet, uiDataLen);
	else {
		pPkgSess->dwSendTimeSec = stConfig.stTimeCur.tv_sec;
		pPkgSess->dwSendTimeUsec = stConfig.stTimeCur.tv_usec;
		pPkgSess->bSessStatus = SESS_FLAG_WAIT_RESPONSE;
	}

	iRet = UpdateTimer(pNodeShm, uiDataLen, pData);
	if(iRet != 0) {
		ERROR_LOG("UpdateTimer failed packet length:%d ret:%d", uiDataLen, iRet);
		return 0;
	}
	return 1;
}

static int SendAttrExpire(TimerNode *pNodeShm, unsigned uiDataLen, char *pData)
{
	PKGSESSION *pPkgSess = (PKGSESSION*)pNodeShm->sSessData;
	ReqPkgHead *pHead = (ReqPkgHead*)(pData+1);
	int iRet = 0;

	DEBUG_LOG("send str|attr(%d) to srv:%s timeout - socket:%d resend:%d", ntohl(pHead->dwCmd),
		ipv4_addr_str(pPkgSess->stCmdSessData.attr.dwAttrSrvIP), pPkgSess->iSockIndex, pHead->bResendTimes);
	pHead->bResendTimes++;
	if(pHead->bResendTimes > TRY_MAX_TIMES_PER_SERVER){
		ERROR_LOG("send attr failed, server no response !");
		return 0;
	}
	else if(pHead->bResendTimes <= TRY_MAX_TIMES_PER_SERVER) {
		// 超时时间设置为上一次的2倍
		pNodeShm->uiTimeOut *= 2;
		if(pNodeShm->uiTimeOut > PKG_TIMEOUT_MAX_MS)
			pNodeShm->uiTimeOut = PKG_TIMEOUT_MAX_MS;
		iRet = SendPacket(pPkgSess->iSockIndex, NULL, pData, uiDataLen);
	}

	if(iRet != (int)uiDataLen)
		ERROR_LOG("SendPacket failed ! ret:%d packet length:%d", iRet, uiDataLen);
	else {
		pPkgSess->dwSendTimeSec = stConfig.stTimeCur.tv_sec;
		pPkgSess->dwSendTimeUsec = stConfig.stTimeCur.tv_usec;
		pPkgSess->bSessStatus = SESS_FLAG_WAIT_RESPONSE;
	}

	iRet = UpdateTimer(pNodeShm, uiDataLen, pData);
	if(iRet != 0) {
		ERROR_LOG("UpdateTimer failed packet length:%d ret:%d", uiDataLen, iRet);
		return 0;
	}
	return 1;
}

static int SendAppLogExpire(TimerNode *pNodeShm, unsigned uiDataLen, char *pData)
{
	PKGSESSION *pPkgSess = (PKGSESSION*)pNodeShm->sSessData;
	ReqPkgHead *pHead = (ReqPkgHead*)(pData+1);
	int iRet = 0;

	DEBUG_LOG("send applog to srv:%s timeout - socket:%d last send time:%u:%u resend:%d",
		ipv4_addr_str(pPkgSess->stCmdSessData.applog.dwAppLogSrvIP), pPkgSess->iSockIndex,
		pPkgSess->dwSendTimeSec, pPkgSess->dwSendTimeUsec, pHead->bResendTimes);
	pHead->bResendTimes++;
	if(pHead->bResendTimes > TRY_MAX_TIMES_PER_SERVER){
		ERROR_LOG("send applog failed, server no response !");
		return 0;
	}
	else if(pHead->bResendTimes <= TRY_MAX_TIMES_PER_SERVER) {
		// 超时时间设置为上一次的2倍
		pNodeShm->uiTimeOut *= 2;
		if(pNodeShm->uiTimeOut > PKG_TIMEOUT_MAX_MS)
			pNodeShm->uiTimeOut = PKG_TIMEOUT_MAX_MS;
		iRet = SendPacket(pPkgSess->iSockIndex, NULL, pData, uiDataLen);
	}

	if(iRet != (int)uiDataLen)
		ERROR_LOG("SendPacket failed ! ret:%d packet length:%d", iRet, uiDataLen);
	else {
		pPkgSess->dwSendTimeSec = stConfig.stTimeCur.tv_sec;
		pPkgSess->dwSendTimeUsec = stConfig.stTimeCur.tv_usec;
		pPkgSess->bSessStatus = SESS_FLAG_WAIT_RESPONSE;
	}

	iRet = UpdateTimer(pNodeShm, uiDataLen, pData);
	if(iRet != 0) {
		ERROR_LOG("UpdateTimer failed packet length:%d ret:%d", uiDataLen, iRet);
		return 0;
	}
	return 1;
}

static int HelloFirstExpire(TimerNode *pNodeShm, unsigned uiDataLen, char *pData)
{
	static std::set<uint32_t> s_setHelloChecked;

	PKGSESSION *pPkgSess = (PKGSESSION*)pNodeShm->sSessData;
	ReqPkgHead *pHead = (ReqPkgHead*)(pData+1);

	DEBUG_LOG("first hello timeout - hello socket:%d last send time:%u:%u resend:%d hello flag:%d",
		pPkgSess->iSockIndex, pPkgSess->dwSendTimeSec, pPkgSess->dwSendTimeUsec, pHead->bResendTimes, 
		pPkgSess->stCmdSessData.hello.bHelloFlag);

	pHead->bResendTimes++;
	if(pHead->bResendTimes > TRY_MAX_TIMES_PER_SERVER) {
		// hello 包探测失败，重启服务
		FATAL_LOG("hello check failed, restart agent ! - try server count:%lu", s_setHelloChecked.size());
		stConfig.pReportShm->cIsAgentRun = 2;
		return -1;
	}
	else {
		// 超时时间设置为上一次的2倍
		pNodeShm->uiTimeOut *= 2;
		if(pNodeShm->uiTimeOut > PKG_TIMEOUT_MAX_MS)
			pNodeShm->uiTimeOut = PKG_TIMEOUT_MAX_MS;
	}

	int iRet = SendPacket(pPkgSess->iSockIndex, NULL, pData, uiDataLen);
	if(iRet != (int)uiDataLen)
		ERROR_LOG("SendPacket failed ! ret:%d packet length:%d", iRet, uiDataLen);
	else {
		pPkgSess->dwSendTimeSec = stConfig.stTimeCur.tv_sec;
		pPkgSess->dwSendTimeUsec = stConfig.stTimeCur.tv_usec;
		pPkgSess->bSessStatus = SESS_FLAG_WAIT_RESPONSE;
	}

	iRet = UpdateTimer(pNodeShm, uiDataLen, pData);
	if(iRet != 0) {
		ERROR_LOG("UpdateTimer failed packet length:%d ret:%d", uiDataLen, iRet);
		return 0;
	}
	return 1;
}

int MakeRepPluginInfoToServer()
{
    // 用于循环上报插件信息
    static int s_iLastSendPluginIdx = -1;

    // 启动后首次调用, 全部重新上报下
    if(s_iLastSendPluginIdx < 0) { 
		int iPlugCount = 0;
        for(int i=0; i < MAX_INNER_PLUS_COUNT; i++)  {
            if(stConfig.pReportShm->stPluginInfo[i].iPluginId != 0) {
                stConfig.pReportShm->stPluginInfo[i].dwLastReportSelfInfoTime = 0; 
				iPlugCount++;
			}
        } 
		stConfig.pReportShm->iPluginInfoCount = iPlugCount;
        s_iLastSendPluginIdx = 0; 
    }    

    CBasicPacket pkg; 

    // head
    ReqPkgHead stHead;
    pkg.InitReqPkgHead(&stHead, CMD_MONI_SEND_PLUGIN_INFO);

    // cmd content
    static char sContentBuf[1000] = {0}; 
    MonitorRepPluginInfoContent stInfo;
    memset(&stInfo, 0, MYSIZEOF(stInfo));
    uint16_t wContentLen = sizeof(stInfo);

    char *pbuf = sContentBuf+sizeof(stInfo);
    TRepPluginInfoFirst stPluginFirst;
    TRepPluginInfo stPlugin;
    int i = s_iLastSendPluginIdx, j = 0; 
    for(j=0; wContentLen < 1000-sizeof(TRepPluginInfoFirst) && j < MAX_INNER_PLUS_COUNT; j++) 
    {
        if(stConfig.pReportShm->stPluginInfo[i].iPluginId != 0)
        {
            if(stConfig.pReportShm->stPluginInfo[i].dwLastReportSelfInfoTime != 0) {
                // 非首次上报信息
                if(stConfig.pReportShm->stPluginInfo[i].dwLastReportAttrTime ==
                    stConfig.pReportShm->stPluginInfo[i].dwRep_LastReportAttrTime
                    && stConfig.pReportShm->stPluginInfo[i].dwLastReportLogTime ==
                    stConfig.pReportShm->stPluginInfo[i].dwRep_LastReportLogTime
                    && stConfig.pReportShm->stPluginInfo[i].dwLastHelloTime <
                    stConfig.pReportShm->stPluginInfo[i].dwRep_LastHelloTime+15)
                {
                    // 信息没有变化，不用上报
                    i++;
                    if(i >= MAX_INNER_PLUS_COUNT)
                        i = 0;
                    continue;
                }

                stPlugin.iPluginId = htonl(stConfig.pReportShm->stPluginInfo[i].iPluginId);
                stPlugin.dwLastReportAttrTime = htonl(stConfig.pReportShm->stPluginInfo[i].dwLastReportAttrTime);
                stPlugin.dwLastReportLogTime = htonl(stConfig.pReportShm->stPluginInfo[i].dwLastReportLogTime);
                stPlugin.dwLastHelloTime = htonl(stConfig.pReportShm->stPluginInfo[i].dwLastHelloTime);
                stPlugin.dwConfigFileTime = htonl(stConfig.pReportShm->stPluginInfo[i].dwCfgFileLastModTime);

                *((uint8_t*)pbuf) = (uint8_t)sizeof(stPlugin);
                pbuf += sizeof(uint8_t);
                memcpy(pbuf, &stPlugin, sizeof(stPlugin));
                pbuf += sizeof(stPlugin);
                wContentLen += sizeof(uint8_t) + sizeof(stPlugin);
                DEBUG_LOG("not first report plugin info, plugin:%d(%s)",
                    stConfig.pReportShm->stPluginInfo[i].iPluginId, stConfig.pReportShm->stPluginInfo[i].szPlusName);
            }else {
                // 首次上报插件信息
                stPluginFirst.iPluginId = htonl(stConfig.pReportShm->stPluginInfo[i].iPluginId);
                strcpy(stPluginFirst.szVersion, stConfig.pReportShm->stPluginInfo[i].szVersion);
                stPluginFirst.iLibVerNum = htonl(stConfig.pReportShm->stPluginInfo[i].iLibVerNum);
                stPluginFirst.dwLastReportAttrTime = htonl(stConfig.pReportShm->stPluginInfo[i].dwLastReportAttrTime);
                stPluginFirst.dwLastReportLogTime = htonl(stConfig.pReportShm->stPluginInfo[i].dwLastReportLogTime);
                stPluginFirst.dwPluginStartTime = htonl(stConfig.pReportShm->stPluginInfo[i].dwPluginStartTime);
                stPluginFirst.dwLastHelloTime = htonl(stConfig.pReportShm->stPluginInfo[i].dwLastHelloTime);
                stPluginFirst.bPluginNameLen = strlen(stConfig.pReportShm->stPluginInfo[i].szPlusName)+1;
                stPluginFirst.dwConfigFileTime = htonl(stConfig.pReportShm->stPluginInfo[i].dwCfgFileLastModTime);
                *((uint8_t*)pbuf) = (uint8_t)sizeof(stPluginFirst)+stPluginFirst.bPluginNameLen;
                pbuf += sizeof(uint8_t);
                memcpy(pbuf, &stPluginFirst, sizeof(stPluginFirst));
                pbuf += sizeof(stPluginFirst);
                wContentLen += 1 + sizeof(stPluginFirst);

                // 插件名
                memcpy(pbuf, stConfig.pReportShm->stPluginInfo[i].szPlusName, stPluginFirst.bPluginNameLen);
                pbuf += stPluginFirst.bPluginNameLen;
                wContentLen += stPluginFirst.bPluginNameLen;
                DEBUG_LOG("first report plugin info, plugin:%d(%s), plugin start:%s",
                    stConfig.pReportShm->stPluginInfo[i].iPluginId,
                    stConfig.pReportShm->stPluginInfo[i].szPlusName, uitodate(stPluginFirst.dwPluginStartTime));
            }
            stConfig.pReportShm->stPluginInfo[i].dwRep_LastReportAttrTime = stConfig.pReportShm->stPluginInfo[i].dwLastReportAttrTime;
            stConfig.pReportShm->stPluginInfo[i].dwRep_LastReportLogTime = stConfig.pReportShm->stPluginInfo[i].dwLastReportLogTime;
            stConfig.pReportShm->stPluginInfo[i].dwRep_LastHelloTime = stConfig.pReportShm->stPluginInfo[i].dwLastHelloTime;

            stConfig.pReportShm->stPluginInfo[i].dwLastReportSelfInfoTime = stConfig.dwCurTime;
            stInfo.bPluginCount++;
       }
       i++;
       if(i >= MAX_INNER_PLUS_COUNT)
           i = 0;
    }
    s_iLastSendPluginIdx = i;
    if(stInfo.bPluginCount <= 0)
        return 0;

    memcpy(sContentBuf, &stInfo, sizeof(stInfo));
    pkg.InitCmdContent((void*)sContentBuf, wContentLen);

	// 签名
	char sSig[MAX_SIGNATURE_LEN+MYSIZEOF(TSignature)]={0};
	TSignature *psig = (TSignature*)sSig;
	if(stConfig.iEnableEncryptData) {
		MonitorCommSig stSigInfo;
		stSigInfo.dwSeq = htonl(pkg.m_dwReqSeq);
		stSigInfo.dwCmd = htonl(pkg.m_dwReqCmd);
		if(InitSignature(psig, &stSigInfo, stConfig.pReportShm->sRandKey, MT_SIGNATURE_TYPE_COMMON) < 0)
			return 0;
	}
	pkg.InitSignature(psig);

	// tlv
	char sTlvBuf[128];
	TPkgBody *pbody = (TPkgBody*)sTlvBuf;
	TlvMoniCommInfo stTlvInfo;
	stTlvInfo.iMtClientIndex = htonl(stConfig.pReportShm->iMtClientIndex);
	stTlvInfo.iMachineId = htonl(stConfig.pReportShm->iMachineId);
	stTlvInfo.dwReserved_1 = htonl(stConfig.dwLocalIp);

	int iTlvBodyLen = MYSIZEOF(TPkgBody);
	iTlvBodyLen += SetWTlv(
		pbody->stTlv, TLV_MONI_COMM_INFO, MYSIZEOF(stTlvInfo), (const char*)&stTlvInfo);
	pbody->bTlvNum = 1;
	pkg.InitPkgBody(pbody, iTlvBodyLen);
    return pkg.MakeReqPkg(stConfig.pPkg, &stConfig.iPkgLen);
}

int DealRespRepPluginInfo(CBasicPacket &pkg)
{
    if(pkg.m_bRetCode != NO_ERROR) {
        WARN_LOG("cmd report plugin ret failed ! (%d)", pkg.m_bRetCode);
        return pkg.m_bRetCode;
    }    

    char sCmdContentBuf[2048] = {0}; 
    int iBufLen = (int)(MYSIZEOF(sCmdContentBuf));
    MonitorRepPluginInfoContentResp *presp = NULL;
	if(stConfig.iEnableEncryptData) {
		size_t iDecSigLen = 0;
		aes_decipher_data((const uint8_t*)pkg.m_pstCmdContent, pkg.m_wCmdContentLen,
			(uint8_t*)sCmdContentBuf, &iDecSigLen, (const uint8_t*)stConfig.pReportShm->sRandKey, AES_128);
		iBufLen = iDecSigLen;
		presp = (MonitorRepPluginInfoContentResp*)sCmdContentBuf;
	}
	else {
		iBufLen = pkg.m_wCmdContentLen;
    	presp = (MonitorRepPluginInfoContentResp*)pkg.m_pstCmdContent;
	}

    if(iBufLen != (int)(presp->bPluginCount*sizeof(MonitorPluginCheckResult)+sizeof(MonitorRepPluginInfoContentResp))) {
        REQERR_LOG("check report plugin response data len failed %d != %d, encrypt:%d",
            iBufLen, (int)(presp->bPluginCount*sizeof(MonitorPluginCheckResult)+sizeof(MonitorRepPluginInfoContentResp)),
			stConfig.iEnableEncryptData);
        return ERR_CHECK_DATA_FAILED;
    }    

    MonitorPluginCheckResult *pRlt = (MonitorPluginCheckResult*)((char*)presp+sizeof(MonitorRepPluginInfoContentResp));
    int iOk = 0, iFail = 0, iNeedCfg = 0;
    for(int i=0,j=0; i < presp->bPluginCount; i++, pRlt++) {
        pRlt->iPluginId = ntohl(pRlt->iPluginId);
        for(j=0; j < MAX_INNER_PLUS_COUNT; j++) {
            if(pRlt->iPluginId == stConfig.pReportShm->stPluginInfo[j].iPluginId) 
                break;
        }
        if(j >= MAX_INNER_PLUS_COUNT) {
            REQERR_LOG("not find plugin:%d", pRlt->iPluginId);
            continue;
        }

        if(pRlt->bCheckResult) {
            stConfig.pReportShm->stPluginInfo[j].bCheckRet = 1; 
            INFO_LOG("report plugin id:%d, name:%s check failed", 
                pRlt->iPluginId, stConfig.pReportShm->stPluginInfo[j].szPlusName);
            iFail++;
        }
        else {
            stConfig.pReportShm->stPluginInfo[j].bCheckRet = 0; 
            iOk++;
        }

        if(pRlt->bNeedReportCfg) {
            SendPluginConfig(stConfig.pReportShm->stPluginInfo+j);
            iNeedCfg++;
        }
    }
	DEBUG_LOG("report plugin info response, count:%d, ok:%d, fail:%d, need cfg:%d",
        presp->bPluginCount, iOk, iFail, iNeedCfg);
    return 0;
}

static int ReportPluginInfoExpire(TimerNode *pNodeShm, unsigned uiDataLen, char *pData)
{
    PKGSESSION *pPkgSess = (PKGSESSION*)pNodeShm->sSessData;
    ReqPkgHead *pHead = (ReqPkgHead*)(pData+1);
    int iRet = 0; 

    DEBUG_LOG("report plugin info(cmd:%d) to srv:%s timeout - socket:%d resend:%d", ntohl(pHead->dwCmd),
        ipv4_addr_str(pPkgSess->stCmdSessData.plugin.dwConfigSrvIP), pPkgSess->iSockIndex, pHead->bResendTimes);
    pHead->bResendTimes++;
    if(pHead->bResendTimes > TRY_MAX_TIMES_PER_SERVER){
        ERROR_LOG("report plugin info failed, server no response !");
        return 0;
    }    
    else if(pHead->bResendTimes <= TRY_MAX_TIMES_PER_SERVER) {
        pNodeShm->uiTimeOut *= 2;
        if(pNodeShm->uiTimeOut > PKG_TIMEOUT_MAX_MS)
            pNodeShm->uiTimeOut = PKG_TIMEOUT_MAX_MS;
        iRet = SendPacket(pPkgSess->iSockIndex, NULL, pData, uiDataLen);
    }    

    if(iRet != (int)uiDataLen)
        ERROR_LOG("SendPacket failed ! ret:%d packet length:%d", iRet, uiDataLen);
    else {
        pPkgSess->dwSendTimeSec = stConfig.stTimeCur.tv_sec;
        pPkgSess->dwSendTimeUsec = stConfig.stTimeCur.tv_usec;
        pPkgSess->bSessStatus = SESS_FLAG_WAIT_RESPONSE;
    }   
    iRet = UpdateTimer(pNodeShm, uiDataLen, pData);
    if(iRet != 0) { 
        ERROR_LOG("UpdateTimer failed packet length:%d ret:%d", uiDataLen, iRet);
        return 0;
    }
    return 1;
}

int OnPkgExpire(TimerNode *pNodeShm, unsigned uiDataLen, char *pData)
{
	DEBUG_LOG("on expire - key:%u data len:%u", pNodeShm->uiKey, uiDataLen);
	ReqPkgHead *pPkgHead = (ReqPkgHead*)(pData+1);
	int iRet = 0;
	switch(ntohl(pPkgHead->dwCmd)) {
		case CMD_MONI_SEND_HELLO_FIRST:
			iRet = HelloFirstExpire(pNodeShm, uiDataLen, pData);
			break;

		case CMD_MONI_SEND_HELLO:
		case CMD_MONI_CHECK_LOG_CONFIG:
		case CMD_MONI_CHECK_APP_CONFIG:
		case CMD_MONI_CHECK_SYSTEM_CONFIG:
			if(!IsHelloValid()) {
				FATAL_LOG("hello is invalid !");
				stConfig.pReportShm->cIsAgentRun = 2;
				return 0;
			}
			iRet = OnCmdExpire(pNodeShm, uiDataLen, pData);
			break;

		case CMD_MONI_SEND_LOG:
			iRet = SendAppLogExpire(pNodeShm, uiDataLen, pData);
			break;

		case CMD_MONI_SEND_PLUGIN_INFO:
			iRet = ReportPluginInfoExpire(pNodeShm, uiDataLen, pData);
			break;

		case CMD_MONI_SEND_STR_ATTR:
		case CMD_MONI_SEND_ATTR:
			iRet = SendAttrExpire(pNodeShm, uiDataLen, pData);
			break;
		default:
			ERROR_LOG("unknow pkg cmd:%u", ntohl(pPkgHead->dwCmd));
			break;
	}
	return iRet; 
}

// 维持 hello 心跳，进行网速测试, 并记录
static uint32_t MakeHelloPkg(uint32_t dwResponseTimeMs, uint32_t dwHelloTimes)
{
	CBasicPacket pkg;

	// head
	ReqPkgHead stHead;
	pkg.InitReqPkgHead(&stHead, CMD_MONI_SEND_HELLO);

	// cmd content
	MonitorHelloContent stInfo;
	memset(&stInfo, 0, MYSIZEOF(stInfo));
	stInfo.dwHelloTimes = htonl(dwHelloTimes);
	stInfo.dwServerResponseTime = htonl(dwResponseTimeMs);
    stInfo.dwBindDES-PWESetTime = htonl(stConfig.pReportShm->dwBindDES-PWESetTime);
    if(stConfig.szCloudUrl[0] != '\0')
        strncpy(stInfo.szDES-PWEUrl, stConfig.szCloudUrl, sizeof(stInfo.szDES-PWEUrl));
    else
        strncpy(stInfo.szDES-PWEUrl, stConfig.szDownCloudUrl, sizeof(stInfo.szDES-PWEUrl));

	// some config for check
	stInfo.dwAttrSrvIp = htonl(stConfig.pReportShm->dwAttrSrvIp);
	stInfo.wAttrServerPort = htons(stConfig.pReportShm->wAttrServerPort);

	pkg.InitCmdContent((void*)&stInfo, (uint16_t)MYSIZEOF(stInfo));

	// 签名
	char sSig[MAX_SIGNATURE_LEN+MYSIZEOF(TSignature)]={0};
	TSignature *psig = (TSignature*)sSig;
	if(stConfig.iEnableEncryptData) {
		MonitorCommSig stSigInfo;
		stSigInfo.dwSeq = htonl(pkg.m_dwReqSeq);
		stSigInfo.dwCmd = htonl(pkg.m_dwReqCmd);
		if(InitSignature(psig, &stSigInfo, stConfig.pReportShm->sRandKey, MT_SIGNATURE_TYPE_COMMON) < 0)
			return 0;
	}
	pkg.InitSignature(psig);

	// tlv
	char sTlvBuf[128];
	TPkgBody *pbody = (TPkgBody*)sTlvBuf;
	TlvMoniCommInfo stTlvInfo;
	stTlvInfo.iMtClientIndex = htonl(stConfig.pReportShm->iMtClientIndex);
	stTlvInfo.iMachineId = htonl(stConfig.pReportShm->iMachineId);
	stTlvInfo.dwReserved_1 = htonl(stConfig.dwLocalIp);
	int iTlvBodyLen = MYSIZEOF(TPkgBody);
	iTlvBodyLen += SetWTlv(
		pbody->stTlv, TLV_MONI_COMM_INFO, MYSIZEOF(stTlvInfo), (const char*)&stTlvInfo);
	pbody->bTlvNum = 1;
	pkg.InitPkgBody(pbody, iTlvBodyLen);
	return pkg.MakeReqPkg(stConfig.pPkg, &stConfig.iPkgLen);
}

int MakeHelloToServer(PKGSESSION *psess_last)
{
	static uint32_t dwHelloTimes = 1;
	stConfig.pPkgSess = (PKGSESSION*)stConfig.sSessBuf;
	memcpy(stConfig.pPkgSess, psess_last, MYSIZEOF(PKGSESSION));
	stConfig.pPkg = stConfig.sSessBuf+MYSIZEOF(PKGSESSION);
	stConfig.iPkgLen = PKG_BUFF_LENGTH;

	uint32_t dwResponseTimeMs = 0;
	if(psess_last->bSessStatus == SESS_FLAG_RESPONSED)
		dwResponseTimeMs = GET_DIFF_TIME_MS(psess_last->dwSendTimeSec, psess_last->dwSendTimeUsec);
	uint32_t dwKey = MakeHelloPkg(dwResponseTimeMs, dwHelloTimes++); 
	if(dwKey == 0)
	{
		stConfig.pReportShm->cIsAgentRun = 2;
		ERROR_LOG("MakeHelloPkg failed !");
		return MTREPORT_ERROR_LINE;
	}

	int iRet = 0;
	uint32_t dwExpireTimeMs = 0;
	if(psess_last->bSessStatus != SESS_FLAG_RESPONSED) {
		iRet = SendPacket(psess_last->iSockIndex, NULL, stConfig.pPkg, stConfig.iPkgLen);
		if(iRet != stConfig.iPkgLen) {
			ERROR_LOG("SendPacket failed, pkglen:%d, ret:%d", stConfig.iPkgLen, iRet); 
			return MTREPORT_ERROR_LINE;
		}
		stConfig.pPkgSess->dwSendTimeSec = stConfig.stTimeCur.tv_sec;
		stConfig.pPkgSess->dwSendTimeUsec = stConfig.stTimeCur.tv_usec;
		stConfig.pPkgSess->bSessStatus = SESS_FLAG_WAIT_RESPONSE;
		dwExpireTimeMs = GetMaxResponseTime(psess_last->iSockIndex)+1000;
	}
	else {
		dwExpireTimeMs = CMD_HELLO_SEND_TIME_MS;
		stConfig.pPkgSess->bSessStatus = SESS_FLAG_TIMEOUT_SENDPKG;
		stConfig.pPkgSess->dwSendTimeSec = 0;
		stConfig.pPkgSess->dwSendTimeUsec = 0;
	}
	if(stConfig.pReportShm->stSysCfg.wHelloPerTimeSec != 0)
		dwExpireTimeMs = TIME_SEC_TO_MS(stConfig.pReportShm->stSysCfg.wHelloPerTimeSec);
	
	// hello 包添加到定时器
	iRet = AddTimer(dwKey, dwExpireTimeMs, OnPkgExpire,
		stConfig.pPkgSess, MYSIZEOF(PKGSESSION), stConfig.iPkgLen, stConfig.pPkg);
	if(iRet < 0) {
		ERROR_LOG("AddTimer failed ! pkglen:%d, key:%u, ret:%d", stConfig.iPkgLen, dwKey, iRet);
		return MTREPORT_ERROR_LINE;
	}
	DEBUG_LOG("add timer key:%u - datalen:%u", dwKey, stConfig.iPkgLen);
	return 0;
}

int DealResponseHello(CBasicPacket &pkg)
{
	if(pkg.m_bRetCode != NO_ERROR) {
		WARN_LOG("cmd hello ret failed ! (%d)", pkg.m_bRetCode);
		return pkg.m_bRetCode;
	}

	PKGSESSION *psess = stConfig.pPkgSess;
	char sCmdContentBuf[1024] = {0};
	MonitorHelloContentResp *presp = NULL;
	if(stConfig.iEnableEncryptData) {
		size_t iDecSigLen = 0;
		aes_decipher_data((const uint8_t*)pkg.m_pstCmdContent, pkg.m_wCmdContentLen,
			(uint8_t*)sCmdContentBuf, &iDecSigLen, (const uint8_t*)stConfig.pReportShm->sRandKey, AES_128);
		if(iDecSigLen != sizeof(MonitorHelloContentResp)) {
			REQERR_LOG("MtDecrypt failed - key:%s datalen:%d, check:%d != %d",
				DumpStrByMask(stConfig.pReportShm->sRandKey, 16), pkg.m_wCmdContentLen, 
				(int)iDecSigLen, (int)sizeof(MonitorHelloContentResp));
			return ERR_DECRYPT_FAILED;
		}
		presp = (MonitorHelloContentResp*)sCmdContentBuf;
	}
	else {
		presp = (MonitorHelloContentResp*)pkg.m_pstCmdContent;
		if(pkg.m_wCmdContentLen != sizeof(MonitorHelloContentResp)) {
			REQERR_LOG("check cmd content length failed, %d != %lu", 
				(int)pkg.m_wCmdContentLen, sizeof(MonitorHelloContentResp));
			return ERR_INVALID_CMD_CONTENT;
		}
	}

	if(stConfig.pReportShm->iMtClientIndex != (int32_t)ntohl(presp->iMtClientIndex)){
		WARN_LOG("client index changed from %d to %d",
			stConfig.pReportShm->iMtClientIndex, ntohl(presp->iMtClientIndex));
		stConfig.pReportShm->iMtClientIndex = ntohl(presp->iMtClientIndex);
	}

	// config change check
	if(presp->bConfigChange)
	{
		if(presp->wAttrServerPort != 0 && presp->dwAttrSrvIp != 0
			&& (stConfig.pReportShm->dwAttrSrvIp != ntohl(presp->dwAttrSrvIp)
			|| stConfig.pReportShm->wAttrServerPort != ntohs(presp->wAttrServerPort)))
		{
			INFO_LOG("attr server changed old %s:%d", 
				ipv4_addr_str(stConfig.pReportShm->dwAttrSrvIp), stConfig.pReportShm->wAttrServerPort);
			stConfig.pReportShm->dwAttrSrvIp = ntohl(presp->dwAttrSrvIp);
			stConfig.pReportShm->wAttrServerPort = ntohs(presp->wAttrServerPort);
			INFO_LOG("attr server changed new %s:%d", 
				ipv4_addr_str(stConfig.pReportShm->dwAttrSrvIp), stConfig.pReportShm->wAttrServerPort);
		}
	}

	uint32_t dwTimeMs = GET_DIFF_TIME_MS(psess->dwSendTimeSec, psess->dwSendTimeUsec);
	stConfig.pReportShm->dwLastHelloOkTime = stConfig.dwCurTime;

    if(stConfig.pReportShm->dwBindDES-PWESetTime != ntohl(presp->dwBindDES-PWESetTime))  {
        stConfig.pReportShm->dwBindDES-PWESetTime = ntohl(presp->dwBindDES-PWESetTime);
	    stConfig.pReportShm->iBindCloudUserId = ntohl(presp->iBindCloudUserId);
        if(stConfig.pReportShm->iBindCloudUserId != 0) {
            strncpy(stConfig.pReportShm->szBindCloudKey, presp->szBindCloudKey, sizeof(stConfig.pReportShm->szBindCloudKey)-1);
            stConfig.pReportShm->szBindCloudKey[sizeof(stConfig.pReportShm->szBindCloudKey)-1] = '\0';
        }
    }

    presp->szRespDES-PWEUrl[sizeof(presp->szRespDES-PWEUrl)-1] = '\0';
    if(strcmp(presp->szRespDES-PWEUrl, stConfig.szDownCloudUrl)) {
        strncpy(stConfig.szDownCloudUrl, presp->szRespDES-PWEUrl, sizeof(stConfig.szDownCloudUrl)-1);
        stConfig.szDownCloudUrl[sizeof(stConfig.szDownCloudUrl)-1] = '\0';
        INFO_LOG("update download DES-PWE url to:%s", stConfig.szDownCloudUrl);

        TConfigItemList list;
        TConfigItem *pitem = new TConfigItem;
        pitem->strConfigName = "DES-PWE_DOWNLOAD_URL";
        pitem->strConfigValue = presp->szRespDES-PWEUrl;
        list.push_back(pitem);
		if(UpdateConfigFile(stConfig.szCurPath, MTREPORT_CONFIG, list) < 0) 
            WARN_LOG("UpdateConfigFile :%s, config:DES-PWE_DOWNLOAD_URL to:%s failed!",
                MTREPORT_CONFIG, presp->szRespDES-PWEUrl);
		ReleaseConfigList(list);
    }

	INFO_LOG("hello response check ok, use time:%u (ms)", dwTimeMs);
	stConfig.bCheckHelloStart = true;
	return 0;
}

int DealResponseHelloFirst(CBasicPacket &pkg)
{
	PKGSESSION *psess = stConfig.pPkgSess;
	char sCmdContentBuf[1024] = {0};
	size_t iDecSigLen = 0;
	aes_decipher_data((const uint8_t*)pkg.m_pstCmdContent, pkg.m_wCmdContentLen,
	    (uint8_t*)sCmdContentBuf, &iDecSigLen, (const uint8_t*)psess->stCmdSessData.hello.sRespEncKey, AES_128);
	if(iDecSigLen != sizeof(MonitorHelloFirstContentResp)) {
		REQERR_LOG("MtDecrypt failed - key:%s datalen:%d, check:%d != %d",
			DumpStrByMask(psess->stCmdSessData.hello.sRespEncKey, 16), pkg.m_wCmdContentLen, 
			(int)iDecSigLen, (int)sizeof(MonitorHelloFirstContentResp));
		return ERR_DECRYPT_FAILED;
	}

	if(stConfig.iEnableEncryptData) {
		memcpy(stConfig.pReportShm->sRandKey, psess->stCmdSessData.hello.sRespEncKey, 16);
		INFO_LOG("set randkey: [ %s ]", DumpStrByMask(stConfig.pReportShm->sRandKey, 16));
	}

	MonitorHelloFirstContentResp *presp = (MonitorHelloFirstContentResp*)sCmdContentBuf;
	stConfig.pReportShm->iMtClientIndex = ntohl(presp->iMtClientIndex);
	stConfig.pReportShm->iMachineId = ntohl(presp->iMachineId);
	stConfig.pReportShm->dwConnServerIp = presp->dwConnServerIp;
	stConfig.pReportShm->bFirstHelloCheckOk = 1;
	stConfig.pReportShm->dwLastHelloOkTime = stConfig.dwCurTime;
	stConfig.pReportShm->wAttrServerPort = ntohs(presp->wAttrSrvPort);
	stConfig.pReportShm->dwAttrSrvIp = ntohl(presp->dwAttrSrvIp);

    if(stConfig.pReportShm->dwBindDES-PWESetTime != ntohl(presp->dwBindDES-PWESetTime))  {
        stConfig.pReportShm->dwBindDES-PWESetTime = ntohl(presp->dwBindDES-PWESetTime);
	    stConfig.pReportShm->iBindCloudUserId = ntohl(presp->iBindCloudUserId);
        if(stConfig.pReportShm->iBindCloudUserId != 0) {
            strncpy(stConfig.pReportShm->szBindCloudKey, presp->szBindCloudKey, sizeof(stConfig.pReportShm->szBindCloudKey)-1);
            stConfig.pReportShm->szBindCloudKey[sizeof(stConfig.pReportShm->szBindCloudKey)-1] = '\0';
        }
    }

	INFO_LOG("first hello response ok - client index:%d, machine:%d, enc:%d, bind (user:%d)", 
		stConfig.pReportShm->iMtClientIndex,  stConfig.pReportShm->iMachineId, stConfig.iEnableEncryptData,
		stConfig.pReportShm->iBindCloudUserId);

	stConfig.pReportShm->dwConnCfgServerIp = inet_addr(stConfig.szSrvIp_master);
	stConfig.pReportShm->wConnCfgServerPort = stConfig.iSrvPort;

	TConfigItemList list;
	TConfigItem *pitem = NULL;
	if(presp->szNewMasterSrvIp[0] != '\0' && strcmp(presp->szNewMasterSrvIp, stConfig.szSrvIp_master))
	{
		pitem = new TConfigItem;
		pitem->strConfigName = "SERVER_MASTER";
		pitem->strConfigValue = presp->szNewMasterSrvIp;
		list.push_back(pitem);
	}

	if(presp->wNewSrvPort != 0 && ntohs(presp->wNewSrvPort) != stConfig.iSrvPort) 
	{
		pitem = new TConfigItem;
		pitem->strConfigName = "SERVER_PORT";
		pitem->strConfigValue = itoa(ntohs(presp->wNewSrvPort));
		list.push_back(pitem);
	}

	DEBUG_LOG("mip:%s port:%d size:%u",
		presp->szNewMasterSrvIp, ntohs(presp->wNewSrvPort), (unsigned)(list.size()));
	if(list.size() > 0)
	{
		UpdateConfigFile(stConfig.szCurPath, MTREPORT_CONFIG, list);
		ReleaseConfigList(list);
	}
	return 0;
}

static int CmpSLogConfig(const void *a, const void *b)
{
	if(((SLogConfig*)a)->dwCfgId > ((SLogConfig*)b)->dwCfgId)
		return 1;
	else if(((SLogConfig*)a)->dwCfgId < ((SLogConfig*)b)->dwCfgId)
		return -1;
	return 0;
}

static int CmpAppConfig(const void *a, const void *b)
{
	return ((AppInfo*)a)->iAppId - ((AppInfo*)b)->iAppId;
}

static int ReadJsonFromFile(CmdS2cPreInstallContentReq *pct, Json &js, const char *pfile)
{
    FILE *fp = fopen(pfile, "rb");
    if(!fp) {
        PLUGIN_INST_LOG("open file:%s failed, msg:%s", pfile, strerror(errno));
        return ERROR_LINE;
    }

    std::string str;
    char sBuf[1024];
    int iRet = 0, iSize = 0;
    while((iRet=fread(sBuf, 1, 1024, fp)) > 0) {
        if(ferror(fp)) {
            PLUGIN_INST_LOG("read file:%s failed, msg:%s", pfile, strerror(errno));
            fclose(fp);
            return ERROR_LINE;
        }
        iSize += iRet;
        str.append(sBuf, iRet);
        if(feof(fp)) 
            break;
    }
    fclose(fp);

    size_t iParseIdx = 0;
    js.Parse(str.c_str(), iParseIdx);
    if(iParseIdx != str.size()) {
        PLUGIN_INST_LOG("parse json failed %lu != %lu, file:%s", iParseIdx, str.size(), pfile);
        return ERROR_LINE;
    }
    PLUGIN_INST_LOG("read plugin tmp json file size:%d", iSize);
    return 0;
}

static int WriteEncJsonToFile(CmdS2cPreInstallContentReq *pct, Json &js_down, const char *pfile, const char *pkey)
{
    char sEncBuf[1024] = {0};

    if((int)(js_down["ret"]) != 0 || !js_down.HasValue("pack_len") || !js_down.HasValue("pack_seg_len"))
    {
        PLUGIN_INST_LOG("download plugin packet ret:%d, msg:%s, has(%d, %d)", (int)(js_down["ret"]),
            (js_down.HasValue("msg") ? (const char*)(js_down["msg"]) : ""), js_down.HasValue("pack_len"),
            js_down.HasValue("pack_seg_len"));
        return ERROR_LINE;
    }

    int iPluginPackLen = (int)(js_down["pack_len"]);
    int iSegLen = (int)(js_down["pack_seg_len"]);

    std::string strPack;
    Base64 b64;
    size_t iDecSigLen = 0;
    if(iPluginPackLen > 2*iSegLen) {
        FILE *fp = fopen(pfile, "w+");
        if(!fp) {
            PLUGIN_INST_LOG("create file:%s failed, msg:%s", pfile, strerror(errno));
            return ERROR_LINE; 
        }

        // read and write start seg
        b64.decode(js_down["start_seg_base64_pack_str"], strPack);
        if((int)strPack.size() != (int)(js_down["start_seg_enc_str_len"]) || (int)(js_down["start_seg_enc_str_len"]) > 500)
        {
            PLUGIN_INST_LOG("check plugin packet start seg enc string length failed, %d != %d or segLen:%d > 500",
                (int)(strPack.size()), (int)(js_down["start_seg_enc_str_len"]), (int)(js_down["start_seg_enc_str_len"]));
            fclose(fp);
            return ERROR_LINE;
        }
        iDecSigLen = 0;
        aes_decipher_data((const uint8_t*)strPack.c_str(), 
            strPack.size(), (uint8_t*)sEncBuf, &iDecSigLen, (const uint8_t*)pkey, AES_128);
        if((int)iDecSigLen != iSegLen) {
            PLUGIN_INST_LOG("check plugin packet start seg dec length failed, %d != %d", (int)iDecSigLen, iSegLen);
            fclose(fp);
            return ERROR_LINE;
        }
        if(fwrite(sEncBuf, 1, iDecSigLen, fp) != iDecSigLen) {
            PLUGIN_INST_LOG("write file:%s failed, msg:%s, length:%lu", pfile, strerror(errno), iDecSigLen);
            fclose(fp);
            return ERROR_LINE; 
        }

        // read and write mid seg
        b64.decode(js_down["mid_seg_base64_str"], strPack);
        if((int)strPack.size() != iPluginPackLen-2*iSegLen)
        {
            PLUGIN_INST_LOG("check plugin packet mid seg length failed, %d != %d", 
                (int)(strPack.size()), iPluginPackLen-2*iSegLen);
            fclose(fp);
            return ERROR_LINE;
        }
        if(fwrite(strPack.c_str(), 1, strPack.size(), fp) != strPack.size()) {
            PLUGIN_INST_LOG("write file:%s failed, msg:%s, length:%lu", pfile, strerror(errno), strPack.size());
            fclose(fp);
            return ERROR_LINE; 
        }

        // read and write end seg
        b64.decode(js_down["end_seg_base64_pack_str"], strPack);
        if((int)strPack.size() != (int)(js_down["end_seg_enc_str_len"]) || (int)(js_down["end_seg_enc_str_len"]) > 500)
        {
            PLUGIN_INST_LOG("check plugin packet end seg enc string length failed, %d != %d or segLen:%d > 500",
                (int)(strPack.size()), (int)(js_down["end_seg_enc_str_len"]), (int)(js_down["end_seg_enc_str_len"]));
            fclose(fp);
            return ERROR_LINE;
        }
        iDecSigLen = 0;
        aes_decipher_data((const uint8_t*)strPack.c_str(), 
            strPack.size(), (uint8_t*)sEncBuf, &iDecSigLen, (const uint8_t*)pkey, AES_128);
        if((int)iDecSigLen != iSegLen) {
            PLUGIN_INST_LOG("check plugin packet end seg dec length failed, %d != %d", (int)iDecSigLen, iSegLen);
            fclose(fp);
            return ERROR_LINE;
        }
        if(fwrite(sEncBuf, 1, iDecSigLen, fp) != iDecSigLen) {
            PLUGIN_INST_LOG("write file:%s failed, msg:%s, length:%lu", pfile, strerror(errno), iDecSigLen);
            fclose(fp);
            return ERROR_LINE; 
        }
        fclose(fp);
    }
    else {
        b64.decode(js_down["base64_pack_str"], strPack);
        if((int)strPack.size() != (int)(js_down["enc_str_len"]) || iPluginPackLen > 500)
        {
            PLUGIN_INST_LOG("check plugin packet enc string length failed, %d != %d or packLen:%d > 500",
                (int)(strPack.size()), (int)(js_down["enc_str_len"]), iPluginPackLen);
            return ERROR_LINE;
        }

        iDecSigLen = 0;
        aes_decipher_data((const uint8_t*)strPack.c_str(), 
            strPack.size(), (uint8_t*)sEncBuf, &iDecSigLen, (const uint8_t*)pkey, AES_128);
        if((int)iDecSigLen != iPluginPackLen) {
            PLUGIN_INST_LOG("check plugin packet length failed, %d != %d", (int)iDecSigLen, iPluginPackLen);
            return ERROR_LINE;
        }

        FILE *fp = fopen(pfile, "w+");
        if(!fp) {
            PLUGIN_INST_LOG("create file:%s failed, msg:%s", pfile, strerror(errno));
            return ERROR_LINE; 
        }
        if(fwrite(sEncBuf, 1, iDecSigLen, fp) != iDecSigLen) {
            PLUGIN_INST_LOG("write file:%s failed, msg:%s, length:%lu", pfile, strerror(errno), iDecSigLen);
            fclose(fp);
            return ERROR_LINE; 
        }
        fclose(fp);
    }
    PLUGIN_INST_LOG("make plugin tar file:%s, size:%d ok", pfile, iPluginPackLen);
    return 0;
}

static int DownloadPluginPacket(CmdS2cPreInstallContentReq *pct, Json &jsret)
{
    const char *plugin_name = pct->sPluginName;
    std::ostringstream sInstallPath;
    if(stConfig.szPlusPath[0] != '/') 
        sInstallPath << stConfig.szCurPath << "/" << stConfig.szPlusPath << "/" << plugin_name;
    else 
        sInstallPath << stConfig.szPlusPath << "/" << plugin_name;

    std::ostringstream sInstallLogFile;
    sInstallLogFile << stConfig.szCurPath << "/plugin_install_log/" << plugin_name << "_install.log ";

    // 创建部署目录
    std::ostringstream ss;
    ss << "mkdir -p " << sInstallPath.str() << "; echo $?";
    std::string strResult;
    if(get_cmd_result(ss.str().c_str(), strResult) == 0 && strResult.length() > 0) {
        if(strResult != "0") {
            SendPreInstallStatusToServer(pct, EV_PREINSTALL_ERR_MKDIR);
            PLUGIN_INST_LOG("mkdir failed, cmd:%s, result:%s, msg:%s", ss.str().c_str(), strResult.c_str(), strerror(errno));
            return 1;
        }
        PLUGIN_INST_LOG("create plugin install dir, execute cmd:%s, result:%s", ss.str().c_str(), strResult.c_str());
    }
    else {
        SendPreInstallStatusToServer(pct, EV_PREINSTALL_ERR_MKDIR);
        PLUGIN_INST_LOG("execute cmd:%s failed, msg:%s", ss.str().c_str(), strerror(errno));
        return 2;
    }

    const char *pkey = NULL;
    std::ostringstream ss_local_tmp;
    ss_local_tmp << plugin_name << ".tmp";
    std::ostringstream ss_down;
    ss_down << "cd " << sInstallPath.str() << "; wget -a " << sInstallLogFile.str() << " -T 30 -O " << ss_local_tmp.str();

    if(pct->bIsInnerMode) {
        std::string strurl(pct->sLocalCfgUrl), strDomain, strDir;
        size_t s = strurl.find("//");
        size_t e = strurl.find("/", s+2);
        if(s != std::string::npos && e != std::string::npos && e > s+2) {
            strDomain = strurl.substr(s+2, e-s-2);
        }
        else {
            PLUGIN_INST_LOG("get local domain from url:%s failed !", pct->sLocalCfgUrl);
            SendPreInstallStatusToServer(pct, EV_PREINSTALL_ERR_GET_LOCAL_URL);
            return 3;
        }

        size_t e_d = strurl.rfind("/");
        if(e != std::string::npos && e_d != std::string::npos && e_d > e)
            strDir = strurl.substr(e, e_d-e+1);
        else {
            PLUGIN_INST_LOG("get local web dir from url:%s failed ", pct->sLocalCfgUrl);
            SendPreInstallStatusToServer(pct, EV_PREINSTALL_ERR_GET_LOCAL_URL);
            return 4;
        }

        if(stConfig.szLocalDomain[0] != '\0')
            ss_down << " \"http://" << stConfig.szLocalDomain;
        else 
            ss_down << " \"http://" << strDomain;
        ss_down << strDir << plugin_name << "_install_" 
            << pct->iMachineId << "_" << pct->iPluginId << ".tar.gz" << "\"; echo 0;";
    }
    else {
        if(stConfig.pReportShm->iBindCloudUserId == 0 && jsret.HasValue("download_str")) 
            pkey = jsret["download_str"];
        else if(stConfig.pReportShm->iBindCloudUserId != 0) 
            pkey = stConfig.pReportShm->szBindCloudKey;
        else {
            PLUGIN_INST_LOG("get key failed for download plugin");
            SendPreInstallStatusToServer(pct, EV_PREINSTALL_ERR_GET_KEY_FAILED);
            return 11;
        }

        // 生成校验字符串
        char sEncBuf[1024] = {0};
        const char *pstr = (const char*)(jsret["download_ver"]);
        int iStrVerLen = (int)strlen(pstr);
        int iSigBufLen = ((iStrVerLen>>4)+1)<<4;
        if(iSigBufLen > (int)sizeof(sEncBuf)) {
            PLUGIN_INST_LOG("need more space");
            SendPreInstallStatusToServer(pct, EV_PREINSTALL_ERR_GET_URL_RET);
            return 12;
        }
        aes_cipher_data((const uint8_t*)pstr, iStrVerLen, (uint8_t*)sEncBuf, (const uint8_t*)pkey, AES_128);
        Base64 b64;
        std::string strAccess;
        b64.encode(sEncBuf, iSigBufLen, strAccess); 

        const char *pXrkUrl = stConfig.szDownCloudUrl;
        if(stConfig.szCloudUrl[0] != '\0') {
            pXrkUrl = stConfig.szCloudUrl;
            if(strcmp(stConfig.szCloudUrl, stConfig.szDownCloudUrl)) {
                WARN_LOG("use local DES-PWE url:%s, download is:%s", stConfig.szCloudUrl, stConfig.szDownCloudUrl);
            }
        }

        // 使用 wget 执行下载，默认使用 http 协议
        if(strncmp(pXrkUrl, "http://", 7) && strncmp(pXrkUrl, "https://", 8))
            ss_down << " \"http://" << pXrkUrl << "/" << "cgi-bin/mt_slog_open?action=download_plugin";
        else
            ss_down << " \"" << pXrkUrl << "/" << "cgi-bin/mt_slog_open?action=download_plugin";
        ss_down << "&plugin=" << pct->iPluginId << "&user=" << stConfig.pReportShm->iBindCloudUserId; 
        ss_down << "&enc_str_len=" << iSigBufLen;
        ss_down << "&dec_str_len=" << iStrVerLen << "&access_str=" << url_escape(strAccess.c_str()) << "\"; echo 0";
    }

    // 下载并解密生成压缩包
    get_cmd_result(ss_down.str().c_str(), strResult);
    PLUGIN_INST_LOG("download packet, execute cmd:%s, result:%s", ss_down.str().c_str(), strResult.c_str());
    std::ostringstream ss_abs_tmp;
    ss_abs_tmp << sInstallPath.str() << "/" << ss_local_tmp.str();

    Json js_down;
    if(ReadJsonFromFile(pct, js_down, ss_abs_tmp.str().c_str()) != 0) {
        PLUGIN_INST_LOG("read json from file:%s failed !", ss_abs_tmp.str().c_str());
        SendPreInstallStatusToServer(pct, EV_PREINSTALL_ERR_PARSE_JSON_FILE);
        return 13;
    }

    if(pct->bIsInnerMode) {
        if(stConfig.pReportShm->iBindCloudUserId == 0 && js_down.HasValue("download_str")) 
            pkey = js_down["download_str"];
        else if(stConfig.pReportShm->iBindCloudUserId != 0) 
            pkey = stConfig.pReportShm->szBindCloudKey;
        else {
            PLUGIN_INST_LOG("get key failed !");
            SendPreInstallStatusToServer(pct, EV_PREINSTALL_ERR_GET_KEY_FAILED);
            return 11;
        }
        jsret["plugin_name"] = plugin_name;
    }

    std::ostringstream ss_local_name, ss_abs_plugin_tar;
    ss_local_name << plugin_name << ".tar.gz";
    ss_abs_plugin_tar << sInstallPath.str() << "/" << ss_local_name.str();
    if(WriteEncJsonToFile(pct, js_down, ss_abs_plugin_tar.str().c_str(), pkey) != 0) {
        SendPreInstallStatusToServer(pct, EV_PREINSTALL_ERR_MAKE_PLUGIN_TAR);
        return 14;
    }

    jsret["local_tar_name"] = ss_local_name.str();
    jsret["local_plugin_path"] = sInstallPath.str();

	// 下载开源版配置文件
	ss.str("");
    ss_down.str("");
	ss_local_name.str("");

	if(!strcmp(pct->sDevLang, "javascript"))
		ss_local_name << plugin_name << "_conf.js";
	else
		ss_local_name << "xrk_" << plugin_name << ".conf";
    ss_down << "cd " << sInstallPath.str() << "; wget -a " << sInstallLogFile.str() << " -T 30 -O " << ss_local_name.str();
	if(stConfig.szLocalDomain[0] != '\0') {
		std::string strurl(pct->sLocalCfgUrl);
		size_t s = strurl.find("//");
		size_t e = strurl.find("/", s+2);
		if(s != std::string::npos && e != std::string::npos && e > s+2) {
			strurl.replace(s+2, e-s-2, stConfig.szLocalDomain);
    		ss_down << " " << strurl << "; echo 0"; 
		}
		else {
			// url 校验不符合预期，不进行域替换
			PLUGIN_INST_LOG("change local config url failed(%s)", pct->sLocalCfgUrl);
    		ss_down << " " << pct->sLocalCfgUrl << "; echo 0"; 
		}
	}
	else
    	ss_down << " " << pct->sLocalCfgUrl << "; echo 0"; 

    get_cmd_result(ss_down.str().c_str(), strResult);
    PLUGIN_INST_LOG("download config, execute cmd:%s, result:%s", ss_down.str().c_str(), strResult.c_str());

    // 检查插件开源版配置文件是否存在, 同时删除下载的临时文件
    ss.str("");
    ss << "cd " << sInstallPath.str() << "; rm -f " << ss_local_tmp.str() << ";";
    ss << " [ -s " << ss_local_name.str() << " ] && echo 0 || echo 1";
    get_cmd_result(ss.str().c_str(), strResult);
    if(strResult != "0") {
        SendPreInstallStatusToServer(pct, EV_PREINSTALL_ERR_DOWNLOAD_OPEN_CFG);
        PLUGIN_INST_LOG("download config check failed, cmd:%s, result:%s", ss.str().c_str(), strResult.c_str());
        return 4;
    }

    PLUGIN_INST_LOG("download plugin packet check ok, cmd:%s, result:%s", ss.str().c_str(), strResult.c_str());
    return 0;
}

static int InstallPluginPacket(CmdS2cPreInstallContentReq *pct, Json &jsret)
{
    // 解压压缩包，并检查是否存在部署脚本、配置文件、可执行文件
    std::ostringstream ss;
    ss << "cd " << (const char*)(jsret["local_plugin_path"]) << "; tar -zxf "
        << (const char*)(jsret["local_tar_name"]) << "; [ -x start.sh -a  -x stop.sh "
        << " -a -x restart.sh -a -x auto_install.sh -a -x auto_uninstall.sh ";
    if(!strncmp(pct->sDevLang, g_strDevLangShell.c_str(), g_strDevLangShell.size())) 
        ss << " -a -x xrk_" << (const char*)(jsret["plugin_name"]) << ".sh "
            << " -a -x add_crontab.sh -a -x remove_crontab.sh "
            << " -a -s xrk_" << (const char*)(jsret["plugin_name"]) << ".conf ] && echo 0 || echo 1";
    else if(!strncmp(pct->sDevLang, g_strDevLangJs.c_str(), g_strDevLangJs.size()))
        ss << " -a -s dmt_DES-PWE.js "
            << " -a -s " << (const char*)(jsret["plugin_name"]) << ".js "
            << " -a -s " << (const char*)(jsret["plugin_name"]) << "_conf.js ] && echo 0 || echo 1";
    else  
        ss << " -a -x xrk_" << (const char*)(jsret["plugin_name"]) 
            << " -a -x add_crontab.sh -a -x remove_crontab.sh "
            << " -a -s xrk_" << (const char*)(jsret["plugin_name"]) << ".conf ] && echo 0 || echo 1";

    std::string strResult;
    get_cmd_result(ss.str().c_str(), strResult);
    if(strResult != "0") {
        PLUGIN_INST_LOG("unpacket check failed, cmd:%s, result:%s", ss.str().c_str(), strResult.c_str());
        SendPreInstallStatusToServer(pct, EV_PREINSTALL_ERR_UNPACK);
        return 1;
    }
    PLUGIN_INST_LOG("unpacket check ok, cmd:%s, result:%s", ss.str().c_str(), strResult.c_str());

    ss.str("");
    ss << "export install_log_file=" 
        << stConfig.szCurPath << "/plugin_install_log/" << (const char*)(jsret["plugin_name"]) << "_install.log; "
        << "cd " << (const char*)(jsret["local_plugin_path"]) << "; ./auto_install.sh; rm -f " 
        << (const char*)(jsret["local_tar_name"]);
    get_cmd_result(ss.str().c_str(), strResult);
    if(strResult.find("failed") != std::string::npos) {
        PLUGIN_INST_LOG("install plugin failed, cmd:%s, result:%s", ss.str().c_str(), strResult.c_str());
        SendPreInstallStatusToServer(pct, EV_PREINSTALL_ERR_START_PLUGIN);
        return 2;
    }
    PLUGIN_INST_LOG("install plugin ok, cmd:%s, result:%s", ss.str().c_str(), strResult.c_str());
    if(!strncmp(pct->sDevLang, g_strDevLangJs.c_str(), g_strDevLangJs.size()))
        SendPreInstallStatusToServer(pct, EV_PREINSTALL_CLIENT_INSTALL_PLUGIN_OK);
    else
        SendPreInstallStatusToServer(pct, EV_PREINSTALL_CLIENT_START_PLUGIN);
    stConfig.pReportShm->bHasNewPluginInfo = 1;
    return 0;
}

// s2c modify machine plugin config 
int DealModMachinePluginCfg(CBasicPacket &pkg)
{
    if(pkg.m_pstReqHead->qwSessionId != 0)
        stConfig.qwServerPacketSessId = ntohll(pkg.m_pstReqHead->qwSessionId);

	int iBufLen = 0;
    CmdS2cModMachPluginCfgReq *pct = NULL;
	if(stConfig.iEnableEncryptData) {
		static char sCmdContentBuf[4096+256] = {0};
		size_t iDecSigLen = 0;
		aes_decipher_data((const uint8_t*)pkg.m_pstCmdContent, pkg.m_wCmdContentLen,
			(uint8_t*)sCmdContentBuf, &iDecSigLen, (const uint8_t*)stConfig.pReportShm->sRandKey, AES_128);
		pct = (CmdS2cModMachPluginCfgReq*)sCmdContentBuf;
		iBufLen = (int)iDecSigLen;
	}
	else {
		pct = (CmdS2cModMachPluginCfgReq*)pkg.m_pstCmdContent;
		iBufLen = pkg.m_wCmdContentLen;
	}

    if((int)(pkg.m_wCmdContentLen) <= MYSIZEOF(CmdS2cModMachPluginCfgReq)) {
        REQERR_LOG("modify plugin config, check content failed, %d <= %d",
            (int)(pkg.m_wCmdContentLen), MYSIZEOF(CmdS2cModMachPluginCfgReq));
        return ERR_INVALID_PACKET_LEN;
    }

    pct->dwDownCfgTime = ntohl(pct->dwDownCfgTime);
    pct->iPluginId = ntohl(pct->iPluginId);
    pct->iMachineId = ntohl(pct->iMachineId);
    pct->iConfigLen = ntohl(pct->iConfigLen);
    if((int)(pct->iConfigLen+sizeof(CmdS2cModMachPluginCfgReq)) != iBufLen){
        REQERR_LOG("modify plugin config,  check content length failed, %lu != %d", 
            pct->iConfigLen+sizeof(CmdS2cModMachPluginCfgReq)+1, iBufLen);
        return ERR_INVALID_PACKET_LEN;
    }
    pct->strCfgs[pct->iConfigLen-1] = '\0';

    int iPluginIdx = -1;
    for(int j=0; j < MAX_INNER_PLUS_COUNT; j++) {
        if(pct->iPluginId == stConfig.pReportShm->stPluginInfo[j].iPluginId) {
            iPluginIdx = j;
            break;
        }
    }
    if(iPluginIdx < 0) {
        REQERR_LOG("modify plugin config, not find plugin plugin:%d", pct->iPluginId);
        return ERR_INVALID_PACKET;
    }

    // pct->strCfgs: CFG_ITEM CFG_ITEM_VAL;CFG_ITEM CFG_ITEM_VAL;
	TConfigItemList list;
	TConfigItem *pitem = NULL;
    char *pitem_name = NULL, *pitem_val = NULL, *ptmp_e = NULL;
    char *ptmp = pct->strCfgs;

    // 提取要修改的配置项
    for(int i=0; *ptmp != '\0' && i >= 0;) 
    {
        if(i == 0) {
            // 提取配置项宏名
            ptmp_e = strchr(ptmp, ' ');
            pitem_name = ptmp;
            if(ptmp_e != NULL) {
                *ptmp_e = '\0';
                ptmp = ptmp_e+1;
            }
            else {
                *ptmp = '\0';
            }
            i = 1;
        }
        else {
            // 提取配置项值
            ptmp_e = strchr(ptmp, ';');
            pitem_val = ptmp;
            if(ptmp_e != NULL) {
                *ptmp_e = '\0';
                ptmp = ptmp_e+1;
                i = 0;
            }
            else
                i = -1;

		    pitem = new TConfigItem;
            pitem->strConfigName = pitem_name;
            pitem->strConfigValue = pitem_val;
            DEBUG_LOG("try update config:%s val:%s", pitem_name, pitem_val);
		    list.push_back(pitem);
        }
    }

    // 找到插件配置文件
    std::ostringstream sPath;
    if(stConfig.szPlusPath[0] != '/') 
        sPath << stConfig.szCurPath << "/" << stConfig.szPlusPath << "/" 
            << stConfig.pReportShm->stPluginInfo[iPluginIdx].szPlusName;
    else 
        sPath << stConfig.szPlusPath << "/" << stConfig.pReportShm->stPluginInfo[iPluginIdx].szPlusName;

    // 配置文件名
    std::ostringstream sPluginCfgFile;
    sPluginCfgFile << "xrk_" << stConfig.pReportShm->stPluginInfo[iPluginIdx].szPlusName << ".conf"; 

    bool bUpCfg = false;
    if(list.size() > 0) {
        if(UpdateConfigFile(sPath.str().c_str(), sPluginCfgFile.str().c_str(), list) >= 0) {
            INFO_LOG("modify plugin config, update config item count:%d, config file:%s, restart:%d",
                (int)(list.size()), sPluginCfgFile.str().c_str(), pct->bRestartPlugin);
            bUpCfg = true;
        }
		ReleaseConfigList(list);
    }
    else {
        WARN_LOG("modify plugin config, plugin:%d, cfgs:%s, not find config item", pct->iPluginId, pct->strCfgs);
    }

    CmdS2cModMachPluginCfgResp resp;
    resp.iPluginId = htonl(pct->iPluginId);
    resp.iMachineId = htonl(pct->iMachineId);
    resp.dwDownCfgTime = htonl(pct->dwDownCfgTime);
    SendServerRespPkg((char*)&resp, (int)sizeof(resp), pkg);

    if(bUpCfg)
        SendPluginConfig(stConfig.pReportShm->stPluginInfo+iPluginIdx);

    // 立即重启插件
    if(pct->bRestartPlugin && bUpCfg) {
        std::ostringstream sRestart;
        sRestart << "cd " << sPath.str() << "; ./restart.sh";
        std::string strResult;
        get_cmd_result(sRestart.str().c_str(), strResult);
        if(strResult.find("failed") != std::string::npos)
            WARN_LOG("restart plugin, execute cmd:%s failed", sRestart.str().c_str());
        else
            DEBUG_LOG("restart plugin, execute cmd:%s ok", sRestart.str().c_str());
    }

    return 0;
}

// s2c machine opr plugin notify msg
int DealMachineOprPlugin(CBasicPacket &pkg)
{
	CmdS2cMachOprPluginReq *pct = NULL;
	int iBufLen = 0;

	if(stConfig.iEnableEncryptData) {
		static char sCmdContentBuf[4096+256] = {0};
		size_t iDecSigLen = 0;
		aes_decipher_data((const uint8_t*)pkg.m_pstCmdContent, pkg.m_wCmdContentLen,
			(uint8_t*)sCmdContentBuf, &iDecSigLen, (const uint8_t*)stConfig.pReportShm->sRandKey, AES_128);
		pct = (CmdS2cMachOprPluginReq*)sCmdContentBuf;
		iBufLen = (int)iDecSigLen;
	}
	else {
		pct = (CmdS2cMachOprPluginReq*)pkg.m_pstCmdContent;
		iBufLen = pkg.m_wCmdContentLen;
	}

    if(iBufLen != MYSIZEOF(CmdS2cMachOprPluginReq)) {
        REQERR_LOG("check content failed, %d != %d", iBufLen, MYSIZEOF(CmdS2cMachOprPluginReq));
        return ERR_INVALID_PACKET_LEN;
    }

    if(pkg.m_pstReqHead->qwSessionId != 0)
        stConfig.qwServerPacketSessId = ntohll(pkg.m_pstReqHead->qwSessionId);

    pct->iPluginId = ntohl(pct->iPluginId);
    pct->iMachineId = ntohl(pct->iMachineId);
    pct->iDbId = ntohl(pct->iDbId);
    pct->sPluginName[sizeof(pct->sPluginName)-1] = '\0';

    // 看下本地是否存在插件
    std::ostringstream sInstallPath;
    if(stConfig.szPlusPath[0] != '/') 
        sInstallPath << stConfig.szCurPath << "/" << stConfig.szPlusPath << "/" << pct->sPluginName;
    else 
        sInstallPath << stConfig.szPlusPath << "/" << pct->sPluginName;
    std::ostringstream sPluginInstallFile;
    sPluginInstallFile << sInstallPath.str() << "/auto_install.sh";
    if(!IsFileExist(sPluginInstallFile.str().c_str())) {
        REQERR_LOG("not find plugin file:%s, plugin:%s(%d)", sPluginInstallFile.str().c_str(), pct->sPluginName, pct->iPluginId);

        CmdS2cMachOprPluginResp resp;
        resp.iPluginId = ntohl(pct->iPluginId);
        resp.iMachineId = ntohl(pct->iMachineId);
        resp.iDbId = ntohl(pct->iDbId);
        resp.bOprResult = MACH_OPR_PLUGIN_NOT_FIND;
        SendServerRespPkg((char*)&resp, (int)sizeof(resp), pkg);
        return 0;
    }

    // 相关变更写入插件日志中
    std::ostringstream sInstallLogFile;
    sInstallLogFile << stConfig.szCurPath << "/plugin_install_log";
    struct stat sb;
    if(stat(sInstallLogFile.str().c_str(), &sb) < 0 || !S_ISDIR(sb.st_mode)) {
        std::ostringstream oscmd;
        oscmd << "rm -fr " << sInstallLogFile.str() << " > /dev/null 2>&1; mkdir -p " << sInstallLogFile.str();
        system(oscmd.str().c_str());
        INFO_LOG("create plugin log dir:%s", sInstallLogFile.str().c_str());
        usleep(10);
    }
    CmdS2cMachOprPluginResp resp;
    resp.iPluginId = ntohl(pct->iPluginId);
    resp.iMachineId = ntohl(pct->iMachineId);
    resp.iDbId = ntohl(pct->iDbId);
    resp.bOprResult = MACH_OPR_PLUGIN_SUCCESS;

    std::ostringstream ss;
    std::string strResult;
    switch(pkg.m_dwReqCmd) 
    {
        // 移除插件
        case CMD_MONI_S2C_MACH_ORP_PLUGIN_REMOVE: 
            {
				PLUGIN_INST_LOG("try modify plugin:%d, name:%s, machine id:%d, opr cmd:%u(uninstall)",
					pct->iPluginId, pct->sPluginName,  pct->iMachineId, pkg.m_dwReqCmd);

				ss << "export install_log_file=" << stConfig.szCurPath << "/plugin_install_log/" << pct->sPluginName
					<< "_install.log; cd " << sInstallPath.str() << "; ./auto_uninstall.sh; ";
                get_cmd_result(ss.str().c_str(), strResult);
                if(strResult.find("failed") != std::string::npos) {
                    resp.bOprResult = MACH_OPR_PLUGIN_REMOVE_FAILED;
                    PLUGIN_INST_LOG("remove plugin:%s(%d) failed, result:%s", pct->sPluginName, pct->iPluginId, strResult.c_str());
                }
                else {
                    PLUGIN_INST_LOG("remove plugin:%s(%d), execute:%s ok", pct->sPluginName, pct->iPluginId, ss.str().c_str());

                    // 删除整个插件目录
                    ss.str("");
                    ss << "cd " << sInstallPath.str() << "; cd ..; rm -fr " << pct->sPluginName << " > /dev/null 2>&1;";
                    get_cmd_result(ss.str().c_str(), strResult);
                    PLUGIN_INST_LOG("[ change ] remove plugin:%s(%d) dir:%s cmd:%s",
                        pct->sPluginName, pct->iPluginId, sInstallPath.str().c_str(), ss.str().c_str());

					for(int i=0; i < MAX_INNER_PLUS_COUNT; i++)  {
						if(stConfig.pReportShm->stPluginInfo[i].iPluginId == pct->iPluginId) {
							stConfig.pReportShm->stPluginInfo[i].iPluginId = 0;
							stConfig.pReportShm->iPluginInfoCount--;
							break;
						}
					}
                }
            }
            break;

        // 启用插件
        case CMD_MONI_S2C_MACH_ORP_PLUGIN_ENABLE:
            {
				PLUGIN_INST_LOG("try modify plugin:%d, name:%s, machine id:%d, opr cmd:%u(enable)",
					pct->iPluginId, pct->sPluginName,  pct->iMachineId, pkg.m_dwReqCmd);

                ss << "cd " << sInstallPath.str() << "; ./start.sh; ";
                get_cmd_result(ss.str().c_str(), strResult);
                if(strResult.find("failed") != std::string::npos) {
                    resp.bOprResult = MACH_OPR_PLUGIN_ENABLE_FAILED;
                    PLUGIN_INST_LOG("start plugin:%s(%d) failed, result:%s", pct->sPluginName, pct->iPluginId, strResult.c_str());
                }
                else {
                    PLUGIN_INST_LOG("start plugin:%s(%d), execute:%s ok", pct->sPluginName, pct->iPluginId, ss.str().c_str());
                }
            }
            break;

        // 禁用插件
        case CMD_MONI_S2C_MACH_ORP_PLUGIN_DISABLE:
            {
				PLUGIN_INST_LOG("try modify plugin:%d, name:%s, machine id:%d, opr cmd:%u(disable)",
					pct->iPluginId, pct->sPluginName,  pct->iMachineId, pkg.m_dwReqCmd);

                ss << "cd " << sInstallPath.str() << "; ./stop.sh; ";
                get_cmd_result(ss.str().c_str(), strResult);
                if(strResult.find("failed") != std::string::npos) {
                    resp.bOprResult = MACH_OPR_PLUGIN_DISABLE_FAILED;
                    PLUGIN_INST_LOG("stop plugin:%s(%d) failed, result:%s", pct->sPluginName, pct->iPluginId, strResult.c_str());
                }
                else {
                    PLUGIN_INST_LOG("stop plugin:%s(%d), execute:%s ok", pct->sPluginName, pct->iPluginId, ss.str().c_str());
					for(int i=0; i < MAX_INNER_PLUS_COUNT; i++)  {
						if(stConfig.pReportShm->stPluginInfo[i].iPluginId == pct->iPluginId) {
							stConfig.pReportShm->stPluginInfo[i].iPluginId = 0;
							stConfig.pReportShm->iPluginInfoCount--;
							break;
						}
					}
                }
            }
            break;

        default:
			PLUGIN_INST_LOG("try modify plugin:%d, name:%s, machine id:%d, opr cmd:%u(unknow)",
				pct->iPluginId, pct->sPluginName,  pct->iMachineId, pkg.m_dwReqCmd);
            REQERR_LOG("unknow cmd:%u", pkg.m_dwReqCmd);
            resp.bOprResult = MACH_OPR_PLUGIN_UNKNOW_OPR_CMD;
            return ERR_UNKNOW_CMD;
    }
    SendServerRespPkg((char*)&resp, (int)sizeof(resp), pkg);
	return 0;
}

// s2c install plugin notify msg
int DealPreInstallNotify(CBasicPacket &pkg)
{
	CmdS2cPreInstallContentReq *pct = NULL;
	int iBufLen = 0;
	if(stConfig.iEnableEncryptData) {
		static char sCmdContentBuf[2048+1024] = {0};
		size_t iDecSigLen = 0;
		aes_decipher_data((const uint8_t*)pkg.m_pstCmdContent, pkg.m_wCmdContentLen,
			(uint8_t*)sCmdContentBuf, &iDecSigLen, (const uint8_t*)stConfig.pReportShm->sRandKey, AES_128);
		pct = (CmdS2cPreInstallContentReq*)sCmdContentBuf;
		iBufLen = (int)iDecSigLen;
	}
	else {
		pct = (CmdS2cPreInstallContentReq*)pkg.m_pstCmdContent;
		iBufLen = pkg.m_wCmdContentLen;
	}

    if(iBufLen < (int)sizeof(CmdS2cPreInstallContentReq)
		|| iBufLen != (int)(ntohl(pct->iUrlLen)+sizeof(CmdS2cPreInstallContentReq))) {
        REQERR_LOG("check content failed, %d, %u-%d", 
			iBufLen, MYSIZEOF(CmdS2cPreInstallContentReq), ntohl(pct->iUrlLen));
        return ERR_INVALID_PACKET_LEN;
    }

    pct->iPluginId = ntohl(pct->iPluginId);
    pct->iMachineId = ntohl(pct->iMachineId);
    pct->iDbId = ntohl(pct->iDbId);
    pct->sCheckStr[sizeof(pct->sCheckStr)-1] = '\0';
    pct->sDevLang[sizeof(pct->sDevLang)-1] = '\0';
    pct->sPluginName[sizeof(pct->sPluginName)-1] = '\0';
	pct->iUrlLen = ntohl(pct->iUrlLen);

    std::string strMd5;
    int32_t iSeq = rand();
    if(stConfig.pReportShm->iBindCloudUserId != 0) {
        std::ostringstream ss;
        ss << stConfig.pReportShm->szBindCloudKey << "_8486" << stConfig.pReportShm->iBindCloudUserId << iSeq;
        unsigned char sBindMd5[64];
        Md5HashBuffer(sBindMd5, (unsigned char*)(ss.str().c_str()), ss.str().size());
        strMd5 = OI_DumpHex_Lower(sBindMd5, 0, MD5LEN);
    }

	std::ostringstream sInstallLogFile;
	sInstallLogFile << stConfig.szCurPath << "/plugin_install_log";
	struct stat sb;
	if(stat(sInstallLogFile.str().c_str(), &sb) < 0 || !S_ISDIR(sb.st_mode)) {
		std::ostringstream oscmd;
		oscmd << "rm -fr " << sInstallLogFile.str() << " > /dev/null 2>&1; mkdir -p " << sInstallLogFile.str();
		system(oscmd.str().c_str());
		INFO_LOG("create plugin log dir:%s", sInstallLogFile.str().c_str());
		usleep(10);
	}
	sInstallLogFile << "/" << pct->sPluginName << "_install.log ";

    PLUGIN_INST_LOG("try install plugin:%d, name:%s, check str:%s, machine id:%d, language:%s, IsInnerMode:%d",
        pct->iPluginId, pct->sPluginName, pct->sCheckStr, pct->iMachineId, pct->sDevLang, pct->bIsInnerMode);
    if(pkg.m_pstReqHead->qwSessionId != 0)
        stConfig.qwServerPacketSessId = ntohll(pkg.m_pstReqHead->qwSessionId);
 
    // 上报一下进度
    SendPreInstallStatusToServer(pct, EV_PREINSTALL_TO_CLIENT_OK);

    if(pct->bIsInnerMode) {
        // 内网模式部署，从本地web服务器获取部署包
        Json js;
        if(DownloadPluginPacket(pct, js) == 0) {
            SendPreInstallStatusToServer(pct, EV_PREINSTALL_CLIENT_GET_PACKET);
            InstallPluginPacket(pct, js);

            // 切回工作目录
            chdir(stConfig.szCurPath);
        }
    }
    else {
        // 外网模式部署，从云端获取部署包
        std::ostringstream ss;
	    ss << "resp=`wget -t 3 -O - -T " << stConfig.iPLuginInstallTimeoutSec << " -a " << sInstallLogFile.str();

        const char *pXrkUrl = stConfig.szDownCloudUrl;
        if(stConfig.szCloudUrl[0] != '\0') {
            pXrkUrl = stConfig.szCloudUrl;
            if(strcmp(stConfig.szCloudUrl, stConfig.szDownCloudUrl)) {
                WARN_LOG("use local DES-PWE url:%s, download is:%s", stConfig.szCloudUrl, stConfig.szDownCloudUrl);
            }
        }

        // 使用 wget 执行下载，默认使用 http 协议
        if(strncmp(pXrkUrl, "http://", 7) && strncmp(pXrkUrl, "https://", 8))
	        ss << " \"http://" << pXrkUrl << "/" << "cgi-bin/mt_slog_open?action=open_preinstall_plugin_enc";
        else
	        ss << " \"" << pXrkUrl << "/" << "cgi-bin/mt_slog_open?action=open_preinstall_plugin_enc";

        ss << "&os=" << stConfig.szOs << "&os_arc=" << stConfig.szOsArc << "&agent_ver=" << g_strVersion
            << "&libc_ver=" << stConfig.szLibcVer << "&libcpp_ver=" << stConfig.szLibcppVer
            << "&bind_user_seq=" << iSeq << "&bind_user_md5=" << strMd5 
            << "&plugin=" << pct->iPluginId << "&user=" << stConfig.pReportShm->iBindCloudUserId << "\"`; echo $resp";

        std::string strResult;
        if(get_cmd_result(ss.str().c_str(), strResult) == 0 && strResult.length() > 0) {
            PLUGIN_INST_LOG("get packet url - execute cmd:%s, result:%s", ss.str().c_str(), strResult.c_str());
            Json jsrsp;
            try{
                size_t iParseIdx = 0;
                jsrsp.Parse(strResult.c_str(), iParseIdx);
                if(iParseIdx != strResult.size()) {
                    PLUGIN_INST_LOG("parse json:%s failed, %lu != %lu", strResult.c_str(), iParseIdx, strResult.size());
                    SendPreInstallStatusToServer(pct, EV_PREINSTALL_ERR_GET_URL_PARSE_RET);
                }
                else if((int)(jsrsp["ret"]) != 0) {
                    PLUGIN_INST_LOG("get url failed, result:%s", strResult.c_str());
                    SendPreInstallStatusToServer(pct, EV_PREINSTALL_ERR_GET_URL_RET);
                }
	    		else if(strcmp(pct->sPluginName, (const char*)(jsrsp["plugin_name"]))) {
	    			PLUGIN_INST_LOG("check plugin failed, %s != %s", pct->sPluginName, (const char*)(jsrsp["plugin_name"]));
	    			SendPreInstallStatusToServer(pct, EV_PREINSTALL_ERR_GET_URL_RET);
	    			chdir(stConfig.szCurPath);
	    			return 0;
	    		}
	    	}catch(Exception e) {
	    		SendPreInstallStatusToServer(pct, EV_PREINSTALL_ERR_GET_URL_PARSE_RET);
                PLUGIN_INST_LOG("parse json:%s, exception:%s", strResult.c_str(), e.ToString().c_str());
	    		chdir(stConfig.szCurPath);
                return 0;
            };

            if((int)(jsrsp["ret"]) == 0) {
                SendPreInstallStatusToServer(pct, EV_PREINSTALL_CLIENT_GET_DOWN_URL);
                if(DownloadPluginPacket(pct, jsrsp) == 0) {
                    SendPreInstallStatusToServer(pct, EV_PREINSTALL_CLIENT_GET_PACKET);
                    InstallPluginPacket(pct, jsrsp);
                    // 切回工作目录
                    chdir(stConfig.szCurPath);
                }
            }
        }
        else {
            PLUGIN_INST_LOG("execute cmd:%s, failed", ss.str().c_str());
            SendPreInstallStatusToServer(pct, EV_PREINSTALL_ERR_GET_URL);
        }
    }
	return 0;
}

int DealRespCheckLogConfig(CBasicPacket &pkg)
{
	if(pkg.m_bRetCode != NO_ERROR) {
		WARN_LOG("check log config ret failed ! (%d)", pkg.m_bRetCode);
		return pkg.m_bRetCode;
	}

	TPkgBody *pRespTlvBody = NULL; 
	int iBufLen = 0;
	if(stConfig.iEnableEncryptData) {
		static char sCmdContentBuf[2048+1024] = {0};
		size_t iDecSigLen = 0;
		aes_decipher_data((const uint8_t*)pkg.m_pstCmdContent, pkg.m_wCmdContentLen,
			(uint8_t*)sCmdContentBuf, &iDecSigLen, (const uint8_t*)stConfig.pReportShm->sRandKey, AES_128);
		pRespTlvBody = (TPkgBody*)sCmdContentBuf;
		iBufLen = (int)iDecSigLen;
	}
	else {
		pRespTlvBody = (TPkgBody*)pkg.m_pstCmdContent;
		iBufLen = pkg.m_wCmdContentLen;
	}

	if(CheckPkgBody(pRespTlvBody, iBufLen) < 0) {
		REQERR_LOG("check log config tlvBody failed, length:%d", iBufLen);
		return ERR_CHECK_DATA_FAILED;
	}

// 网络数据结构字节序转换
#define LOG_CONFIG_NTOH(cfg) \
	(cfg).dwSeq = ntohl((cfg).dwSeq); \
	(cfg).dwCfgId = ntohl((cfg).dwCfgId); \
	(cfg).iAppId = ntohl((cfg).iAppId); \
	(cfg).iModuleId = ntohl((cfg).iModuleId); \
	(cfg).iLogType = ntohl((cfg).iLogType); \
	(cfg).dwSpeedFreq = ntohl((cfg).dwSpeedFreq); \
	(cfg).wTestKeyCount = ntohs((cfg).wTestKeyCount); 

// 网络数据结构转存到本地数据结构
#define NET_LOG_CONFIG_TO_LOCAL(local, net) \
	local.dwSeq = net.dwSeq; \
	local.dwCfgId = net.dwCfgId; \
	local.iAppId = net.iAppId; \
	local.iModuleId = net.iModuleId; \
	local.iLogType = net.iLogType; \
	local.dwSpeedFreq = net.dwSpeedFreq; \
	local.wTestKeyCount = net.wTestKeyCount; \
	if(net.wTestKeyCount > 0) \
	    memcpy(local.stTestKeys, net.stTestKeys, MYSIZEOF(SLogTestKey)*local.wTestKeyCount); 

	TWTlv *pTlv = NULL;
	int iTlvIdx = 0;
	MtSLogConfig *pcfg = NULL;
	SLogConfig *pcfgLocal = NULL;
	SLogConfig stCfgTmp;
	do {
		pTlv = GetWTlvType2_list(pRespTlvBody, &iTlvIdx);
		if(pTlv == NULL)
			break;

		if(ntohs(pTlv->wType) == TLV_MONI_CONFIG_MOD
			|| ntohs(pTlv->wType) == TLV_MONI_CONFIG_ADD)
		{ 
			pcfg = (MtSLogConfig*)(pTlv->sValue);
			LOG_CONFIG_NTOH(*pcfg);
		}

		if(ntohs(pTlv->wType) == TLV_MONI_CONFIG_ADD) { // 新增
			if(stConfig.pReportShm->wLogConfigCount >= MAX_LOG_CONFIG_COUNT) {
				ERROR_LOG("log config count over limit :%d", MAX_LOG_CONFIG_COUNT);
				break;
			}
			memset(stConfig.pReportShm->stLogConfig+stConfig.pReportShm->wLogConfigCount, 0, MYSIZEOF(SLogConfig));
			NET_LOG_CONFIG_TO_LOCAL(stConfig.pReportShm->stLogConfig[stConfig.pReportShm->wLogConfigCount], (*pcfg));
			stConfig.pReportShm->wLogConfigCount++;
			qsort(stConfig.pReportShm->stLogConfig, stConfig.pReportShm->wLogConfigCount,
				MYSIZEOF(SLogConfig), CmpSLogConfig);
			INFO_LOG("add log config id:%u seq:%u", pcfg->dwCfgId, pcfg->dwSeq);
		}
		else {
			if(ntohs(pTlv->wType) != TLV_MONI_CONFIG_MOD) 
				// 删除的情况，value 为 config id
				stCfgTmp.dwCfgId = ntohl(*(uint32_t*)(pTlv->sValue));
			else
				stCfgTmp.dwCfgId = pcfg->dwCfgId;
			pcfgLocal = (SLogConfig*)bsearch(&stCfgTmp, stConfig.pReportShm->stLogConfig,
				stConfig.pReportShm->wLogConfigCount, MYSIZEOF(SLogConfig), CmpSLogConfig);
			if(pcfgLocal == NULL) {
				ERROR_LOG("find log config id:%u failed !", pcfg->dwCfgId);
				continue;
			}
			if(ntohs(pTlv->wType) == TLV_MONI_CONFIG_MOD) { // 修改
				INFO_LOG("mod log config id:%u seq:%u(old seq:%u)", pcfg->dwCfgId, pcfg->dwSeq, pcfgLocal->dwSeq);
				NET_LOG_CONFIG_TO_LOCAL((*pcfgLocal), (*pcfg));
			}
			else  { // 删除
				int idx = ((char*)pcfgLocal - (char*)(stConfig.pReportShm->stLogConfig)) / MYSIZEOF(SLogConfig);
				if(idx < (int)(stConfig.pReportShm->wLogConfigCount-1)) 
					memmove(pcfgLocal, pcfgLocal+1, (stConfig.pReportShm->wLogConfigCount-1-idx)*MYSIZEOF(SLogConfig));
				stConfig.pReportShm->wLogConfigCount--;
				INFO_LOG("delete log config id:%u", pcfg->dwCfgId);
			}
		}
	}while(pTlv != NULL);
	stConfig.pReportShm->dwLastSyncLogConfigTime = stConfig.dwCurTime;
	return 0;
}

static uint32_t MakeCheckLogConfigPkg(uint32_t dwLastResponseTimeMs)
{
	CBasicPacket pkg;

	// head
	ReqPkgHead stHead;
	pkg.InitReqPkgHead(&stHead, CMD_MONI_CHECK_LOG_CONFIG);

	// cmd content
	static char sCheckBuf[MYSIZEOF(LogConfigReq)*MAX_LOG_CONFIG_COUNT+MYSIZEOF(ContentCheckLogConfig)];
	ContentCheckLogConfig *pCtInfo = (ContentCheckLogConfig*)sCheckBuf;
	pCtInfo->dwServerResponseTime = htonl(dwLastResponseTimeMs);
	pCtInfo->wLogConfigCount = htons(stConfig.pReportShm->wLogConfigCount);
	for(int i=0; i < stConfig.pReportShm->wLogConfigCount; i++) {
		if(!(stConfig.dwRestartFlag & RESTART_FLAG_CHECK_LOG_CONFIG))
			pCtInfo->stLogConfigList[i].dwSeq = htonl(stConfig.pReportShm->stLogConfig[i].dwSeq-1);
		else
			pCtInfo->stLogConfigList[i].dwSeq = htonl(stConfig.pReportShm->stLogConfig[i].dwSeq);
		pCtInfo->stLogConfigList[i].dwCfgId = htonl(stConfig.pReportShm->stLogConfig[i].dwCfgId);
	}
	stConfig.dwRestartFlag |= RESTART_FLAG_CHECK_LOG_CONFIG;

	int iContentLen = MYSIZEOF(ContentCheckLogConfig)
		+ MYSIZEOF(pCtInfo->stLogConfigList[0]) * stConfig.pReportShm->wLogConfigCount;
	pkg.InitCmdContent(pCtInfo, iContentLen);

	// 签名
	char sSig[MAX_SIGNATURE_LEN+MYSIZEOF(TSignature)]={0};
	TSignature *psig = (TSignature*)sSig;
	if(stConfig.iEnableEncryptData) {
		MonitorCommSig stSigInfo;
		stSigInfo.dwSeq = htonl(pkg.m_dwReqSeq);
		stSigInfo.dwCmd = htonl(pkg.m_dwReqCmd);
		if(InitSignature(psig, &stSigInfo, stConfig.pReportShm->sRandKey, MT_SIGNATURE_TYPE_COMMON) < 0)
			return -1;
	}
	pkg.InitSignature(psig);

	// tlv
	char sTlvBuf[128];
	TPkgBody *pbody = (TPkgBody*)sTlvBuf;
	TlvMoniCommInfo stTlvInfo;
	stTlvInfo.iMtClientIndex = htonl(stConfig.pReportShm->iMtClientIndex);
	stTlvInfo.iMachineId = htonl(stConfig.pReportShm->iMachineId);
	int iTlvBodyLen = MYSIZEOF(TPkgBody);
	iTlvBodyLen += SetWTlv(
		pbody->stTlv, TLV_MONI_COMM_INFO, MYSIZEOF(stTlvInfo), (const char*)&stTlvInfo);
	pbody->bTlvNum = 1;
	pkg.InitPkgBody(pbody, iTlvBodyLen);

	return pkg.MakeReqPkg(stConfig.pPkg, &stConfig.iPkgLen);
}

void InitCheckLogConfig(PKGSESSION *psess, uint32_t dwTimeOutMs, uint32_t dwSrvRespTime)
{
	if(!IsHelloValid()) {
		FATAL_LOG("check log config stop -- hello is invalid !");
		stConfig.pReportShm->cIsAgentRun = 2;
		return;
	}

	memset(&stConfig.sSessBuf, 0, MYSIZEOF(stConfig.sSessBuf));
	stConfig.pPkgSess = (PKGSESSION*)stConfig.sSessBuf;
	stConfig.pPkg = stConfig.sSessBuf+MYSIZEOF(PKGSESSION);
	stConfig.iPkgLen = PKG_BUFF_LENGTH;

	stConfig.pPkgSess->iSockIndex = psess->iSockIndex;
	stConfig.pPkgSess->bSessStatus = SESS_FLAG_TIMEOUT_SENDPKG;
	uint32_t dwKey = MakeCheckLogConfigPkg(dwSrvRespTime);
	uint32_t dwExpireTimeMs = (dwTimeOutMs != 0) ? dwTimeOutMs : (rand()%CMD_CHECK_CONFIG_TIME_MS+1);
	if(dwExpireTimeMs < 5*1000)
		dwExpireTimeMs = 5*1000;
	
	// 添加到定时器
	int iRet = AddTimer(dwKey, dwExpireTimeMs, OnPkgExpire,
		stConfig.pPkgSess, MYSIZEOF(PKGSESSION), stConfig.iPkgLen, stConfig.pPkg);
	if(iRet < 0) {
		ERROR_LOG("AddTimer failed ! pkglen:%d, key:%u, ret:%d", stConfig.iPkgLen, dwKey, iRet);
	}
	DEBUG_LOG("(check log config) - add timer key:%u - datalen:%u, socket idx:%d", 
		dwKey, stConfig.iPkgLen, psess->iSockIndex);
}

int DealRespCheckAppConfig(CBasicPacket &pkg)
{
	if(pkg.m_bRetCode != NO_ERROR) {
		WARN_LOG("cmd check app config ret failed ! (%d)", pkg.m_bRetCode);
		return pkg.m_bRetCode;
	}

	TPkgBody *pRespTlvBody = NULL; 
	int iBufLen = 0;
	if(stConfig.iEnableEncryptData) {
		static char sCmdContentBuf[2048+1024] = {0};
		size_t iDecSigLen = 0;
		aes_decipher_data((const uint8_t*)pkg.m_pstCmdContent, pkg.m_wCmdContentLen,
			(uint8_t*)sCmdContentBuf, &iDecSigLen, (const uint8_t*)stConfig.pReportShm->sRandKey, AES_128);
		pRespTlvBody = (TPkgBody*)sCmdContentBuf;
		iBufLen = (int)iDecSigLen;
	}
	else {
		pRespTlvBody = (TPkgBody*)pkg.m_pstCmdContent;
		iBufLen = pkg.m_wCmdContentLen;
	}

	if(CheckPkgBody(pRespTlvBody, iBufLen) < 0) {
		REQERR_LOG("check app config tlvBody failed, length:%d", iBufLen);
		return ERR_CHECK_DATA_FAILED;
	}

#define APP_CONFIG_NTOH(cfg) \
	(cfg).iAppId = ntohl((cfg).iAppId); \
	(cfg).wModuleCount = ntohs((cfg).wModuleCount); \
	(cfg).dwSeq = ntohl((cfg).dwSeq); \
	(cfg).dwAppSrvMaster = ntohl((cfg).dwAppSrvMaster); \
	(cfg).wLogSrvPort = ntohs((cfg).wLogSrvPort); 

#define NET_APP_CONFIG_TO_LOCAL(local, net) \
	local.iAppId = net.iAppId; \
	local.dwAppSrvMaster = net.dwAppSrvMaster; \
	local.wLogSrvPort = net.wLogSrvPort; \
	local.bAppType = net.bAppType; \
	local.wModuleCount = net.wModuleCount; \
	local.dwSeq = net.dwSeq; 

	TWTlv *pTlv = NULL;
	int iTlvIdx = 0;
	MtAppInfo *pcfg = NULL;
	AppInfo *pcfgLocal = NULL;
	AppInfo stCfgTmp;
	do {
		pTlv = GetWTlvType2_list(pRespTlvBody, &iTlvIdx);
		if(pTlv == NULL)
			break;

		if(ntohs(pTlv->wType) == TLV_MONI_CONFIG_MOD
			|| ntohs(pTlv->wType) == TLV_MONI_CONFIG_ADD)
		{ 
			pcfg = (MtAppInfo*)(pTlv->sValue);
			APP_CONFIG_NTOH(*pcfg);
		}

		if(ntohs(pTlv->wType) == TLV_MONI_CONFIG_ADD) { // 新增
			if(stConfig.pReportShm->wAppConfigCount >= MAX_APP_COUNT) {
				ERROR_LOG("app config count over limit :%d", MAX_APP_COUNT);
				break;
			}
			memset(stConfig.pReportShm->stAppConfigList+stConfig.pReportShm->wAppConfigCount, 0, MYSIZEOF(AppInfo));
			NET_APP_CONFIG_TO_LOCAL(stConfig.pReportShm->stAppConfigList[stConfig.pReportShm->wAppConfigCount], (*pcfg));
			stConfig.pReportShm->wAppConfigCount++;
			qsort(stConfig.pReportShm->stAppConfigList, stConfig.pReportShm->wAppConfigCount,
				MYSIZEOF(AppInfo), CmpAppConfig);
			INFO_LOG("add app config id:%d seq:%u", pcfg->iAppId, pcfg->dwSeq);
		}
		else {
			if(ntohs(pTlv->wType) != TLV_MONI_CONFIG_MOD) 
				// 删除的情况，value 为 app id
				stCfgTmp.iAppId = ntohl(*(int32_t*)(pTlv->sValue));
			else
				stCfgTmp.iAppId = pcfg->iAppId;
			pcfgLocal = (AppInfo*)bsearch(&stCfgTmp, stConfig.pReportShm->stAppConfigList,
				stConfig.pReportShm->wAppConfigCount, MYSIZEOF(AppInfo), CmpAppConfig);
			if(pcfgLocal == NULL) {
				ERROR_LOG("find app config id:%d failed !", pcfg->iAppId);
				continue;
			}
			if(ntohs(pTlv->wType) == TLV_MONI_CONFIG_MOD) { // 修改
				INFO_LOG("mod app config id:%d seq:%u(old seq:%u)", pcfg->iAppId, pcfg->dwSeq, pcfgLocal->dwSeq);
				NET_APP_CONFIG_TO_LOCAL((*pcfgLocal), (*pcfg));
			}
			else  { // 删除
				int idx = ((char*)pcfgLocal - (char*)(stConfig.pReportShm->stAppConfigList)) / MYSIZEOF(AppInfo);
				if(idx < (int)(stConfig.pReportShm->wAppConfigCount-1)) 
					memmove(pcfgLocal, pcfgLocal+1, (stConfig.pReportShm->wAppConfigCount-1-idx)*MYSIZEOF(AppInfo));
				stConfig.pReportShm->wAppConfigCount--;
				INFO_LOG("delete app config id:%d", pcfg->iAppId);
			}
		}
	}while(pTlv != NULL);
	stConfig.pReportShm->dwLastSyncAppConfigTime = stConfig.dwCurTime;
	return 0;
}

static uint32_t MakeCheckAppConfigPkg(uint32_t dwLastResponseTimeMs)
{
	CBasicPacket pkg;

	// head
	ReqPkgHead stHead;
	pkg.InitReqPkgHead(&stHead, CMD_MONI_CHECK_APP_CONFIG);

	// cmd content
	static char sCheckBuf[MYSIZEOF(AppInfoReq)*MAX_APP_COUNT+MYSIZEOF(ContentCheckAppInfo)];
	ContentCheckAppInfo *pCtInfo = (ContentCheckAppInfo*)sCheckBuf;
	pCtInfo->dwServerResponseTime = htonl(dwLastResponseTimeMs);
	pCtInfo->wAppInfoCount = htons(stConfig.pReportShm->wAppConfigCount);
	for(int i=0; i < stConfig.pReportShm->wAppConfigCount; i++) {
		if(!(stConfig.dwRestartFlag & RESTART_FLAG_CHECK_APP_CONFIG))
			pCtInfo->stAppList[i].dwSeq = htonl(stConfig.pReportShm->stAppConfigList[i].dwSeq-1);
		else
			pCtInfo->stAppList[i].dwSeq = htonl(stConfig.pReportShm->stAppConfigList[i].dwSeq);
		pCtInfo->stAppList[i].iAppId = htonl(stConfig.pReportShm->stAppConfigList[i].iAppId);
	}
	stConfig.dwRestartFlag |= RESTART_FLAG_CHECK_APP_CONFIG;

	int iContentLen = MYSIZEOF(ContentCheckAppInfo)
		+ MYSIZEOF(pCtInfo->stAppList[0]) * stConfig.pReportShm->wAppConfigCount;
	pkg.InitCmdContent(pCtInfo, iContentLen);

	// 签名
	char sSig[MAX_SIGNATURE_LEN+MYSIZEOF(TSignature)]={0};
	TSignature *psig = (TSignature*)sSig;
	if(stConfig.iEnableEncryptData) {
		MonitorCommSig stSigInfo;
		stSigInfo.dwSeq = htonl(pkg.m_dwReqSeq);
		stSigInfo.dwCmd = htonl(pkg.m_dwReqCmd);
		if(InitSignature(psig, &stSigInfo, stConfig.pReportShm->sRandKey, MT_SIGNATURE_TYPE_COMMON) < 0)
			return -1;
	}
	pkg.InitSignature(psig);

	// tlv
	char sTlvBuf[128];
	TPkgBody *pbody = (TPkgBody*)sTlvBuf;
	TlvMoniCommInfo stTlvInfo;
	stTlvInfo.iMtClientIndex = htonl(stConfig.pReportShm->iMtClientIndex);
	stTlvInfo.iMachineId = htonl(stConfig.pReportShm->iMachineId);
	int iTlvBodyLen = MYSIZEOF(TPkgBody);
	iTlvBodyLen += SetWTlv(
		pbody->stTlv, TLV_MONI_COMM_INFO, MYSIZEOF(stTlvInfo), (const char*)&stTlvInfo);
	pbody->bTlvNum = 1;
	pkg.InitPkgBody(pbody, iTlvBodyLen);

	return pkg.MakeReqPkg(stConfig.pPkg, &stConfig.iPkgLen);
}

void InitCheckAppConfig(PKGSESSION *psess, uint32_t dwTimeOutMs, uint32_t dwSrvRespTime)
{
	if(!IsHelloValid()) {
		FATAL_LOG("check app config stop -- hello is invalid !");
		stConfig.pReportShm->cIsAgentRun = 2;
		return;
	}

	memset(&stConfig.sSessBuf, 0, MYSIZEOF(stConfig.sSessBuf));
	stConfig.pPkgSess = (PKGSESSION*)stConfig.sSessBuf;
	stConfig.pPkg = stConfig.sSessBuf+MYSIZEOF(PKGSESSION);
	stConfig.iPkgLen = PKG_BUFF_LENGTH;

	stConfig.pPkgSess->iSockIndex = psess->iSockIndex;
	stConfig.pPkgSess->bSessStatus = SESS_FLAG_TIMEOUT_SENDPKG;
	uint32_t dwKey = MakeCheckAppConfigPkg(dwSrvRespTime);
	uint32_t dwExpireTimeMs = (dwTimeOutMs != 0) ? dwTimeOutMs : (rand()%CMD_CHECK_CONFIG_TIME_MS+1);
	if(dwExpireTimeMs < 5*1000)
		dwExpireTimeMs = 5*1000;
	
	// 添加到定时器
	int iRet = AddTimer(dwKey, dwExpireTimeMs, OnPkgExpire,
		stConfig.pPkgSess, MYSIZEOF(PKGSESSION), stConfig.iPkgLen, stConfig.pPkg);
	if(iRet < 0) {
		ERROR_LOG("AddTimer failed ! pkglen:%d, key:%u, ret:%d", stConfig.iPkgLen, dwKey, iRet);
	}
	DEBUG_LOG("(check app config) - add timer key:%u - datalen:%u, socket idx:%d", 
		dwKey, stConfig.iPkgLen, psess->iSockIndex);
}

int DealRespCheckSystemConfig(CBasicPacket &pkg)
{
	if(pkg.m_bRetCode != NO_ERROR) {
		WARN_LOG("cmd check system config ret failed ! (%d)", pkg.m_bRetCode);
		return pkg.m_bRetCode;
	}

	char sCmdContentBuf[MYSIZEOF(MtSystemConfig) + 128] = {0};
	MtSystemConfig *pcfg = NULL;
	if(stConfig.iEnableEncryptData) {
		size_t iDecSigLen = 0;
		aes_decipher_data((const uint8_t*)pkg.m_pstCmdContent, pkg.m_wCmdContentLen,
			(uint8_t*)sCmdContentBuf, &iDecSigLen, (const uint8_t*)stConfig.pReportShm->sRandKey, AES_128);
		if(iDecSigLen != sizeof(MtSystemConfig)) {
			REQERR_LOG("MtDecrypt failed - key:%s datalen:%d, check:%d != %d",
				DumpStrByMask(stConfig.pReportShm->sRandKey, 16), pkg.m_wCmdContentLen, 
				(int)iDecSigLen, (int)sizeof(MtSystemConfig));
			return ERR_DECRYPT_FAILED;
		}
		pcfg = (MtSystemConfig*)sCmdContentBuf;
	}
	else {
		pcfg = (MtSystemConfig*)pkg.m_pstCmdContent;
		if(pkg.m_wCmdContentLen != sizeof(MtSystemConfig)) {
			REQERR_LOG("check cmd content length failed, %d != %lu", 
				(int)pkg.m_wCmdContentLen, sizeof(MtSystemConfig));
			return ERR_INVALID_CMD_CONTENT;
		}
	}

	pcfg->dwConfigSeq = ntohl(pcfg->dwConfigSeq);
	if(pcfg->dwConfigSeq == stConfig.pReportShm->stSysCfg.dwConfigSeq) {
		DEBUG_LOG("system config not change seq:%u", pcfg->dwConfigSeq);
		return 0;
	}
	stConfig.pReportShm->stSysCfg.wHelloRetryTimes = ntohs(pcfg->wHelloRetryTimes);
	stConfig.pReportShm->stSysCfg.wHelloPerTimeSec = ntohs(pcfg->wHelloPerTimeSec);
	stConfig.pReportShm->stSysCfg.wCheckLogPerTimeSec = ntohs(pcfg->wCheckLogPerTimeSec);
	stConfig.pReportShm->stSysCfg.wCheckAppPerTimeSec = ntohs(pcfg->wCheckAppPerTimeSec);
	stConfig.pReportShm->stSysCfg.wCheckServerPerTimeSec = ntohs(pcfg->wCheckServerPerTimeSec);
	stConfig.pReportShm->stSysCfg.wCheckSysPerTimeSec = ntohs(pcfg->wCheckSysPerTimeSec);
	stConfig.pReportShm->stSysCfg.dwConfigSeq = pcfg->dwConfigSeq;
	stConfig.pReportShm->stSysCfg.bAttrSendPerTimeSec = pcfg->bAttrSendPerTimeSec;
	stConfig.pReportShm->stSysCfg.bLogSendPerTimeSec = pcfg->bLogSendPerTimeSec;
	INFO_LOG("check system config, update, new seq:%u", pcfg->dwConfigSeq);
	return 0;
}

static uint32_t MakeCheckSystemConfigPkg(uint32_t dwLastResponseTimeMs)
{
	CBasicPacket pkg;

	// head
	ReqPkgHead stHead;
	pkg.InitReqPkgHead(&stHead, CMD_MONI_CHECK_SYSTEM_CONFIG);

	// cmd content
	static char sCheckBuf[256];
	ContentCheckSystemCfgReq *pCtInfo = (ContentCheckSystemCfgReq*)sCheckBuf;
	pCtInfo->dwServerResponseTime = htonl(dwLastResponseTimeMs);

	if(!(stConfig.dwRestartFlag & RESTART_FLAG_CHECK_SYSTEM_CONFIG))
	{
		// 重启更新配置
		pCtInfo->dwConfigSeq = htonl(stConfig.pReportShm->stSysCfg.dwConfigSeq-1);
		stConfig.dwRestartFlag |= RESTART_FLAG_CHECK_SYSTEM_CONFIG;
	}
	else
		pCtInfo->dwConfigSeq = htonl(stConfig.pReportShm->stSysCfg.dwConfigSeq);
	int iContentLen = MYSIZEOF(ContentCheckSystemCfgReq);
	pkg.InitCmdContent(pCtInfo, iContentLen);

	// 签名
	char sSig[MAX_SIGNATURE_LEN+MYSIZEOF(TSignature)]={0};
	TSignature *psig = (TSignature*)sSig;
	if(stConfig.iEnableEncryptData) {
		MonitorCommSig stSigInfo;
		stSigInfo.dwSeq = htonl(pkg.m_dwReqSeq);
		stSigInfo.dwCmd = htonl(pkg.m_dwReqCmd);
		if(InitSignature(psig, &stSigInfo, stConfig.pReportShm->sRandKey, MT_SIGNATURE_TYPE_COMMON) < 0)
			return -1;
	}
	pkg.InitSignature(psig);

	// tlv
	char sTlvBuf[128];
	TPkgBody *pbody = (TPkgBody*)sTlvBuf;
	TlvMoniCommInfo stTlvInfo;
	stTlvInfo.iMtClientIndex = htonl(stConfig.pReportShm->iMtClientIndex);
	stTlvInfo.iMachineId = htonl(stConfig.pReportShm->iMachineId);
	int iTlvBodyLen = MYSIZEOF(TPkgBody);
	iTlvBodyLen += SetWTlv(
		pbody->stTlv, TLV_MONI_COMM_INFO, MYSIZEOF(stTlvInfo), (const char*)&stTlvInfo);
	pbody->bTlvNum = 1;
	pkg.InitPkgBody(pbody, iTlvBodyLen);

	return pkg.MakeReqPkg(stConfig.pPkg, &stConfig.iPkgLen);
}

void InitCheckSystemConfig(PKGSESSION *psess, uint32_t dwTimeOutMs, uint32_t dwSrvRespTime)
{
	if(!IsHelloValid()) {
		FATAL_LOG("check system config stop -- hello is invalid !");
		stConfig.pReportShm->cIsAgentRun = 2;
		return;
	}

	memset(&stConfig.sSessBuf, 0, MYSIZEOF(stConfig.sSessBuf));
	stConfig.pPkgSess = (PKGSESSION*)stConfig.sSessBuf;
	stConfig.pPkg = stConfig.sSessBuf+MYSIZEOF(PKGSESSION);
	stConfig.iPkgLen = PKG_BUFF_LENGTH;

	stConfig.pPkgSess->iSockIndex = psess->iSockIndex;
	stConfig.pPkgSess->bSessStatus = SESS_FLAG_TIMEOUT_SENDPKG;
	uint32_t dwKey = MakeCheckSystemConfigPkg(dwSrvRespTime);
	uint32_t dwExpireTimeMs = (dwTimeOutMs != 0) ? dwTimeOutMs : (rand()%CMD_CHECK_CONFIG_TIME_MS+1);
	if(dwExpireTimeMs < 5*1000)
		dwExpireTimeMs = 5*1000;
	
	// 添加到定时器
	int iRet = AddTimer(dwKey, dwExpireTimeMs, OnPkgExpire,
		stConfig.pPkgSess, MYSIZEOF(PKGSESSION), stConfig.iPkgLen, stConfig.pPkg);
	if(iRet < 0) {
		ERROR_LOG("AddTimer failed ! pkglen:%d, key:%u, ret:%d", stConfig.iPkgLen, dwKey, iRet);
	}
	DEBUG_LOG("(check system config) - add timer key:%u - datalen:%u, socket idx:%d", 
		dwKey, stConfig.iPkgLen, psess->iSockIndex);
}

// 读取 applog 日志，设置日志服务器地址, 组 log 上报包
uint32_t MakeAppLogPkg(
	struct sockaddr_in & app_server, char *pAppLogContent, int iAppLogContentLen, int iAppId)
{
	CBasicPacket pkg;

	// head
	ReqPkgHead stHead;
	pkg.InitReqPkgHead(&stHead, CMD_MONI_SEND_LOG);
	*(int32_t*)(stHead.sReserved) = htonl(iAppId);

	// cmd content
	pkg.InitCmdContent((void*)pAppLogContent, (uint16_t)iAppLogContentLen);

	// 签名
	char sSig[MAX_SIGNATURE_LEN+MYSIZEOF(TSignature)]={0};
	TSignature *psig = (TSignature*)sSig;
	MonitorCommSig stSigInfo;
	if(stConfig.iEnableEncryptData) 
		stSigInfo.bEnableEncryptData = 1;
	else
		stSigInfo.bEnableEncryptData = 0;
	stSigInfo.dwSeq = htonl(pkg.m_dwReqSeq);
	stSigInfo.dwCmd = htonl(pkg.m_dwReqCmd);
	if(InitSignature(psig, &stSigInfo, stConfig.szUserKey, MT_SIGNATURE_TYPE_COMMON) < 0)
		return 0;
	pkg.InitSignature(psig);

	// tlv
	char sTlvBuf[128];
	TPkgBody *pbody = (TPkgBody*)sTlvBuf;
	TlvMoniCommInfo stTlvInfo;

	// 这里用 log 服务器上的相关cache 索引
	stTlvInfo.iMtClientIndex = htonl(stConfig.pReportShm->iAppLogSrvMtClientIndex);
	stTlvInfo.iMachineId = htonl(stConfig.pReportShm->iMachineId);
	stTlvInfo.dwReserved_1 = htonl(app_server.sin_addr.s_addr);
	stTlvInfo.wReserved_1 = htons(app_server.sin_port);

	int iTlvBodyLen = MYSIZEOF(TPkgBody);
	iTlvBodyLen += SetWTlv(
		pbody->stTlv, TLV_MONI_COMM_INFO, MYSIZEOF(stTlvInfo), (const char*)&stTlvInfo);
	pbody->bTlvNum = 1;
	pkg.InitPkgBody(pbody, iTlvBodyLen);

	return pkg.MakeReqPkg(stConfig.pPkg, &stConfig.iPkgLen);
}

// 组 attr 上报数据包
uint32_t MakeAttrPkg(struct sockaddr_in & app_server, char *pContent, int iContentLen, bool bIsStrAttr)
{
	CBasicPacket pkg;

	// head
	ReqPkgHead stHead;
	if(bIsStrAttr)
		pkg.InitReqPkgHead(&stHead, CMD_MONI_SEND_STR_ATTR);
	else
		pkg.InitReqPkgHead(&stHead, CMD_MONI_SEND_ATTR);

	// cmd content
	pkg.InitCmdContent((void*)pContent, (uint16_t)iContentLen);

	// 签名
	char sSig[MAX_SIGNATURE_LEN+MYSIZEOF(TSignature)]={0};
	TSignature *psig = (TSignature*)sSig;
	MonitorCommSig stSigInfo;
	if(stConfig.iEnableEncryptData) 
		stSigInfo.bEnableEncryptData = 1;
	else
		stSigInfo.bEnableEncryptData = 0;
	stSigInfo.dwSeq = htonl(pkg.m_dwReqSeq);
	stSigInfo.dwCmd = htonl(pkg.m_dwReqCmd);
	if(InitSignature(psig, &stSigInfo, stConfig.szUserKey, MT_SIGNATURE_TYPE_COMMON) < 0)
		return 0;
	pkg.InitSignature(psig);

	// tlv
	char sTlvBuf[128];
	TPkgBody *pbody = (TPkgBody*)sTlvBuf;
	TlvMoniCommInfo stTlvInfo;

	// 这里用 attr 服务器上的相关cache 索引
	stTlvInfo.iMtClientIndex = htonl(stConfig.pReportShm->iAttrSrvMtClientIndex);
	stTlvInfo.iMachineId = htonl(stConfig.pReportShm->iMachineId);
	stTlvInfo.dwReserved_1 = htonl(app_server.sin_addr.s_addr);
	stTlvInfo.wReserved_1 = htons(app_server.sin_port);

	int iTlvBodyLen = MYSIZEOF(TPkgBody);
	iTlvBodyLen += SetWTlv(
		pbody->stTlv, TLV_MONI_COMM_INFO, MYSIZEOF(stTlvInfo), (const char*)&stTlvInfo);
	pbody->bTlvNum = 1;
	pkg.InitPkgBody(pbody, iTlvBodyLen);

	return pkg.MakeReqPkg(stConfig.pPkg, &stConfig.iPkgLen);
}

