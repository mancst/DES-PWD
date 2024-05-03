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

#ifndef __STDC_FORMAT_MACROS  // for inttypes.h: uint32_t ...
#define __STDC_FORMAT_MACROS
#endif
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <assert.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <map>
#include <math.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ipc.h>      
#include <fcntl.h>
#include <sys/wait.h>
#include <sstream>
#include "aes.h"
#include "mt_shm.h"
#include "mt_report.h"
#include "sv_socket.h"
#include "sv_cfg.h"
#include "sv_str.h"
#include "sv_timer.h"
#include "mtreport_client.h"
#include "mtreport_protoc.h"

extern MtReport g_mtReport;

const std::string g_strVersion = "open DES-PWE - v1.0.0";
std::string g_strCmpTime = __DATE__ " " __TIME__; 
const std::string g_strVersionDetail = "version - open DES-PWE - v1.0.0 cmp@" __DATE__ " " __TIME__;
CONFIG stConfig;

int InitReportLog();
int InitReportAttr();
int InitReportPluginInfo();
int OnEventTimer(TimerNode *pNodeShm, unsigned uiDataLen, char *pData);

int get_cmd_result(const char *cmd, std::string &strResult)
{
    static char s_buf[4096] = {0}; 

	strResult = "";
    FILE *fp = popen(cmd, "r");
    if(!fp) {
        ERROR_LOG("popen failed, cmd:%s, msg:%s", cmd, strerror(errno));
        return ERROR_LINE;
    }    

    if(fgets(s_buf, sizeof(s_buf), fp)) {
        strResult = s_buf;
        size_t pos = strResult.find("\n");
        if(pos != std::string::npos)
            strResult.replace(pos, 1, ""); 
        pos = strResult.find("\r");
        if(pos != std::string::npos)
            strResult.replace(pos, 1, ""); 
        pos = strResult.find("\r\n");
        if(pos != std::string::npos)
            strResult.replace(pos, 2, ""); 
        DEBUG_LOG("cmd:%s, get result:%s", cmd, s_buf);
    }    
    pclose(fp);
    return 0;
}

void OnSelectTimeout(time_t uiTime)
{
}

void OnSocketError(struct MtReportSocket *pstSock, time_t uiTime)
{
	if(errno != EINTR)
		ERROR_LOG("socket error, msg:%s", strerror(errno));
}

void SendErrorRespToReq(CBasicPacket &pkg, int iRet, struct MtSocket *psock)
{
    static char s_buf[1024];

    // 不可对响应包回复
    if(ntohl(pkg.m_pstReqHead->dwRespMagicNum) == MAGIC_RESPONSE_NUM)
        return;

    int iBufLen = (int)(sizeof(s_buf));
    if(pkg.MakeRespPkg(iRet, s_buf, &iBufLen) > 0 && iBufLen > 0) {
        SendPacket(psock, s_buf, iBufLen);
        INFO_LOG("send error response to server:%s:%d, error:%d, session id:%lu",
            inet_ntoa(psock->last_recv_remote.sin_addr), ntohs(psock->last_recv_remote.sin_port), iRet,
            ntohll(pkg.m_pstReqHead->qwSessionId));
    }
    else {
        ERROR_LOG("send error response to server:%s:%d failed, error code:%d",
           inet_ntoa(psock->last_recv_remote.sin_addr), ntohs(psock->last_recv_remote.sin_port), iRet);
    }
}

void OnMtreportPkg(struct MtSocket *psock, char * sBuf, int iLen, time_t uiTime)
{
	DEBUG_LOG("get packet from - %s:%d lengh:%d",
		inet_ntoa(psock->last_recv_remote.sin_addr), ntohs(psock->last_recv_remote.sin_port), iLen);

	CBasicPacket pkg;
	int iRet = 0;
	if((iRet=pkg.CheckBasicPacket(sBuf, iLen)) != NO_ERROR) {
		REQERR_LOG("CheckBasicPacket failed ret:%d", iRet);
		if(pkg.m_pstReqHead == NULL)
			return ;
	}

	// 先看看是否是 S2C	push 请求 --------- start
	switch(pkg.m_dwReqCmd) {
        case CMD_MONI_S2C_PRE_INSTALL_NOTIFY:
            iRet = DealPreInstallNotify(pkg);
			if(stConfig.fpPluginInstallLogFile) {
				fclose(stConfig.fpPluginInstallLogFile);
				stConfig.fpPluginInstallLogFile = NULL;
			}
            if(iRet != 0 && pkg.m_pstReqHead->qwSessionId != 0)
                SendErrorRespToReq(pkg, iRet, psock);
            return;

        case CMD_MONI_S2C_MACH_ORP_PLUGIN_MOD_CFG:
            iRet = DealModMachinePluginCfg(pkg);
            if(stConfig.fpPluginInstallLogFile) {
                fclose(stConfig.fpPluginInstallLogFile);
                stConfig.fpPluginInstallLogFile = NULL;
            }
            // 服务器端使用可靠udp，需要回复
            if(iRet != 0 && pkg.m_pstReqHead->qwSessionId != 0)
                SendErrorRespToReq(pkg, iRet, psock);
            return;

        case CMD_MONI_S2C_MACH_ORP_PLUGIN_REMOVE:
        case CMD_MONI_S2C_MACH_ORP_PLUGIN_ENABLE:
        case CMD_MONI_S2C_MACH_ORP_PLUGIN_DISABLE:
            iRet = DealMachineOprPlugin(pkg);
            if(stConfig.fpPluginInstallLogFile) {
                fclose(stConfig.fpPluginInstallLogFile);
                stConfig.fpPluginInstallLogFile = NULL;
            }
            // 服务器端使用可靠udp，需要回复
            if(iRet != 0 && pkg.m_pstReqHead->qwSessionId != 0)
                SendErrorRespToReq(pkg, iRet, psock);
            return;

		default:
			break;
	}

	TimerNode *pNode = NULL;
	stConfig.uiSessDataLen = MYSIZEOF(stConfig.sSessBuf)-MYSIZEOF(PKGSESSION);
	if((iRet=GetTimer(pkg.m_dwReqSeq, &pNode, &stConfig.uiSessDataLen, stConfig.sSessBuf)) < 0) {
		REQERR_LOG("GetTimer failed ret:%d, timer key:%u", iRet, pkg.m_dwReqSeq);
		return;
	}
	stConfig.pPkgSess = (PKGSESSION*)pNode->sSessData;
	uint32_t dwTimeMs = GET_DIFF_TIME_MS(stConfig.pPkgSess->dwSendTimeSec, stConfig.pPkgSess->dwSendTimeUsec);
	ModMaxResponseTime(stConfig.pPkgSess->iSockIndex, dwTimeMs);

	DEBUG_LOG("receive packet cmd:%u seq:%u ret:%d resptime:%u ms",
		pkg.m_dwReqCmd, pkg.m_dwReqSeq, pkg.m_bRetCode, dwTimeMs);

	stConfig.pPkgSess->bSessStatus = SESS_FLAG_RESPONSED;

	switch(pkg.m_dwReqCmd) {
		case CMD_MONI_SEND_HELLO_FIRST:
			if(pkg.m_bRetCode != NO_ERROR) {
				pNode->uiKey = 0; 
				FATAL_LOG("server response failed ret:(%d), msg:(%s)!", 
					pkg.m_bRetCode, CBasicPacket::GetErrMsg(pkg.m_bRetCode));
				stConfig.pReportShm->cIsAgentRun = 2;
				exit(-1);
			}
			iRet = DealResponseHelloFirst(pkg);
			if(iRet == 0) {
				MakeHelloToServer(stConfig.pPkgSess);

				InitCheckSystemConfig((PKGSESSION*)pNode->sSessData, 1);
				InitCheckLogConfig((PKGSESSION*)pNode->sSessData, 1);
				InitCheckAppConfig((PKGSESSION*)pNode->sSessData, 1);

				if((iRet=InitReportAttr()) < 0) {
					FATAL_LOG("init report attr failed , ret:%d", iRet);
					stConfig.pReportShm->cIsAgentRun = 2;
					return ;
				}

				if((iRet=InitReportLog()) < 0) {
					FATAL_LOG("init report log failed , ret:%d", iRet);
					stConfig.pReportShm->cIsAgentRun = 2;
					return ;
				}

				if((iRet=InitReportPluginInfo()) < 0) {
					FATAL_LOG("init report plugin info failed , ret:%d", iRet);
					stConfig.pReportShm->cIsAgentRun = 2;
					return ;
				}
			}
			else {
				stConfig.pReportShm->cIsAgentRun = 2;
				FATAL_LOG("DealResponseHelloFirst failed ret:%d - process restart !", iRet);
			}
			break;

		case CMD_MONI_SEND_HELLO:
			iRet = DealResponseHello(pkg);
			MakeHelloToServer(stConfig.pPkgSess);
			break;

		case CMD_MONI_CHECK_LOG_CONFIG:
			iRet = DealRespCheckLogConfig(pkg);
			InitCheckLogConfig((PKGSESSION*)pNode->sSessData, 
				TIME_SEC_TO_MS(stConfig.pReportShm->stSysCfg.wCheckLogPerTimeSec), dwTimeMs);
			break;

		case CMD_MONI_CHECK_APP_CONFIG:
			iRet = DealRespCheckAppConfig(pkg);
			InitCheckAppConfig((PKGSESSION*)pNode->sSessData, 
				TIME_SEC_TO_MS(stConfig.pReportShm->stSysCfg.wCheckAppPerTimeSec), dwTimeMs);
			break;

		case CMD_MONI_CHECK_SYSTEM_CONFIG:
			iRet = DealRespCheckSystemConfig(pkg);
			InitCheckSystemConfig((PKGSESSION*)pNode->sSessData, 
				TIME_SEC_TO_MS(stConfig.pReportShm->stSysCfg.wCheckSysPerTimeSec), dwTimeMs);
			break;

		case CMD_MONI_SEND_LOG: {
			AppInfo & stInfo = stConfig.pReportShm->stAppConfigList[
				stConfig.pPkgSess->stCmdSessData.applog.iAppInfoIdx];
			stInfo.dwLastTrySendLogTime = stConfig.dwCurTime;
			if(pkg.m_bRetCode != 0) 
			{
				ERROR_LOG("send app log to server:%s failed, appid:%d ret:%d",
					ipv4_addr_str(stConfig.pPkgSess->stCmdSessData.applog.dwAppLogSrvIP),
					stInfo.iAppId, pkg.m_bRetCode);
				stInfo.dwTrySendFailedCount++;
			}
			else
			{
				DEBUG_LOG("send app log to server:%s success, appid:%d",
					ipv4_addr_str(stConfig.pPkgSess->stCmdSessData.applog.dwAppLogSrvIP), stInfo.iAppId);
				stConfig.pReportShm->dwLastReportLogOkTime = stConfig.dwCurTime;
				stInfo.dwSendOkCount++;
			}
		}
		break;

		case CMD_MONI_SEND_PLUGIN_INFO:
			iRet = DealRespRepPluginInfo(pkg);
			break;

		case CMD_MONI_SEND_STR_ATTR:
		case CMD_MONI_SEND_ATTR:
			if(pkg.m_bRetCode != 0) 
				ERROR_LOG("report str|attr(%d) to server:%s failed, ret:%d", pkg.m_dwReqCmd,
					ipv4_addr_str(stConfig.pPkgSess->stCmdSessData.attr.dwAttrSrvIP), pkg.m_bRetCode);
			else {
				DEBUG_LOG("report str|attr(%d) to server:%s success", pkg.m_dwReqCmd,
					ipv4_addr_str(stConfig.pPkgSess->stCmdSessData.attr.dwAttrSrvIP));
				stConfig.pReportShm->dwLastReportAttrOkTime = stConfig.dwCurTime;
			}
			break;

		default:
			REQERR_LOG("unknow packet cmd:%u", pkg.m_dwReqCmd);
			break;
	}

	pNode->uiKey = 0; 
	if(iRet != 0)
		REQERR_LOG("deal response packet failed, cmd:%u, ret:%d, msg:%s", pkg.m_dwReqCmd, iRet, pkg.GetErrMsg(iRet));
}

void SendPluginConfig(TInnerPlusInfo *plugin)
{
	stConfig.pPkgSess = (PKGSESSION*)stConfig.sSessBuf;
	stConfig.pPkg = stConfig.sSessBuf+MYSIZEOF(PKGSESSION);
	stConfig.iPkgLen = PKG_BUFF_LENGTH;

    INFO_LOG("report plugin:%s(%d) config to server:%s:%d", 
        plugin->szPlusName, plugin->iPluginId, stConfig.szSrvIp_master, stConfig.iSrvPort);
    uint32_t dwKey = 0;
    if((dwKey=MakePluginConfigReportPkg(plugin)) > 0) {
        struct sockaddr_in addr_server;
        addr_server.sin_family = PF_INET;
        addr_server.sin_port = htons(stConfig.iSrvPort);
        addr_server.sin_addr.s_addr = inet_addr(stConfig.szSrvIp_master);
        int iRet = SendPacket(stConfig.iConfigSocketIndex, &addr_server, stConfig.pPkg, stConfig.iPkgLen);
        if(iRet != stConfig.iPkgLen) {
		    ERROR_LOG("SendPacket(report plugin config) failed, pkglen:%d, ret:%d", stConfig.iPkgLen, iRet);
        }
        else {
            stConfig.pPkgSess->iSockIndex = stConfig.iConfigSocketIndex;
            stConfig.pPkgSess->dwSendTimeSec = stConfig.stTimeCur.tv_sec;
            stConfig.pPkgSess->dwSendTimeUsec = stConfig.stTimeCur.tv_usec;
            stConfig.pPkgSess->bSessStatus = SESS_FLAG_WAIT_RESPONSE;
            stConfig.pPkgSess->stCmdSessData.plugin_cfg.iPluginId = plugin->iPluginId;
            int iTime = GetMaxResponseTime(stConfig.pPkgSess->iSockIndex)+1000;
            iRet = AddTimer(dwKey, iTime, OnPkgExpire,
                stConfig.pPkgSess, MYSIZEOF(PKGSESSION), stConfig.iPkgLen, stConfig.pPkg);
            if(iRet < 0) {
                ERROR_LOG("AddTimer(report plugin config) failed ! pkglen:%d, key:%u, ret:%d", stConfig.iPkgLen, dwKey, iRet);
                return ;
            }
            DEBUG_LOG("report plugin config to %s:%d, plugin id:%d, name:%s", 
                stConfig.szSrvIp_master, stConfig.iSrvPort, plugin->iPluginId, plugin->szPlusName);
        }
    }
    else {
        ERROR_LOG("MakePluginConfigReportPkg failed !");
    }
}


int TimerHashCmpFun(const void *pKey, const void *pNode)
{
	if(((const TimerNode*)pNode)->dwProcessId != stConfig.dwProcessId) {
		((TimerNode*)pNode)->uiKey = 0;
	}

	if(((const TimerNode*)pNode)->qwExpireTime+TIME_MS_TO_US(5*60*1000) < stConfig.qwCurTime) {
		((TimerNode*)pNode)->uiKey = 0;
	}

	if(((const TimerNode*)pKey)->uiKey == ((const TimerNode*)pNode)->uiKey)
		return 0;
	return 1;
}

int TimerHashLevelWarn(uint32_t dwCurUse, uint32_t dwTotal)
{
	return 0;
}

void SetCurTime()
{
	gettimeofday(&stConfig.stTimeCur, NULL);
	stConfig.dwCurTime = stConfig.stTimeCur.tv_sec;
	stConfig.qwCurTime = (uint64_t)stConfig.stTimeCur.tv_sec*1000000 + stConfig.stTimeCur.tv_usec;

	struct tm stTm;
	char sBuf[64];
	localtime_r(&stConfig.stTimeCur.tv_sec, &stTm);
	strftime(sBuf, MYSIZEOF(sBuf), "%Y-%m-%d %H:%M:%S", &stTm);
	sprintf(stConfig.szTimeCur, "%s.%06u ", sBuf, (uint32_t)stConfig.stTimeCur.tv_usec);
}

bool LogFreqCheck()
{
	SetCurTime();
	if(stConfig.dwCurTime <= stConfig.dwLastCheckLogFreqTime)
	{
		if(stConfig.iLogLimitPerSec != 0 && stConfig.iLogLimitPerSec <= stConfig.iLogWriteCount)
			return false;
		stConfig.iLogWriteCount++;
		return true;
	}

	stConfig.iLogWriteCount = 1;
	stConfig.dwLastCheckLogFreqTime = stConfig.dwCurTime;
	return true;
}

void TryOpenPluginInstallLogFile(CmdS2cPreInstallContentReq *plug)
{
    std::ostringstream ss;
    ss << stConfig.szCurPath << "/plugin_install_log/" << plug->sPluginName << "_install.log";
    if(stConfig.fpPluginInstallLogFile) {
        fclose(stConfig.fpPluginInstallLogFile);
        stConfig.fpPluginInstallLogFile = NULL;
    }
	stConfig.fpPluginInstallLogFile = fopen(ss.str().c_str(), "a+");
	if(NULL == stConfig.fpPluginInstallLogFile) {
		ERROR_LOG("open plugin log file : %s failed, msg:%s", ss.str().c_str(), strerror(errno));
	}
}
void TryOpenPluginInstallLogFile(CmdS2cMachOprPluginReq *plug)
{
    static CmdS2cPreInstallContentReq s_p;
    strncpy(s_p.sPluginName, plug->sPluginName, sizeof(s_p.sPluginName));
    TryOpenPluginInstallLogFile(&s_p);
}

static void TryReOpenLocalLogFile()
{
	static uint32_t s_dwLastReOpenTime = 0;
	if(stConfig.dwCurTime < s_dwLastReOpenTime+10)
		return ;
	s_dwLastReOpenTime = stConfig.dwCurTime;

	if(stConfig.fpLogFile != NULL) {
		fclose(stConfig.fpLogFile);
		stConfig.fpLogFile = NULL;
	}

	struct stat stStat;
	bool bOverLimit = false, bGetStat = false;
	if(0 == stat(stConfig.szLocalLogFile, &stStat)) {
		if (stStat.st_size > stConfig.iMaxLocalLogFileSize) 
			bOverLimit = true;
		bGetStat = true;
	}

	if(stConfig.szLocalLogFile[0] != '\0') {
		if(bOverLimit)
			stConfig.fpLogFile = fopen(stConfig.szLocalLogFile, "w+");
		else
			stConfig.fpLogFile = fopen(stConfig.szLocalLogFile, "a+");
		if(NULL == stConfig.fpLogFile) {
			fprintf(stderr, "open file : %s failed, msg:%s", stConfig.szLocalLogFile, strerror(errno));
		}
		else if(bGetStat && bOverLimit) {
			DEBUG_LOG("log file:%s, size limit:%d, cur size:%lu, over limit:%d", 
				stConfig.szLocalLogFile, stConfig.iMaxLocalLogFileSize, stStat.st_size, bOverLimit);
		}
	}
}

void ReleaseConfigList(TConfigItemList & list)
{
	TConfigItemList::iterator it = list.begin();
	TConfigItem *pitem = NULL;
	for(; it != list.end();)
	{
		pitem = *it;
		delete pitem;
		it = list.erase(it);
	}
	list.clear();
}

// 加载透明 udp 代理配置
static int LoadProxyInfo(int iCount)
{
    std::ostringstream sport, ssrv_addr, srv_port;
    char sAddr[512] = {0};

    for(int i=0; i < iCount && i < MAX_PROXY_COUNT; i++) {
        sport.str("");
        sport << "PROXY_LISTEN_PORT_" << i;
        ssrv_addr.str("");
        ssrv_addr << "PROXY_SERVER_ADDR_" << i;
        srv_port.str("");
        srv_port << "PROXY_SERVER_PORT_" << i;
	    if(LoadConfig(MTREPORT_CONFIG, 
            sport.str().c_str(), CFG_INT, &stConfig.stProxy[i].iListenPort, 0,
            ssrv_addr.str().c_str(), CFG_STRING, sAddr, "", MYSIZEOF(sAddr),
            srv_port.str().c_str(), CFG_INT, &stConfig.stProxy[i].iProxyPort, 0,
		    (void*)NULL) < 0)
        {
            ERROR_LOG("load proxy info failed, from file:%s, idx:%d", MTREPORT_CONFIG, i);
            return ERROR_LINE;
        }

        if(stConfig.stProxy[i].iListenPort <= 0 || stConfig.stProxy[i].iListenPort >= 65535
            || stConfig.stProxy[i].iProxyPort <= 0 || stConfig.stProxy[i].iProxyPort >= 65535)
        {
            ERROR_LOG("invalid proxy config - %d %d %s", 
                stConfig.stProxy[i].iListenPort, stConfig.stProxy[i].iProxyPort, sAddr);
            return ERROR_LINE;
        }

        if(INADDR_NONE == inet_addr(sAddr)) {
            struct hostent *hptr;
            if((hptr=gethostbyname(sAddr)) == NULL) {
                ERROR_LOG("gethostbyname failed addr:%s msg:%s", sAddr, strerror(errno));
                return ERROR_LINE;
            }
            else if(hptr->h_addrtype != AF_INET){
                ERROR_LOG("gethostbyname failed url:%s address type not eq(%d!=%d)", sAddr, hptr->h_addrtype, AF_INET);
                return ERROR_LINE;
            }
            else {
                char **pptr, str[32];
                for(pptr = hptr->h_addr_list; *pptr!=NULL; pptr++) {
                    INFO_LOG("get ip:%s from addr:%s", inet_ntop(hptr->h_addrtype, *pptr, str, MYSIZEOF(str)), sAddr);
                    stConfig.stProxy[i].dwProxySrvIp = inet_addr(str);
                    break;
                }
            }
        }
        else {
            stConfig.stProxy[i].dwProxySrvIp = inet_addr(sAddr);
        }
        if(!stConfig.stProxy[i].dwProxySrvIp) {
            ERROR_LOG("get proxy server ip failed, addr:%s", sAddr);
            return ERROR_LINE;
        }

        INFO_LOG("load proxy info : listen port:%d, proxy server:%s:%d, index:%d", 
            stConfig.stProxy[i].iListenPort, ipv4_addr_str(stConfig.stProxy[i].dwProxySrvIp), 
            stConfig.stProxy[i].iProxyPort, i);
        stConfig.iProxyCount++;
        memset(stConfig.stProxy[i].stProxyInfo, 0, sizeof(stConfig.stProxy[i].stProxyInfo));
    }
    INFO_LOG("proxy count:%d, proxy only:%d", stConfig.iProxyCount, stConfig.iProxyOnly);
    return 0;
}

static int Init()
{
	int32_t iCfgShmKey = 0;
	int32_t iRet = 0;
    int32_t iProxyCount = 0;
	char szTypeString[300] = {0};
	if((iRet=LoadConfig(MTREPORT_CONFIG,
		"SERVER_MASTER", CFG_STRING, stConfig.szSrvIp_master, "192.168.128.128", MYSIZEOF(stConfig.szSrvIp_master),
		"SERVER_PORT", CFG_INT, &stConfig.iSrvPort, 27000,
		"LOG_FREQ_LIMIT_PER_SEC", CFG_INT, &stConfig.iLogLimitPerSec, 5,
		"AGENT_ACCESS_KEY", CFG_STRING, stConfig.szUserKey, "232k8s8d8f20@#@%@#$@skdfj2351%^", MYSIZEOF(stConfig.szUserKey), 
		"PLUS_PATH", CFG_STRING, stConfig.szPlusPath, "./DES-PWE_plugin", MYSIZEOF(stConfig.szPlusPath),
		"MTREPORT_SHM_KEY", CFG_INT, &iCfgShmKey, MT_REPORT_DEF_SHM_KEY,
		"AGENT_CLIENT_IP", CFG_STRING, stConfig.szCustLocalIP, "", MYSIZEOF(stConfig.szCustLocalIP),
		"CUST_ATTR_SRV_IP", CFG_STRING, stConfig.szCustAttrSrvIp, "", MYSIZEOF(stConfig.szCustAttrSrvIp),
		"CUST_ATTR_SRV_PORT", CFG_INT, &stConfig.iCustAttrSrvPort, 0,
		"CUST_LOG_SRV_IP", CFG_STRING, stConfig.szCustLogSrvIp, "", MYSIZEOF(stConfig.szCustLogSrvIp),
		"CUST_LOG_SRV_PORT", CFG_INT, &stConfig.iCustLogSrvPort, 0,
		"ENABLE_ENCRYPT_DATA", CFG_INT, &stConfig.iEnableEncryptData, 0,
		"CLIENT_LOCAL_LOG_FILE", CFG_STRING, stConfig.szLocalLogFile, 
			"./slog_mtreport_client.log", MYSIZEOF(stConfig.szLocalLogFile),
		"CLIENT_LOCAL_LOG_TYPE", CFG_STRING, szTypeString, "warn|error|fatal", MYSIZEOF(szTypeString),
		"CLIENT_LOCAL_LOG_SIZE", CFG_INT, &stConfig.iMaxLocalLogFileSize, 1024*1024*20,
		"MAX_RUN_MINS", CFG_INT, &stConfig.iMaxRunMins, 7*24*3600,
		"DES-PWE_URL", CFG_STRING, stConfig.szCloudUrl, "", MYSIZEOF(stConfig.szCloudUrl),
		"DES-PWE_DOWNLOAD_URL", CFG_STRING, stConfig.szDownCloudUrl, "", MYSIZEOF(stConfig.szDownCloudUrl),
		"INSTALL_PLUGIN_TIMEOUT_SEC", CFG_INT, &stConfig.iPLuginInstallTimeoutSec, 30,
		"DES-PWE_LOCAL_DOMAIN", CFG_STRING, stConfig.szLocalDomain, "", MYSIZEOF(stConfig.szLocalDomain),
		"LOCAL_OS", CFG_STRING, stConfig.szOs, "", MYSIZEOF(stConfig.szOs),
		"LOCAL_OS_ARC", CFG_STRING, stConfig.szOsArc, "", MYSIZEOF(stConfig.szOsArc),
		"LOCAL_LIBC_VER", CFG_STRING, stConfig.szLibcVer, "", MYSIZEOF(stConfig.szLibcVer),
		"LOCAL_LIBCPP_VER", CFG_STRING, stConfig.szLibcppVer, "", MYSIZEOF(stConfig.szLibcppVer),
        "IP_TTL", CFG_INT, &stConfig.ttl, 64, 
        "PROXY_COUNT", CFG_INT, &iProxyCount, 0,
        "PROXY_ONLY", CFG_INT, &stConfig.iProxyOnly, 0,
		(void*)NULL)) < 0)
	{   
		printf("read config failed, from config file:%s\n", MTREPORT_CONFIG);
		return -1;
	} 

	// 启动信息记录到日志, 频率限制先改大
	int iLogLimitPerSecSave = stConfig.iLogLimitPerSec;
	int iLocalLogTypeSave = GetLogTypeByStr(szTypeString);
	stConfig.iLogLimitPerSec = 10000;
	stConfig.iLocalLogType = 255;

	stConfig.fpLogFile = NULL;
	TryReOpenLocalLogFile();

	if(stConfig.iPLuginInstallTimeoutSec < 5)
		stConfig.iPLuginInstallTimeoutSec = 5;
	INFO_LOG("write log limit per sec:%d, type:%d, type str:%s, max run time mins:%d, plugin install time:%d", 
		stConfig.iLogLimitPerSec, stConfig.iLocalLogType, szTypeString, stConfig.iMaxRunMins, stConfig.iPLuginInstallTimeoutSec);

	gettimeofday(&stConfig.stTimeCur, NULL);
	stConfig.dwCurTime = stConfig.stTimeCur.tv_sec;
	srand(stConfig.dwCurTime);
	iRet = MtReport_Init_ByKey(0, iCfgShmKey, 0666|IPC_CREAT); 
	if(iRet < 0 || g_mtReport.pMtShm == NULL) {
		FATAL_LOG("get client shm failed key:%d size:%u ret:%d", iCfgShmKey, MYSIZEOF(MTREPORT_SHM), iRet);
		return -2;
	}
	stConfig.pReportShm = g_mtReport.pMtShm;

	if(strstr(stConfig.szSrvIp_master, "127.0.0.1")) {
		FATAL_LOG("invalid server address, must not be 127.0.0.1(%s)", stConfig.szSrvIp_master);
		return MTREPORT_ERROR_LINE;
	}

	INFO_LOG("%s shm ok -- size:%u, current path:%s",
		(iRet==1 ? "create" : "init"), MYSIZEOF(MTREPORT_SHM), stConfig.szCurPath); 
	INFO_LOG("read config - server:%s:%d ", stConfig.szSrvIp_master, stConfig.iSrvPort);
	INFO_LOG("local cust ip:%s, cmp time:%s", stConfig.szCustLocalIP, g_strCmpTime.c_str());

	// 基础信息，用于一键部署插件
	if(stConfig.szOs[0] == '\0') {
		std::string str;
		get_cmd_result("./run_tool.sh getos", str);
		memset(stConfig.szOs, 0, sizeof(stConfig.szOs));
		if(str.find("failed") != std::string::npos || str == "") {
			strncpy(stConfig.szOs, str.c_str(), sizeof(stConfig.szOs)-1);
			WARN_LOG("get os type failed, set to comm linux");
			strncpy(stConfig.szOs, "CommLinux", sizeof(stConfig.szOs)-1);
		}
		else
			strncpy(stConfig.szOs, str.c_str(), sizeof(stConfig.szOs)-1);
	}

	if(stConfig.szOsArc[0] == '\0') {
		std::string str;
		get_cmd_result("./run_tool.sh getarc", str);
		memset(stConfig.szOsArc, 0, sizeof(stConfig.szOsArc));
		strncpy(stConfig.szOsArc, str.c_str(), sizeof(stConfig.szOsArc)-1);
	}

	if(stConfig.szLibcVer[0] == '\0') {
		std::string str;
		get_cmd_result("./run_tool.sh getlibc", str);
		memset(stConfig.szLibcVer, 0, sizeof(stConfig.szLibcVer));
		strncpy(stConfig.szLibcVer, str.c_str(), sizeof(stConfig.szLibcVer)-1);
	}

	if(stConfig.szLibcppVer[0] == '\0') {
		std::string str;
		get_cmd_result("./run_tool.sh getlibcpp", str);
		memset(stConfig.szLibcppVer, 0, sizeof(stConfig.szLibcppVer));
		strncpy(stConfig.szLibcppVer, str.c_str(), sizeof(stConfig.szLibcppVer)-1);
	}

	INFO_LOG("get local system info - os:%s, arc:%s, libc ver:%s, libcpp ver:%s",
	    stConfig.szOs, stConfig.szOsArc, stConfig.szLibcVer, stConfig.szLibcppVer);
	INFO_LOG("write log limit per sec:%d, type:(%d)(%s)", iLogLimitPerSecSave, iLocalLogTypeSave, szTypeString);

    if(iProxyCount > 0)  {
        if(LoadProxyInfo(iProxyCount) < 0)
            return ERROR_LINE;
    }
    else if(stConfig.iProxyOnly) {
        ERROR_LOG("proxy config invalid, have no proxy but proxy only !");
        return ERROR_LINE;
    }
    else
        stConfig.iProxyCount = 0;

	stConfig.iLogLimitPerSec = iLogLimitPerSecSave;
	stConfig.iLocalLogType = iLocalLogTypeSave;
	return 0;
}

int InitReportAttr()
{
	int iRet = 0;
	int iSock = socket(AF_INET, SOCK_DGRAM, 0);
	if(iSock < 0) {
		ERROR_LOG("create socket failed, msg:%s", strerror(errno));
		return MTREPORT_ERROR_LINE;
	}
	InitSocketComm(iSock);

	iRet = AddSocket(iSock, NULL, OnMtreportPkg, NULL);
	if(iRet < 0) {
		ERROR_LOG("AddSocket failed, ret:%d", iRet); 
		return MTREPORT_ERROR_LINE;
	}
	DEBUG_LOG("add app attr socket index:%d sock fd:%d", iRet, iSock);
	stConfig.iAttrSocketIndex = iRet;

	stConfig.pEnvSess = (ENVSESSION*)stConfig.sSessBuf;
	stConfig.pEnvSess->iSockIndex = iRet;
	stConfig.pEnvSess->dwEventSetTimeSec = stConfig.stTimeCur.tv_sec;
	stConfig.pEnvSess->dwEventSetTimeUsec = stConfig.stTimeCur.tv_usec;
	stConfig.pEnvSess->bEventType = EVENT_TYPE_M_ATTR;

	uint32_t dwKey = rand();

	stConfig.pEnvSess->iExpireTimeMs = 10*1000;
	iRet = AddTimer(dwKey, stConfig.pEnvSess->iExpireTimeMs,
		OnEventTimer, stConfig.pEnvSess, MYSIZEOF(ENVSESSION), 0, NULL);
	if(iRet < 0) {
		ERROR_LOG("AddTimer for attr failed, key:%u, ret:%d", dwKey, iRet);
		return MTREPORT_ERROR_LINE;
	}
	DEBUG_LOG("add attr timer key:%u", dwKey);
	return 0;
}

// 收到代理 server 端发送过来的数据包
void OnProxyServerPkg(struct MtSocket *psock, char * sBuf, int iLen, time_t uiTime)
{
    int iProxy = -1;
    if(psock->iPrivLen != (int)sizeof(int)) {
		ERROR_LOG("check priv length failed %d != %d, packet from server:%s:%d",
            psock->iPrivLen, (int)sizeof(int),
			inet_ntoa(psock->last_recv_remote.sin_addr), ntohs(psock->last_recv_remote.sin_port));
		return;
	}
    iProxy = *((int*)(psock->sPrivBuf));
    if(iProxy < 0 || iProxy >= stConfig.iProxyCount) {
		ERROR_LOG("check priv proxy:%d (%d), packet from server:%s:%d",
            iProxy, stConfig.iProxyCount,
			inet_ntoa(psock->last_recv_remote.sin_addr), ntohs(psock->last_recv_remote.sin_port));
        return;
    }

    int iProxyInfo = -1;
    for(int i=0; i < MAX_CLIENT_PER_PROXY; i++) {
        if(stConfig.stProxy[iProxy].stProxyInfo[i].iSrvSock == psock->isock)
        {
            iProxyInfo = i;
            break;
        }
    }

    if(iProxyInfo < 0) {
        ERROR_LOG("not find proxy server sock:%d, server:%s:%d", psock->isock, 
			inet_ntoa(psock->last_recv_remote.sin_addr), ntohs(psock->last_recv_remote.sin_port));
        return;
    }

    TProxyInfo &proxyInfo = stConfig.stProxy[iProxy].stProxyInfo[iProxyInfo];
    proxyInfo.dwLastRecvPackTime = stConfig.dwCurTime;

	struct sockaddr_in addr_client;
	addr_client.sin_family = PF_INET;
	addr_client.sin_port = htons(proxyInfo.wClientPort);
	addr_client.sin_addr.s_addr = proxyInfo.dwClientIp;

	int iRet = SendPacket(stConfig.stProxy[iProxy].iListenSockIdx, &addr_client, sBuf, iLen);
	if(iRet != iLen) {
		ERROR_LOG("SendPacket (proxy server pkg) failed, pkglen:%d, ret:%d, server:%s:%d", iLen, iRet,
			inet_ntoa(psock->last_recv_remote.sin_addr), ntohs(psock->last_recv_remote.sin_port));
		return ;
	}
	DEBUG_LOG("forward proxy server pkg from:%s:%d, to client:%s:%d",
		inet_ntoa(psock->last_recv_remote.sin_addr), ntohs(psock->last_recv_remote.sin_port),
        ipv4_addr_str(proxyInfo.dwClientIp), proxyInfo.wClientPort);
}

// 收到被代理端发送过来的数据包
void OnProxyClientPkg(struct MtSocket *psock, char * sBuf, int iLen, time_t uiTime)
{
    uint32_t dwClientIp = psock->last_recv_remote.sin_addr.s_addr;
    uint16_t wClientPort = ntohs(psock->last_recv_remote.sin_port);
	DEBUG_LOG("get packet from proxy client - %s:%d lengh:%d", ipv4_addr_str(dwClientIp), wClientPort, iLen);
    int iProxy = -1;
    for(int i=0; i < stConfig.iProxyCount; i++) {
        if(stConfig.stProxy[i].iListenSock == psock->isock) {
            iProxy = i;
            break;
        }
    }
    if(iProxy < 0) {
        ERROR_LOG("not find proxy info, proxy sock:%d", psock->isock);
        return;
    }
	struct sockaddr_in addr_server;
	addr_server.sin_family = PF_INET;
	addr_server.sin_port = htons(stConfig.stProxy[iProxy].iProxyPort);
	addr_server.sin_addr.s_addr = stConfig.stProxy[iProxy].dwProxySrvIp;

    int iProxyInfo = -1, iFreeProxyInfo = -1;
    for(int i=0; i < MAX_CLIENT_PER_PROXY; i++) 
    {
        if(stConfig.stProxy[iProxy].stProxyInfo[i].dwClientIp == dwClientIp
            && stConfig.stProxy[iProxy].stProxyInfo[i].wClientPort == wClientPort) 
        {
            iProxyInfo = i;
            break;
        }
        else if(iFreeProxyInfo < 0) 
        {
            // 300 表示代理通道保存时间，5 分钟代理通道没有数据包将被回收使用
            if(stConfig.stProxy[iProxy].stProxyInfo[i].dwClientIp == 0
                || stConfig.stProxy[iProxy].stProxyInfo[i].dwLastRecvPackTime+300 <= stConfig.dwCurTime)
            {
                iFreeProxyInfo = i;
            }
        }
    }

    int iRet = 0;
    if(iProxyInfo >= 0) {
		stConfig.stProxy[iProxy].stProxyInfo[iProxyInfo].dwLastRecvPackTime = stConfig.dwCurTime;
		iRet = SendPacket(stConfig.stProxy[iProxy].stProxyInfo[iProxyInfo].iSrvSockIdx, &addr_server, sBuf, iLen);
		if(iRet != iLen) {
			ERROR_LOG("SendPacket failed, pkglen:%d, ret:%d, proxy - client:%s:%d, server:%s:%d", iLen, iRet,
                inet_ntoa(psock->last_recv_remote.sin_addr), wClientPort, 
                ipv4_addr_str(stConfig.stProxy[iProxy].dwProxySrvIp), stConfig.stProxy[iProxy].iProxyPort); 
			return;
		}
    }
    else if(iFreeProxyInfo >= 0) {
        TProxyInfo &proxyInfo = stConfig.stProxy[iProxy].stProxyInfo[iFreeProxyInfo];
		int iSock = socket(AF_INET, SOCK_DGRAM, 0);
		if(iSock < 0) {
			ERROR_LOG("create proxy server socket failed, msg:%s", strerror(errno));
			return;
		}
		InitSocketComm(iSock);
		iRet = AddSocket(iSock, NULL, OnProxyServerPkg, NULL, (char*)&iProxy, sizeof(iProxy));
		if(iRet < 0) {
			ERROR_LOG("AddSocket failed, ret:%d", iRet); 
			return; 
		}

        proxyInfo.iProxyIdx = iProxy;
        proxyInfo.iSrvSockIdx = iRet;
        proxyInfo.iSrvSock = iSock;
        proxyInfo.dwClientIp = dwClientIp;
        proxyInfo.wClientPort = wClientPort;
        proxyInfo.dwLastRecvPackTime = stConfig.dwCurTime;

		INFO_LOG("add proxy server socket index:%d fd:%d, proxy - client:%s:%d, server:%s:%d", iRet, iSock,
            inet_ntoa(psock->last_recv_remote.sin_addr), wClientPort, 
            ipv4_addr_str(stConfig.stProxy[iProxy].dwProxySrvIp), stConfig.stProxy[iProxy].iProxyPort); 
	
		iRet = SendPacket(proxyInfo.iSrvSockIdx, &addr_server, sBuf, iLen);
		if(iRet != iLen) {
			ERROR_LOG("SendPacket failed, pkglen:%d, ret:%d, proxy - client:%s:%d, server:%s:%d", iLen, iRet,
                inet_ntoa(psock->last_recv_remote.sin_addr), wClientPort, 
                ipv4_addr_str(stConfig.stProxy[iProxy].dwProxySrvIp), stConfig.stProxy[iProxy].iProxyPort); 
			return;
		}
    }
    else {
        ERROR_LOG("proxy - %s:%d, client over limit:%d",
            ipv4_addr_str(stConfig.stProxy[iProxy].dwProxySrvIp), stConfig.stProxy[iProxy].iProxyPort,
            MAX_CLIENT_PER_PROXY);
        return;
    }
}

int InitProxy() 
{
    int iSock = 0;
    int iRet = 0;

	// 创建代理服务 socket
    for(int i = 0; i < stConfig.iProxyCount; i++) {
        iSock = socket(AF_INET, SOCK_DGRAM, 0);
        if(iSock < 0) {
            FATAL_LOG("create socket failed, msg:%s", strerror(errno));
            return MTREPORT_ERROR_LINE;
        }
        InitSocketComm(iSock);

        struct sockaddr_in addr;
        memset(&addr, 0, MYSIZEOF(addr));
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(stConfig.stProxy[i].iListenPort);

        if(bind(iSock, (struct sockaddr *)&addr,  MYSIZEOF(addr)) < 0) {
            FATAL_LOG("bind failed, proxy port:%d msg:%s", stConfig.stProxy[i].iListenPort, strerror(errno));
            return MTREPORT_ERROR_LINE;
        }

        //bind
        iRet = AddSocket(iSock, NULL, OnProxyClientPkg, NULL);
        if(iRet < 0) {
            FATAL_LOG("AddSocket failed, ret:%d", iRet); 
            return MTREPORT_ERROR_LINE;
        }
        stConfig.stProxy[i].iListenSockIdx = iRet;
        stConfig.stProxy[i].iListenSock = iSock;

        DEBUG_LOG("add proxy socket index:%d, sock fd:%d, proxy:%s:%d", iRet, iSock,
            ipv4_addr_str(stConfig.stProxy[i].dwProxySrvIp), stConfig.stProxy[i].iProxyPort);
    }
    return 0;
}

int InitHello() 
{
    int iRet = 0;
    int iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if(iSock < 0) {
        ERROR_LOG("create socket failed, msg:%s", strerror(errno));
        return MTREPORT_ERROR_LINE;
    }
    InitSocketComm(iSock);

    if(setsockopt(iSock, IPPROTO_IP, IP_TTL, (char *)&stConfig.ttl, sizeof(stConfig.ttl)) != 0)
    {
        ERROR_LOG("setsockopt(IPPROTO_IP, IP_TTL), msg:%s", strerror(errno));
        return MTREPORT_ERROR_LINE;
    }

	// reinit
	stConfig.pReportShm->iMtClientIndex = -1;
	stConfig.pReportShm->iMachineId = -1;
	stConfig.pReportShm->dwPkgSeq = rand();

	struct sockaddr_in addr_server;
	addr_server.sin_family = PF_INET;
	addr_server.sin_port = htons(stConfig.iSrvPort);
	addr_server.sin_addr.s_addr = inet_addr(stConfig.szSrvIp_master);
	iRet = AddSocket(iSock, &addr_server, OnMtreportPkg, NULL);
	if(iRet < 0) {
		ERROR_LOG("AddSocket failed, ret:%d", iRet); 
		return MTREPORT_ERROR_LINE;
	}
	DEBUG_LOG("add socket index:%d sock fd:%d", iRet, iSock);
	stConfig.iConfigSocketIndex = iRet;

	stConfig.pPkgSess = (PKGSESSION*)stConfig.sSessBuf;
	stConfig.pPkg = stConfig.sSessBuf+MYSIZEOF(PKGSESSION);
	stConfig.iPkgLen = PKG_BUFF_LENGTH;
	uint32_t dwKey = MakeFirstHelloPkg(addr_server.sin_addr.s_addr);
	stConfig.pPkgSess->iSockIndex = iRet;
	iRet = SendPacket(iRet, NULL, stConfig.pPkg, stConfig.iPkgLen);
	if(dwKey == 0 || iRet != stConfig.iPkgLen) {
		ERROR_LOG("SendPacket failed, pkglen:%d, ret:%d", stConfig.iPkgLen, iRet); 
		return MTREPORT_ERROR_LINE;
	}
	stConfig.pPkgSess->dwSendTimeSec = stConfig.stTimeCur.tv_sec;
	stConfig.pPkgSess->dwSendTimeUsec = stConfig.stTimeCur.tv_usec;
	stConfig.pPkgSess->bSessStatus = SESS_FLAG_WAIT_RESPONSE;

	iRet = AddTimer(dwKey, PKG_TIMEOUT_MS, OnPkgExpire, 
		stConfig.pPkgSess, MYSIZEOF(PKGSESSION), stConfig.iPkgLen, stConfig.pPkg);
	if(iRet < 0) {
		ERROR_LOG("AddTimer failed ! pkglen:%d, key:%u, ret:%d", stConfig.iPkgLen, dwKey, iRet);
		return MTREPORT_ERROR_LINE;
	}
	DEBUG_LOG("add timer key:%u - datalen:%u", dwKey, stConfig.iPkgLen);
	return 0;
}

inline int attr_cmp(const void *a, const void *b)
{
	return ((AttrNode*)a)->iAttrID - ((AttrNode*)b)->iAttrID;
}

inline int str_attr_cmp(const void *pKey, const void *pNode)
{
    if(((const StrAttrNode*)pKey)->iStrAttrId == ((const StrAttrNode*)pNode)->iStrAttrId
        && !strcmp(((const StrAttrNode*)pKey)->szStrInfo, ((const StrAttrNode*)pNode)->szStrInfo))
    {
        return 0;
    }
	if(((const StrAttrNode*)pKey)->iStrAttrId > ((const StrAttrNode*)pNode)->iStrAttrId
		|| (((const StrAttrNode*)pKey)->iStrAttrId==((const StrAttrNode*)pNode)->iStrAttrId
			&& strcmp(((const StrAttrNode*)pKey)->szStrInfo, ((const StrAttrNode*)pNode)->szStrInfo) > 0))
	{
	    return 1;
	}
	return -1;
}

void ReadStrAttr()
{
	StrAttrNode *pNode = NULL;
	StrAttrNode *pNodeSearch = NULL;

	bool bReverse = false;
	_HashTableHead *pTableHead = (_HashTableHead*)g_mtReport.stStrAttrHash.pHash;
	pNode = (StrAttrNode*)GetFirstNode(&g_mtReport.stStrAttrHash);
	if(pNode == NULL && pTableHead->dwNodeUseCount > 0) {
		bReverse = true;
		pNode = (StrAttrNode*)GetFirstNodeRevers(&g_mtReport.stStrAttrHash);
		ERROR_LOG("man by bug - has node:%u, bug get first null", pTableHead->dwNodeUseCount);
	}

	int j = 0;
	for(j=0; pNode != NULL && j < MTATTR_HASH_NODE*STATIC_HASH_ROW_MAX 
		&& stConfig.wReadStrAttrCount < MAX_STR_ATTR_READ_PER_EACH; j++) 
	{
		if(!HASH_NODE_IS_NODE_USED(pNode))
		{
			ERROR_LOG("check node error in str attr list [not use] -- attr:%d value:%d",
				pNode->iStrAttrId, pNode->iStrVal);
		}
		if(pNode->bSyncProcess < 1)
		{
			ERROR_LOG("check node error in str attr list [syncflag] -- attr:%d value:%d bSyncProcess:%d",
				pNode->iStrAttrId, pNode->iStrVal, pNode->bSyncProcess);
		}

		pNodeSearch = (StrAttrNode*)bsearch(pNode, 
			stConfig.stStrAttrRead, stConfig.wReadStrAttrCount, MYSIZEOF(StrAttrNode), str_attr_cmp);
		if(pNodeSearch != NULL)
			pNodeSearch->iStrVal += pNode->iStrVal;
		else if(pNode->iStrVal > 0) {
			memcpy(stConfig.stStrAttrRead+stConfig.wReadStrAttrCount, pNode, MYSIZEOF(StrAttrNode));
			stConfig.wReadStrAttrCount++;
			qsort(stConfig.stStrAttrRead, stConfig.wReadStrAttrCount, MYSIZEOF(StrAttrNode), attr_cmp);
		}
		RemoveHashNode(&g_mtReport.stStrAttrHash, pNode);
		memset(pNode, 0, sizeof(StrAttrNode));

		if(bReverse)
			pNode = (StrAttrNode*)GetCurNodeRevers(&g_mtReport.stStrAttrHash);
		else
			pNode = (StrAttrNode*)GetCurNode(&g_mtReport.stStrAttrHash);
	}

	if(bReverse && pTableHead->dwNodeUseCount <=1 )  {
		ResetHashTable(&g_mtReport.stStrAttrHash);
		g_mtReport.stStrAttrHash.bAccessCheck = 1;
	}
	else if(!bReverse && pTableHead->dwNodeUseCount <= 0) {
		g_mtReport.stStrAttrHash.bAccessCheck = 0;
	}

	if(j >= MTATTR_HASH_NODE*STATIC_HASH_ROW_MAX)
	{
		ERROR_LOG("read str attr node hash may bug -- link node(%d>=%d)", 
			j, MTATTR_HASH_NODE*STATIC_HASH_ROW_MAX);
	}
}

void SendServerRespPkg(char *pRespContent, int iRespContentLen, CBasicPacket &pkg)
{
    stConfig.pPkgSess = (PKGSESSION*)stConfig.sSessBuf;
	stConfig.pPkg = stConfig.sSessBuf+MYSIZEOF(PKGSESSION);
	stConfig.iPkgLen = PKG_BUFF_LENGTH;

    INFO_LOG("send packet to server:%s:%d, cmd:%u, seq:%u, session:%lu",
        stConfig.szSrvIp_master, stConfig.iSrvPort, pkg.m_dwReqCmd, pkg.m_dwReqSeq, stConfig.qwServerPacketSessId);
    if(MakeServerRespPkg(pRespContent, iRespContentLen, pkg) > 0) {
        struct sockaddr_in addr_server;
        addr_server.sin_family = PF_INET;
        addr_server.sin_port = htons(stConfig.iSrvPort);
        addr_server.sin_addr.s_addr = inet_addr(stConfig.szSrvIp_master);
        int iRet = SendPacket(stConfig.iConfigSocketIndex, &addr_server, stConfig.pPkg, stConfig.iPkgLen);
        if(iRet != stConfig.iPkgLen) {
		    ERROR_LOG("SendPacket(report preinstall status) failed, pkglen:%d, ret:%d", stConfig.iPkgLen, iRet);
        }
    }
}

void ReadAttr()
{
	AttrNode *pNode = NULL;
	int i=0, j=0;

	// first read spec attr
	if(g_mtReport.pMtShm->iAttrSpecialCount > 0
		&& g_mtReport.pMtShm->iAttrSpecialCount <= MAX_ATTR_READ_PER_EACH) {
		memcpy(stConfig.stAttrRead, 
			g_mtReport.pMtShm->sArrtListSpec, MYSIZEOF(AttrNode)*g_mtReport.pMtShm->iAttrSpecialCount);
		stConfig.wReadAttrCount = g_mtReport.pMtShm->iAttrSpecialCount;
		g_mtReport.pMtShm->iAttrSpecialCount = 0;
		qsort(stConfig.stAttrRead, stConfig.wReadAttrCount, MYSIZEOF(AttrNode), attr_cmp);
	}
	else if(g_mtReport.pMtShm->iAttrSpecialCount > MAX_ATTR_READ_PER_EACH) {
		memcpy(stConfig.stAttrRead, 
			g_mtReport.pMtShm->sArrtListSpec, MYSIZEOF(AttrNode)*MAX_ATTR_READ_PER_EACH);
		stConfig.wReadAttrCount = MAX_ATTR_READ_PER_EACH;
		g_mtReport.pMtShm->iAttrSpecialCount -= MAX_ATTR_READ_PER_EACH;
		memmove(g_mtReport.pMtShm->sArrtListSpec, 
			(char*)(g_mtReport.pMtShm->sArrtListSpec)+MYSIZEOF(AttrNode)*MAX_ATTR_READ_PER_EACH,
			MYSIZEOF(AttrNode)*g_mtReport.pMtShm->iAttrSpecialCount);
		return;
	}

	// read attr hash
	AttrNode *pNodeSearch = NULL;
	for(i=0; i < MTATTR_SHM_DEF_COUNT; i++)
	{
		bool bReverse = false;

		_HashTableHead *pTableHead = (_HashTableHead*)g_mtReport.stAttrHash[i].pHash;
		pNode = (AttrNode*)GetFirstNode(&g_mtReport.stAttrHash[i]);
		if(pNode == NULL && pTableHead->dwNodeUseCount > 0) {
			bReverse = true;
			pNode = (AttrNode*)GetFirstNodeRevers(&g_mtReport.stAttrHash[i]);
			ERROR_LOG("man by bug - has node:%u, bug get first null", pTableHead->dwNodeUseCount);
		}

		for(j=0; pNode != NULL && j < MTATTR_HASH_NODE*STATIC_HASH_ROW_MAX 
			&& stConfig.wReadAttrCount < MAX_ATTR_READ_PER_EACH; j++) 
		{
			if(!HASH_NODE_IS_NODE_USED(pNode))
			{
				ERROR_LOG("check node error in attr list [not use] -- attr:%d value:%d",
					pNode->iAttrID, pNode->iCurValue);
			}
			if(pNode->bSyncProcess < 1)
			{
				ERROR_LOG("check node error in attr list [syncflag] -- attr:%d value:%d bSyncProcess:%d",
					pNode->iAttrID, pNode->iCurValue, pNode->bSyncProcess);
			}

			pNodeSearch = (AttrNode*)bsearch(pNode, 
				stConfig.stAttrRead, stConfig.wReadAttrCount, MYSIZEOF(AttrNode), attr_cmp);
			if(pNodeSearch != NULL)
				pNodeSearch->iCurValue += pNode->iCurValue;
			else if(pNode->iCurValue > 0) {
				memcpy(stConfig.stAttrRead+stConfig.wReadAttrCount, pNode, MYSIZEOF(AttrNode));
				stConfig.wReadAttrCount++;
				qsort(stConfig.stAttrRead, stConfig.wReadAttrCount, MYSIZEOF(AttrNode), attr_cmp);
			}
			RemoveHashNode(&g_mtReport.stAttrHash[i], pNode);
			memset(pNode, 0, sizeof(AttrNode));

			if(bReverse)
				pNode = (AttrNode*)GetCurNodeRevers(&g_mtReport.stAttrHash[i]);
			else
				pNode = (AttrNode*)GetCurNode(&g_mtReport.stAttrHash[i]);
		}

		if(bReverse && pTableHead->dwNodeUseCount <=1 )  {
			ResetHashTable(&g_mtReport.stAttrHash[i]);
			g_mtReport.stAttrHash[i].bAccessCheck = 1;
		}
		else if(!bReverse && pTableHead->dwNodeUseCount <= 0) {
			g_mtReport.stAttrHash[i].bAccessCheck = 0;
		}

		if(j >= MTATTR_HASH_NODE*STATIC_HASH_ROW_MAX)
		{
			ERROR_LOG("read attr node hash may bug -- link node(%d>=%d)", 
				j, MTATTR_HASH_NODE*STATIC_HASH_ROW_MAX);
		}

		if(stConfig.wReadAttrCount >= MAX_ATTR_READ_PER_EACH)
			break;
	}
}

int EnvSendStrAttrToServer()
{
	if(stConfig.pReportShm->dwAttrSrvIp <= 0 && stConfig.iCustAttrSrvPort==0)
	{
		ERROR_LOG("bug - have no attr server !");
		return MTREPORT_ERROR_LINE;
	}

	stConfig.pPkgSess = (PKGSESSION*)stConfig.sSessBuf;
	stConfig.pPkg = stConfig.sSessBuf+MYSIZEOF(PKGSESSION);
	stConfig.iPkgLen = MAX_ATTR_PKG_LENGTH;

	struct sockaddr_in addr_server;
	addr_server.sin_family = PF_INET;

	if(stConfig.szCustAttrSrvIp[0] != '\0' && stConfig.iCustAttrSrvPort != 0)
	{
		addr_server.sin_port = htons(stConfig.iCustAttrSrvPort);
		addr_server.sin_addr.s_addr = inet_addr(stConfig.szCustAttrSrvIp);
	}
	else
	{
		addr_server.sin_port = htons(stConfig.pReportShm->wAttrServerPort);
		addr_server.sin_addr.s_addr = stConfig.pReportShm->dwAttrSrvIp;
	}

	static char strAttrSendBuf[MAX_ATTR_PKG_LENGTH];
	StrAttrNodeClient *pNodeBuf = NULL;
	int iUseBufLen = 0, i = 0, iTmpLen = 0;
	for(i=0; i < stConfig.wReadStrAttrCount; i++)
	{
		DEBUG_LOG("report str attr:%d, str:%s, value:%d", 
			stConfig.stStrAttrRead[i].iStrAttrId, stConfig.stStrAttrRead[i].szStrInfo,
			stConfig.stStrAttrRead[i].iStrVal);
		iTmpLen = strlen(stConfig.stStrAttrRead[i].szStrInfo)+1;
		if(iTmpLen+iUseBufLen+sizeof(StrAttrNodeClient) >= sizeof(strAttrSendBuf))
		    break;

		pNodeBuf = (StrAttrNodeClient*)(strAttrSendBuf+iUseBufLen);
		pNodeBuf->iStrAttrId = htonl(stConfig.stStrAttrRead[i].iStrAttrId);
		pNodeBuf->iStrVal = htonl(stConfig.stStrAttrRead[i].iStrVal);
		pNodeBuf->iStrLen = htonl(iTmpLen);
		memcpy(pNodeBuf->szStrInfo, stConfig.stStrAttrRead[i].szStrInfo, iTmpLen);
		iUseBufLen += sizeof(StrAttrNodeClient) + iTmpLen;
	}

	if(i < stConfig.wReadStrAttrCount) {
		ERROR_LOG("need more space to send str attr - %d < %d, use:%d", i, stConfig.wReadStrAttrCount, iUseBufLen);
	}

	uint32_t dwKey = 0;
	if(stConfig.iEnableEncryptData) {
		static char sAttrBufEnc[MAX_ATTR_PKG_LENGTH+256];
		int iAttrBufLenEnc = ((iUseBufLen>>4)+1)<<4;
		if(iAttrBufLenEnc > (int)sizeof(sAttrBufEnc)) {
			ERROR_LOG("need more space %d > %d", iAttrBufLenEnc, (int)sizeof(sAttrBufEnc));
			return MTREPORT_ERROR_LINE;
		}
		aes_cipher_data((const uint8_t*)strAttrSendBuf, iUseBufLen, 
			(uint8_t*)sAttrBufEnc, (const uint8_t*)stConfig.pReportShm->sRandKey, AES_128);
		dwKey = MakeAttrPkg(addr_server, sAttrBufEnc, iAttrBufLenEnc, true);
	}
	else
		dwKey = MakeAttrPkg(addr_server, (char*)strAttrSendBuf, iUseBufLen, true);

	stConfig.pPkgSess->iSockIndex = stConfig.pEnvSess->iSockIndex; 
	int iRet = SendPacket(stConfig.pPkgSess->iSockIndex, &addr_server, stConfig.pPkg, stConfig.iPkgLen);
	if(iRet != stConfig.iPkgLen) {
		ERROR_LOG("SendPacket(str attr) failed, pkglen:%d, ret:%d", stConfig.iPkgLen, iRet);
		return MTREPORT_ERROR_LINE;
	}
	else {
		if(g_mtReport.pMtShm->dwFirstAttrReportTime == 0)
			g_mtReport.pMtShm->dwFirstAttrReportTime = stConfig.dwCurTime;

		stConfig.pPkgSess->dwSendTimeSec = stConfig.stTimeCur.tv_sec;
		stConfig.pPkgSess->dwSendTimeUsec = stConfig.stTimeCur.tv_usec;
		stConfig.pPkgSess->bSessStatus = SESS_FLAG_WAIT_RESPONSE;
		stConfig.pPkgSess->stCmdSessData.attr.bTrySrvCount = 1;
		stConfig.pPkgSess->stCmdSessData.attr.dwAttrSrvIP = addr_server.sin_addr.s_addr;
		SetSocketAddress(
			stConfig.pPkgSess->iSockIndex, addr_server.sin_addr.s_addr, ntohs(addr_server.sin_port));

		int iTime = GetMaxResponseTime(stConfig.pPkgSess->iSockIndex)+1000;
		iRet = AddTimer(dwKey, iTime, OnPkgExpire,
			stConfig.pPkgSess, MYSIZEOF(PKGSESSION), stConfig.iPkgLen, stConfig.pPkg);
		if(iRet < 0) {
			ERROR_LOG("AddTimer(app log) failed ! pkglen:%d, key:%u, ret:%d", stConfig.iPkgLen, dwKey, iRet);
			return MTREPORT_ERROR_LINE;
		}
	}
	return 1;
}

int EnvSendAttrToServer()
{
	if(stConfig.pReportShm->dwAttrSrvIp <= 0 && stConfig.iCustAttrSrvPort==0)
	{
		ERROR_LOG("bug - have no attr server !");
		return MTREPORT_ERROR_LINE;
	}

	stConfig.pPkgSess = (PKGSESSION*)stConfig.sSessBuf;
	stConfig.pPkg = stConfig.sSessBuf+MYSIZEOF(PKGSESSION);
	stConfig.iPkgLen = MAX_ATTR_PKG_LENGTH;

	struct sockaddr_in addr_server;
	addr_server.sin_family = PF_INET;

	if(stConfig.szCustAttrSrvIp[0] != '\0' && stConfig.iCustAttrSrvPort != 0)
	{
		addr_server.sin_port = htons(stConfig.iCustAttrSrvPort);
		addr_server.sin_addr.s_addr = inet_addr(stConfig.szCustAttrSrvIp);
	}
	else
	{
		addr_server.sin_port = htons(stConfig.pReportShm->wAttrServerPort);
		addr_server.sin_addr.s_addr = stConfig.pReportShm->dwAttrSrvIp;
	}

	static AttrNodeClient stAttrRead[MAX_ATTR_READ_PER_EACH];
	for(int i=0; i < stConfig.wReadAttrCount && i < MAX_ATTR_READ_PER_EACH; i++){
		DEBUG_LOG("report attr:%d value:%d", 
			stConfig.stAttrRead[i].iAttrID, stConfig.stAttrRead[i].iCurValue);
		stAttrRead[i].iAttrID = htonl(stConfig.stAttrRead[i].iAttrID);
		stAttrRead[i].iCurValue = htonl(stConfig.stAttrRead[i].iCurValue);
	}

	uint32_t dwKey = 0;
	if(stConfig.iEnableEncryptData) {
		static char sAttrBufEnc[MAX_ATTR_PKG_LENGTH+256];
		int iAttrBufLenEnc = (((stConfig.wReadAttrCount*MYSIZEOF(AttrNodeClient))>>4)+1)<<4;
		if(iAttrBufLenEnc > (int)sizeof(sAttrBufEnc)) {
			ERROR_LOG("need more space %d > %d", iAttrBufLenEnc, (int)sizeof(sAttrBufEnc));
			return MTREPORT_ERROR_LINE;
		}
		aes_cipher_data((const uint8_t*)stAttrRead, stConfig.wReadAttrCount*MYSIZEOF(AttrNodeClient), 
			(uint8_t*)sAttrBufEnc, (const uint8_t*)stConfig.pReportShm->sRandKey, AES_128);
		dwKey = MakeAttrPkg(addr_server, sAttrBufEnc, iAttrBufLenEnc);
	}
	else
		dwKey = MakeAttrPkg(addr_server, (char*)stAttrRead, stConfig.wReadAttrCount*MYSIZEOF(AttrNodeClient));

	stConfig.pPkgSess->iSockIndex = stConfig.pEnvSess->iSockIndex; 
	int iRet = SendPacket(stConfig.pPkgSess->iSockIndex, &addr_server, stConfig.pPkg, stConfig.iPkgLen);
	if(iRet != stConfig.iPkgLen) {
		ERROR_LOG("SendPacket(attr) failed, pkglen:%d, ret:%d", stConfig.iPkgLen, iRet);
		return MTREPORT_ERROR_LINE;
	}
	else {
		if(g_mtReport.pMtShm->dwFirstAttrReportTime == 0)
			g_mtReport.pMtShm->dwFirstAttrReportTime = stConfig.dwCurTime;

		stConfig.pPkgSess->dwSendTimeSec = stConfig.stTimeCur.tv_sec;
		stConfig.pPkgSess->dwSendTimeUsec = stConfig.stTimeCur.tv_usec;
		stConfig.pPkgSess->bSessStatus = SESS_FLAG_WAIT_RESPONSE;
		stConfig.pPkgSess->stCmdSessData.attr.bTrySrvCount = 1;
		stConfig.pPkgSess->stCmdSessData.attr.dwAttrSrvIP = addr_server.sin_addr.s_addr;
		SetSocketAddress(
			stConfig.pPkgSess->iSockIndex, addr_server.sin_addr.s_addr, ntohs(addr_server.sin_port));

		int iTime = GetMaxResponseTime(stConfig.pPkgSess->iSockIndex)+1000;
		iRet = AddTimer(dwKey, iTime, OnPkgExpire,
			stConfig.pPkgSess, MYSIZEOF(PKGSESSION), stConfig.iPkgLen, stConfig.pPkg);
		if(iRet < 0) {
			ERROR_LOG("AddTimer(app log) failed ! pkglen:%d, key:%u, ret:%d", stConfig.iPkgLen, dwKey, iRet);
			return MTREPORT_ERROR_LINE;
		}
	}
	return 1;
}

int ReadAppLogToBuf(MTLog *plog)
{
	static char sLogBuf[MTREPORT_LOG_MAX_LENGTH+256];
	assert(plog->iAppId && plog->iModuleId);

	int iTmpLen = 0, iRet = 0;
	int iUseBufLen = MYSIZEOF(LogInfo);
	LogInfo *pLogBuf = (LogInfo*)sLogBuf;

	pLogBuf->dwLogSeq = htonl(plog->dwLogSeq);
	pLogBuf->iAppId = htonl(plog->iAppId);
	pLogBuf->iModuleId = htonl(plog->iModuleId);
	pLogBuf->dwLogConfigId = htonl(plog->dwLogConfigId);
	pLogBuf->wLogType = htons(plog->wLogType);
	pLogBuf->qwLogTime = htonll(plog->qwLogTime);

	if(plog->iCustVmemIndex > 0) {
		iTmpLen = (int)(MYSIZEOF(sLogBuf)-iUseBufLen);
		if((iRet=MtReport_GetFromVmem(plog->iCustVmemIndex, pLogBuf->sLog, &iTmpLen)) >= 0){
			pLogBuf->wCustDataLen = htons(iTmpLen);
			iUseBufLen += iTmpLen;
			MtReport_FreeVmem(plog->iCustVmemIndex);
			plog->iCustVmemIndex = 0;
		}
		else {
			pLogBuf->wCustDataLen = 0;
			ERROR_LOG("MtReport_GetFromVmem failed index:%d, ret:%d", plog->iCustVmemIndex, iRet);
		}
	}
	else
		pLogBuf->wCustDataLen = 0;

	char *pbuf = pLogBuf->sLog+ntohs(pLogBuf->wCustDataLen);
	int iLogContentLen = 0;
	if(plog->iVarmemIndex < 0) {
		strcpy(pbuf, plog->sLogContent);
		iLogContentLen = strlen(plog->sLogContent)+1; 
	}
	else {
		memcpy(pbuf, plog->sLogContent, MYSIZEOF(plog->sLogContent)-4);
		pbuf += MYSIZEOF(plog->sLogContent)-4;
		iLogContentLen = MYSIZEOF(plog->sLogContent)-4;
		int iTmpLen = MYSIZEOF(sLogBuf)-iUseBufLen-iLogContentLen-1;
		if((iRet=MtReport_GetFromVmem(plog->iVarmemIndex, pbuf, &iTmpLen)) >= 0) {
			uint32_t dwCheck = *((uint32_t*)(plog->sLogContent+MYSIZEOF(plog->sLogContent)-4));
			if(dwCheck != *(uint32_t*)pbuf)
				ERROR_LOG("vmem shm log check failed (%u != %u)", dwCheck, *(uint32_t*)pbuf);
			iLogContentLen += iTmpLen+1; 
		}
		else 
			ERROR_LOG("MtReport_GetFromVmem failed index:%d, ret:%d", plog->iVarmemIndex, iRet);
		MtReport_FreeVmem(plog->iVarmemIndex);
		plog->iVarmemIndex = 0;
	}

	pLogBuf->wLogDataLen = htons(iLogContentLen);
	iUseBufLen += iLogContentLen;

	DEBUG_LOG("------- read app:[custlen:%d-loglen:%d-use buf:%d] log:[app:%d-module:%d]--[%s] --",
		ntohs(pLogBuf->wCustDataLen), iLogContentLen, iUseBufLen, plog->iAppId, plog->iModuleId, pbuf);

	for(int i=0; i < stConfig.bReadAppTypeCount; i++) {
		if(stConfig.stAppLogRead[i].iReadAppId == plog->iAppId
			&& MAX_APP_LOG_PKG_LENGTH-stConfig.stAppLogRead[i].iReadAppLogBufLen >= iUseBufLen) {
			memcpy(stConfig.stAppLogRead[i].sAppLogBuf+stConfig.stAppLogRead[i].iReadAppLogBufLen,
				sLogBuf, iUseBufLen);
			stConfig.stAppLogRead[i].iReadAppLogBufLen += iUseBufLen;
			return 0;
		}
	}

	if(stConfig.bReadAppTypeCount >= MAX_APP_TYPE_READ_PER_EACH)
	{
		return -1;
	}

	TAppLogRead &stAppLog = stConfig.stAppLogRead[stConfig.bReadAppTypeCount];
	stAppLog.iReadAppId = plog->iAppId;
	stAppLog.iReadAppLogBufLen = iUseBufLen;
	assert(iUseBufLen < (int)sizeof(stAppLog.sAppLogBuf));
	memcpy(stAppLog.sAppLogBuf, sLogBuf, iUseBufLen);
	for(int i=0; i < stConfig.pReportShm->wAppConfigCount; i++) {
		if(stConfig.pReportShm->stAppConfigList[i].iAppId == plog->iAppId) {
			stAppLog.iAppInfoIdx =  i;
			stConfig.bReadAppTypeCount++;
			return 0;
		}
	}
	ERROR_LOG("find appinfo failed, appid:%d", plog->iAppId);
	return 0;
}

void ReadAppLog()
{
	MTLog *plog = NULL;
	bool bReadSpecLog = true;

	while(stConfig.pReportShm->iLogSpecialReadIdx >= 0 &&
		stConfig.pReportShm->iLogSpecialReadIdx != g_mtReport.pMtShm->iLogSpecialWriteIdx)
	{
		bReadSpecLog = true;
		plog = stConfig.pReportShm->sLogListSpec+stConfig.pReportShm->iLogSpecialReadIdx;
		if(plog->dwLogSeq == 0) {
			if((uint32_t)(plog->qwLogTime/1000000ULL+5) <= (uint32_t)(time(NULL))) {
				if(stConfig.pReportShm->iLogSpecialReadIdx+1 >= MTLOG_LOG_SPECIAL_COUNT)
					stConfig.pReportShm->iLogSpecialReadIdx = 0;
				else
					stConfig.pReportShm->iLogSpecialReadIdx++;
			}
			break;
		}

		if(0 == plog->iAppId) {
			SLogConfig *pcfg = NULL;
			for(int i=0; i < g_mtReport.pMtShm->wLogConfigCount; i++)
			{
				if(g_mtReport.pMtShm->stLogConfig[i].dwCfgId == plog->dwLogConfigId) {
					pcfg = g_mtReport.pMtShm->stLogConfig+i;
					break;
				}
			}
			if(pcfg == NULL) {
				if(stConfig.qwCurTime >= plog->qwLogTime+MTLOG_LOG_TIMEOUTMS_IN_SPEC*1000) {
					ERROR_LOG("log timeout in special, log config id:%u", plog->dwLogConfigId);
					bReadSpecLog = false;
				}
				else {
					WARN_LOG("not find log config :%u", plog->dwLogConfigId);
					break; 
				}
			}
			else if(!(pcfg->iLogType & plog->wLogType))
				bReadSpecLog = false;
			else {
				plog->iAppId = pcfg->iAppId;
				plog->iModuleId = pcfg->iModuleId;
			}
		}

		if(bReadSpecLog && ReadAppLogToBuf(plog) < 0) {
			return;
		}
		if(stConfig.pReportShm->iLogSpecialReadIdx+1 >= MTLOG_LOG_SPECIAL_COUNT)
			stConfig.pReportShm->iLogSpecialReadIdx = 0;
		else
			stConfig.pReportShm->iLogSpecialReadIdx++;
	}
	if(stConfig.pReportShm->iLogSpecialReadIdx == g_mtReport.pMtShm->iLogSpecialWriteIdx)
		stConfig.pReportShm->iLogSpecialReadIdx = -1;

	for(int i=0; i < MTLOG_SHM_DEF_COUNT; i++) 
	{
		if(SYNC_CAS_GET(&(stConfig.pReportShm->stLogShm[i].bTryGetLogIndex))) {
			while(stConfig.pReportShm->stLogShm[i].iLogStarIndex >= 0
				&& stConfig.pReportShm->stLogShm[i].iLogStarIndex!=stConfig.pReportShm->stLogShm[i].iWriteIndex)
			{
				plog = stConfig.pReportShm->stLogShm[i].sLogList+stConfig.pReportShm->stLogShm[i].iLogStarIndex;
				if(plog->dwLogSeq == 0)
				{
					if((uint32_t)(plog->qwLogTime/1000000ULL+5) >= (uint32_t)(time(NULL))) {
						if(stConfig.pReportShm->stLogShm[i].iLogStarIndex+1 >= MTLOG_SHM_RECORDS_COUNT)
							stConfig.pReportShm->stLogShm[i].iLogStarIndex = 0;
						else
							stConfig.pReportShm->stLogShm[i].iLogStarIndex++;
					}
					break;
				}

				if(ReadAppLogToBuf(plog) < 0) {
					SYNC_CAS_FREE(stConfig.pReportShm->stLogShm[i].bTryGetLogIndex);
					return;
				}
				if(stConfig.pReportShm->stLogShm[i].iLogStarIndex+1 >= MTLOG_SHM_RECORDS_COUNT)
					stConfig.pReportShm->stLogShm[i].iLogStarIndex = 0;
				else
					stConfig.pReportShm->stLogShm[i].iLogStarIndex++;
			}
			if(stConfig.pReportShm->stLogShm[i].iLogStarIndex==stConfig.pReportShm->stLogShm[i].iWriteIndex)
				stConfig.pReportShm->stLogShm[i].iLogStarIndex = -1;
			SYNC_CAS_FREE(stConfig.pReportShm->stLogShm[i].bTryGetLogIndex);
		}
	}
}

int EnvSendAppLogToServer(TAppLogRead &stAppLog)
{
	AppInfo & stInfo = stConfig.pReportShm->stAppConfigList[stAppLog.iAppInfoIdx];
	if(stInfo.dwAppSrvMaster <= 0) {
		ERROR_LOG("have no app master server appid:%d", stInfo.iAppId);
		return MTREPORT_ERROR_LINE;
	}

	stConfig.pPkgSess = (PKGSESSION*)stConfig.sSessBuf;
	stConfig.pPkg = stConfig.sSessBuf+MYSIZEOF(PKGSESSION);
	stConfig.iPkgLen = MYSIZEOF(stConfig.sSessBuf)-MYSIZEOF(PKGSESSION);

	struct sockaddr_in addr_server;
	addr_server.sin_family = PF_INET;

	if(stConfig.szCustLogSrvIp[0] != '\0' && stConfig.iCustLogSrvPort != 0)
	{
		addr_server.sin_port = htons(stConfig.iCustLogSrvPort);
		addr_server.sin_addr.s_addr = inet_addr(stConfig.szCustLogSrvIp);
	}
	else
	{
		addr_server.sin_port = htons(stInfo.wLogSrvPort);
		addr_server.sin_addr.s_addr = stInfo.dwAppSrvMaster;
	}

	uint32_t dwKey = 0;
	if(stConfig.iEnableEncryptData) {
		static char sAppLogBufEnc[MAX_APP_LOG_PKG_LENGTH+256];
		int iLogBufLenEnc = ((stAppLog.iReadAppLogBufLen>>4)+1)<<4;
		if(iLogBufLenEnc > (int)sizeof(sAppLogBufEnc)) {
			ERROR_LOG("need more space %d > %d", iLogBufLenEnc, (int)sizeof(sAppLogBufEnc));
			return MTREPORT_ERROR_LINE;
		}
		aes_cipher_data((const uint8_t*)stAppLog.sAppLogBuf, stAppLog.iReadAppLogBufLen,
			(uint8_t*)sAppLogBufEnc, (const uint8_t*)stConfig.pReportShm->sRandKey, AES_128);
		dwKey = MakeAppLogPkg(addr_server, sAppLogBufEnc, iLogBufLenEnc, stInfo.iAppId);
	}
	else
		dwKey = MakeAppLogPkg(addr_server, stAppLog.sAppLogBuf, stAppLog.iReadAppLogBufLen, stInfo.iAppId);

	int32_t iRet = 0;
	stConfig.pPkgSess->iSockIndex = stConfig.pEnvSess->iSockIndex; 
	stInfo.dwTrySendCount++;
	iRet = SendPacket(stConfig.pPkgSess->iSockIndex, &addr_server, stConfig.pPkg, stConfig.iPkgLen);
	if(iRet != stConfig.iPkgLen) {
		ERROR_LOG("SendPacket(app log) failed, pkglen:%d, ret:%d", stConfig.iPkgLen, iRet);
		stInfo.dwTrySendFailedCount++;
		return MTREPORT_ERROR_LINE;
	}
	else {
		stConfig.pPkgSess->dwSendTimeSec = stConfig.stTimeCur.tv_sec;
		stConfig.pPkgSess->dwSendTimeUsec = stConfig.stTimeCur.tv_usec;
		stConfig.pPkgSess->bSessStatus = SESS_FLAG_WAIT_RESPONSE;
		stConfig.pPkgSess->stCmdSessData.applog.bIsUseBackupSrv = 0;
		stConfig.pPkgSess->stCmdSessData.applog.iAppInfoIdx = stAppLog.iAppInfoIdx;
		stConfig.pPkgSess->stCmdSessData.applog.dwAppLogSrvIP = stInfo.dwAppSrvMaster;

		int iTime = GetMaxResponseTime(stConfig.pPkgSess->iSockIndex)+1000;
		iRet = AddTimer(dwKey, iTime, OnPkgExpire,
			stConfig.pPkgSess, MYSIZEOF(PKGSESSION), stConfig.iPkgLen, stConfig.pPkg);
		if(iRet < 0) {
			ERROR_LOG("AddTimer(app log) failed ! pkglen:%d, key:%u, ret:%d", stConfig.iPkgLen, dwKey, iRet);
			stInfo.dwTrySendFailedCount++;
			return MTREPORT_ERROR_LINE;
		}
		DEBUG_LOG("send log to %s:%d, app info -- id:%d packet len:%d", 
			ipv4_addr_str(stInfo.dwAppSrvMaster), stInfo.wLogSrvPort, stInfo.iAppId, stConfig.iPkgLen);
	}
	return 1;
}

int EnvSendPluginInfoToServer(int iSockIdx=-1)
{
    stConfig.pPkgSess = (PKGSESSION*)stConfig.sSessBuf;
    stConfig.pPkg = stConfig.sSessBuf+MYSIZEOF(PKGSESSION);
    stConfig.iPkgLen = MAX_ATTR_PKG_LENGTH;

    struct sockaddr_in addr_server;
    addr_server.sin_family = PF_INET;
    addr_server.sin_port = htons(stConfig.iSrvPort);
    addr_server.sin_addr.s_addr = inet_addr(stConfig.szSrvIp_master);
    uint32_t dwKey = MakeRepPluginInfoToServer();
    if(!dwKey)
        return 0;

    if(iSockIdx < 0)
    	stConfig.pPkgSess->iSockIndex = stConfig.pEnvSess->iSockIndex; 
    else
    	stConfig.pPkgSess->iSockIndex = iSockIdx; 

    int iRet = SendPacket(stConfig.pPkgSess->iSockIndex, &addr_server, stConfig.pPkg, stConfig.iPkgLen);
    if(iRet != stConfig.iPkgLen) {
        ERROR_LOG("SendPacket(report plugin info) failed, pkglen:%d, ret:%d", stConfig.iPkgLen, iRet);
        return MTREPORT_ERROR_LINE;
    }    
    else {
        stConfig.pPkgSess->dwSendTimeSec = stConfig.stTimeCur.tv_sec;
        stConfig.pPkgSess->dwSendTimeUsec = stConfig.stTimeCur.tv_usec;
        stConfig.pPkgSess->bSessStatus = SESS_FLAG_WAIT_RESPONSE;
		stConfig.pPkgSess->stCmdSessData.plugin.bTrySrvCount = 1;
        stConfig.pPkgSess->stCmdSessData.plugin.dwConfigSrvIP = addr_server.sin_addr.s_addr;
        SetSocketAddress(
            stConfig.pPkgSess->iSockIndex, addr_server.sin_addr.s_addr, ntohs(addr_server.sin_port));

        int iTime = GetMaxResponseTime(stConfig.pPkgSess->iSockIndex)+1000;
        iRet = AddTimer(dwKey, iTime, OnPkgExpire,
            stConfig.pPkgSess, MYSIZEOF(PKGSESSION), stConfig.iPkgLen, stConfig.pPkg);
        if(iRet < 0) {
            ERROR_LOG("AddTimer(report plugin info) failed ! pkglen:%d, key:%u, ret:%d", stConfig.iPkgLen, dwKey, iRet);
            return MTREPORT_ERROR_LINE;
        }
    }
    return 1;
}

int OnEventTimer(TimerNode *pNodeShm, unsigned uiDataLen, char *pData)
{
	int iRet = 0, i=0, j=0, iAttrFreq = 0;
	int iSendAttr = 0, iSendStrAttr = 0, iSendPlugin = 0;

	stConfig.pEnvSess = (ENVSESSION*)pNodeShm->sSessData;
	switch(stConfig.pEnvSess->bEventType)
	{
		case EVENT_TYPE_APP_LOG:
			iRet = 1;
			if(!IsHelloValid()) {
				WARN_LOG("event send app log failed -- hello is invalid , wait ... !");
				break;
			}

			for(i=0; i < COUNT_APP_LOG_PACKET_SEND_PER;) 
			{
				stConfig.bReadAppTypeCount = 0;
				ReadAppLog();
				if(stConfig.bReadAppTypeCount <= 0)
					break;

				for(j=0; j < stConfig.bReadAppTypeCount; j++, i++)
					if(EnvSendAppLogToServer(stConfig.stAppLogRead[j]) < 0)
						break;
			}

            if(i >= COUNT_APP_LOG_PACKET_SEND_PER) {
                // 日志比较多，间隔调短一些
                pNodeShm->uiTimeOut = rand()%900+100;
            }
            else if(i >= COUNT_APP_LOG_PACKET_SEND_PER/2) {
                // 日志量一般
                if(stConfig.pReportShm->stSysCfg.bLogSendPerTimeSec != 0)
                    pNodeShm->uiTimeOut = stConfig.pReportShm->stSysCfg.bLogSendPerTimeSec*1000/2;
            }
            else {
                // 日志量不大
                if(stConfig.pReportShm->stSysCfg.bLogSendPerTimeSec != 0)
                    pNodeShm->uiTimeOut = stConfig.pReportShm->stSysCfg.bLogSendPerTimeSec*1000;
            }

			DEBUG_LOG("re add event read log timer, timeout:%d ms", pNodeShm->uiTimeOut);
			break;

		case EVENT_TYPE_REPORT_PLUGIN_INFO:
			iRet = 1;
			if(!IsHelloValid()) {
				WARN_LOG("event send plugin info failed -- hello is invalid , wait ...");
				sleep(5);
				break;
			}
			iSendPlugin = EnvSendPluginInfoToServer();
			pNodeShm->uiTimeOut = (7+(stConfig.qwCurTime%4))*1000;
			DEBUG_LOG("send plugin info, count:%d, re add event plugin info timer, timeout:%d ms",
				iSendPlugin, pNodeShm->uiTimeOut);
			break;

		case EVENT_TYPE_M_ATTR:
			iRet = 1;
			if(!IsHelloValid()) {
				WARN_LOG("event send attr failed -- hello is invalid , wait ...");
				break;
			}

			for(i=0; i < COUNT_ATTR_PACKET_SEND_PER; i++,iAttrFreq++) {
				stConfig.wReadAttrCount = 0;
				ReadAttr();
				if(stConfig.wReadAttrCount <= 0)
					break;

				iSendAttr += stConfig.wReadAttrCount;
				if(EnvSendAttrToServer() < 0)
					break;
			}

			for(i=0; i < COUNT_ATTR_PACKET_SEND_PER; i++,iAttrFreq++) {
				stConfig.wReadStrAttrCount = 0;
				ReadStrAttr();
				if(stConfig.wReadStrAttrCount <= 0)
					break;

				iSendStrAttr += stConfig.wReadStrAttrCount;
				if(EnvSendStrAttrToServer() < 0)
					break;
			}

            if(iAttrFreq >= 2*COUNT_ATTR_PACKET_SEND_PER) {
                // 监控点数据比较多，尽快发送
                pNodeShm->uiTimeOut = rand()%900+100;
            }
            else if(iAttrFreq >= COUNT_ATTR_PACKET_SEND_PER) {
                // 监控点数据一般
    			if(stConfig.pReportShm->stSysCfg.bAttrSendPerTimeSec != 0)
    				pNodeShm->uiTimeOut = stConfig.pReportShm->stSysCfg.bAttrSendPerTimeSec*1000/2;
            }
            else {
			    if(stConfig.pReportShm->stSysCfg.bAttrSendPerTimeSec != 0)
			    	pNodeShm->uiTimeOut = stConfig.pReportShm->stSysCfg.bAttrSendPerTimeSec*1000;
            }
			DEBUG_LOG("send attr:%d, str attr:%d, re add event read attr timer, timeout:%d ms", 
				iSendAttr, iSendStrAttr, pNodeShm->uiTimeOut);
			break;

		default:
			WARN_LOG("unknow event type:%d", stConfig.pEnvSess->bEventType);
			return -1;
	}

	if(iRet == 1) {
		stConfig.pEnvSess->iExpireTimeMs = pNodeShm->uiTimeOut;
		iRet = UpdateTimer(pNodeShm, uiDataLen, pData);
		if(iRet != 0) {
			FATAL_LOG("Update event timer failed data length:%d ret:%d", uiDataLen, iRet);
			stConfig.pReportShm->cIsAgentRun = 2;
			return 0;
		}
		return 1;
	}
	return iRet;
}

int InitReportPluginInfo()
{
    stConfig.pEnvSess = (ENVSESSION*)stConfig.sSessBuf;
    stConfig.pEnvSess->iSockIndex = stConfig.iConfigSocketIndex; 
    stConfig.pEnvSess->dwEventSetTimeSec = stConfig.stTimeCur.tv_sec;
    stConfig.pEnvSess->dwEventSetTimeUsec = stConfig.stTimeCur.tv_usec;
    stConfig.pEnvSess->bEventType = EVENT_TYPE_REPORT_PLUGIN_INFO;

    uint32_t dwKey = rand();
    stConfig.pEnvSess->iExpireTimeMs = 15*1000;
    int iRet = AddTimer(dwKey, stConfig.pEnvSess->iExpireTimeMs,
        OnEventTimer, stConfig.pEnvSess, MYSIZEOF(ENVSESSION), 0, NULL);
    if(iRet < 0) { 
        ERROR_LOG("AddTimer for report plugin info failed, key:%u, ret:%d", dwKey, iRet);
        return MTREPORT_ERROR_LINE;
    }    
    DEBUG_LOG("add report plugin info timer key:%u", dwKey);
    return 0;
}

int InitReportLog()
{
	int iRet = 0;
	int iSock = socket(AF_INET, SOCK_DGRAM, 0);
	if(iSock < 0) {
		ERROR_LOG("create socket failed, msg:%s", strerror(errno));
		return MTREPORT_ERROR_LINE;
	}
	InitSocketComm(iSock);

	iRet = AddSocket(iSock, NULL, OnMtreportPkg, NULL);
	if(iRet < 0) {
		ERROR_LOG("AddSocket failed, ret:%d", iRet); 
		return MTREPORT_ERROR_LINE;
	}
	DEBUG_LOG("add applog socket index:%d sock fd:%d", iRet, iSock);
	stConfig.iLogSocketIndex = iRet;

	stConfig.pEnvSess = (ENVSESSION*)stConfig.sSessBuf;
	stConfig.pEnvSess->iSockIndex = iRet;
	stConfig.pEnvSess->dwEventSetTimeSec = stConfig.stTimeCur.tv_sec;
	stConfig.pEnvSess->dwEventSetTimeUsec = stConfig.stTimeCur.tv_usec;
	stConfig.pEnvSess->bEventType = EVENT_TYPE_APP_LOG;

	uint32_t dwKey = rand();

	stConfig.pEnvSess->iExpireTimeMs = 10*1000;
	iRet = AddTimer(dwKey, stConfig.pEnvSess->iExpireTimeMs,
		OnEventTimer, stConfig.pEnvSess, MYSIZEOF(ENVSESSION), 0, NULL);
	if(iRet < 0) {
		ERROR_LOG("AddTimer for applog failed, key:%u, ret:%d", dwKey, iRet);
		return MTREPORT_ERROR_LINE;
	}
	DEBUG_LOG("add app log timer key:%u", dwKey);
	return 0;
}

void ShowMtSystemConfig(MtSystemConfig & str)
{
	MtSystemConfig *pstr = &str;
	SHOW_FIELD_VALUE_UINT(wHelloRetryTimes);
	SHOW_FIELD_VALUE_UINT(wHelloPerTimeSec);
	SHOW_FIELD_VALUE_UINT(wCheckLogPerTimeSec);
	SHOW_FIELD_VALUE_UINT(wCheckAppPerTimeSec);
	SHOW_FIELD_VALUE_UINT(wCheckServerPerTimeSec);
	SHOW_FIELD_VALUE_UINT(wCheckSysPerTimeSec);
	SHOW_FIELD_VALUE_UINT(dwConfigSeq);
	SHOW_FIELD_VALUE_UINT(bAttrSendPerTimeSec);
	SHOW_FIELD_VALUE_UINT(bLogSendPerTimeSec);
	printf("\n");
}

void ShowAppInfo(AppInfo &str)
{
	AppInfo *pstr = &str;
	SHOW_FIELD_VALUE_INT(iAppId);
	SHOW_FIELD_VALUE_UINT_IP(dwAppSrvMaster);
	SHOW_FIELD_VALUE_UINT(wLogSrvPort);
	SHOW_FIELD_VALUE_UINT(bAppType);
	SHOW_FIELD_VALUE_UINT(wModuleCount);
	SHOW_FIELD_VALUE_UINT(dwSeq);
	SHOW_FIELD_VALUE_UINT_TIME(dwLastTrySendLogTime);
	SHOW_FIELD_VALUE_UINT(dwTrySendCount);
	SHOW_FIELD_VALUE_UINT(dwSendOkCount);
	SHOW_FIELD_VALUE_UINT(dwTrySendFailedCount);
	printf("\n");
}

void ShowSLogTestKey(SLogTestKey &str)
{
	SLogTestKey *pstr = &str;
	SHOW_FIELD_VALUE_UINT(bKeyType);
	SHOW_FIELD_VALUE_CSTR(szKeyValue);
	printf("\n");
}

void ShowSLogConfig(SLogConfig &str)
{
	SLogConfig *pstr = &str;
	SHOW_FIELD_VALUE_UINT(dwSeq);
	SHOW_FIELD_VALUE_UINT(dwCfgId);
	SHOW_FIELD_VALUE_INT(iAppId);
	SHOW_FIELD_VALUE_INT(iModuleId);
	SHOW_FIELD_VALUE_INT(iLogType);
	SHOW_FIELD_VALUE_UINT(wTestKeyCount);
	SHOW_FIELD_VALUE_FUN_COUNT(wTestKeyCount, stTestKeys, ShowSLogTestKey);
	SHOW_FIELD_VALUE_UINT(dwSpeedFreq);
	SHOW_FIELD_VALUE_UINT(bLogFreqUseFlag);
	SHOW_FIELD_VALUE_INT(iWriteLogCount);
	SHOW_FIELD_VALUE_UINT64(qwLastFreqTime);
	printf("\n");
}

void ShowPlugin()
{
	printf("\n\n--------------  plugin info as followed : ------------------\n");
    {
	    MTREPORT_SHM *pstr = g_mtReport.pMtShm;
        SHOW_FIELD_VALUE_UINT(bAddPluginInfoFlag);
        SHOW_FIELD_VALUE_UINT(iPluginInfoCount);
        SHOW_FIELD_VALUE_UINT(bHasNewPluginInfo);
    }
    {
        TInnerPlusInfo *pstr = NULL;
        for(int i=0; i < MAX_INNER_PLUS_COUNT; i++) {
            if(g_mtReport.pMtShm->stPluginInfo[i].iPluginId == 0)
                continue;
            printf("plugin info:%d\n", i);
            pstr = g_mtReport.pMtShm->stPluginInfo+i;
            SHOW_FIELD_VALUE_CSTR(szPlusName);
            SHOW_FIELD_VALUE_INT(iPluginId);
            SHOW_FIELD_VALUE_CSTR(szVersion);
            SHOW_FIELD_VALUE_INT(iLibVerNum);
            SHOW_FIELD_VALUE_INT(iLogConfigId);
            SHOW_FIELD_VALUE_UINT_TIME(dwLastReportAttrTime);
            SHOW_FIELD_VALUE_UINT_TIME(dwLastReportLogTime);
            SHOW_FIELD_VALUE_UINT_TIME(dwLastHelloTime);
            SHOW_FIELD_VALUE_UINT_TIME(dwPluginStartTime);
            SHOW_FIELD_VALUE_UINT_TIME(dwLastReportSelfInfoTime);
            SHOW_FIELD_VALUE_UINT_TIME(dwRep_LastReportLogTime);
            SHOW_FIELD_VALUE_UINT_TIME(dwRep_LastReportAttrTime);
            SHOW_FIELD_VALUE_UINT_TIME(dwRep_LastHelloTime);
            SHOW_FIELD_VALUE_UINT(bCheckRet);
            SHOW_FIELD_VALUE_UINT(dwCfgFileLastModTime);
        }
    }
}

void ShowShm()
{
	if(stConfig.pReportShm == NULL) {
		printf("config shm is NULL !\n");
		return ;
	}

	MTREPORT_SHM *pstr= stConfig.pReportShm;
	printf("\n\n--------------  config shm info as followed : ------------------\n");
	SHOW_FIELD_VALUE_UINT(bFirstHelloCheckOk);
	SHOW_FIELD_VALUE_INT(iMtClientIndex);
	SHOW_FIELD_VALUE_INT(iMachineId);
	SHOW_FIELD_VALUE_UINT_IP(dwConnServerIp);
	SHOW_FIELD_VALUE_MASK_CSTR(16, sRandKey);
	SHOW_FIELD_VALUE_UINT(dwPkgSeq);
	SHOW_FIELD_VALUE_UINT_TIME(dwLastHelloOkTime);
	SHOW_FIELD_VALUE_UINT_TIME(dwClientProcessStartTime);
	SHOW_FIELD_VALUE_INT(iAttrSrvMtClientIndex);
	SHOW_FIELD_VALUE_INT(iAppLogSrvMtClientIndex);
	SHOW_FIELD_VALUE_UINT_IP(dwConnCfgServerIp);
	SHOW_FIELD_VALUE_UINT(wConnCfgServerPort);
	SHOW_FIELD_VALUE_UINT(dwBindDES-PWESetTime);
	SHOW_FIELD_VALUE_INT(iBindCloudUserId);
    if(pstr->iBindCloudUserId != 0)
        SHOW_FIELD_VALUE_CSTR(szBindCloudKey);

	// systen config
	printf("\n");
	SHOW_FIELD_VALUE_FUN(stSysCfg, ShowMtSystemConfig);

	// app config
	SHOW_FIELD_VALUE_UINT_TIME(dwLastSyncAppConfigTime);
	SHOW_FIELD_VALUE_UINT(wAppConfigCount);
	SHOW_FIELD_VALUE_FUN_COUNT(wAppConfigCount, stAppConfigList, ShowAppInfo);

	// app log config
	printf("\n");
	SHOW_FIELD_VALUE_UINT_TIME(dwLastSyncLogConfigTime);
	SHOW_FIELD_VALUE_UINT(wLogConfigCount);
	SHOW_FIELD_VALUE_FUN_COUNT(wLogConfigCount, stLogConfig, ShowSLogConfig);
	if(stConfig.szCustLogSrvIp[0] != '\0' && stConfig.iCustLogSrvPort != 0)
		printf("\tcust log server:%s:%d\n", stConfig.szCustLogSrvIp, stConfig.iCustLogSrvPort);

	SHOW_FIELD_VALUE_UINT(bModifyServerFlag);

	// attr server config 
	printf("\n");
	SHOW_FIELD_VALUE_UINT(wAttrServerPort);
	SHOW_FIELD_VALUE_UINT_IP(dwAttrSrvIp);
	if(stConfig.szCustAttrSrvIp[0] != '\0' && stConfig.iCustAttrSrvPort != 0)
		printf("\tcust attrserver:%s:%d\n", stConfig.szCustAttrSrvIp, stConfig.iCustAttrSrvPort);

	printf("\n-----log report info ---- \n");
	SHOW_FIELD_VALUE_UINT(dwLogSeq);
	SHOW_FIELD_VALUE_INT(iLogSpecialWriteIdx);
	SHOW_FIELD_VALUE_INT(iLogSpecialReadIdx);
	SHOW_FIELD_VALUE_UINT(wSpecLogFailed);
	SHOW_FIELD_VALUE_UINT(wLogVmemFailed);
	SHOW_FIELD_VALUE_UINT(wLogFreqLimited);
	SHOW_FIELD_VALUE_UINT(wLogTruncate);
	SHOW_FIELD_VALUE_UINT(wLogInShmFull);
	SHOW_FIELD_VALUE_UINT(wLogWriteInVmem);
	SHOW_FIELD_VALUE_UINT(wLogCustInVmem);
	SHOW_FIELD_VALUE_UINT(wLogCustVmemFailed);
	SHOW_FIELD_VALUE_UINT(dwLogBySpecCount);
	SHOW_FIELD_VALUE_UINT(dwLogTypeLimited);
	SHOW_FIELD_VALUE_UINT_TIME(dwFirstLogWriteTime);
	SHOW_FIELD_VALUE_UINT_TIME(dwLastReportLogOkTime);
	SHOW_FIELD_VALUE_UINT(dwWriteLogCount);
	SHOW_FIELD_VALUE_UINT64(qwWriteLogBytes);

	printf("\n-----attr report info ---- \n");
	SHOW_FIELD_VALUE_INT(cIsAttrInit);
	SHOW_FIELD_VALUE_INT(iAttrSpecialCount);
	SHOW_FIELD_VALUE_UINT(wAttrReportFailed);
	SHOW_FIELD_VALUE_UINT(dwAttrReportBySpecCount);
	SHOW_FIELD_VALUE_UINT_TIME(dwFirstAttrReportTime);
	SHOW_FIELD_VALUE_UINT_TIME(dwLastReportAttrOkTime);
	SHOW_FIELD_VALUE_UINT(dwAttrReportCount);

	printf("\n-----vmem info ---- \n");
	SHOW_FIELD_VALUE_INT(cIsAgentRun);
	SHOW_FIELD_VALUE_INT(cIsVmemInit);
	SHOW_FIELD_VALUE_INT(iBindCloudUserId);

    // plugin info
    ShowPlugin();
}

void ShowMTLogCust(MTLogCust & str)
{
	MTLogCust *pstr = &str;
	SHOW_FIELD_VALUE_UINT(dwCust_1);
	SHOW_FIELD_VALUE_UINT(dwCust_2);
	SHOW_FIELD_VALUE_INT(iCust_3);
	SHOW_FIELD_VALUE_INT(iCust_4);
	SHOW_FIELD_VALUE_BIN(8, szCust_5);
	SHOW_FIELD_VALUE_BIN(16, szCust_6);
	SHOW_FIELD_VALUE_UINT(bCustFlag);
}

void ShowSharedHashTable(SharedHashTable &str)
{
	SharedHashTable *pstr = &str;
	SHOW_FIELD_VALUE_UINT(dwNodeSize);
	SHOW_FIELD_VALUE_UINT(dwNodeCount);
	SHOW_FIELD_VALUE_UINT(dwRealNodeCount);
	SHOW_FIELD_VALUE_UINT(dwShareMemBytes);
	SHOW_FIELD_VALUE_UINT_COUNT_2(STATIC_HASH_ROW_MAX, dwNodeNums);
	SHOW_FIELD_VALUE_UINT_TIME(dwLastUpdateTime);
	SHOW_FIELD_VALUE_UINT(bInitSuccess);
	SHOW_FIELD_VALUE_UINT(bAccessCheck);
	SHOW_FIELD_VALUE_UINT(dwCurIndex);
	SHOW_FIELD_VALUE_UINT(dwCurIndexRevers);
	SHOW_FIELD_VALUE_POINT(fun_cmp);
	SHOW_FIELD_VALUE_POINT(fun_war);
	SHOW_FIELD_VALUE_POINT(pHash);
}

void ShowGlobal()
{
	printf("\n\n--------------  global g_mtReport info as followed : ------------------\n");
	MtReport *pstr = &g_mtReport;
	SHOW_FIELD_VALUE_INT(cIsInit);
	SHOW_FIELD_VALUE_INT(cIsAttrInit);
	SHOW_FIELD_VALUE_FUN_COUNT_2(MTATTR_SHM_DEF_COUNT, stAttrHash, ShowSharedHashTable);
	SHOW_FIELD_VALUE_INT(cIsVmemInit);
}

void ShowSharedHashTable(SharedHashTable *pstr)
{
	printf("--- SharedHashTable info: ----\n");
	SHOW_FIELD_VALUE_UINT(dwNodeSize);
	SHOW_FIELD_VALUE_UINT(dwNodeCount);
	SHOW_FIELD_VALUE_UINT(dwRealNodeCount);
	SHOW_FIELD_VALUE_UINT(dwShareMemBytes);
	SHOW_FIELD_VALUE_UINT_COUNT_2(STATIC_HASH_ROW_MAX, dwNodeNums);
	SHOW_FIELD_VALUE_UINT_TIME(dwLastUpdateTime);
	SHOW_FIELD_VALUE_INT(bInitSuccess);
	SHOW_FIELD_VALUE_INT(bAccessCheck);
	SHOW_FIELD_VALUE_UINT(dwCurIndex);
	SHOW_FIELD_VALUE_UINT(dwCurIndexRevers);
	printf("\n");
}

void ShowHashTableHead(_HashTableHead *pstr)
{
	printf("--- SharedHashTable head info: ----\n");
	SHOW_FIELD_VALUE_UINT(dwNodeStartIndex);
	SHOW_FIELD_VALUE_UINT(dwNodeEndIndex);
	SHOW_FIELD_VALUE_UINT(dwNodeUseCount);
	SHOW_FIELD_VALUE_UINT(dwNodeSize);
	SHOW_FIELD_VALUE_UINT(dwNodeCount);
	SHOW_FIELD_VALUE_UINT(dwWriteProcessId);
	SHOW_FIELD_VALUE_UINT_TIME(dwLastUseTimeSec);
	SHOW_FIELD_VALUE_UINT(bHashUseFlag);
	printf("\n");
}

void ShowStrAttr(int iShowMax)
{
	printf("\n\n--------------  show first:%d str attr as followed : ------------------\n", iShowMax);
	StrAttrNode *pstr = NULL;
	ShowSharedHashTable(&g_mtReport.stStrAttrHash);
	ShowHashTableHead(MtReport_GetHashTableHead(&g_mtReport.stStrAttrHash));

	if(iShowMax < 0) {
		InitGetAllHashNode();
		for(int i = 0; ((pstr=(StrAttrNode*)GetAllHashNode(&g_mtReport.stStrAttrHash)) != NULL); i++)
		{
			printf("\n\nnode :%d \n", i);
			_HashNodeHead *pHead = NODE_TO_NODE_HEAD(pstr);
			printf("\tbIsUsed:%d\n", pHead->bIsUsed);
			printf("\tdwNodePreIndex:%u\n", pHead->dwNodePreIndex);
			printf("\tdwNodeNextIndex:%u\n\n", pHead->dwNodeNextIndex);

			SHOW_FIELD_VALUE_INT(iStrAttrId);
			SHOW_FIELD_VALUE_INT(iStrVal);
			SHOW_FIELD_VALUE_CSTR(szStrInfo);
			SHOW_FIELD_VALUE_UINT(bSyncProcess);
			printf("\n\n");
		}
		return;
	}
	
	if(iShowMax == 0) {
		pstr = (StrAttrNode*)GetFirstNodeRevers(&g_mtReport.stStrAttrHash);
		for(int j=0; pstr != NULL; j++) {
			printf("\n\nnode :%d \n", j);
			_HashNodeHead *pHead = NODE_TO_NODE_HEAD(pstr);
			printf("\tbIsUsed:%d\n", pHead->bIsUsed);
			printf("\tdwNodePreIndex:%u\n", pHead->dwNodePreIndex);
			printf("\tdwNodeNextIndex:%u\n\n", pHead->dwNodeNextIndex);

			SHOW_FIELD_VALUE_INT(iStrAttrId);
			SHOW_FIELD_VALUE_INT(iStrVal);
			SHOW_FIELD_VALUE_CSTR(szStrInfo);
			SHOW_FIELD_VALUE_UINT(bSyncProcess);
			printf("\n\n");
			pstr = (StrAttrNode*)GetNextNodeByNodeRevers(&g_mtReport.stStrAttrHash, pstr);
		}
		return;
	}

	pstr = (StrAttrNode*)GetFirstNode(&g_mtReport.stStrAttrHash);
	for(int j=0; pstr != NULL && j < iShowMax; j++) {
		SHOW_FIELD_VALUE_INT(iStrAttrId);
		SHOW_FIELD_VALUE_INT(iStrVal);
		SHOW_FIELD_VALUE_CSTR(szStrInfo);
		SHOW_FIELD_VALUE_UINT(bSyncProcess);
		printf("\n");
		pstr = (StrAttrNode*)GetNextNodeByNode(&g_mtReport.stStrAttrHash, pstr);
	}
}

void ShowAttr(int iShowMax)
{
	printf("\n\n--------------  show first:%d attr as followed : ------------------\n", iShowMax);
	AttrNode *pstr = NULL;
	for(int i=0,j=0; i < MTATTR_SHM_DEF_COUNT && j < iShowMax; i++){
		printf("---- in hash table:%d ---- \n", i);
		ShowSharedHashTable(&g_mtReport.stAttrHash[i]);
		ShowHashTableHead(
			MtReport_GetHashTableHead(&g_mtReport.stAttrHash[i]));
		pstr = (AttrNode*)GetFirstNode(&g_mtReport.stAttrHash[i]);
		while(pstr != NULL && j < iShowMax) {
			SHOW_FIELD_VALUE_INT(iAttrID);
			SHOW_FIELD_VALUE_INT(iCurValue);
			j++;
			printf("\n");
			pstr = (AttrNode*)GetNextNodeByNode(&g_mtReport.stAttrHash[i], pstr);
		}
	}
}

void ShowHelp()
{
	printf("\t show shm\n");
	printf("\t show global\n");
	printf("\t show attr [count]\n");
	printf("\t show attrid id\n");
	printf("\t show strattr [count]\n");
	printf("\t show strattrid id str\n");
	printf("\t show plugin \n");
	printf("\t show vmem \n");
}

void ShowConfig()
{
    std::cout << "SERVER_MASTER" << std::endl
        << "SERVER_PORT" << std::endl
        << "LOG_FREQ_LIMIT_PER_SEC" << std::endl
        << "AGENT_ACCESS_KEY" << std::endl
        << "PLUS_PATH" << std::endl
        << "MTREPORT_SHM_KEY" << std::endl
        << "AGENT_CLIENT_IP" << std::endl
        << "CUST_ATTR_SRV_IP" << std::endl
        << "CUST_ATTR_SRV_PORT" << std::endl
        << "CUST_LOG_SRV_IP" << std::endl
        << "CUST_LOG_SRV_PORT" << std::endl
        << "ENABLE_ENCRYPT_DATA" << std::endl
        << "CLIENT_LOCAL_LOG_FILE" << std::endl
        << "CLIENT_LOCAL_LOG_TYPE" << std::endl
        << "CLIENT_LOCAL_LOG_SIZE" << std::endl
        << "MAX_RUN_MINS" << std::endl
        << "DES-PWE_CLOUD_URL" << std::endl
        << "INSTALL_PLUGIN_TIMEOUT_SEC" << std::endl
        << "DES-PWE_LOCAL_DOMAIN" << std::endl
        << "LOCAL_OS" << std::endl
        << "LOCAL_OS_ARC" << std::endl
        << "LOCAL_LIBC_VER" << std::endl
        << "LOCAL_LIBCPP_VER" << std::endl
        << "IP_TTL" << std::endl
        << "PROXY_COUNT" << std::endl
        << "PROXY_LISTEN_PORT_0" << std::endl
        << "PROXY_SERVER_ADDR_0" << std::endl
        << "PROXY_SERVER_PORT_" << std::endl
        << "PROXY_ONLY" << std::endl;
}

void ShowInfo(int argc, char* argv[])
{
	if(argc < 3) { 
		ShowHelp();
		printf("invalid argument !\n");
		return;
	}
	if(!strcmp(argv[2], "help")) {
		ShowHelp();
        return;
	}

	if(!strcmp(argv[2], "vmem")) {
        MtReport_show_shm();
        return;
    }

	if(!strcmp(argv[2], "shm")) {
		ShowShm();
		return;
	}
	if(!strcmp(argv[2], "global")) {
		ShowGlobal();
		return;
	}
	if(!strcmp(argv[2], "attr")) {
		if(argc > 3)
			ShowAttr(atoi(argv[3]));
		else
			ShowAttr(10);
		return;
	}
	if(!strcmp(argv[2], "strattr")) {
		if(argc > 3)
			ShowStrAttr(atoi(argv[3]));
		else
			ShowStrAttr(10);
		return;
	}

	if(!strcmp(argv[2], "config")) {
        ShowConfig();
    }

	if(!strcmp(argv[2], "plugin")) {
        ShowPlugin();
    }
}

void deal_exist_sig(int sig) 
{
	// kill -s 15
	if(sig == SIGTERM) {
		INFO_LOG("get exist signal:%d", sig);
		stConfig.pReportShm->cIsAgentRun = 0;
	}    
}

void CheckAllPlugin()
{
    static uint32_t s_dwLastCheckPluginTime = 0;
    if(s_dwLastCheckPluginTime+60+(stConfig.qwCurTime%20) > stConfig.dwCurTime)
        return;
    s_dwLastCheckPluginTime = stConfig.dwCurTime;

    for(int i=0; i < MAX_INNER_PLUS_COUNT; i++) {
        if(stConfig.pReportShm->stPluginInfo[i].iPluginId != 0) {
            if(stConfig.pReportShm->stPluginInfo[i].dwLastHelloTime+3600 <= stConfig.dwCurTime) {
                if(stConfig.pReportShm->stPluginInfo[i].iLogConfigId != 0) {
                    MtReport_Log_For_CfgId(
                        0, stConfig.pReportShm->stPluginInfo[i].iLogConfigId, MTLOG_TYPE_WARN, stConfig.qwCurTime, 
                        "plugin no heart over 1 hour, remove it from shm, last start time:%s",
                        uitodate(stConfig.pReportShm->stPluginInfo[i].dwPluginStartTime));
                }
                else {
                    WARN_LOG(
                        "plugin(%d, %s) no heart over 1 hour, remove it from shm, last start time:%s",
                        stConfig.pReportShm->stPluginInfo[i].iPluginId, stConfig.pReportShm->stPluginInfo[i].szPlusName,
                        uitodate(stConfig.pReportShm->stPluginInfo[i].dwPluginStartTime));
                }
                stConfig.pReportShm->stPluginInfo[i].iPluginId = 0;
                stConfig.pReportShm->iPluginInfoCount--;
            }
            else if(stConfig.pReportShm->stPluginInfo[i].dwLastHelloTime+300 <= stConfig.dwCurTime) {
                std::ostringstream sspath;
                if(stConfig.szPlusPath[0] != '/') 
                    sspath << stConfig.szCurPath << "/" << stConfig.szPlusPath << "/";
                else 
                    sspath << stConfig.szPlusPath << "/";
                sspath << stConfig.pReportShm->stPluginInfo[i].szPlusName;
                std::string strAbsFile(sspath.str() + "/zombie_restart.sh");
                if(!IsFileExist(strAbsFile.c_str())) {
                    // 无僵死重启脚本
                    if(stConfig.pReportShm->stPluginInfo[i].iLogConfigId != 0) {
                        MtReport_Log_For_CfgId(
                            0, stConfig.pReportShm->stPluginInfo[i].iLogConfigId, MTLOG_TYPE_WARN, stConfig.qwCurTime, 
                            "plugin no heart over 5 minutes, last hello time:%s",
                            uitodate(stConfig.pReportShm->stPluginInfo[i].dwLastHelloTime));
                    }
                    else {
                        WARN_LOG(
                            "plugin(%d, %s) no heart over 5 minutes, last hello time:%s",
                            stConfig.pReportShm->stPluginInfo[i].iPluginId, stConfig.pReportShm->stPluginInfo[i].szPlusName,
                            uitodate(stConfig.pReportShm->stPluginInfo[i].dwLastHelloTime));
                    }
                }
                else {
                    // 调用僵死重启脚本
                    std::ostringstream oscmd;
                    oscmd << "cd " << sspath.str() << "; ./zombie_restart.sh > /dev/null 2>&1";
                    system(oscmd.str().c_str());
                    if(stConfig.pReportShm->stPluginInfo[i].iLogConfigId != 0) {
                        MtReport_Log_For_CfgId(
                            0, stConfig.pReportShm->stPluginInfo[i].iLogConfigId, MTLOG_TYPE_WARN, stConfig.qwCurTime, 
                            "plugin no heart over 5 minutes, last hello time:%s, try zombie_restart.sh",
                            uitodate(stConfig.pReportShm->stPluginInfo[i].dwLastHelloTime));
                    }
                    else {
                        WARN_LOG(
                            "plugin(%d, %s) no heart over 5 minutes, last hello time:%s, try zombie_restart.sh",
                            stConfig.pReportShm->stPluginInfo[i].iPluginId, stConfig.pReportShm->stPluginInfo[i].szPlusName,
                            uitodate(stConfig.pReportShm->stPluginInfo[i].dwLastHelloTime));
                    }
                }
            }
        }
    }
}

int MonitorBusiProcess(int subPid, int argc)
{
	INFO_LOG("start monitor process:%d, self:%d", subPid, getpid());
	char sCmd[128] = {0};
	int iKillSubPidCount = 0;

	while(iKillSubPidCount < 10) {
		usleep(10000);
        SetCurTime();

        CheckAllPlugin();

		int iStatus = 0;
		int iPid = waitpid(-1, &iStatus, WNOHANG);
		if (iPid > 0)
		{
			if (WIFEXITED(iStatus) != 0)
				INFO_LOG("PID: [%d] %d exited normally!", subPid, iPid);
			else if (WIFSIGNALED(iStatus) != 0)
				INFO_LOG("PID: [%d] %d exited bacause of signal ID: [%d] has been catched!", subPid, iPid, WTERMSIG(iStatus));
			else if (WIFSTOPPED(iStatus) != 0)
				INFO_LOG("PID: [%d] %d exited because of stoped! ID: [%d]", subPid, iPid, WSTOPSIG(iStatus));
			else
				INFO_LOG("PID: [%d] %d exited abnormally!", subPid, iPid);
			break;
		}

		if(iKillSubPidCount <= 0 && stConfig.bCheckHelloStart && !IsHelloValid())
		{
			INFO_LOG("PID: [%d] %d check hello failed, dwLastHelloOkTime:%s(%u),  dwFirstLogWriteTime:%u, dwFirstAttrReportTime:%u",
				subPid, iPid, uitodate(stConfig.pReportShm->dwLastHelloOkTime), stConfig.pReportShm->dwLastHelloOkTime, 
				stConfig.pReportShm->dwFirstLogWriteTime, stConfig.pReportShm->dwFirstAttrReportTime);
			iKillSubPidCount = 1;
			snprintf(sCmd, sizeof(sCmd), "kill -s 15 %d", subPid);
			system(sCmd);
		}
		else if(iKillSubPidCount > 0 && iKillSubPidCount < 5) {
			iKillSubPidCount++;
			snprintf(sCmd, sizeof(sCmd), "kill -s 15 %d", subPid);
			system(sCmd);
		}
		else if(iKillSubPidCount >= 5 && iKillSubPidCount < 10) {
			iKillSubPidCount++;
			snprintf(sCmd, sizeof(sCmd), "kill -9 %d", subPid);
			system(sCmd);
		}
	}

	INFO_LOG("PID: [%d] %d exit, kill count:%d, argc:%d, cIsAgentRun:%d", 
		subPid, getpid(), iKillSubPidCount, argc, stConfig.pReportShm->cIsAgentRun);
	if(argc <= 1 && stConfig.pReportShm->cIsAgentRun != 0)
		return 1;
	return 0;
}

int main(int argc, char* argv[])
{
	int iRet = 0;

	// 保存下当前工作目录
	if(getcwd(stConfig.szCurPath, sizeof(stConfig.szCurPath)) == NULL){
		printf("get current path failed, msg:%s\n", strerror(errno));
		return -1;
	}

	SetCurTime();
	if((iRet=Init()) < 0) {
		ERROR_LOG("Init failed, ret:%d", iRet);
		return MTREPORT_ERROR_LINE;
	}
	stConfig.iAttrSocketIndex = -1;
	stConfig.iConfigSocketIndex = -1;
	stConfig.iLogSocketIndex = -1;
	if(argc > 1 && strstr(argv[1], "show"))
	{
		ShowInfo(argc, argv);
		return 0;
	}

	daemon(1, 0);
	do {
		int iPid = fork();
		signal(SIGCHLD, SIG_DFL);
		if(iPid > 0) {
			sleep(3);
			if(MonitorBusiProcess(iPid, argc) > 0)
				continue;
			return 0;
		}
		else if(iPid < 0) {
			ERROR_LOG("fork failed, msg:%s", strerror(errno));
			return -1;
		}
		else {
			DEBUG_LOG("sub process:%d start", getpid());
			break;
		}
	}while(true);

	stConfig.dwRestartFlag = 0;
	signal(SIGTERM, deal_exist_sig);

	stConfig.pReportShm->dwLastHelloOkTime = 0;
	stConfig.pReportShm->bFirstHelloCheckOk = 0;

	assert(TIMER_DATA_MAX_LENGTH > PKG_BUFF_LENGTH);
	SetCurTime();

#ifndef MT_DEBUG 
	if(stConfig.pReportShm->dwClientProcessStartTime != 0
		&& stConfig.pReportShm->dwClientProcessStartTime+5 > stConfig.dwCurTime) {
		int iSleep = stConfig.pReportShm->dwClientProcessStartTime+5-stConfig.dwCurTime;
		INFO_LOG("process restart freq, wait : %d s", iSleep);
		sleep(iSleep);
		stConfig.pReportShm->dwClientProcessStartTime = stConfig.dwCurTime+iSleep;
	}
#endif
	SetCurTime();
	stConfig.pReportShm->dwClientProcessStartTime = stConfig.dwCurTime;

	if((iRet=InitSocket(OnSelectTimeout, OnSocketError)) < 0) {
		ERROR_LOG("InitSocket failed, ret:%d", iRet);
		return MTREPORT_ERROR_LINE;
	}

	iRet = InitTimer(MYSIZEOF(TimerNode), TIMER_HASH_NODE_COUNT, TimerHashCmpFun, TimerHashLevelWarn);
	if(iRet < 0) {
		ERROR_LOG("InitTimer failed, ret:%d", iRet);
		stConfig.pReportShm->cIsAgentRun = 0;
		return MTREPORT_ERROR_LINE;
	}
	stConfig.dwProcessId = getpid();
	stConfig.bCheckHelloStart = false;

    if(stConfig.iProxyCount > 0 && InitProxy() < 0) {
        FATAL_LOG("init proxy failed !");
        return MTREPORT_ERROR_LINE;
    }

    if(!stConfig.iProxyOnly) {
        if((iRet=InitHello()) < 0) {
            FATAL_LOG("init hello failed , ret:%d", iRet);
            return MTREPORT_ERROR_LINE;
        }
    }
    INFO_LOG("mtreport_client start ok");

    for(stConfig.pReportShm->cIsAgentRun = 1; stConfig.pReportShm->cIsAgentRun == 1; )
	{
		SetCurTime();
		iRet = CheckTimer();
		CheckSocket(stConfig.dwCurTime, iRet);
        MtReport_ScanNode(1000);
		TryReOpenLocalLogFile();
		if(stConfig.bCheckHelloStart && !IsHelloValid())
		{
			ERROR_LOG("%s", "hello invalid , process will restart ");
			stConfig.pReportShm->cIsAgentRun = 2;
			break;
		}

		if(stConfig.iMaxRunMins*60+stConfig.pReportShm->dwClientProcessStartTime <= stConfig.dwCurTime)
		{
			INFO_LOG("run over time limits:%d mins, try restart sub process", stConfig.iMaxRunMins);
			stConfig.pReportShm->cIsAgentRun = 2;
			break;
		}

        if(stConfig.pReportShm->bHasNewPluginInfo) {
            EnvSendPluginInfoToServer(stConfig.iConfigSocketIndex);
            stConfig.pReportShm->bHasNewPluginInfo = 0;
        }
	}

	iRet = MtReport_CheckVmem();

	INFO_LOG("get busy free vmem count:%d, max data lenght to vmem:%u", 
		iRet, stConfig.uiMaxDataLenToVmem);
	return 0;
}

