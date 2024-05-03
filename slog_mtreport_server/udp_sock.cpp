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

   模块 slog_mtreport_server 功能:
        管理 agent slog_mtreport_client 的接入下发迁移云管系统配置

****/

#include <supper_log.h>
#include <basic_packet.h>
#include <errno.h>
#include <sv_struct.h>
#include <mt_report.h>
#include <aes.h>
#include <md5.h>
#include <supper_log.h>
#include <sv_struct.h>
#include "mtreport_server.h"
#include "udp_sock.h"
#include "comm.pb.h"


#define LOG_CHECK_COPY_CONFIG_INFO(pCfgInfo) \
	stLogConfig.dwSeq = htonl(pCfgInfo->dwConfigSeq); \
	stLogConfig.dwCfgId = htonl(pCfgInfo->dwCfgId); \
	stLogConfig.iAppId = htonl(pCfgInfo->iAppId); \
	stLogConfig.iModuleId = htonl(pCfgInfo->iModuleId); \
	stLogConfig.iLogType = htonl(pCfgInfo->iLogType); \
	stLogConfig.dwSpeedFreq = htonl(pCfgInfo->dwSpeedFreq); \
	stLogConfig.wTestKeyCount = htons(pCfgInfo->wTestKeyCount); \
    if(pCfgInfo->wTestKeyCount > 0) \
        memcpy(stLogConfig.stTestKeys, pCfgInfo->stTestKeys, MYSIZEOF(SLogTestKey)*pCfgInfo->wTestKeyCount); 

// 阿里云收不到本机发往本机的外网IP数据包问题fix  @ 2021-05-07
// agent 和日志服务器同机部署时，日志服务器地址改为回环地址: 127.0.0.1
#define APPINFO_CHECK_COPY_INFO \
	stAppInfo.dwSeq = htonl(pAppInfo->dwSeq); \
	stAppInfo.iAppId = htonl(pAppInfo->iAppId); \
	stAppInfo.wModuleCount = htons(pAppInfo->wModuleCount); \
	stAppInfo.wLogSrvPort = htons(pAppInfo->wLogSrvPort); \
    if(m_pstMachInfo && slog.IsIpMatchMachine(m_pstMachInfo, pAppInfo->dwAppSrvMaster)) \
	    stAppInfo.dwAppSrvMaster = htonl(g_dwLoopIP);  \
    else \
	    stAppInfo.dwAppSrvMaster = htonl(pAppInfo->dwAppSrvMaster); 

void CUdpSock::Init() 
{
	slog.ClearAllCust();
	memset(m_sDecryptBuf, 0, MYSIZEOF(m_sDecryptBuf));
	m_pMtClient = NULL;
	m_pConfig = stConfig.pShmConfig;
	m_pAppInfo = slog.GetAppInfo();
	m_iUserMasterIndex = -1;
	m_iMtClientIndex = -1;
	m_dwAgentClientIp = 0;
	m_bIsFirstHello = false;
	m_pstMachInfo = NULL;
}

int CUdpSock::InitSignature(TSignature *psig, void *pdata, const char *pKey, int bSigType)
{
	return 0;
}

CUdpSock::CUdpSock(ISocketHandler&h): UdpSocket(h), CBasicPacket()
{
	Attach(CreateSocket(PF_INET, SOCK_DGRAM, "udp"));
    SetRecvTTL();
	Init();

	// g++ -E -P xx > xx -- 宏扩展输出
	DEBUG_LOG("on CUdpSock construct");
}

CUdpSock::~CUdpSock()
{
	DEBUG_LOG("on CUdpSock destruct");
}

int32_t CUdpSock::SendResponsePacket(const char*pkg, int len)
{
	SendToBuf(m_addrRemote, pkg, len, 0);
	DEBUG_LOG("send response packet to client pkglen:%d, addr:%s", len, m_addrRemote.Convert(true).c_str());
	return 0;
}

int32_t CUdpSock::CheckSignature()
{
	MonitorCommSig *pcommSig = NULL;
	size_t iDecSigLen = 0;

	switch(m_pstSig->bSigType) {
		case MT_SIGNATURE_TYPE_HELLO_FIRST:
			aes_decipher_data((const uint8_t*)m_pstSig->sSigValue, ntohs(m_pstSig->wSigLen),
			    (uint8_t*)m_sDecryptBuf, &iDecSigLen, (const uint8_t*)stConfig.psysConfig->szAgentAccessKey, AES_256);
			if(iDecSigLen != sizeof(MonitorHelloSig)) {
				REQERR_LOG("decrypt failed, %lu != %lu, siglen:%d, key:%s",
					iDecSigLen, sizeof(MonitorHelloSig), ntohs(m_pstSig->wSigLen), 
					stConfig.psysConfig->szAgentAccessKey);
				return ERR_DECRYPT_FAILED;
			}
			break; 

		case MT_SIGNATURE_TYPE_COMMON:
			if(!m_pMtClient->bEnableEncryptData)
				break;
			aes_decipher_data((const uint8_t*)m_pstSig->sSigValue, ntohs(m_pstSig->wSigLen),
			    (uint8_t*)m_sDecryptBuf, &iDecSigLen, (const uint8_t*)m_pMtClient->sRandKey, AES_128);
			if(iDecSigLen != sizeof(MonitorCommSig)) {
				REQERR_LOG("decrypt failed, %lu != %lu, siglen:%d, key:%s",
					iDecSigLen, sizeof(MonitorCommSig), ntohs(m_pstSig->wSigLen), 
					m_pMtClient->sRandKey);
				return ERR_DECRYPT_FAILED;
			}
			pcommSig = (MonitorCommSig*)m_sDecryptBuf;
			if(ntohl(pcommSig->dwSeq) != m_dwReqSeq) {
				REQERR_LOG("invalid signature info - seq(%u,%u)", ntohl(pcommSig->dwSeq), m_dwReqSeq);
				return ERR_INVALID_SIGNATURE_INFO;
			}
			break; 

		default:
			break;
	}
	return 0;
}

int CUdpSock::SaveMachineInfoToDb(MonitorHelloFirstContent *pctinfo)
{
	IM_SQL_PARA* ppara = NULL;
	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return SLOG_ERROR_LINE;
	}

    AddParameter(&ppara, "last_hello_time", stConfig.dwCurrentTime, "DB_CAL");
    AddParameter(&ppara, "start_time", stConfig.dwCurrentTime, "DB_CAL");
    AddParameter(&ppara, "cmp_time", pctinfo->sCmpTime, NULL);
    AddParameter(&ppara, "agent_version", pctinfo->sVersion, NULL);
    if(pctinfo->sOsInfo[0] != '\0')
        AddParameter(&ppara, "agent_os", pctinfo->sOsInfo, NULL);
    if(pctinfo->sLibcVer[0] != '\0')
        AddParameter(&ppara, "libc_ver", pctinfo->sLibcVer, NULL);
    if(pctinfo->sLibcppVer[0] != '\0')
        AddParameter(&ppara, "libcpp_ver", pctinfo->sLibcppVer, NULL);
    if(pctinfo->sOsArc[0] != '\0')
        AddParameter(&ppara, "os_arc", pctinfo->sOsArc, NULL);

	if(m_pstMachInfo != NULL) {
		m_pstMachInfo->dwAgentStartTime = stConfig.dwCurrentTime;
		m_pstMachInfo->dwLastHelloTime = stConfig.dwCurrentTime;
	}

	MyQuery myqu(stConfig.qu, stConfig.db);
	Query & qu = myqu.GetQuery();
	std::string strSql;
	strSql = "update mt_machine set";
	JoinParameter_Set(&strSql, qu.GetMysql(), ppara);
	strSql += " where xrk_id=";
	strSql += itoa(m_iRemoteMachineId);
	ReleaseParameter(&ppara);
	if(!qu.execute(strSql)) {
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("set machine key for machine id:%d ok", m_iRemoteMachineId);
	return 0;
}

void CUdpSock::InitMtClientInfo()
{
	if(m_pMtClient == NULL) {
		ERR_LOG("bug -- m_pMtClient(%p)", m_pMtClient);
		return;
	}

	m_pMtClient->dwLastHelloTime = stConfig.dwCurrentTime;
	m_pMtClient->dwFirstHelloTime = stConfig.dwCurrentTime;
	m_pMtClient->dwAddress = m_addrRemote.GetAddr();
	m_pMtClient->wBasePort = m_addrRemote.GetPort();
}

void CUdpSock::SendRealInfo()
{
	/*
	::comm::SysconfigInfo stInfo;
	std::string strContent;

	CBasicPacket pkg;

	// ReqPkgHead 
	ReqPkgHead stHead;
	pkg.InitReqPkgHead(&stHead, CMD_INNER_SEND_REALINFO, slog.m_iRand);
	*(int32_t*)(stHead.sReserved) = htonl(slog.GetLocalMachineId());

	// TSignature - empty
	// [cmd content]
	pkg.InitCmdContent((void*)strContent.c_str(), strContent.length());
	// TPkgBody - empty

	if(pkg.MakeReqPkg() > 0) {
		Ipv4Address addr;
		addr.SetAddress(stConfig.pCenterServer->dwIp, stConfig.pCenterServer->wPort);
		SendToBuf(addr, pkg, iPkgLen, 0);
		DEBUG_LOG("SendLogToServer ok, server:%s:%d", psrv->szIpV4, psrv->wPort);
	}
	*/
}

int CUdpSock::GetMtClientInfo(MonitorHelloFirstContent *pctinfo)
{
	uint32_t isFind = 0;

	// 非 first hello 一定存在相关索引
	if(!m_bIsFirstHello && (m_iRemoteMachineId <= 0 || m_iMtClientIndex < 0)) {
		REQERR_LOG("invalid packet - not first hello packet have no some cache !");
		return ERR_INVALID_PACKET;
	}

	if((m_dwReqCmd == CMD_MONI_SEND_HELLO || m_dwReqCmd == CMD_MONI_SEND_HELLO_FIRST)
		&& m_dwAgentClientIp == 0) 
	{
		REQERR_LOG("invalid packet - agent ip is 0");
		return ERR_INVALID_PACKET;
	}

	if(m_iRemoteMachineId <= 0) {
		// first hello
		m_pstMachInfo = slog.GetMachineInfo(m_dwAgentClientIp);
		if(m_pstMachInfo != NULL) {
			m_iRemoteMachineId = m_pstMachInfo->id;
			INFO_LOG("get mtclient machine id:%d", m_iRemoteMachineId);
		}
	}
	else {
		// not first hello
		m_pstMachInfo = slog.GetMachineInfo(m_iRemoteMachineId, NULL);
		if(m_pstMachInfo == NULL 
			|| (m_dwReqCmd == CMD_MONI_SEND_HELLO && !slog.IsIpMatchMachine(m_pstMachInfo, m_dwAgentClientIp))) 
		{
			REQERR_LOG("invalid, have no machine for id:%d", m_iRemoteMachineId);
			return ERR_NOT_FIND_MACHINE;
		}
	}

    uint32_t dwConnIp = m_addrRemote.GetAddr();
    MyQuery myqu(stConfig.qu, stConfig.db);
    Query & qu = myqu.GetQuery();
	
	// 新客户端接入，写入数据库自动注册
	if(m_iRemoteMachineId <= 0 && pctinfo) {
		snprintf(stConfig.szSql, MYSIZEOF(stConfig.szSql), 
			"select xrk_id,ip1,ip2,ip3,ip4,xrk_status from mt_machine where (ip1=%u or ip2=%u or ip3=%u or ip4=%u) ",
			m_dwAgentClientIp,m_dwAgentClientIp,m_dwAgentClientIp,m_dwAgentClientIp);
		MyQuery myqu(stConfig.qu, stConfig.db);
		Query & qu = myqu.GetQuery();
		if(!qu.get_result(stConfig.szSql)) {
			ERR_LOG("execute sql:%s failed !", stConfig.szSql);
			return ERR_SERVER;
		}

		if(qu.num_rows() <= 0 || !qu.fetch_row() || qu.getval("xrk_status") != 0)
		{
			int iDbMachineId = 0;
			if(qu.num_rows() > 0)
			    iDbMachineId = qu.getval("xrk_id");
			qu.free_result();

			if(iDbMachineId > 0) 
				snprintf(stConfig.szSql, MYSIZEOF(stConfig.szSql),
					"replace into mt_machine set xrk_name=\'%s\',ip1=%u,create_time=\'%s\',"
					"mod_time=now(),machine_desc=\'系统自动添加\',xrk_status=0,xrk_id=%d,"
                    "client_ip=%u,gw_ip=%u,conn_ip=%u,recv_ttl=%d,cfg_ttl=%d",
                    pctinfo->szHostName, m_dwAgentClientIp, uitodate(slog.m_stNow.tv_sec), iDbMachineId,
                    pctinfo->dwClientIp, pctinfo->dwGwIp, dwConnIp, m_iRecvTTL, pctinfo->iClientTTL);
			else
				snprintf(stConfig.szSql, MYSIZEOF(stConfig.szSql),
					"insert into mt_machine set xrk_name=\'%s\',ip1=%u,create_time=\'%s\',"
					"mod_time=now(),machine_desc=\'系统自动添加\',client_ip=%u,gw_ip=%u,conn_ip=%u,recv_ttl=%d,cfg_ttl=%d",
                    pctinfo->szHostName, m_dwAgentClientIp, uitodate(slog.m_stNow.tv_sec), 
                    pctinfo->dwClientIp, pctinfo->dwGwIp, dwConnIp, m_iRecvTTL, pctinfo->iClientTTL);
			if(!qu.execute(stConfig.szSql)){
				ERR_LOG("execute sql:%s failed !", stConfig.szSql);
				return ERR_SERVER;
			}
			if(iDbMachineId > 0)
				m_iRemoteMachineId = iDbMachineId;
			else
				m_iRemoteMachineId = qu.insert_id();
			INFO_LOG("insert new machine for agent client ip:%s, machine id:%d",
				ipv4_addr_str(m_dwAgentClientIp), m_iRemoteMachineId);
		}
		else {
			qu.fetch_row();
			m_iRemoteMachineId = qu.getval("xrk_id");
			qu.free_result();
            snprintf(stConfig.szSql, MYSIZEOF(stConfig.szSql), "update mt_machine set mod_time=now(),"
                "client_ip=%u,gw_ip=%u,conn_ip=%u,recv_ttl=%d,cfg_ttl=%d where xrk_id=%d",
                pctinfo->dwClientIp, pctinfo->dwGwIp, dwConnIp, m_iRecvTTL, pctinfo->iClientTTL, m_iRemoteMachineId);
			qu.execute(stConfig.szSql);
			INFO_LOG("get remote machine id:%d by sql ok", m_iRemoteMachineId);
		}
	}
    else if(pctinfo) {
        snprintf(stConfig.szSql, MYSIZEOF(stConfig.szSql), "update mt_machine set mod_time=now(),"
            "client_ip=%u,gw_ip=%u,conn_ip=%u,recv_ttl=%d,cfg_ttl=%d where xrk_id=%d",
            pctinfo->dwClientIp, pctinfo->dwGwIp, dwConnIp, m_iRecvTTL, pctinfo->iClientTTL, m_iRemoteMachineId);
		qu.execute(stConfig.szSql);
    }

	if(m_iMtClientIndex >= 0) {
		m_pMtClient = slog.GetMtClientInfo(m_iRemoteMachineId, m_iMtClientIndex);
		if(m_pMtClient != NULL) {
			DEBUG_LOG("GetMtClientInfo by cache index:%d ok ", m_iMtClientIndex);
		}
	}

	if(m_pMtClient == NULL) {
		m_pMtClient = slog.GetMtClientInfo(m_iRemoteMachineId, &isFind);
		if(m_pMtClient != NULL) {
			m_iMtClientIndex = slog.GetMtClientInfoIndex(m_pMtClient);
			if(!isFind) {
				// 只有 first hello 才可能找不到 client !
				if(!m_bIsFirstHello) {
					REQERR_LOG("not first hello, but have no mtclient !");
					return ERR_INVALID_PACKET;
				}
				m_pMtClient->dwAgentClientAddress = m_dwAgentClientIp;
				m_pMtClient->iMachineId = m_iRemoteMachineId;
				if(stConfig.psysConfig->wCurClientCount == 0) {
					ILINK_SET_FIRST(stConfig.psysConfig, iMtClientListIndexStart, iMtClientListIndexEnd,
						m_pMtClient, iPreIndex, iNextIndex, m_iMtClientIndex);
					stConfig.psysConfig->wCurClientCount = 1;
				}
				else {
					MtClientInfo *pfirst = slog.GetMtClientInfo(stConfig.psysConfig->iMtClientListIndexStart);
					ILINK_INSERT_FIRST(stConfig.psysConfig, iMtClientListIndexStart,
						m_pMtClient, iPreIndex, iNextIndex, pfirst, m_iMtClientIndex);
					stConfig.psysConfig->wCurClientCount++;
				}
				INFO_LOG("create MtClientInfo machine id:%d index:%d count:%d", 
					m_iRemoteMachineId, m_iMtClientIndex, stConfig.psysConfig->wCurClientCount);
			}
			else
				DEBUG_LOG("GetMtClientInfo by client address ok, index:%d", m_iMtClientIndex);
		}
	}

	if(m_pMtClient == NULL) {
		REQERR_LOG("get mtclient info failed !");
		return ERR_NOT_FIND_MACHINE;
	}
	return 0;
}

int CUdpSock::SetKeyToMachineTable()
{
	IM_SQL_PARA* ppara = NULL;
	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return SLOG_ERROR_LINE;
	}
	AddParameter(&ppara, "rand_key", m_pMtClient->sRandKey, NULL);
	MyQuery myqu(stConfig.qu, stConfig.db);
	Query & qu = myqu.GetQuery();

	std::string strSql;
	strSql = "update mt_machine set";
	JoinParameter_Set(&strSql, qu.GetMysql(), ppara);
	strSql += " where xrk_id=";
	strSql += itoa(m_iRemoteMachineId);
	ReleaseParameter(&ppara);
	if(!qu.execute(strSql)) {
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("set machine key for machine id:%d ok", m_iRemoteMachineId);
	return 0;
}

int CUdpSock::GetLocalPlugin(Json &js_plugin, int iPluginId)
{
    char sSqlBuf[128] = {0};
    MyQuery myqu(stConfig.qu, stConfig.db);
    Query & qu = myqu.GetQuery();

    snprintf(sSqlBuf, sizeof(sSqlBuf),
        "select pb_info from mt_plugin where open_plugin_id=%d", iPluginId);
    qu.get_result(sSqlBuf);
    if(qu.num_rows() <= 0 || qu.fetch_row() == NULL) {
        WARN_LOG("not find plugin:%d", iPluginId);
        qu.free_result();
        return SLOG_ERROR_LINE;
    }

    const char *pinfo = qu.getstr("pb_info");
    size_t iParseIdx = 0;
    size_t iReadLen = strlen(pinfo);
    js_plugin.Parse(pinfo, iParseIdx);
    if(iParseIdx != iReadLen) {
        WARN_LOG("parse json content, size:%u!=%u", (uint32_t)iParseIdx, (uint32_t)iReadLen);
    }
    qu.free_result();
    return 0;
}

int CUdpSock::MakePreInstallNotifyPkg(TEventInfo &stEvLocal, std::ostringstream &sCfgUrl, uint64_t & qwSessionId)
{
    TEventPreInstallPlugin &ev = stEvLocal.ev.stPreInstall;

    // head
    ReqPkgHead stHead;
    InitReqPkgHead(&stHead, CMD_MONI_S2C_PRE_INSTALL_NOTIFY, stConfig.dwPkgSeq++);
    stHead.qwSessionId = htonll(qwSessionId);

    // cmd content
	char sBuf[1200] = {0};
	CmdS2cPreInstallContentReq *req = (CmdS2cPreInstallContentReq*)sBuf;
    req->iPluginId = htonl(ev.iPluginId);
    req->iMachineId = htonl(ev.iMachineId);
    req->iDbId = htonl(ev.iDbId);
    if(stEvLocal.iEventType == EVENT_INNER_MODE_PREINSTALL_PLUGIN)
        req->bIsInnerMode = 1;
    else
        req->bIsInnerMode = 0;

	Json jsp;
	if(GetLocalPlugin(jsp, ev.iPluginId) < 0)
		return SLOG_ERROR_LINE;
	strncpy(req->sDevLang, (const char*)(jsp["dev_language"]), sizeof(req->sDevLang)-1);
	strncpy(req->sPluginName, (const char*)(jsp["plus_name"]), sizeof(req->sPluginName)-1);

	if(m_pConfig->stSysCfg.sPreInstallCheckStr[0] != '\0')
	    strncpy(req->sCheckStr, m_pConfig->stSysCfg.sPreInstallCheckStr, sizeof(req->sCheckStr)-1);
	else
		req->sCheckStr[0] = '\0';
	if(sCfgUrl.str().length()+1 + sizeof(*req) > sizeof(sBuf)) {
		ERR_LOG("need more space to save config url:%s", sCfgUrl.str().c_str());
		return SLOG_ERROR_LINE;
	}
	req->iUrlLen = htonl(sCfgUrl.str().length()+1);
	strcpy(req->sLocalCfgUrl, sCfgUrl.str().c_str());

	if(m_pMtClient->bEnableEncryptData) {
		static char sContentBuf[2048+256];
		int iUseBufLen = (int)sizeof(*req)+ntohl(req->iUrlLen);
		int iSigBufLen = ((iUseBufLen>>4)+1)<<4;
		if(iSigBufLen > (int)sizeof(sContentBuf)) {
			ERR_LOG("need more space %d > %d", iSigBufLen, (int)sizeof(sContentBuf));
			return ERR_SERVER;
		}
		aes_cipher_data((const uint8_t*)req, iUseBufLen,
			(uint8_t*)sContentBuf, (const uint8_t*)m_pMtClient->sRandKey, AES_128);
		InitCmdContent(sContentBuf, iSigBufLen);
	}
	else {
    	InitCmdContent(req, sizeof(*req)+ntohl(req->iUrlLen));
	}

    DEBUG_LOG("packet content length:%d, seq:%u, IsInnerMode:%d", 
        (int)(sizeof(*req)+ntohl(req->iUrlLen)), stHead.dwSeq, req->bIsInnerMode);
    return MakeReqPkg(NULL, NULL);
}

void CUdpSock::OnMachOprPluginExpire(TMachOprPluginSess *psess_data)
{
    std::ostringstream ss;
    ss << "update mt_plugin_machine set opr_proc=" << EV_MOP_OPR_RESPONSE_TIMEOUT;
    ss << " where xrk_id=" << psess_data->iDbId;
    MyQuery myqu(stConfig.qu, stConfig.db);
    Query & qu = myqu.GetQuery();
    qu.execute(ss.str());
}

void OnMachOprPluginExpire(UdpSessionInfo *psess)
{
    TMachOprPluginSess *psess_data = (TMachOprPluginSess*)(psess->GetSessBuf());
    CUdpSock *psock = (CUdpSock*)psess_data->psock;
    psock->OnMachOprPluginExpire(psess_data);
}

void CUdpSock::OnPreInstallPluginExpire(TPreInstallPluginSess *psess_data)
{
    std::ostringstream ss;
    ss << "update mt_plugin_machine set install_proc=" << EV_PREINSTALL_ERR_RESPONSE_TIMEOUT;
    ss << " where xrk_id=" << psess_data->iDbId;
    MyQuery myqu(stConfig.qu, stConfig.db);
    Query & qu = myqu.GetQuery();
    qu.execute(ss.str());
}

void OnPreInstallPluginExpire(UdpSessionInfo *psess)
{
    TPreInstallPluginSess *psess_data = (TPreInstallPluginSess*)(psess->GetSessBuf());
    CUdpSock *psock = (CUdpSock*)psess_data->psock;
    psock->OnPreInstallPluginExpire(psess_data);
}

void CUdpSock::DealEventPreInstall(TEventInfo &stEvLocal)
{
    TEventPreInstallPlugin &ev = stEvLocal.ev.stPreInstall;
    m_pMtClient = slog.GetMtClientInfo(ev.iMachineId, (uint32_t*)NULL);
    if(!m_pMtClient) {
        ERR_LOG("not find machine client, machine:%d", ev.iMachineId);
        return;
    }    

    MyQuery myqu(stConfig.qu, stConfig.db);
    Query & qu = myqu.GetQuery();
    std::ostringstream ss;
    ss << "select install_proc,local_cfg_url from mt_plugin_machine where xrk_id=" << ev.iDbId << " and xrk_status=0";
    if(!qu.get_result(ss.str().c_str()) || qu.num_rows() <= 0 || !qu.fetch_row()) {
        WARN_LOG("check preinstall failed, machine:%d, plugin:%d", ev.iMachineId, ev.iPluginId);
        qu.free_result();
        return;
    }    
    if(qu.getval("install_proc") != EV_PREINSTALL_DB_RECV)
    {    
        WARN_LOG("check preinstall process failed, proc:%d != %d, machine:%d, plugin:%d",
            qu.getval("install_proc"), EV_PREINSTALL_DB_RECV, ev.iMachineId, ev.iPluginId);
        qu.free_result();
        return;
    }    

	ss.str("");
	ss << qu.getstr("local_cfg_url");
    qu.free_result();

    uint64_t qwSessionId = TIME_SEC_TO_USEC(slog.m_stNow.tv_sec)+slog.m_stNow.tv_usec;

    m_iCommLen = MakePreInstallNotifyPkg(stEvLocal, ss, qwSessionId);
    if(m_iCommLen > 0) {
        UdpSessionInfo *psess = new UdpSessionInfo(qwSessionId);
        TPreInstallPluginSess *psess_data = (TPreInstallPluginSess*)(psess->GetSessBuf());
        psess_data->iEventType = stEvLocal.iEventType;
        psess_data->iPluginId = ev.iPluginId;
        psess_data->iMachineId = ev.iMachineId;
        psess_data->iDbId = ev.iDbId;
        psess_data->psock = this;

        psess->SetUdpPack(m_sCommBuf, m_iCommLen);
        psess->SetRemote(m_pMtClient->dwAddress, m_pMtClient->wBasePort);
        psess->SetTimeoutMs(5*1000);
        psess->SetSessExpireCallBack(::OnPreInstallPluginExpire);
        SendToBuf(psess, slog.m_stNow);
        INFO_LOG("send preinstall plugin event to client :%s:%d pkg len:%d, local cfg url:%s",
            ipv4_addr_str(m_pMtClient->dwAddress), m_pMtClient->wBasePort, m_iCommLen, ss.str().c_str());

        // 更新安装进度到 db 
        ss.str("");
        ss << "update mt_plugin_machine set install_proc=" << EV_PREINSTALL_TO_CLIENT_START;
        ss << " where xrk_id=" << ev.iDbId << " and xrk_status=0";
        qu.execute(ss.str().c_str());
    }
}

int CUdpSock::MakeMachOprPluginNotifyPkg(TEventInfo &event, uint64_t & qwSessionId)
{
    TEventMachinePluginOpr & ev = event.ev.stMachPlugOpr;

	// head
	ReqPkgHead stHead;
    if(EVENT_MULTI_MACH_PLUGIN_REMOVE == event.iEventType)
    	InitReqPkgHead(&stHead, CMD_MONI_S2C_MACH_ORP_PLUGIN_REMOVE, stConfig.dwPkgSeq++);
    else if(EVENT_MULTI_MACH_PLUGIN_ENABLE == event.iEventType)
    	InitReqPkgHead(&stHead, CMD_MONI_S2C_MACH_ORP_PLUGIN_ENABLE, stConfig.dwPkgSeq++);
    else if(EVENT_MULTI_MACH_PLUGIN_DISABLE == event.iEventType)
    	InitReqPkgHead(&stHead, CMD_MONI_S2C_MACH_ORP_PLUGIN_DISABLE, stConfig.dwPkgSeq++);
    else if(EVENT_MULTI_MACH_PLUGIN_CFG_MOD == event.iEventType)
    	InitReqPkgHead(&stHead, CMD_MONI_S2C_MACH_ORP_PLUGIN_MOD_CFG, stConfig.dwPkgSeq++);
    else {
        ERR_LOG("unknow event type:%d", event.iEventType);
        return SLOG_ERROR_LINE;
    }
    stHead.qwSessionId = htonll(qwSessionId);

    // cmd content
    int iContentLen = 0;
    static char s_packBuf[4096];
    char *pContent = (char*)s_packBuf;
    if(EVENT_MULTI_MACH_PLUGIN_CFG_MOD == event.iEventType) {
        CmdS2cModMachPluginCfgReq *preq = (CmdS2cModMachPluginCfgReq*)s_packBuf;
        preq->iPluginId = htonl(ev.iPluginId);
        preq->iMachineId = htonl(ev.iMachineId);

        MyQuery myqu(stConfig.qu, stConfig.db);
        Query & qu = myqu.GetQuery();
        std::ostringstream ss;
        ss << "select opr_start_time,down_cfgs_list,down_cfgs_restart from mt_plugin_machine where xrk_id=" << ev.iDbId;
        qu.get_result(ss.str().c_str());
        qu.fetch_row();

        if(qu.getuval("opr_start_time")+30 < slog.m_stNow.tv_sec) {
            WARN_LOG("download plugin config timeout, machine:%d, plugin:%d", ev.iMachineId, ev.iPluginId);
            qu.free_result();
            return SLOG_ERROR_LINE;
        }

        const char *pstrCfgs = qu.getstr("down_cfgs_list");
        if(sizeof(CmdS2cModMachPluginCfgReq)+strlen(pstrCfgs)+1 > sizeof(s_packBuf)) {
            WARN_LOG("need more space %lu > %lu, machine:%d, plugin:%d", 
                sizeof(CmdS2cModMachPluginCfgReq)+strlen(pstrCfgs)+1, sizeof(s_packBuf), ev.iMachineId, ev.iPluginId);
            qu.free_result();
            return SLOG_ERROR_LINE;
        }

        preq->dwDownCfgTime = ntohl(qu.getuval("opr_start_time"));
        preq->iConfigLen = ntohl(strlen(pstrCfgs)+1);
        preq->bRestartPlugin = qu.getval("down_cfgs_restart");

        // 包含字符串结尾 '\0'
        memcpy(preq->strCfgs, pstrCfgs, strlen(pstrCfgs)+1);
        iContentLen = (int)(sizeof(CmdS2cModMachPluginCfgReq)+strlen(preq->strCfgs)+1);
        qu.free_result();
    }
    else {
        CmdS2cMachOprPluginReq *preq = (CmdS2cMachOprPluginReq*)s_packBuf;
        memset(preq, 0, sizeof(*preq));

        preq->iPluginId = htonl(ev.iPluginId);
        preq->iMachineId = htonl(ev.iMachineId);
        preq->iDbId = htonl(ev.iDbId);

		MyQuery myqu(stConfig.qu, stConfig.db);
		Query & qu = myqu.GetQuery();
		std::ostringstream ss;
		ss << "select plugin_name from mt_plugin where open_plugin_id=" << ev.iPluginId;
		if(qu.get_result(ss.str().c_str()) && qu.num_rows() > 0) {
			qu.fetch_row();
			strncpy(preq->sPluginName, qu.getstr("plugin_name"), sizeof(preq->sPluginName)); 
			qu.free_result();
		}
		else {
            ERR_LOG("get plugin:%d name failed!", ev.iPluginId);
            return SLOG_ERROR_LINE;
        }
        iContentLen = (int)sizeof(*preq);
    }

	if(m_pMtClient->bEnableEncryptData) {
		static char sContentBuf[4096+256];
		int iSigBufLen = ((iContentLen>>4)+1)<<4;
		if(iSigBufLen > (int)sizeof(sContentBuf)) {
			ERR_LOG("need more space %d > %d", iSigBufLen, (int)sizeof(sContentBuf));
			return ERR_SERVER;
		}
		aes_cipher_data((const uint8_t*)pContent, iContentLen,
			(uint8_t*)sContentBuf, (const uint8_t*)m_pMtClient->sRandKey, AES_128);
		InitCmdContent(sContentBuf, iSigBufLen);
	}
	else {
	    InitCmdContent(pContent, iContentLen);
	}

	// 使用内部缓存组包
	return MakeReqPkg(NULL, NULL); 
}

void CUdpSock::DealEventMachOprPlugin(TEventInfo &event)
{
    TEventMachinePluginOpr &ev = event.ev.stMachPlugOpr;
    m_pMtClient = slog.GetMtClientInfo(ev.iMachineId, (uint32_t*)NULL);
    if(!m_pMtClient) {
        ERR_LOG("not find machine client, machine:%d", ev.iMachineId);
        return;
    }

    MyQuery myqu(stConfig.qu, stConfig.db);
    Query & qu = myqu.GetQuery();

    // 检查下任务数据是否正确
    std::ostringstream ss;
    ss << "select opr_proc,xrk_status from mt_plugin_machine where xrk_id=" << ev.iDbId; 
    if(!qu.get_result(ss.str().c_str()) || qu.num_rows() <= 0 || !qu.fetch_row()) {
        WARN_LOG("check machine opr plugin failed, machine:%d, plugin:%d, user:%u",
            ev.iMachineId, ev.iPluginId, ev.dwUserMasterId);
        qu.free_result();
        return;
    }
    if(qu.getval("opr_proc") != EV_MOP_OPR_DB_RECV)
    {
        WARN_LOG("check machine opr plugin process failed, proc:%d != %d, machine:%d, plugin:%d, event:%d",
            qu.getval("opr_proc"), EV_MOP_OPR_START, ev.iMachineId, ev.iPluginId, event.iEventType);
        qu.free_result();
        return;
    }
    qu.free_result();

    uint64_t qwSessionId = TIME_SEC_TO_USEC(slog.m_stNow.tv_sec)+slog.m_stNow.tv_usec;
    m_iCommLen = MakeMachOprPluginNotifyPkg(event, qwSessionId);
    if(m_iCommLen > 0) {
        UdpSessionInfo *psess = new UdpSessionInfo(qwSessionId);
        TMachOprPluginSess *psess_data = (TMachOprPluginSess*)(psess->GetSessBuf());
        psess_data->iEventType = event.iEventType;
        psess_data->iPluginId = ev.iPluginId;
        psess_data->iMachineId = ev.iMachineId;
        psess_data->iDbId = ev.iDbId;
        psess_data->psock = this;

        psess->SetUdpPack(m_sCommBuf, m_iCommLen);
        psess->SetRemote(m_pMtClient->dwAddress, m_pMtClient->wBasePort);
        psess->SetTimeoutMs(5*1000);
        psess->SetSessExpireCallBack(::OnMachOprPluginExpire);
        SendToBuf(psess, slog.m_stNow);
        INFO_LOG("send machine opr plugin:%d event to client :%s:%d pkg len:%d, user:%u, sess:%lu", ev.iPluginId,
            ipv4_addr_str(m_pMtClient->dwAddress), m_pMtClient->wBasePort, m_iCommLen, ev.dwUserMasterId, qwSessionId);

        // 更新安装进度到 db 
        ss.str("");
        ss << "update mt_plugin_machine set opr_proc=" << EV_MOP_OPR_DOWNLOAD;
        ss << " where xrk_id=" << ev.iDbId << " and xrk_status=0";
        qu.execute(ss.str().c_str());
    }
    else {
        // 更新安装进度到 db 
        ss.str("");
        ss << "update mt_plugin_machine set opr_proc=" << EV_MOP_OPR_FAILED;
        ss << " where xrk_id=" << ev.iDbId << " and xrk_status=0";
        qu.execute(ss.str().c_str());
        WARN_LOG("deal machine opr plugin:%d, event to client :%s:%d failed, pkglen:%d, event:%d", ev.iPluginId,
            ipv4_addr_str(m_pMtClient->dwAddress), m_pMtClient->wBasePort, m_iCommLen, event.iEventType);
    }
}

void CUdpSock::DealEvent()
{           
    if(!SYNC_FLAG_CAS_GET(&(m_pConfig->stSysCfg.bEventModFlag)))
        return;
    TEventInfo stEvLocal;
	stEvLocal.iEventType = 0;
    for(int i=0; i < MAX_EVENT_COUNT; i++) {
        if(m_pConfig->stSysCfg.stEvent[i].bEventStatus == EVENT_STATUS_INIT_SET
            && m_pConfig->stSysCfg.stEvent[i].dwExpireTime >= slog.m_stNow.tv_sec) 
        {
            memcpy(&stEvLocal, m_pConfig->stSysCfg.stEvent+i, sizeof(stEvLocal));
            m_pConfig->stSysCfg.stEvent[i].bEventStatus = EVENT_STATUS_FIN;
            break;
        }
    }  
    SYNC_FLAG_CAS_FREE(m_pConfig->stSysCfg.bEventModFlag);

    if(!stEvLocal.iEventType)
        return;

    Init();
 
    if(stEvLocal.iEventType == EVENT_PREINSTALL_PLUGIN
        || stEvLocal.iEventType == EVENT_INNER_MODE_PREINSTALL_PLUGIN)
    {
        DealEventPreInstall(stEvLocal);
    }
    else if(EVENT_MULTI_MACH_PLUGIN_REMOVE == stEvLocal.iEventType
        || EVENT_MULTI_MACH_PLUGIN_CFG_MOD == stEvLocal.iEventType
        || EVENT_MULTI_MACH_PLUGIN_ENABLE == stEvLocal.iEventType
        || EVENT_MULTI_MACH_PLUGIN_DISABLE== stEvLocal.iEventType)
    {
        DealEventMachOprPlugin(stEvLocal);
    }
    else {
        WARN_LOG("unsupport event :%d", stEvLocal.iEventType);
    }
}

int CUdpSock::DealCmdHelloFirst()
{
	if(m_wCmdContentLen != MYSIZEOF(MonitorHelloFirstContent)) {
		REQERR_LOG("invalid cmd content %u != %u", m_wCmdContentLen, MYSIZEOF(MonitorHelloFirstContent));
		return ERR_INVALID_CMD_CONTENT;
	}

	MonitorHelloFirstContent *pctinfo = (MonitorHelloFirstContent*)m_pstCmdContent;
	m_iMtClientIndex = ntohl(pctinfo->iMtClientIndex);
	m_iRemoteMachineId = ntohl(pctinfo->iMachineId);
    pctinfo->sCmpTime[sizeof(pctinfo->sCmpTime)-1] = '\0';
    pctinfo->sVersion[sizeof(pctinfo->sVersion)-1] = '\0';
    pctinfo->sOsInfo[sizeof(pctinfo->sOsInfo)-1] = '\0';
    pctinfo->sOsArc[sizeof(pctinfo->sOsArc)-1] = '\0';
    pctinfo->sLibcVer[sizeof(pctinfo->sLibcVer)-1] = '\0';
    pctinfo->sLibcppVer[sizeof(pctinfo->sLibcppVer)-1] = '\0';
    pctinfo->iClientTTL = ntohl(pctinfo->iClientTTL);
    pctinfo->dwGwIp = ntohl(pctinfo->dwGwIp);
    pctinfo->dwClientIp = ntohl(pctinfo->dwClientIp);
    ChangeDbString(pctinfo->szHostName);

	m_pstReqHead->sEchoBuf[ sizeof(m_pstReqHead->sEchoBuf) - 1 ] = '\0';

	DEBUG_LOG("get cmd first hello - client index:%d, machine id:%d, ver:%d, cmp time:%s",
		m_iMtClientIndex, m_iRemoteMachineId, ntohs(m_pstReqHead->wVersion), m_pstReqHead->sEchoBuf);

	// 签名部分处理
	int iRet = 0;
	if((iRet=CheckSignature()) != NO_ERROR)
		return iRet;
	MonitorHelloSig *psigInfo = (MonitorHelloSig*)m_sDecryptBuf;
	if(ntohl(psigInfo->dwPkgSeq) != m_dwReqSeq)
	{
		REQERR_LOG("invalid signature info - seq(%u,%u)", ntohl(psigInfo->dwPkgSeq), m_dwReqSeq);
		return ERR_INVALID_SIGNATURE_INFO;
	}
	m_dwAgentClientIp = ntohl(psigInfo->dwAgentClientIp);

    m_iRecvTTL = GetRecvTTL();
	INFO_LOG("check packet ok remote client ip:%s, conn ip:%s, ttl:%d",
		ipv4_addr_str(m_dwAgentClientIp), m_addrRemote.Convert().c_str(), m_iRecvTTL);

	// 获取请求客户端
	if((iRet=GetMtClientInfo(pctinfo)) != 0)
		return iRet;

	// 首个 hello 重新初始化
	InitMtClientInfo(); 

	SaveMachineInfoToDb(pctinfo);
	if(psigInfo->bEnableEncryptData) {
		m_pMtClient->bEnableEncryptData = 1;
		memcpy(m_pMtClient->sRandKey, psigInfo->sRespEncKey, sizeof(m_pMtClient->sRandKey));

		if(SetKeyToMachineTable() < 0)
			return AckToReq(ERR_SET_MACHINE_KEY_FAILED);
		DEBUG_LOG("set encrypt data key:%s", psigInfo->sRespEncKey);
	}
	else
		m_pMtClient->bEnableEncryptData = 0;

	// 响应处理
	MonitorHelloFirstContentResp stResp;
	memset(&stResp, 0, sizeof(stResp));

	stResp.iMtClientIndex = htonl(m_iMtClientIndex);
	stResp.iMachineId = htonl(m_iRemoteMachineId);
	stResp.dwConnServerIp = m_pMtClient->dwAddress;
	stResp.wAttrSrvPort = htons(stConfig.psysConfig->wAttrSrvPort);

    // 阿里云收不到本机发往本机的外网IP数据包问题fix  @ 2021-05-07
    // agent 和监控点服务器同机部署时，监控点服务器地址改为回环地址: 127.0.0.1
    uint32_t dwAttrSrvMasterIp = stConfig.psysConfig->dwAttrSrvMasterIp;
    if(m_pstMachInfo && slog.IsIpMatchMachine(m_pstMachInfo, dwAttrSrvMasterIp))
        dwAttrSrvMasterIp = g_dwLoopIP;
	stResp.dwAttrSrvIp = htonl(dwAttrSrvMasterIp);
    if(ntohl(pctinfo->dwBindDES-PWESetTime) != stConfig.psysConfig->dwDES-PWESetTime) {
        stResp.dwBindDES-PWESetTime = htonl(stConfig.psysConfig->dwDES-PWESetTime);
        stResp.iBindCloudUserId = htonl(stConfig.psysConfig->iDES-PWEUid);
        if(stConfig.psysConfig->iDES-PWEUid != 0) {
            strncpy(stResp.szBindCloudKey, stConfig.psysConfig->szDES-PWEKey, sizeof(stResp.szBindCloudKey)-1);
        }
        INFO_LOG("bind DES-PWE changed, down bind(user:%u, key:%s, time:%u)", stConfig.psysConfig->iDES-PWEUid, 
            stConfig.psysConfig->szDES-PWEKey, stConfig.psysConfig->dwDES-PWESetTime);
    }
    else 
        stResp.dwBindDES-PWESetTime = pctinfo->dwBindDES-PWESetTime;
	INFO_LOG("set attr server %s:%d", ipv4_addr_str(dwAttrSrvMasterIp), stConfig.psysConfig->wAttrSrvPort);
	
	// 下发下最新的接入服务器 slog_mtreport_server 地址
	int iStartIdx = 0;
	SLogServer *psrv_m = slog.GetValidServerByType(SRV_TYPE_CONNECT, &iStartIdx);
	if(psrv_m != NULL)
	{
		memcpy(stResp.szNewMasterSrvIp, psrv_m->szIpV4, sizeof(stResp.szNewMasterSrvIp));
		stResp.wNewSrvPort = htons(psrv_m->wPort);
		DEBUG_LOG("set master svr:%s:%d", psrv_m->szIpV4, psrv_m->wPort);
	}

	char sEncBuf[1024] = {0};
	int iSigBufLen = ((sizeof(stResp)>>4)+1)<<4;
	if(iSigBufLen > MAX_SIGNATURE_LEN)
		ERR_LOG("need more space %d > %d", iSigBufLen, MAX_SIGNATURE_LEN);
	aes_cipher_data((const uint8_t*)&stResp,
		sizeof(stResp), (uint8_t*)sEncBuf, (const uint8_t*)psigInfo->sRespEncKey, AES_128);

	// 成功响应
	InitCmdContent(sEncBuf, iSigBufLen);
	AckToReq(NO_ERROR); 
	DEBUG_LOG("response first hello cmd content len:%d", iSigBufLen);
	return NO_ERROR;
}

int CUdpSock::DealCommInfo()
{
	TWTlv *pTlv = GetWTlvByType2(TLV_MONI_COMM_INFO, m_pstBody);
	if(pTlv == NULL || ntohs(pTlv->wLen) != MYSIZEOF(TlvMoniCommInfo)) {
		REQERR_LOG("get tlv(%d) failed, or invalid len(%d-%u)",
			TLV_MONI_COMM_INFO, (pTlv!=NULL ? ntohs(pTlv->wLen) : 0), MYSIZEOF(TlvMoniCommInfo));
		return ERR_CHECK_TLV;
	}
	TlvMoniCommInfo *pctinfo = (TlvMoniCommInfo*)pTlv->sValue;
	m_iMtClientIndex = ntohl(pctinfo->iMtClientIndex);
	m_iRemoteMachineId = ntohl(pctinfo->iMachineId);
	if(m_dwReqCmd == CMD_MONI_SEND_HELLO)
		m_dwAgentClientIp = ntohl(pctinfo->dwReserved_1);

	DEBUG_LOG("get comminfo by tlv MtClientIndex:%d", m_iMtClientIndex);

	// 获取请求客户端
	if(GetMtClientInfo() != 0)
		return ERR_INVALID_PACKET;

	// 签名部分处理
	int iRet = 0;
	if((iRet=CheckSignature()) != NO_ERROR)
		return iRet;

    if(m_pstReqHead->qwSessionId != 0) {
        uint64_t sessid = ntohll(m_pstReqHead->qwSessionId);
        OnRecvUdpSess(sessid);
    }
	return NO_ERROR;
}

int CUdpSock::DealCmdHello()
{
	if(m_wCmdContentLen != MYSIZEOF(MonitorHelloContent)) {
		REQERR_LOG("invalid cmd content %u != %u", m_wCmdContentLen, MYSIZEOF(MonitorHelloContent));
		return ERR_INVALID_CMD_CONTENT;
	}

	int iRet = 0;
	if((iRet=DealCommInfo()) != NO_ERROR)
		return iRet;

	MonitorHelloContent *pctinfo = (MonitorHelloContent*)m_pstCmdContent;
	m_pMtClient->dwLastHelloTime = stConfig.dwCurrentTime;
	DEBUG_LOG("get hello , server response time:%u ms , hello times:%u update last hello time -",
		ntohl(pctinfo->dwServerResponseTime), ntohl(pctinfo->dwHelloTimes));

    pctinfo->dwServerResponseTime = ntohl(pctinfo->dwServerResponseTime);
    SetHelloTimeToMachineTable(pctinfo->dwServerResponseTime);
	if((int)(pctinfo->dwServerResponseTime) > m_pMtClient->iServerResponseTimeMs) {
		m_pMtClient->iServerResponseTimeMs = pctinfo->dwServerResponseTime;
		INFO_LOG("update max server response time to:%u", m_pMtClient->iServerResponseTimeMs);
	}

	// 成功响应
	MonitorHelloContentResp stResp;
    memset(&stResp, 0, sizeof(stResp));
	stResp.iMtClientIndex = htonl(m_iMtClientIndex);

    if(ntohl(pctinfo->dwBindDES-PWESetTime) != stConfig.psysConfig->dwDES-PWESetTime) {
        stResp.dwBindDES-PWESetTime = htonl(stConfig.psysConfig->dwDES-PWESetTime);
        stResp.iBindCloudUserId = htonl(stConfig.psysConfig->iDES-PWEUid);
        if(stConfig.psysConfig->iDES-PWEUid != 0) {
            strncpy(stResp.szBindCloudKey, stConfig.psysConfig->szDES-PWEKey, sizeof(stResp.szBindCloudKey)-1);
        }
        INFO_LOG("bind DES-PWE changed, down bind(user:%u, key:%s, time:%u)", stConfig.psysConfig->iDES-PWEUid, 
            stConfig.psysConfig->szDES-PWEKey, stConfig.psysConfig->dwDES-PWESetTime);
    }
    else 
        stResp.dwBindDES-PWESetTime = pctinfo->dwBindDES-PWESetTime;

    // 阿里云收不到本机发往本机的外网IP数据包问题fix  @ 2021-05-07
    // agent 和监控点服务器同机部署时，监控点服务器地址改为回环地址: 127.0.0.1
    uint32_t dwAttrSrvMasterIp = stConfig.psysConfig->dwAttrSrvMasterIp;
    if(m_pstMachInfo && slog.IsIpMatchMachine(m_pstMachInfo, dwAttrSrvMasterIp))
        dwAttrSrvMasterIp = g_dwLoopIP;
	if(ntohl(pctinfo->dwAttrSrvIp) != dwAttrSrvMasterIp
		|| pctinfo->wAttrServerPort != htons(stConfig.psysConfig->wAttrSrvPort))
	{
		stResp.bConfigChange = 1;
		stResp.wAttrServerPort = htons(stConfig.psysConfig->wAttrSrvPort);
		stResp.dwAttrSrvIp = htonl(dwAttrSrvMasterIp);
		INFO_LOG("set new attr server :%s:%d", ipv4_addr_str(dwAttrSrvMasterIp), stConfig.psysConfig->wAttrSrvPort);
	}
	else
	{
		stResp.bConfigChange = 0;
	}

    user::SystemOtherInfo stPb;
    pctinfo->szDES-PWEUrl[sizeof(pctinfo->szDES-PWEUrl)-1] = '\0';
    if(slog.GetSystemOtherInfo(stPb) >= 0 && stPb.has_cust_cloud_url()) {
        if(strcmp(pctinfo->szDES-PWEUrl, stPb.cust_cloud_url().c_str())) {
            WARN_LOG("agent client:%s, DES-PWE url not equal (%s, %s)",
                m_addrRemote.Convert(true).c_str(), pctinfo->szDES-PWEUrl, stPb.cust_cloud_url().c_str());
        }
        strncpy(stResp.szRespDES-PWEUrl, stPb.cust_cloud_url().c_str(), sizeof(stResp.szRespDES-PWEUrl));
    }
    else {
        ERR_LOG("get SystemOtherInfo failed, use default");
        strncpy(stResp.szRespDES-PWEUrl, "http://DES-PWE.com", sizeof(stResp.szRespDES-PWEUrl));
    }

	if(m_pMtClient->bEnableEncryptData) {
		char sEncBuf[1024] = {0};
		int iSigBufLen = ((sizeof(stResp)>>4)+1)<<4;
		if(iSigBufLen > (int)sizeof(sEncBuf)) {
			ERR_LOG("need more space %d > %d", iSigBufLen, (int)sizeof(sEncBuf));
			return ERR_SERVER;
		}
		aes_cipher_data((const uint8_t*)&stResp,
			sizeof(stResp), (uint8_t*)sEncBuf, (const uint8_t*)m_pMtClient->sRandKey, AES_128);
		InitCmdContent(sEncBuf, iSigBufLen);
	}
	else
		InitCmdContent(&stResp, sizeof(stResp));

	AckToReq(NO_ERROR); 
	return NO_ERROR;
}

int CUdpSock::InnerDealLogConfigCheck(int iFirstIdx, int iInfoCount, ContentCheckLogConfig *pctinfo,
	int &iAddCount, int &iModCount, int &iSameCount, int &iUseBufLen, int iMaxBufLen, TPkgBody *pRespTlvBody)
{
	if(iFirstIdx < 0 || iFirstIdx >= MAX_SLOG_CONFIG_COUNT)
	{
		WARN_LOG("invalid index:%d, max:%d", iFirstIdx, MAX_SLOG_CONFIG_COUNT);
		return SLOG_ERROR_LINE;
	}

	int iTmpLen = 0;
	SLogClientConfig *pCfgInfo = m_pConfig->stConfig+iFirstIdx;
	MtSLogConfig stLogConfig;
	for(int i=0,j=0; i < iInfoCount; i++) 
	{
		for(j=0; j < pctinfo->wLogConfigCount; j++) 
		{
			if(pCfgInfo->dwCfgId == pctinfo->stLogConfigList[j].dwCfgId) 
			{
				SET_BIT(pctinfo->stLogConfigList[j].dwCfgFlag, MONI_CONFIG_FLAG_CHECKED);

				// check config seq
				if(pCfgInfo->dwConfigSeq != pctinfo->stLogConfigList[j].dwSeq) 
				{
					iTmpLen = MYSIZEOF(TWTlv) + MYSIZEOF(stLogConfig) - sizeof(SLogTestKey)*MAX_SLOG_TEST_KEYS_PER_CONFIG 
						+ pCfgInfo->wTestKeyCount*sizeof(SLogTestKey);
					if(iUseBufLen+iTmpLen > iMaxBufLen)
						break;

					INFO_LOG("log config change id:%u seq:%u(client:%u), tmpLen:%d, testkey:%d",
						pCfgInfo->dwCfgId, pCfgInfo->dwConfigSeq, 
						pctinfo->stLogConfigList[j].dwSeq, iTmpLen, pCfgInfo->wTestKeyCount);

					LOG_CHECK_COPY_CONFIG_INFO(pCfgInfo);

					iUseBufLen += SetWTlv((TWTlv*)((char*)pRespTlvBody+iUseBufLen), TLV_MONI_CONFIG_MOD, 
						iTmpLen-(int)MYSIZEOF(TWTlv), (const char*)(&stLogConfig));
					pRespTlvBody->bTlvNum++;
					iModCount++;
					iTmpLen = 0;
				}
				else 
					iSameCount++;
				break; 
			}
		}

		if(j >= pctinfo->wLogConfigCount) 
		{
			iTmpLen = MYSIZEOF(TWTlv) + MYSIZEOF(stLogConfig) - sizeof(SLogTestKey)*MAX_SLOG_TEST_KEYS_PER_CONFIG 
				+ pCfgInfo->wTestKeyCount*sizeof(SLogTestKey);
			if(iUseBufLen+iTmpLen > iMaxBufLen)
				break;

			INFO_LOG("add log config to client config id:%u seq:%u, tmpLen:%d, testkey:%d", 
				pCfgInfo->dwCfgId, pCfgInfo->dwConfigSeq, iTmpLen, pCfgInfo->wTestKeyCount);

			LOG_CHECK_COPY_CONFIG_INFO(pCfgInfo);

			iUseBufLen += SetWTlv((TWTlv*)((char*)pRespTlvBody+iUseBufLen), TLV_MONI_CONFIG_ADD,
				iTmpLen-(int)MYSIZEOF(TWTlv), (const char*)(&stLogConfig));
			pRespTlvBody->bTlvNum++;
			iAddCount++;
			iTmpLen = 0;
		}

		if(iTmpLen > 0) // 缓冲区不足了
			break;

		if(pCfgInfo->iNextIndex >= 0)
			pCfgInfo = m_pConfig->stConfig+pCfgInfo->iNextIndex;
		else
			pCfgInfo = NULL;
	}

	return iTmpLen;
}

int CUdpSock::SetLogConfigCheckInfo(ContentCheckLogConfig *pctinfo)
{
	static char sRespBuf[1200];
	memset(sRespBuf, 0, sizeof(sRespBuf));

	TPkgBody *pRespTlvBody = (TPkgBody*)sRespBuf;
	int iUseBufLen = MYSIZEOF(TPkgBody), i=0, j=0;

	pctinfo->wLogConfigCount = ntohs(pctinfo->wLogConfigCount);
	for(i=0; i < pctinfo->wLogConfigCount; i++) {
#define LOG_CHECK_32_TO_LOCAL(field) \
		pctinfo->stLogConfigList[i].field = ntohl(pctinfo->stLogConfigList[i].field);
		LOG_CHECK_32_TO_LOCAL(dwCfgId);
		LOG_CHECK_32_TO_LOCAL(dwSeq);
		LOG_CHECK_32_TO_LOCAL(dwCfgFlag);
#undef LOG_CHECK_32_TO_LOCAL 
		CLEAR_BIT(pctinfo->stLogConfigList[i].dwCfgFlag, MONI_CONFIG_FLAG_CHECKED);
	}

	int iAddCount = 0, iModCount = 0, iDelCount = 0, iSameCount = 0, iTmpLen = 0;

	if(stConfig.psysConfig->wLogConfigCount > 0)
		iTmpLen = InnerDealLogConfigCheck(stConfig.psysConfig->iLogConfigIndexStart, stConfig.psysConfig->wLogConfigCount, 
			pctinfo, iAddCount, iModCount, iSameCount, iUseBufLen, (int)MYSIZEOF(sRespBuf), pRespTlvBody);

	// 检查是否有被删除的 log config, 通过检查处理标志是否设置实现
	uint32_t dwCfgIdDel = 0;
	for(j=0; iTmpLen <= 0 && j < pctinfo->wLogConfigCount; j++) 
	{
		if(IS_SET_BIT(pctinfo->stLogConfigList[j].dwCfgFlag, MONI_CONFIG_FLAG_CHECKED))
			continue;

		iTmpLen = MYSIZEOF(uint32_t)+MYSIZEOF(TWTlv);
		if(iUseBufLen+iTmpLen > (int)MYSIZEOF(sRespBuf))
			break;

		dwCfgIdDel = htonl(pctinfo->stLogConfigList[j].dwCfgId);
		iUseBufLen += SetWTlv((TWTlv*)((char*)pRespTlvBody+iUseBufLen),
			TLV_MONI_CONFIG_DEL, MYSIZEOF(dwCfgIdDel), (const char*)(&dwCfgIdDel));
		pRespTlvBody->bTlvNum++;
		INFO_LOG("delete log config id:%u", pctinfo->stLogConfigList[j].dwCfgId);
		iDelCount++;
		iTmpLen = 0;
	}
	if(iTmpLen > 0)
		WARN_LOG("need more space %d+%d > %u", iUseBufLen, iTmpLen, MYSIZEOF(sRespBuf));

	if(m_pMtClient->bEnableEncryptData) {
		static char sContentBuf[2048+256];
		int iSigBufLen = ((iUseBufLen>>4)+1)<<4;
		if(iSigBufLen > (int)sizeof(sContentBuf)) {
			ERR_LOG("need more space %d > %d", iSigBufLen, (int)sizeof(sContentBuf));
			return ERR_SERVER;
		}
		aes_cipher_data((const uint8_t*)pRespTlvBody, iUseBufLen,
			(uint8_t*)sContentBuf, (const uint8_t*)m_pMtClient->sRandKey, AES_128);
		InitCmdContent(sContentBuf, iSigBufLen);
	}
	else {
		InitCmdContent(pRespTlvBody, iUseBufLen);
	}
	INFO_LOG("check log config count add:%d mod:%d del:%d not change:%d, content len:%u, tlvnum:%d",
		iAddCount, iModCount, iDelCount, iSameCount, iUseBufLen, pRespTlvBody->bTlvNum);
	return NO_ERROR;
}

int CUdpSock::DealCmdCheckLogConfig()
{
	if(m_wCmdContentLen < MYSIZEOF(ContentCheckLogConfig)) {
		REQERR_LOG("invalid cmd content %u < %u", m_wCmdContentLen, MYSIZEOF(ContentCheckLogConfig));
		return ERR_INVALID_CMD_CONTENT;
	}

	int iRet = 0;
	if((iRet=DealCommInfo()) != NO_ERROR)
		return iRet;

	ContentCheckLogConfig *pctinfo = (ContentCheckLogConfig*)m_pstCmdContent;
	if(m_wCmdContentLen != MYSIZEOF(ContentCheckLogConfig)
		+ MYSIZEOF(pctinfo->stLogConfigList[0])*ntohs(pctinfo->wLogConfigCount)){
		REQERR_LOG("check cmd content failed %d != %u", m_wCmdContentLen,
			MYSIZEOF(ContentCheckLogConfig)+MYSIZEOF(uint32_t)*ntohs(pctinfo->wLogConfigCount));
		return ERR_INVALID_CMD_CONTENT;
	}

	DEBUG_LOG("get check log config, server response time:%u ms , log config count:%d",
		ntohl(pctinfo->dwServerResponseTime), ntohs(pctinfo->wLogConfigCount));
	if((int)ntohl(pctinfo->dwServerResponseTime) > m_pMtClient->iServerResponseTimeMs) {
		m_pMtClient->iServerResponseTimeMs = ntohl(pctinfo->dwServerResponseTime);
		INFO_LOG("update max server response time to:%u", m_pMtClient->iServerResponseTimeMs);
	}

	if((iRet=SetLogConfigCheckInfo(pctinfo)) != NO_ERROR)
		return iRet;
	AckToReq(NO_ERROR); 
	return NO_ERROR;
}

int CUdpSock::InnerDealAppInfoCheck(int iFirstAppIdx, int iAppInfoCount, ContentCheckAppInfo *pctinfo,
	int &iAddCount, int &iModCount, int &iSameCount, int &iUseBufLen, int iMaxBufLen, TPkgBody *pRespTlvBody)
{
	if(iFirstAppIdx < 0 || iFirstAppIdx >= MAX_SLOG_APP_COUNT)
	{
		WARN_LOG("invalid app index:%d, max:%d", iFirstAppIdx, MAX_SLOG_APP_COUNT);
		return SLOG_ERROR_LINE;
	}

	int iTmpLen = 0, iAppInfoLen = 0;
	AppInfo *pAppInfo = m_pAppInfo->stInfo+iFirstAppIdx;
	MtAppInfo stAppInfo;
	for(int i=0,j=0; i < iAppInfoCount; i++) 
	{
		for(j=0; j < pctinfo->wAppInfoCount; j++) 
		{
			if(pAppInfo->iAppId == pctinfo->stAppList[j].iAppId) {

				SET_BIT(pctinfo->stAppList[j].dwCfgFlag, MONI_CONFIG_FLAG_CHECKED);

				// check config seq
				if(pAppInfo->dwSeq != pctinfo->stAppList[j].dwSeq) 
				{
					iAppInfoLen = MYSIZEOF(MtAppInfo);
					iTmpLen = MYSIZEOF(TWTlv) + iAppInfoLen;
					if(iUseBufLen+iTmpLen > iMaxBufLen)
						break;

					INFO_LOG("app info change app id:%u seq:%u(client:%u)",
						pAppInfo->iAppId, pAppInfo->dwSeq, pctinfo->stAppList[j].dwSeq);

					// copy new config to resp
					APPINFO_CHECK_COPY_INFO;

					iUseBufLen += SetWTlv((TWTlv*)(pRespTlvBody+iUseBufLen), 
						TLV_MONI_CONFIG_MOD, iAppInfoLen, (const char*)(&stAppInfo));
					pRespTlvBody->bTlvNum++;
					iModCount++;
					iTmpLen = 0;
				}
				else 
					iSameCount++;
				break;
			}
		}

		if(j >= pctinfo->wAppInfoCount) {
			iAppInfoLen = MYSIZEOF(MtAppInfo);
			iTmpLen = MYSIZEOF(TWTlv) + iAppInfoLen;
			if(iUseBufLen+iTmpLen > iMaxBufLen)
				break;

			INFO_LOG("add app info to client appid:%u seq:%u", pAppInfo->iAppId, pAppInfo->dwSeq);

			APPINFO_CHECK_COPY_INFO;

			iUseBufLen += SetWTlv((TWTlv*)(pRespTlvBody+iUseBufLen), TLV_MONI_CONFIG_ADD,
				iAppInfoLen, (const char*)(&stAppInfo));
			pRespTlvBody->bTlvNum++;
			iAddCount++;
			iTmpLen = 0;
		}

		if(iTmpLen > 0) // 缓冲区不足了
			break;

		if(pAppInfo->iNextIndex >= 0 && pAppInfo->iNextIndex < MAX_SLOG_APP_COUNT)
			pAppInfo = m_pAppInfo->stInfo+pAppInfo->iNextIndex;
		else
			pAppInfo = NULL;
	}

	return iTmpLen;
}

int CUdpSock::SetAppInfoCheck(ContentCheckAppInfo *pctinfo)
{
	static char sRespBuf[1200];
	memset(sRespBuf, 0, sizeof(sRespBuf));

	TPkgBody *pRespTlvBody = (TPkgBody*)sRespBuf;
	int iUseBufLen = MYSIZEOF(TPkgBody), i=0, j=0;

	pctinfo->wAppInfoCount = ntohs(pctinfo->wAppInfoCount);
	for(i=0; i < pctinfo->wAppInfoCount; i++) {
#define APP_INFO_CHECK_32_TO_LOCAL(field) \
		pctinfo->stAppList[i].field = ntohl(pctinfo->stAppList[i].field);
		APP_INFO_CHECK_32_TO_LOCAL(iAppId);
		APP_INFO_CHECK_32_TO_LOCAL(dwSeq);
		APP_INFO_CHECK_32_TO_LOCAL(dwCfgFlag);
#undef APP_INFO_CHECK_32_TO_LOCAL 
		CLEAR_BIT(pctinfo->stAppList[i].dwCfgFlag, MONI_CONFIG_FLAG_CHECKED);
	}

	int iAddCount = 0, iModCount = 0, iDelCount = 0, iSameCount = 0, iTmpLen = 0;

	// 检查私有 app 配置情况
	if(stConfig.psysConfig->wAppInfoCount > 0)
		iTmpLen = InnerDealAppInfoCheck(stConfig.psysConfig->iAppInfoIndexStart, stConfig.psysConfig->wAppInfoCount, 
			pctinfo, iAddCount, iModCount, iSameCount, iUseBufLen, (int)MYSIZEOF(sRespBuf), pRespTlvBody);
	
	// 检查是否有被删除的 app, 通过检查处理标志是否设置实现
	uint32_t iAppIdDel = 0;
	for(j=0; iTmpLen <= 0 && j < pctinfo->wAppInfoCount; j++) 
	{
		if(IS_SET_BIT(pctinfo->stAppList[j].dwCfgFlag, MONI_CONFIG_FLAG_CHECKED))
			continue;

		iTmpLen = MYSIZEOF(int32_t)+MYSIZEOF(TWTlv);
		if(iUseBufLen+iTmpLen > (int)MYSIZEOF(sRespBuf))
			break;

		iAppIdDel = htonl(pctinfo->stAppList[j].iAppId);
		iUseBufLen += SetWTlv((TWTlv*)((char*)pRespTlvBody+iUseBufLen),
			TLV_MONI_CONFIG_DEL, MYSIZEOF(iAppIdDel), (const char*)(&iAppIdDel));
		INFO_LOG("delete app info appid:%d", pctinfo->stAppList[j].iAppId);
		pRespTlvBody->bTlvNum++;
		iDelCount++;
		iTmpLen = 0;
	}

	if(iTmpLen > 0)
		WARN_LOG("need more space %d > %u", iUseBufLen+iTmpLen, MYSIZEOF(sRespBuf));

	if(m_pMtClient->bEnableEncryptData) {
		static char sContentBuf[2048+256];
		int iSigBufLen = ((iUseBufLen>>4)+1)<<4;
		if(iSigBufLen > (int)sizeof(sContentBuf)) {
			ERR_LOG("need more space %d > %d", iSigBufLen, (int)sizeof(sContentBuf));
			return ERR_SERVER;
		}
		aes_cipher_data((const uint8_t*)pRespTlvBody, iUseBufLen,
			(uint8_t*)sContentBuf, (const uint8_t*)m_pMtClient->sRandKey, AES_128);
		InitCmdContent(sContentBuf, iSigBufLen);
	}
	else {
		InitCmdContent(pRespTlvBody, iUseBufLen);
	}

	INFO_LOG("check app config count add:%d mod:%d del:%d not change:%d", iAddCount, iModCount, iDelCount, iSameCount);
	return NO_ERROR;
}

int CUdpSock::DealCmdCheckAppInfo()
{
	if(m_wCmdContentLen < MYSIZEOF(ContentCheckAppInfo)) {
		REQERR_LOG("invalid cmd content %u < %u", m_wCmdContentLen, MYSIZEOF(ContentCheckAppInfo));
		return ERR_INVALID_CMD_CONTENT;
	}

	int iRet = 0;
	if((iRet=DealCommInfo()) != NO_ERROR)
		return iRet;

	ContentCheckAppInfo *pctinfo = (ContentCheckAppInfo*)m_pstCmdContent;
	if(m_wCmdContentLen != MYSIZEOF(ContentCheckAppInfo)
		+ MYSIZEOF(pctinfo->stAppList[0])*ntohs(pctinfo->wAppInfoCount)){
		REQERR_LOG("check cmd content failed %d != %u", m_wCmdContentLen,
			MYSIZEOF(ContentCheckAppInfo)+MYSIZEOF(uint32_t)*ntohs(pctinfo->wAppInfoCount));
		return ERR_INVALID_CMD_CONTENT;
	}

	DEBUG_LOG("get check app info, server response time:%u ms , app info count:%d",
		ntohl(pctinfo->dwServerResponseTime), ntohs(pctinfo->wAppInfoCount));
	if((int)ntohl(pctinfo->dwServerResponseTime) > m_pMtClient->iServerResponseTimeMs) {
		m_pMtClient->iServerResponseTimeMs = ntohl(pctinfo->dwServerResponseTime);
		INFO_LOG("update max server response time to:%u", m_pMtClient->iServerResponseTimeMs);
	}

	if((iRet=SetAppInfoCheck(pctinfo)) != NO_ERROR)
		return iRet;
	AckToReq(NO_ERROR); 
	return NO_ERROR;
}

int CUdpSock::SetSystemConfigCheck(ContentCheckSystemCfgReq *pctinfo)
{
	static char sRespBuf[1024];
	memset(sRespBuf, 0, sizeof(sRespBuf));

	MtSystemConfigClient *pResp = (MtSystemConfigClient*)sRespBuf;
	pResp->dwConfigSeq = htonl(stConfig.psysConfig->dwConfigSeq);

	// system config 
	if(stConfig.psysConfig->dwConfigSeq != ntohl(pctinfo->dwConfigSeq))
	{
		pResp->wHelloRetryTimes = htons(stConfig.psysConfig->wHelloRetryTimes);
		pResp->wHelloPerTimeSec = htons(stConfig.psysConfig->wHelloPerTimeSec);
		pResp->wCheckLogPerTimeSec = htons(stConfig.psysConfig->wCheckLogPerTimeSec);
		pResp->wCheckAppPerTimeSec = htons(stConfig.psysConfig->wCheckAppPerTimeSec);
		pResp->wCheckServerPerTimeSec = htons(stConfig.psysConfig->wCheckServerPerTimeSec);
		pResp->wCheckSysPerTimeSec = htons(stConfig.psysConfig->wCheckSysPerTimeSec);
		pResp->bAttrSendPerTimeSec = stConfig.psysConfig->bAttrSendPerTimeSec;
		pResp->bLogSendPerTimeSec = stConfig.psysConfig->bLogSendPerTimeSec;
		INFO_LOG("system config change old seq:%u new seq:%u", 
			ntohl(pctinfo->dwConfigSeq), stConfig.psysConfig->dwConfigSeq);
	}
	else  {
		DEBUG_LOG("system config not change - seq:%u", stConfig.psysConfig->dwConfigSeq);
	}

	if(m_pMtClient->bEnableEncryptData) {
		static char sContentBuf[1024+256];
		int iSigBufLen = ((sizeof(*pResp)>>4)+1)<<4;
		if(iSigBufLen > (int)sizeof(sContentBuf)) {
			ERR_LOG("need more space %d > %d", iSigBufLen, (int)sizeof(sContentBuf));
			return ERR_SERVER;
		}
		aes_cipher_data((const uint8_t*)pResp,
			sizeof(*pResp), (uint8_t*)sContentBuf, (const uint8_t*)m_pMtClient->sRandKey, AES_128);
		InitCmdContent(sContentBuf, iSigBufLen);
	}
	else {
		InitCmdContent(pResp, sizeof(*pResp));
	}
	return NO_ERROR;
}

int CUdpSock::DealCmdCheckSystemConfig()
{
	if(m_wCmdContentLen != MYSIZEOF(ContentCheckSystemCfgReq)) {
		REQERR_LOG("invalid cmd content %u < %u", m_wCmdContentLen, MYSIZEOF(ContentCheckSystemCfgReq));
		return ERR_INVALID_CMD_CONTENT;
	}

	int iRet = 0;
	if((iRet=DealCommInfo()) != NO_ERROR)
		return iRet;

	ContentCheckSystemCfgReq *pctinfo = (ContentCheckSystemCfgReq*)m_pstCmdContent;
	DEBUG_LOG("get check system config, seq:%u", ntohl(pctinfo->dwConfigSeq));

	if((int)ntohl(pctinfo->dwServerResponseTime) > m_pMtClient->iServerResponseTimeMs) {
		m_pMtClient->iServerResponseTimeMs = ntohl(pctinfo->dwServerResponseTime);
		INFO_LOG("update max server response time to:%u", m_pMtClient->iServerResponseTimeMs);
	}

	if((iRet=SetSystemConfigCheck(pctinfo)) != NO_ERROR)
		return iRet;

	AckToReq(NO_ERROR); 
	return NO_ERROR;
}

int CUdpSock::SetHelloTimeToMachineTable(int iRespMs)
{
    IM_SQL_PARA* ppara = NULL;
    if(InitParameter(&ppara) < 0) { 
        ERR_LOG("sql parameter init failed !");
        return SLOG_ERROR_LINE;
    }    
    AddParameter(&ppara, "last_hello_time", stConfig.dwCurrentTime, "DB_CAL");
    AddParameter(&ppara, "server_response_time_ms", iRespMs, "DB_CAL");

    MyQuery myqu(stConfig.qu, stConfig.db);
    Query & qu = myqu.GetQuery();

    std::string strSql;
    strSql = "update mt_machine set";
    JoinParameter_Set(&strSql, qu.GetMysql(), ppara);
    strSql += " where xrk_id=";
    strSql += itoa(m_iRemoteMachineId);
    ReleaseParameter(&ppara);
    if(!qu.execute(strSql)) {
        ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
        return SLOG_ERROR_LINE;
    }    
    DEBUG_LOG("set machine hello time for machine id:%d ok", m_iRemoteMachineId);
    return 0;
}

int CUdpSock::DealCmdReportPluginInfo()
{
    std::list<MonitorPluginCheckResult> stPluginCheck;

    if(m_wCmdContentLen <= MYSIZEOF(MonitorRepPluginInfoContent)) {
        REQERR_LOG("invalid cmd content %u != %u", m_wCmdContentLen, MYSIZEOF(MonitorRepPluginInfoContent));
        return ERR_INVALID_CMD_CONTENT;
    }    

    int iRet = 0; 
    if((iRet=DealCommInfo()) != NO_ERROR)
        return iRet;

    MonitorRepPluginInfoContent *pctinfo = (MonitorRepPluginInfoContent*)m_pstCmdContent;
    DEBUG_LOG("report plugin info, plugin count:%d", pctinfo->bPluginCount);

    uint8_t *pInfo = (uint8_t*)(pctinfo->plugins), iItemLen = 0; 
    TRepPluginInfoFirst *pstFirst;
    TRepPluginInfo *pstInfo;
    uint16_t wLen = m_wCmdContentLen-sizeof(MonitorRepPluginInfoContent);
    int i = 0; 

    MyQuery myqu(stConfig.qu, stConfig.db);
    Query & qu = myqu.GetQuery();
    MonitorPluginCheckResult stCheck;
	const char *ptmp = NULL;
    for(i=0; wLen > 0 && i < pctinfo->bPluginCount; i++, pInfo += 1+iItemLen, wLen -= 1+iItemLen) {
        iItemLen = *pInfo;
        memset(&stCheck, 0, sizeof(stCheck));
        if(*pInfo == sizeof(*pstInfo)) {
            pstInfo = (TRepPluginInfo*)(pInfo+1);
            pstInfo->iPluginId = ntohl(pstInfo->iPluginId);
            pstInfo->dwLastReportAttrTime = ntohl(pstInfo->dwLastReportAttrTime);
            pstInfo->dwLastReportLogTime = ntohl(pstInfo->dwLastReportLogTime);
            pstInfo->dwLastHelloTime = ntohl(pstInfo->dwLastHelloTime);
            if(pstInfo->dwLastHelloTime == 0)
                pstInfo->dwLastHelloTime = slog.m_stNow.tv_sec;
            pstInfo->dwConfigFileTime = ntohl(pstInfo->dwConfigFileTime);

			stCheck.iPluginId = pstInfo->iPluginId;

			std::ostringstream ss;
            ss << "select cfg_file_time from mt_plugin_machine where machine_id=" << m_iRemoteMachineId;
            ss << " and open_plugin_id=" << pstInfo->iPluginId << " and xrk_status=0 ";
			qu.get_result(ss.str());
			qu.fetch_row();
            if(pstInfo->dwConfigFileTime != qu.getuval("cfg_file_time")) {
				DEBUG_LOG("plugin:%d, config need report:%u != %u", 
					stCheck.iPluginId, pstInfo->dwConfigFileTime, qu.getuval("cfg_file_time"));
				stCheck.bNeedReportCfg = 1;
			}
			qu.free_result();

        	ss.str("");
            ss << "update mt_plugin_machine set last_hello_time=" << pstInfo->dwLastHelloTime << ", install_proc=0";
            if(pstInfo->dwLastReportAttrTime > 0)
                ss << ", last_attr_time=" << pstInfo->dwLastReportAttrTime;
            if(pstInfo->dwLastReportLogTime > 0)
                ss << ", last_log_time=" << pstInfo->dwLastReportLogTime;
            ss << " where machine_id=" << m_iRemoteMachineId;
            ss << " and open_plugin_id=" << pstInfo->iPluginId;
            qu.execute(ss.str().c_str());
			if(qu.affected_rows() < 1) {
				WARN_LOG("update failed, affected_rows < 1, plugin:%d", pstInfo->iPluginId);
			}
            stCheck.bCheckResult = 0;
            stPluginCheck.push_back(stCheck);
            DEBUG_LOG("report plugin info ok - plugin:%d", pstInfo->iPluginId);
        }else if(*pInfo > sizeof(*pstFirst))  {
            pstFirst = (TRepPluginInfoFirst*)(pInfo+1);
            pstFirst->iPluginId = ntohl(pstFirst->iPluginId);
            pstFirst->iLibVerNum = ntohl(pstFirst->iLibVerNum);
            pstFirst->dwLastReportAttrTime = ntohl(pstFirst->dwLastReportAttrTime);
            pstFirst->dwLastReportLogTime = ntohl(pstFirst->dwLastReportLogTime);
            pstFirst->dwPluginStartTime = ntohl(pstFirst->dwPluginStartTime);
            pstFirst->dwLastHelloTime = ntohl(pstFirst->dwLastHelloTime);
            if(pstFirst->dwLastHelloTime == 0)
                pstFirst->dwLastHelloTime = slog.m_stNow.tv_sec;

            pstFirst->dwConfigFileTime = ntohl(pstFirst->dwConfigFileTime);
            stCheck.iPluginId = pstFirst->iPluginId;

            //  合法性校验, 是否安装，部署名是否匹配
			std::ostringstream ss;
			ss << "select plugin_name from mt_plugin where open_plugin_id=" << pstFirst->iPluginId
				<< " and xrk_status=0";
			if(qu.get_result(ss.str().c_str()) && qu.num_rows() > 0) {
				qu.fetch_row();
				ptmp = qu.getstr("plugin_name");
				if(pstFirst->bPluginNameLen < 1 || 
					(ptmp && strncmp(ptmp, pstFirst->sPluginName, pstFirst->bPluginNameLen)))
				{
					pstFirst->sPluginName[pstFirst->bPluginNameLen-1] = '\0';
					WARN_LOG("plugin:%d, name not match:%s, %s", pstFirst->iPluginId, ptmp, pstFirst->sPluginName); 
					stCheck.bCheckResult = 1; 
					stPluginCheck.push_back(stCheck); 
					qu.free_result();
					continue;
				}
			}
			else {
				WARN_LOG("not find plugin:%d", pstFirst->iPluginId);
				stCheck.bCheckResult = 1; 
				stPluginCheck.push_back(stCheck);
				qu.free_result();
				continue;
			}
			qu.free_result();

			ss.str("");
            ss << "select xrk_id,cfg_file_time from mt_plugin_machine where machine_id=" << m_iRemoteMachineId;
            ss << " and open_plugin_id=" << pstFirst->iPluginId << " and xrk_status=0 ";
            if(qu.get_result(ss.str().c_str()) && qu.num_rows() > 0) {
                qu.fetch_row();
                int id = qu.getval("xrk_id");
                if(pstFirst->dwConfigFileTime != qu.getuval("cfg_file_time"))
                    stCheck.bNeedReportCfg = 1;
                qu.free_result();

                ss.str("");
                ss << "update mt_plugin_machine set install_proc=0,last_hello_time=" << pstFirst->dwLastHelloTime;
                if(pstFirst->dwLastReportAttrTime > 0)
                    ss << ", last_attr_time=" << pstFirst->dwLastReportAttrTime;
                if(pstFirst->dwLastReportLogTime > 0)
                    ss << ", last_log_time=" << pstFirst->dwLastReportLogTime;
                ss << ", lib_ver_num=" << pstFirst->iLibVerNum;
                ss << ", plugin_version=\'" << pstFirst->szVersion << "\'";
                ss << ", start_time=" << pstFirst->dwPluginStartTime << " where xrk_id=" << id;
                qu.execute(ss.str().c_str());

                if(pstFirst->dwLastReportAttrTime > 0) {
                    ss.str("");
                    ss << "update mt_plugin_machine set start_attr_time=" << pstFirst->dwLastReportAttrTime
                        << " where start_attr_time=0 and xrk_id=" << id;
                    qu.execute(ss.str().c_str());
                }
            }
            else {
                qu.free_result();

                ss.str("");
                ss << "insert into mt_plugin_machine set last_attr_time=" << pstFirst->dwLastReportAttrTime;
                ss << ", last_log_time=" << pstFirst->dwLastReportLogTime;
                ss << ", last_hello_time=" << pstFirst->dwLastHelloTime;
                ss << ", lib_ver_num=" << pstFirst->iLibVerNum;
                ss << ", start_time=" << pstFirst->dwPluginStartTime;
                ss << ", machine_id=" << m_iRemoteMachineId;
                ss << ", open_plugin_id=" << pstFirst->iPluginId;
				ss << ", local_cfg_url=\'server add\'";
                ss << ", plugin_version=\'" << pstFirst->szVersion << "\'";
                qu.execute(ss.str().c_str());
                stCheck.bNeedReportCfg = 1;
            }
            DEBUG_LOG("first report plugin info ok - plugin:%d", pstFirst->iPluginId);
            stCheck.bCheckResult = 0;
            stPluginCheck.push_back(stCheck);
        }
        else {
            REQERR_LOG("invalid report plugin info, len:%d", *pInfo);
            break;
        }
    }
    if(i < pctinfo->bPluginCount || wLen > 0) {
        REQERR_LOG("report plugin check failed, count:%d<%d, len:%u", i, pctinfo->bPluginCount, wLen);
        return ERR_INVALID_PACKET;
    }

    // 成功响应
    char sRspBuf[512] = {0};
    MonitorRepPluginInfoContentResp *pstResp = (MonitorRepPluginInfoContentResp*)sRspBuf;
    MonitorPluginCheckResult *pRlt = (MonitorPluginCheckResult*)(sRspBuf+sizeof(MonitorRepPluginInfoContentResp));
	pstResp->bPluginCount=(int)(stPluginCheck.size());
    std::list<MonitorPluginCheckResult>::iterator it = stPluginCheck.begin();
    int iOk = 0, iFail = 0, iNeedCfg = 0;
	for(; it != stPluginCheck.end(); it++) {
        pRlt->iPluginId = htonl(it->iPluginId);
        pRlt->bCheckResult = it->bCheckResult;
        pRlt->bNeedReportCfg = it->bNeedReportCfg;
        pRlt++;
        if(it->bCheckResult)
            iFail++;
        else
            iOk++;
        if(it->bNeedReportCfg)
            iNeedCfg++;
    }
    wLen = sizeof(MonitorRepPluginInfoContentResp)+sizeof(MonitorPluginCheckResult)*pstResp->bPluginCount;

	if(m_pMtClient->bEnableEncryptData) {
		static char sContentBuf[1024+256];
		int iSigBufLen = ((wLen>>4)+1)<<4;
		if(iSigBufLen > (int)sizeof(sContentBuf)) {
			ERR_LOG("need more space %d > %d", iSigBufLen, (int)sizeof(sContentBuf));
			return ERR_SERVER;
		}
		aes_cipher_data((const uint8_t*)pstResp,
			wLen, (uint8_t*)sContentBuf, (const uint8_t*)m_pMtClient->sRandKey, AES_128);
		InitCmdContent(sContentBuf, iSigBufLen);
	}
	else {
		InitCmdContent(pstResp, wLen);
	}

    DEBUG_LOG("report plugin info, plugin count:%d, ok:%d, failed:%d, content len:%u",
        pctinfo->bPluginCount, iOk, iFail, wLen);
    AckToReq(NO_ERROR);
    return NO_ERROR;
}

int CUdpSock::DealCmdReportPluginCfg()
{
    static std::string s_plugCfgStr("DES-PWE_cfgs:");
	if(m_wCmdContentLen <= MYSIZEOF(CmdSendPluginConfigContent)) {
		REQERR_LOG("invalid cmd content %u != %u", m_wCmdContentLen, MYSIZEOF(CmdSendPluginConfigContent));
		return ERR_INVALID_CMD_CONTENT;
	}

	int iRet = 0;
	if((iRet=DealCommInfo()) != NO_ERROR)
		return iRet;

	CmdSendPluginConfigContent *pctinfo = (CmdSendPluginConfigContent*)m_pstCmdContent;
    pctinfo->iPluginId = ntohl(pctinfo->iPluginId);
    pctinfo->iConfigLen = ntohl(pctinfo->iConfigLen);

    if(m_wCmdContentLen != pctinfo->iConfigLen) {
        REQERR_LOG("check length failed, %d != %d", m_wCmdContentLen, pctinfo->iConfigLen);
        return ERR_INVALID_PACKET;
    }

    // strCfgs 格式：DES-PWE_cfgs:最后修改时间;CFG_ITEM CFG_ITEM_VAL;CFG_ITEM CFG_ITEM_VAL;CFG_ITEM CFG_ITEM_VAL;...
    int iCfgLen = pctinfo->iConfigLen - (int)sizeof(CmdSendPluginConfigContent);
    pctinfo->strCfgs[iCfgLen-1] = '\0';
    if(strncmp(pctinfo->strCfgs, s_plugCfgStr.c_str(), s_plugCfgStr.size())) {
        REQERR_LOG("check plugin config failed, start string:%s not match", s_plugCfgStr.c_str());
        return ERR_INVALID_PACKET;
    }
	DEBUG_LOG("report plugin config info, plugin:%d, cfgs:%s", pctinfo->iPluginId, pctinfo->strCfgs);

    // 提取插件配置最后修改时间
    uint32_t dwLastModTime = 0;
    char *ptmp = pctinfo->strCfgs+s_plugCfgStr.size();
    char *ptmp_e = strchr(pctinfo->strCfgs, ';');
    if(ptmp_e != NULL) {
        *ptmp_e = '\0';
        dwLastModTime = strtoul(ptmp, NULL, 10);
        ptmp = ptmp_e+1;
    }
    else {
        dwLastModTime = strtoul(ptmp, NULL, 10);
        *ptmp = '\0';
    }
    std::ostringstream ss;

    // 提取全部可修改的配置项
    char *pitem_name = NULL, *pitem_val = NULL;
    for(int i=0; *ptmp != '\0' && i >= 0;) {
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
            if(ss.str().size() > 0)
                ss << ";" << pitem_name << " " << pitem_val;
            else
                ss << pitem_name << " " << pitem_val;
        }
    }

	MyQuery myqu(stConfig.qu, stConfig.db);
	Query & qu = myqu.GetQuery();

    IM_SQL_PARA* ppara = NULL;
    InitParameter(&ppara);
    AddParameter(&ppara, "cfg_file_time", dwLastModTime, "DB_CAL");
    AddParameter(&ppara, "xrk_cfgs_list", ss.str().c_str(), NULL);
    std::string strSql;
    strSql = "update mt_plugin_machine set";
    JoinParameter_Set(&strSql, qu.GetMysql(), ppara);
    strSql += " where open_plugin_id=";
    strSql += itoa(pctinfo->iPluginId);
    strSql += " and xrk_status=0";
    strSql += " and machine_id=";
    strSql += itoa(m_iRemoteMachineId);
    ReleaseParameter(&ppara);
    if(!qu.execute(strSql))
    {
        ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
        return SLOG_ERROR_LINE;
    }

	// 成功响应
    char sRspBuf[64] = {0};
	CmdSendPluginConfigContentResp *pstResp = (CmdSendPluginConfigContentResp*)sRspBuf;
    pstResp->iPluginId = ntohl(pctinfo->iPluginId);
    pstResp->dwLastModConfigTime = ntohl(dwLastModTime);

    uint16_t wLen = sizeof(*pstResp);
	if(m_pMtClient->bEnableEncryptData) {
		static char sContentBuf[1024+256];
		int iSigBufLen = ((wLen>>4)+1)<<4;
		if(iSigBufLen > (int)sizeof(sContentBuf)) {
			ERR_LOG("need more space %d > %d", iSigBufLen, (int)sizeof(sContentBuf));
			return ERR_SERVER;
		}
		aes_cipher_data((const uint8_t*)pstResp,
			wLen, (uint8_t*)sContentBuf, (const uint8_t*)m_pMtClient->sRandKey, AES_128);
		InitCmdContent(sContentBuf, iSigBufLen);
	}
	else {
		InitCmdContent(pstResp, wLen);
	}

	AckToReq(NO_ERROR); 
	return NO_ERROR;
}

int CUdpSock::DealCmdModMachPluginCfgResp()
{
	if(m_wCmdContentLen != MYSIZEOF(CmdS2cModMachPluginCfgResp)) {
		REQERR_LOG("invalid cmd content %u != %u", m_wCmdContentLen, MYSIZEOF(CmdS2cModMachPluginCfgResp));
		return ERR_INVALID_CMD_CONTENT;
	}

	int iRet = 0;
	if((iRet=DealCommInfo()) != NO_ERROR)
		return iRet;

	CmdS2cModMachPluginCfgResp *pctinfo = (CmdS2cModMachPluginCfgResp*)m_pstCmdContent;
    pctinfo->iPluginId = ntohl(pctinfo->iPluginId);
    pctinfo->iMachineId = ntohl(pctinfo->iMachineId);
    pctinfo->dwDownCfgTime = ntohl(pctinfo->dwDownCfgTime);

	MyQuery myqu(stConfig.qu, stConfig.db);
	Query & qu = myqu.GetQuery();

    // session 校验，校验通过后才能操作数据库
    TMachOprPluginSess *psess_data = (TMachOprPluginSess*)GetSessData();
    if(!psess_data || psess_data->iPluginId != pctinfo->iPluginId
        || psess_data->iMachineId != pctinfo->iMachineId) 
    {
        REQERR_LOG("udp session check failed:%p, plugin:%d, machine:%d", 
            psess_data, pctinfo->iPluginId, pctinfo->iMachineId);
        return ERR_INVALID_PACKET;
    }

    std::ostringstream ss;
    ss << "update mt_plugin_machine set opr_proc=" << EV_MOP_OPR_SUCCESS
        << " where xrk_id=" << psess_data->iDbId << " and opr_start_time=" 
        << pctinfo->dwDownCfgTime << " and xrk_status=0"; 
    qu.execute(ss.str().c_str());
    DEBUG_LOG("modify plugin config ok, plugin:%d, machine:%d", pctinfo->iPluginId, pctinfo->iMachineId);
    return 0;
}

int CUdpSock::DealCmdMachOprPluginResp()
{
	if(m_wCmdContentLen != MYSIZEOF(CmdS2cMachOprPluginResp)) {
		REQERR_LOG("invalid cmd content %u != %u", m_wCmdContentLen, MYSIZEOF(CmdS2cMachOprPluginResp));
		return ERR_INVALID_CMD_CONTENT;
	}

	int iRet = 0;
	if((iRet=DealCommInfo()) != NO_ERROR)
		return iRet;

	CmdS2cMachOprPluginResp *pctinfo = (CmdS2cMachOprPluginResp*)m_pstCmdContent;
    pctinfo->iPluginId = ntohl(pctinfo->iPluginId);
    pctinfo->iMachineId = ntohl(pctinfo->iMachineId);
    pctinfo->iDbId = ntohl(pctinfo->iDbId);
	DEBUG_LOG("machine:%d opr plugin:%d, result:%d", pctinfo->iMachineId, pctinfo->iPluginId, pctinfo->bOprResult);
    if(pctinfo->bOprResult > MACH_OPR_PLUGIN_RET_MAX)
    {
        REQERR_LOG("machine:%d opr plugin:%d, unknow result:%d",
            pctinfo->iMachineId, pctinfo->iPluginId, pctinfo->bOprResult);
        return ERR_INVALID_PACKET;
    }

	MyQuery myqu(stConfig.qu, stConfig.db);
	Query & qu = myqu.GetQuery();

    // session 校验，校验通过后才能操作数据库
    TMachOprPluginSess *psess_data = (TMachOprPluginSess*)GetSessData();
    if(!psess_data || psess_data->iPluginId != pctinfo->iPluginId ||
        psess_data->iMachineId != pctinfo->iMachineId || psess_data->iDbId != pctinfo->iDbId)
    {
        REQERR_LOG("udp session check failed:%p, plugin:%d, machine:%d, dbid:%d", psess_data,
            pctinfo->iPluginId, pctinfo->iMachineId, pctinfo->iDbId);
        return ERR_INVALID_PACKET;
    }

    std::ostringstream ss;
    if(pctinfo->bOprResult != MACH_OPR_PLUGIN_SUCCESS) {
        if(pctinfo->bOprResult == MACH_OPR_PLUGIN_NOT_FIND)
            ss << "update mt_plugin_machine set opr_proc=" << EV_MOP_OPR_FAILED << ",xrk_status=1";
        else
            ss << "update mt_plugin_machine set opr_proc=" << EV_MOP_OPR_FAILED;
        WARN_LOG("machine opr plugin failed, plugin:%d, machine:%d, opr event:%d, result:%d",
            pctinfo->iPluginId, pctinfo->iMachineId, psess_data->iEventType, pctinfo->bOprResult);
    }
    else  {
        ss << "update mt_plugin_machine set opr_proc=" << EV_MOP_OPR_SUCCESS;
        if(psess_data->iEventType == EVENT_MULTI_MACH_PLUGIN_REMOVE)
            ss << ",xrk_status=1";
        else if(psess_data->iEventType == EVENT_MULTI_MACH_PLUGIN_ENABLE)
            ss << ",last_hello_time=" << slog.m_stNow.tv_sec;
        else if(psess_data->iEventType == EVENT_MULTI_MACH_PLUGIN_DISABLE)
            ss << ",last_hello_time=0";
    }
    ss << " where xrk_id=" << pctinfo->iDbId << " and xrk_status=0"; 
    qu.execute(ss.str().c_str());
    return 0;
}

int CUdpSock::DealCmdPreInstallReport()
{
	if(m_wCmdContentLen != MYSIZEOF(CmdPreInstallReportContent)) {
		REQERR_LOG("invalid cmd content %u != %u", m_wCmdContentLen, MYSIZEOF(CmdPreInstallReportContent));
		return ERR_INVALID_CMD_CONTENT;
	}

	int iRet = 0;
	if((iRet=DealCommInfo()) != NO_ERROR)
		return iRet;

	CmdPreInstallReportContent *pctinfo = (CmdPreInstallReportContent*)m_pstCmdContent;
    pctinfo->iPluginId = ntohl(pctinfo->iPluginId);
    pctinfo->iMachineId = ntohl(pctinfo->iMachineId);
    pctinfo->iDbId = ntohl(pctinfo->iDbId);
    pctinfo->iStatus = ntohl(pctinfo->iStatus);
	pctinfo->sDevLang[sizeof(pctinfo->sDevLang)-1] = '\0';
    if(m_pConfig->stSysCfg.sPreInstallCheckStr[0] != '\0'
        && !IsStrEqual(pctinfo->sCheckStr, m_pConfig->stSysCfg.sPreInstallCheckStr)) {
        REQERR_LOG("check preinstall string failed !");
        return ERR_INVALID_PACKET;
    }
	DEBUG_LOG("report preinstall status, plugin:%d, status:%d", pctinfo->iPluginId, pctinfo->iStatus);

    if((pctinfo->iStatus < EV_PREINSTALL_TO_CLIENT_OK || pctinfo->iStatus > EV_PREINSTALL_STATUS_MAX)
        && (pctinfo->iStatus < EV_PREINSTALL_ERR_MIN || pctinfo->iStatus > EV_PREINSTALL_ERR_MAX))
    {
        REQERR_LOG("check preinstall status failed:%d (%d - %d or %d - %d)",
            pctinfo->iStatus, EV_PREINSTALL_TO_CLIENT_OK, EV_PREINSTALL_STATUS_MAX,
            EV_PREINSTALL_ERR_MIN, EV_PREINSTALL_ERR_MAX);
        return ERR_INVALID_PACKET;
    }

	MyQuery myqu(stConfig.qu, stConfig.db);
	Query & qu = myqu.GetQuery();

    std::ostringstream ss;
    if(IsStrEqual(pctinfo->sDevLang, "javascript") && pctinfo->iStatus == EV_PREINSTALL_CLIENT_INSTALL_PLUGIN_OK) {
        ss << "update mt_plugin_machine set install_proc=0,start_time=" << slog.m_stNow.tv_sec
			<< ",last_hello_time=" << slog.m_stNow.tv_sec << " where xrk_id=" << pctinfo->iDbId << " and xrk_status=0"; 
    }
    else {
        ss << "update mt_plugin_machine set install_proc=" << pctinfo->iStatus;
		if(pctinfo->iStatus == EV_PREINSTALL_CLIENT_INSTALL_PLUGIN_OK)
            ss << ", start_time=" << slog.m_stNow.tv_sec << ", last_hello_time=" << slog.m_stNow.tv_sec;
        ss << " where xrk_id=" << pctinfo->iDbId << " and xrk_status=0 and install_proc < " << pctinfo->iStatus;
    }
    qu.execute(ss.str().c_str());
    return 0;
}

void CUdpSock::OnRawData(const char *buf, size_t len, struct sockaddr *sa, socklen_t sa_len)
{
	int iRet = 0;
	Init();
	if((iRet=CheckBasicPacket(buf, len, sa)) != NO_ERROR) {
		AckToReq(iRet);
		return;
	}

	if(PacketPb()) {
		return;
	}

	switch(m_dwReqCmd) {
		case CMD_MONI_SEND_HELLO_FIRST:
			// 这里加个版本限制，以方便以后升级
			if(ntohs(m_pstReqHead->wVersion) != 1) 
			{
				REQERR_LOG("check version failed ! (%d != 1)", ntohs(m_pstReqHead->wVersion));
				iRet = ERR_INVALID_PACKET;
				break;
			}
			m_bIsFirstHello = true;
			iRet = DealCmdHelloFirst();
			break;

		case CMD_MONI_SEND_HELLO:
			iRet = DealCmdHello();
			break;

		case CMD_MONI_CHECK_LOG_CONFIG:
			iRet = DealCmdCheckLogConfig();
			break;

		case CMD_MONI_CHECK_APP_CONFIG:
			iRet = DealCmdCheckAppInfo();
			break;

		case CMD_MONI_CHECK_SYSTEM_CONFIG:
			iRet = DealCmdCheckSystemConfig();
			break;

		case CMD_MONI_SEND_PLUGIN_INFO:
			iRet = DealCmdReportPluginInfo();
			break;

		case CMD_MONI_PREINSTALL_REPORT:
			DealCmdPreInstallReport();
			break;

        case CMD_MONI_S2C_MACH_ORP_PLUGIN_REMOVE:
        case CMD_MONI_S2C_MACH_ORP_PLUGIN_ENABLE:
        case CMD_MONI_S2C_MACH_ORP_PLUGIN_DISABLE:
            DealCmdMachOprPluginResp();
            break;

        case CMD_SEND_PLUGIN_CONFIG:
            iRet = DealCmdReportPluginCfg();
            break;

        case CMD_MONI_S2C_MACH_ORP_PLUGIN_MOD_CFG:
            DealCmdModMachPluginCfgResp();
            break;

		default:
			REQERR_LOG("unknow cmd:%u", m_dwReqCmd);
			iRet = ERR_UNKNOW_CMD;
			break;
	}

	if(iRet != NO_ERROR)
		AckToReq(iRet);
}

#undef APPINFO_CHECK_COPY_INFO
#undef LOG_CHECK_COPY_CONFIG_INFO

