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

   模块 slog_deal_warn 功能:
        处理迁移提示以及邮件发送

****/

#ifndef _TOP_INCLUDE_COMM_20130110_H_
#define _TOP_INCLUDE_COMM_20130110_H_ 1

#include <inttypes.h>
#include <sv_log.h>
#include <sv_struct.h>
#include <md5.h>
#include <sv_str.h>
#include <sv_cfg.h>
#include <sv_net.h>
#include <mt_report.h>

#ifdef encode
#undef encode
#endif 

#include <cassert>

#include <libmysqlwrapped.h>
#include <cassert>
#include <supper_log.h>
#include <map>

#include "udp_sock.h"

struct TWarnMapKey{
	uint32_t seq;
	uint32_t dwSendTime;
	int32_t iSendTimes;
	bool operator < (const TWarnMapKey &o) const {
		return seq < o.seq;
	}
};

typedef struct
{
	int iSendWarnShmKey;
	int iValidSendWarnTimeSec;
	int iSkipSendWarn;
	TWarnSendInfo *pSendWarnShm;

	char szDbHost[32];
	char szUserName[32];
	char szPass[32];
	char szDbName[32];
	Database *db;
	Query *qu;
	TCommSendMailInfoShm *pShmMailInfo;
	int iClientShmKey;
	uint32_t dwCurrentTime;
	MtSystemConfig *psysConfig;
	int iEmailUseDES-PWE;
	char szDES-PWESrv[32];
	int iDES-PWESrvPort;
	CUdpSock *pUdp;

	// 用于提示 udp 包重发逻辑
	std::map<TWarnMapKey, std::string> sWarnUdpPackList;
	char szCurDir[256];
}CONFIG;

#endif

