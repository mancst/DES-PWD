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

   开发库 mtagent_api_open 说明:
        itc云框架系统内部公共库，提供各种公共的基础函数调用

****/

#ifndef __SUPPER_LOG_H__
#define __SUPPER_LOG_H__  (1)

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include "Socket.h"
#include "ISocketHandler.h"
#include "StdLog.h"
#include "sv_struct.h"
#include "FloginSession.h"
#include <libmysqlwrapped.h>
#include "pid_guard.h"
#include "Json.h"
#include "user.pb.h"
#include "top_proto.pb.h"
#include "sv_log.h"
#include "sv_cfg.h"
#include "sv_vmem.h"
#include "sv_shm.h"
#include "oi_bmh.h"
#include "sv_str.h"
#include "sv_freq_ctrl.h"
#include "sv_coredump.h"
#include "sv_net.h"
#include "sv_time.h"
#include "comm.pb.h"
#include "sv_struct.h"
#include "mt_shared_hash.h"

const std::string g_version_info("v3.4"); 
const std::string g_cmp_info("DES-PWE - compile at: " __DATE__ " " __TIME__); 
const std::string g_str_unknow("unknow");

#ifdef SOCKETS_NAMESPACE
using namespace SOCKETS_NAMESPACE;
#endif 

int IsStrEqual(const char *str1, const char *str2);
void GetOsType(const char *pos, std::string &osType);

#pragma pack(1)

// 通用邮件发送共享内存数据结构
#define MAX_COMM_SEND_MAIL_NODE_COUNT  2000
#define DEFAULT_MAILINFO_SHM_KEY 2018102007
typedef struct
{
	uint32_t dwMailSeq;
	uint32_t dwToUserId;
	uint32_t dwValidTimeUtc;
	char szToEmailAddr[64];
	char szEmailSubject[64];
	char szEmailContent[4096];
	uint32_t dwModifySeq; // 用于异步多进程修改，每次修改数据都改变该值
	char szReserved[12];
	void Show() {
		SHOW_FIELD_VALUE_UINT(dwMailSeq);
		SHOW_FIELD_VALUE_UINT(dwToUserId);
		SHOW_FIELD_VALUE_UINT_TIME(dwValidTimeUtc);
		SHOW_FIELD_VALUE_CSTR(szToEmailAddr);
		SHOW_FIELD_VALUE_CSTR(szEmailSubject);
		SHOW_FIELD_VALUE_CSTR(szEmailContent);
		SHOW_FIELD_VALUE_UINT(dwModifySeq);
	}
}TCommSendMailInfo;

typedef struct 
{
	volatile uint8_t bLockGetShm;
	TCommSendMailInfo stInfo[MAX_COMM_SEND_MAIL_NODE_COUNT];
	void Show() {
		SHOW_FIELD_VALUE_UINT(bLockGetShm);
		SHOW_FIELD_VALUE_STR_COUNT(MAX_COMM_SEND_MAIL_NODE_COUNT, dwMailSeq, MAX_COMM_SEND_MAIL_NODE_COUNT, stInfo);
	}
}TCommSendMailInfoShm;

// 系统实时信息展示用
typedef struct 
{
	uint32_t dwOtherInfoSeq;
    uint8_t bTryChangeFlag;
    int iOtherInfoLen;
	char szOtherInfoBin[2048];
    char szDES-PWEUrl[256];
    char sReserved[256];
	void Show() {
		SHOW_FIELD_VALUE_CSTR(szDES-PWEUrl);
		SHOW_FIELD_VALUE_UINT(dwOtherInfoSeq);
        SHOW_FIELD_VALUE_UINT(bTryChangeFlag);
		SHOW_FIELD_VALUE_INT(iOtherInfoLen);
        user::SystemOtherInfo stOther;
        if(iOtherInfoLen > 0 && !stOther.ParseFromArray(szOtherInfoBin, iOtherInfoLen)) {
            printf("ParseFromArray failed, len:%d\n", iOtherInfoLen);
        }
        else if(iOtherInfoLen > 0)
            printf("other info:%s\n", stOther.ShortDebugString().c_str());
        else
            printf("have no other info !\n");
	}
}TOtherInfoShm;

// 属性提示相关结构 ----------- start

// 可能是最老的存储方式，未划分统计周期数据为(1440*uint32)
#define ATTR_DAY_BIN_TYPE_NONE 0
// 数组带统计周期 (cType+wStaticTime+arrVal(uint32_t[iMaxAttrCountIdx])) 
#define ATTR_DAY_BIN_TYPE_ARRAY_V2 1 
// 压缩带统计周期 (cType+wStaticTime+wCount+arrIdx(uint16_t[wCount]) + arrVal(uint32_t[wCount]))
#define ATTR_DAY_BIN_TYPE_PRESS_V2 2 

inline int GetStaticTimeMaxIdxOfDay(int iStaticTime)
{
    if(!iStaticTime)
        return 1440;
	return 1440/iStaticTime + ((1440%iStaticTime) ? 1 : 0);
}

enum {
	MACHINE_WARN_FLAG_MIN=1,
	MACH_WARN_ALLOW_ALL=1,
	MACH_WARN_DENY_ALL=2,
	MACH_WARN_DENY_BASIC=3,
	MACH_WARN_DENY_EXCEPT=4,
	MACHINE_WARN_FLAG_MAX=4,
};
#define INVALID_MACHINE_WARN_FLAG(t) (t<MACHINE_WARN_FLAG_MIN || t>MACHINE_WARN_FLAG_MAX)

#define PLUGIN_PARENT_APP_ID 119
#define PLUGIN_PARENT_ATTR_TYPE 84

#define WARN_CONFIG_TYPE_VIEW 1
#define WARN_CONFIG_TYPE_MACHINE 2
#define WARN_CONFIG_HASH_NODE_COUNT 100031 //全局配置
#define DEF_WARN_CONFIG_SHM_KEY 2018080101
typedef struct
{
	int32_t iWarnId; // iWarnId -- 0, machine id, view id
	int32_t iAttrId; 
	int32_t iWarnMax;
	int32_t iWarnMin;
	int32_t iWarnWave;
	int32_t iWarnConfigFlag;
	int32_t iWarnConfigId;

	uint32_t dwLastMinWarnTimeSec; // for exception warn last warn time sec
	uint32_t dwLastMaxWarnTimeSec;
	uint32_t dwLastWaveWarnTimeSec;

	uint32_t dwReserved1; // for view warn dwPreLastVal
	uint32_t dwReserved2; // for view warn dwLastVal
	uint32_t dwReserved3; // for view warn last add report time 
	uint32_t dwLastUpdateTime;
	uint32_t dwReserved5;

	char sReserved[16];

	void Show() {
		SHOW_FIELD_VALUE_INT(iWarnId);
		SHOW_FIELD_VALUE_INT(iAttrId);
		SHOW_FIELD_VALUE_INT(iWarnMax);
		SHOW_FIELD_VALUE_INT(iWarnMin);
		SHOW_FIELD_VALUE_INT(iWarnWave);
		SHOW_FIELD_VALUE_INT(iWarnConfigFlag);
		SHOW_FIELD_VALUE_INT(iWarnConfigId);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastMinWarnTimeSec);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastMaxWarnTimeSec);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastWaveWarnTimeSec);
		SHOW_FIELD_VALUE_UINT(dwReserved1);
		SHOW_FIELD_VALUE_UINT(dwReserved2);
		SHOW_FIELD_VALUE_UINT_TIME(dwReserved3);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastUpdateTime);
	}
}TWarnConfig;

#define WARN_ATTR_HASH_NODE_COUNT 10031 // 单机
#define DEF_WARN_ATTR_SHM_KEY 2018072501
typedef struct _TWarnAttrReportInfo
{
	int32_t iAttrId; // 上报属性 id
	int32_t iMachineId; // 上报机器 id
	uint32_t dwPreLastVal; // 最近2分钟统计完成的上报
	uint32_t dwLastVal; // 最近1分钟统计完成的上报
	uint32_t dwCurVal; // 当前时间上报 [min]
	int32_t iMinIdx; // 最后一次上报的统计周期索引, 在 0-1439之间 
	uint32_t dwLastReportIp; // 最后一次上报数据的远程IP
	uint32_t dwLastReportTime;
	uint8_t bAttrDataType; // 数据类型
	uint16_t wStaticTime; // attr 的统计周期 在1-1439之间
	uint32_t dwLastToDbTime; // 最近一次写入 db 的时间(数据写db时一个统计周期只能写一次db)
	uint16_t wLastToDbIdx; // 最近一次写入 db 的统计周期索引
	uint32_t dwReserved3;

	char sReserved[16];

	void Show() {
		SHOW_FIELD_VALUE_INT(iAttrId);
		SHOW_FIELD_VALUE_INT(iMachineId);
		SHOW_FIELD_VALUE_UINT(dwPreLastVal);
		SHOW_FIELD_VALUE_UINT(dwLastVal);
		SHOW_FIELD_VALUE_UINT(dwCurVal);
		SHOW_FIELD_VALUE_INT(iMinIdx);
		SHOW_FIELD_VALUE_UINT_IP(dwLastReportIp);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastReportTime);
		SHOW_FIELD_VALUE_INT(bAttrDataType);
		SHOW_FIELD_VALUE_UINT(wStaticTime);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastToDbTime);
		SHOW_FIELD_VALUE_UINT(wLastToDbIdx);
	}
}TWarnAttrReportInfo;

typedef struct
{
	int32_t iWarnFlag; // view, machine, exception
	int32_t iWarnId; // -- 0, machine id, view id
	int32_t iAttrId;
	int32_t iWarnConfigValue; // min, max, wave config
	uint32_t dwWarnValue;
	uint32_t dwWarnTimeSec;
	uint32_t dwReserved1;
	uint32_t dwReserved2;
}TAttrWarnInfo;

// 提示发送共享内存
#define MAX_WARN_SEND_NODE_SHM 10000
#define DEF_WARN_SEND_INFO_VALID_TIME_SEC 300 
#define DEF_SEND_WARN_SHM_KEY 2018100625
typedef struct 
{
	uint32_t dwWarnId;      // 提示id
	uint32_t dwWarnAddTime; // 添加时间
	uint32_t dwSendWarnFlag; // 提示发送标记
	TAttrWarnInfo stWarn;  // 提示信息
	char sReserved[32];
	uint32_t dwReserved1;
	uint32_t dwReserved2;
	uint32_t dwReserved3;
	uint32_t dwReserved4;
}TWarnSendInfo;

#define MAX_VIEW_INFO_COUNT 200 // 最大视图个数
#define MAX_COUNT_BIND_MACHINES_PER_VIEW 20 // 单个视图最大绑定的机器数

#define VIEW_FLAG_AUTO_BIND_MACHINE 1 // 设置时自动绑定有上报监控点的机器
#define MAX_COUNT_BIND_ATTRS_PER_VIEW 200 // 单个视图最大绑定的属性数

typedef struct
{
	int32_t iViewId;
	int32_t iViewNameVmemIdx;
	uint8_t bViewFlag;
	uint32_t dwLastModTime;
	uint8_t bBindMachineCount;
	int32_t aryBindMachines[MAX_COUNT_BIND_MACHINES_PER_VIEW];
	uint8_t bBindAttrCount;
	int32_t aryBindAttrs[MAX_COUNT_BIND_ATTRS_PER_VIEW];

	// MtSystemConfig 中关联用的索引
	int iPreIndex;
	int iNextIndex;

	char sReserved[8];

	void Show() {
		SHOW_FIELD_VALUE_INT(iViewId);
		SHOW_FIELD_VALUE_INT(iViewNameVmemIdx);
		if(iViewNameVmemIdx > 0)
			printf("\t name:%s\n", MtReport_GetFromVmem_Local(iViewNameVmemIdx));
		SHOW_FIELD_VALUE_UINT(bViewFlag);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastModTime);
		SHOW_FIELD_VALUE_UINT(bBindMachineCount);
		SHOW_FIELD_VALUE_INT_COUNT(bBindMachineCount, aryBindMachines);
		SHOW_FIELD_VALUE_UINT(bBindAttrCount);
		SHOW_FIELD_VALUE_INT_COUNT(bBindAttrCount, aryBindAttrs);
		SHOW_FIELD_VALUE_INT(iPreIndex);
		SHOW_FIELD_VALUE_INT(iNextIndex);
	}
}TViewInfo;

#define MAX_MACHINE_BIND_VIEW_COUNT  20 // 一台机器最多绑定到的视图数
#define DEF_MACHINE_VIEW_CONFIG_SHM_KEY 2018080512
#define MACHINE_VIEW_HASH_NODE_COUNT 10001 
typedef struct {
	int32_t iMachineId;
	int32_t iBindViewCount;
	int32_t aryView[MAX_MACHINE_BIND_VIEW_COUNT];
	char sReserved[8];
	void Show() {
		SHOW_FIELD_VALUE_INT(iMachineId);
		SHOW_FIELD_VALUE_INT(iBindViewCount);
		SHOW_FIELD_VALUE_UINT_COUNT(iBindViewCount, aryView);
	}
}TMachineViewConfigInfo;

#define MAX_ATTR_BIND_VIEW_COUNT 20 // 单个属性最多绑定到的视图数
#define DEF_ATTR_VIEW_CONFIG_SHM_KEY 2018080518
#define ATTR_VIEW_HASH_NODE_COUNT 10003 
typedef struct {
	int32_t iAttrId;
	int32_t iBindViewCount;
	int32_t aryView[MAX_ATTR_BIND_VIEW_COUNT]; // 这里是视图 id
	char sReserved[8];
	void Show() {
		SHOW_FIELD_VALUE_INT(iAttrId);
		SHOW_FIELD_VALUE_INT(iBindViewCount);
		SHOW_FIELD_VALUE_UINT_COUNT(iBindViewCount, aryView);
	}
}TAttrViewConfigInfo;

// 提示状态
#define WARN_DEAL_STATUS_NONE 0 // 提示未处理
#define WARN_DEAL_STATUS_DO 1 // 处理中
#define WARN_DEAL_STATUS_OK 2 // 处理完成
#define WARN_DEAL_STATUS_MASK 3 // 提示屏蔽了

// 属性提示相关结构 ----------- end 

#define MYSIZEOF (unsigned)sizeof
#define MYSTRLEN (unsigned)strlen

// 提示标记
#define ATTR_WARN_FLAG_MAX 1
#define ATTR_WARN_FLAG_MIN 2
#define ATTR_WARN_FLAG_WAVE 4
#define ATTR_WARN_FLAG_TYPE_MACHINE 8
#define ATTR_WARN_FLAG_TYPE_VIEW 16 
#define ATTR_WARN_FLAG_MASK_WARN 32 
#define ATTR_WARN_FLAG_EXCEPTION 64

// 监控点数据类型
enum {
	SUM_REPORT_TYPE_MIN = 1,
	SUM_REPORT_M=1, // 累计量
	SUM_REPORT_MIN=2, // 取每分钟最小值
	EX_REPORT=3, // 异常量
	SUM_REPORT_MAX=4, // 取每分钟最大值
	SUM_REPORT_TOTAL = 5, // 按历史上报累计
	STR_REPORT_D = 6, // 按天累计的字符串型, 一天生成一张饼图表，多个字符串时只显示前几位有上报的字符串
	STR_REPORT_D_IP = 7, // IP 转地址字符串，地址为省级
	DATA_USE_LAST = 8, // 取最新上报值
	SUM_REPORT_TYPE_MAX = 8, // 最大不能超过 255
};

inline uint32_t GetAttrIdxData(uint32_t dwCur, uint32_t dwNew, int iAttrDataType)
{
    if(!dwCur)
        return dwNew;
    switch(iAttrDataType) {
        case SUM_REPORT_M:
        case EX_REPORT:
        case STR_REPORT_D:
        case STR_REPORT_D_IP:
            return dwCur+dwNew;

        case SUM_REPORT_MIN:
            return dwCur < dwNew ? dwCur : dwNew;

        case SUM_REPORT_TOTAL:
        case SUM_REPORT_MAX:
            return dwCur > dwNew ? dwCur : dwNew;

        case DATA_USE_LAST:
            return dwNew;
    }    
    return dwCur+dwNew;
}


// Ip 地址库管理 
#define IPINFO_FLAG_PROV_VMEM 1
#define IPINFO_FLAG_CITY_VMEM 2
#define IPINFO_FLAG_OWNER_VMEM 4
#define IPINFO_HASH_NODE_COUNT 200000
#define IPINFO_HASH_SHM_DEF_KEY 2019041122
#define IPINFO_FILE_MAGIC_STR "_@#@%SKDFSKJskcheck_magi20200411"
typedef struct 
{
	char sStart[16];
	char sEnd[16];
	char scountry[32];
	char sprov[64];
	char scity[64];
	char sowner[32];
}TIpInfoInFile;

typedef struct
{
	uint32_t dwStart;
	uint32_t dwEnd;
	uint8_t bSaveFlag;
	union {
		int32_t iProvVmemIdx;
		char sprov[12];
	};
	union {
		int32_t iCityVmemIdx;
		char scity[16];
	};
	union {
		int32_t iOwnerVmemIdx;
		char sowner[16];
	};
	char sReserved[32];
	void Show() {
		SHOW_FIELD_VALUE_UINT(dwStart);
		SHOW_FIELD_VALUE_UINT_IP(dwStart);
		SHOW_FIELD_VALUE_UINT(dwEnd);
		SHOW_FIELD_VALUE_UINT_IP(dwEnd);
		SHOW_FIELD_VALUE_UINT(bSaveFlag);
		if(bSaveFlag & IPINFO_FLAG_PROV_VMEM)
			printf("\t prov:%s\n", MtReport_GetFromVmem_Local(iProvVmemIdx));
		else
			printf("\t prov:%s\n", sprov);
		if(bSaveFlag & IPINFO_FLAG_CITY_VMEM)
			printf("\t city:%s\n", MtReport_GetFromVmem_Local(iCityVmemIdx));
		else
			printf("\t city:%s\n", scity);
		if(bSaveFlag & IPINFO_FLAG_OWNER_VMEM)
			printf("\t owner:%s\n", MtReport_GetFromVmem_Local(iOwnerVmemIdx));
		else
			printf("\t owner:%s\n", sowner);
	}
}TIpInfo;

typedef struct {
	int iCount;
	TIpInfo ips[IPINFO_HASH_NODE_COUNT];
	char sReserved[16];
}TIpInfoShm;

int IpInfoInitCmp(const void *pKey, const void *pNode);
int IpInfoSearchCmp(const void *pKey, const void *pNode);
typedef int (*T_FUN_IP_CMP)(const void *pKey, const void *pNode);
std::string & GetRemoteRegionInfoNew(const char *premote, int flag=IPINFO_FLAG_PROV_VMEM|IPINFO_FLAG_CITY_VMEM);

// 提示类型
#define ATTR_WARN_TYPE_MACHINE 1 // 单机提示
#define ATTR_WARN_TYPE_VIEW 2 // 视图提示
#define ATTR_WARN_TYPE_EXCEPTION 3 // 异常提示

// 日志生产配置  --------------------------
#define SLOG_CLIENT_CONFIG_DEF_SHMKEY 20132206 // 终端配置默认共享内存 key
#define MAX_SLOG_CONFIG_COUNT 2000 // 日志生产配置最大数目最大值
#define SLOG_APP_INFO_DEF_SHMKEY 20130626 // 终端 app 信息默认共享内存 key
#define MAX_SLOG_APP_COUNT 200 // app 数目最大值
#define IPV4_ADDR_LEN 20 // ipv4 字符串地址长度
#define SLOG_MODULE_COUNT_MAX_PER_APP 50 // 每个应用的最大模块数

#define SLOG_APP_LOG_SHM_KEY_BASE 131711 // app log shm base key
#define SLOG_APP_LOGFILE_SHM_KEY_BASE 3371317 // APP log file shm base key
#define APPLOG_FLAG_SHMKEY_USE_ADD 1 // log shm key 探测算法
#define APPLOG_FLAG_SHMKEY_USE_SUB 2 // log shm key 探测算法
#define APPLOG_FLAG_SHMKEY_USE_MUL 4 // log shm key 探测算法
#define APPLOG_FLAG_SHMKEY_USE_DEV 8 // log shm key 探测算法
#define APPLOG_FLAG_SHMKEY_USE_MOD 16 // log shm key 探测算法
#define APPLOG_FLAG_LOG_WRITED 32 // 该 app 有日志产生
#define APPLOG_FILE_FLAG_SHMKEY_USE_ADD 64 // log file shm key 探测算法
#define APPLOG_FILE_FLAG_SHMKEY_USE_SUB 128 // log file shm key 探测算法
#define APPLOG_FILE_FLAG_SHMKEY_USE_MUL 256 // log file shm key 探测算法
#define APPLOG_FILE_FLAG_SHMKEY_USE_DEV 512 // log file shm key 探测算法
#define APPLOG_FILE_FLAG_SHMKEY_USE_MOD 1024 // log file shm key 探测算法
#define APPLOG_FILE_FLAG_DELETE_OLD_FILE 2048 // 删除最老的日志文件标记 

#define ADD_LOOP_INDEX(idx, max) do { if(idx+1>=max) idx=0; else idx++; }while(0)

// 服务器
#define MAX_SERVICE_COUNT 50 // 服务地址最大数目
#define MAX_SERVICE_PER_SET 50 // 单个 set 最大包含的服务器数目 
#define MAX_SRV_SET_COUNT 50 // set 数目最大值

#define SRV_TYPE_APP_LOG 1 // 服务器类型--app 日志服务器
#define SRV_TYPE_ATTR 2 // 服务器类型-- 监控点服务器, 处理监控点上报
#define SRV_TYPE_ATTR_DB 3 // 服务器类型-- 监控点 mysql 服务器
#define SRV_TYPE_MT_CENTER 4 // 中心服务器
#define SRV_TYPE_CONNECT 5 // 服务器类型-- 接入服务器，slog_mtreport_server
#define SRV_TYPE_WEB 11 // 服务器类型-- memache 服务器，memcache 缓存服务器

typedef struct
{
	uint64_t qwLogSizeInfo;
	uint32_t dwDebugLogsCount;
	uint32_t dwInfoLogsCount;
	uint32_t dwWarnLogsCount;
	uint32_t dwReqerrLogsCount;
	uint32_t dwErrorLogsCount;
	uint32_t dwFatalLogsCount;
	uint32_t dwOtherLogsCount;
	char sReserved[8];
	void Show() {
		SHOW_FIELD_VALUE_UINT64(qwLogSizeInfo);
		SHOW_FIELD_VALUE_UINT(dwDebugLogsCount);
		SHOW_FIELD_VALUE_UINT(dwInfoLogsCount);
		SHOW_FIELD_VALUE_UINT(dwWarnLogsCount);
		SHOW_FIELD_VALUE_UINT(dwReqerrLogsCount);
		SHOW_FIELD_VALUE_UINT(dwErrorLogsCount);
		SHOW_FIELD_VALUE_UINT(dwFatalLogsCount);
		SHOW_FIELD_VALUE_UINT(dwOtherLogsCount);
	}
}TLogStatInfo;
inline uint32_t GetLogCountsByStatInfo(TLogStatInfo &stInfo) {
    uint32_t dwCount = 0;
    dwCount += stInfo.dwDebugLogsCount;
    dwCount += stInfo.dwInfoLogsCount;
    dwCount += stInfo.dwWarnLogsCount;
    dwCount += stInfo.dwReqerrLogsCount;
    dwCount += stInfo.dwErrorLogsCount;
    dwCount += stInfo.dwFatalLogsCount;
    dwCount += stInfo.dwOtherLogsCount;
    return dwCount;
}



typedef struct                 
{                              
	int32_t iModuleId;         
	int32_t iNameVmemIdx;
	char sReserved[8];
	void Show() {
		if(iModuleId == 0)
			return;

		SHOW_FIELD_VALUE_INT(iModuleId);
		SHOW_FIELD_VALUE_INT(iNameVmemIdx);
		if(iNameVmemIdx > 0)
			printf("\t name:%s\n", MtReport_GetFromVmem_Local(iNameVmemIdx));
	}
}TModuleInfo;                  

typedef struct
{
	int32_t iAppId;
	int32_t iNameVmemIdx; // app 名字的 vmem 索引

	// app 的log 服务器
	int32_t iLogSrvIndex;
	uint32_t dwAppSrvSeq; // app 日志主服务器 seq, 变化时说明 srv 有变化了
	uint32_t dwAppSrvMaster; // app 日志主服务器
	uint16_t wLogSrvPort; // route server 根据 iLogSrvSetId，dwSrvSetSeq 设置

	uint8_t bTryAppLogShmFlag; // 用于设置 iAppLogShmKey 时解决多进程并发操作的问题
	int32_t iAppLogShmKey; // 探测 内存log shmkey 算法命中的key
	uint8_t bTryAppLogFileShmFlag; // 用于设置 iAppLogFileShmKey 时解决多进程并发操作的问题
	int32_t iAppLogFileShmKey; // 日志文件 shmkey 
	uint32_t dwLastTryWriteLogTime;
	uint32_t dwAppLogFlag;
	uint32_t dwSeq;

	uint16_t wModuleCount;
	TModuleInfo arrModuleList[SLOG_MODULE_COUNT_MAX_PER_APP];

	uint32_t dwDeleteLogFileTime; // 需要滚动删除的日志文件时间

	uint8_t bReadLogStatInfo; // 日志统计信息是否读取标记
	TLogStatInfo stLogStatInfo; // 用于显示日志统计信息
	uint32_t dwLastModTime; // 配置最后修改时间
	uint32_t dwReserved2;
	int32_t iReserved1;
	int32_t iReserved2;

	char sReserved[15];

	// 用于在 MtSystemConfig 中连接所有全局 app info
	int32_t iPreIndex;
	int32_t iNextIndex;

	void Show() {
		SHOW_FIELD_VALUE_INT(iAppId);
		SHOW_FIELD_VALUE_INT(iNameVmemIdx);
		if(iNameVmemIdx > 0)
			printf("\t name:%s\n", MtReport_GetFromVmem_Local(iNameVmemIdx));
		SHOW_FIELD_VALUE_INT(iLogSrvIndex);
		SHOW_FIELD_VALUE_UINT(dwAppSrvSeq);
		SHOW_FIELD_VALUE_UINT_IP(dwAppSrvMaster);
		SHOW_FIELD_VALUE_UINT(wLogSrvPort);
		SHOW_FIELD_VALUE_UINT(bTryAppLogShmFlag);
		SHOW_FIELD_VALUE_UINT(bTryAppLogFileShmFlag);
		SHOW_FIELD_VALUE_INT(iAppLogShmKey);
		SHOW_FIELD_VALUE_INT(iAppLogFileShmKey);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastTryWriteLogTime);
		SHOW_FIELD_VALUE_UINT(dwAppLogFlag);
		SHOW_FIELD_VALUE_UINT(dwSeq);
		SHOW_FIELD_VALUE_UINT(wModuleCount);
		SHOW_FIELD_VALUE_STR_COUNT(SLOG_MODULE_COUNT_MAX_PER_APP, iModuleId, SLOG_MODULE_COUNT_MAX_PER_APP, arrModuleList);
		SHOW_FIELD_VALUE_UINT_TIME(dwDeleteLogFileTime);
		SHOW_FIELD_VALUE_UINT(bReadLogStatInfo);
		if(bReadLogStatInfo)
			stLogStatInfo.Show();
		SHOW_FIELD_VALUE_INT(iPreIndex);
		SHOW_FIELD_VALUE_INT(iNextIndex);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastModTime);
	}
}AppInfo;

#define MAX_USER_COUNT 1000 // 最大用户数
typedef struct
{
	int32_t iAppCount;
	AppInfo stInfo[MAX_SLOG_APP_COUNT];
	char sReserved[16];
}SLogAppInfo;

#ifndef MAX_SLOG_TEST_KEYS_PER_CONFIG
#define MAX_SLOG_TEST_KEYS_PER_CONFIG 20
#endif
#define SLOG_TEST_KEY_LEN 20 // 染色关键字最大长度
typedef struct
{
	uint32_t dwConfigSeq;

	// comm
	uint32_t dwCfgId;
	int32_t iAppId;
	int32_t iModuleId;
	int32_t iLogType;

	// advance
	uint32_t dwSpeedFreq; // 日志每分钟频率限制
	uint16_t wTestKeyCount;
	SLogTestKey stTestKeys[MAX_SLOG_TEST_KEYS_PER_CONFIG];

	uint32_t dwLogFreqStartTime;
	int32_t iLogWriteCount; 
	char sReserved[23];

	// 用于在 MtSystemConfig 中连接所有 SLogClientConfig 
	int32_t iPreIndex;
	int32_t iNextIndex;

	void Show() {
		SHOW_FIELD_VALUE_UINT(dwConfigSeq);
		SHOW_FIELD_VALUE_UINT(dwCfgId);
		SHOW_FIELD_VALUE_INT(iAppId);
		SHOW_FIELD_VALUE_INT(iModuleId);
		SHOW_FIELD_VALUE_INT(iLogType);
		SHOW_FIELD_VALUE_UINT(dwSpeedFreq);
		SHOW_FIELD_VALUE_UINT(wTestKeyCount);
		SHOW_FIELD_VALUE_STR_COUNT(wTestKeyCount, bKeyType, MAX_SLOG_TEST_KEYS_PER_CONFIG, stTestKeys);
		SHOW_FIELD_VALUE_UINT_TIME(dwLogFreqStartTime);
		SHOW_FIELD_VALUE_INT(iLogWriteCount);
		SHOW_FIELD_VALUE_INT(iPreIndex);
		SHOW_FIELD_VALUE_INT(iNextIndex);
	}
}SLogClientConfig;

#define SEL_MAX_SERVICE_ADDRESS 10 // 按权重选中的服务地址
#define RESP_MAX_SERVICE_ADDRESS 50 // 按权重比值随机选中的服务地址序列
#define SRV_CPU_0_IDLE_WEIGHT 580 
#define SRV_CPU_AVG_IDLE_WEIGHT (1000-SRV_CPU_0_IDLE_WEIGHT)  // 注意 cpu 使用率是千分制上报
#define SRV_CPU_IDLE_WEIGHT 60 // cpu 空闲率对权重的影响值
#define SRV_PACKET_IDLE_WEIGHT (100-SRV_CPU_IDLE_WEIGHT) // 剩余收包量对权重的影响值

#define PER_SERVER_FOR_XXX 2000 // 一台服务器最多可以处理的应用 log 或者 上报attr的用户数
int SrvForAppLogCmp(const void *a, const void *b);
typedef struct
{
	uint16_t wAppCount;
	int32_t aiApp[PER_SERVER_FOR_XXX];
	void Show() {
		SHOW_FIELD_VALUE_UINT(wAppCount);
		SHOW_FIELD_VALUE_INT_COUNT(wAppCount, aiApp);
	}
}ServerForAppList;

// sand_box 机器状态
#define SAND_BOX_ENABLE_NEW 0 // 接收新用户
#define SAND_BOX_ENABLE 1 // 使用中，不接收新用户了
#define SAND_BOX_DISABLEING 2 // 用户数据迁移中
#define SAND_BOX_DISABLED 3 // 已下架机器
typedef struct
{
	uint16_t wType;
	uint8_t bSandBox;
	uint8_t bRegion;
	uint8_t bIdc;
	uint16_t wPort;
	int32_t iWeightCur; // 当前权重 -- dyn set
	int32_t iWeightConfig; // 配置权重，按每秒处理数据包数算
	char szIpV4[18];
	uint32_t dwIp;
	uint32_t dwServiceId; // 服务编号 唯一对应 (ip, port, type)
	uint32_t dwCfgSeq; // 配置seq, 配置改变时相应改变

	union {
		// wType 为 SRV_TYPE_APP_LOG 时, 以该 server 为主服务器的 app 列表，app log 备服务器为同 set 的其他机器
		ServerForAppList stForAppList; 
	};

	uint32_t dwLastReportTime; // 用于判断服务是否存活 dyn set
	uint32_t dwRecvReqTotal; // dyn set
	uint32_t dwRespOkTotal; // dyn set
	uint32_t dwRespFailedTotal; // dyn set

	char sReserved[32];
	void Show() {
		SHOW_FIELD_VALUE_UINT(wType);
		SHOW_FIELD_VALUE_UINT(bSandBox);
		SHOW_FIELD_VALUE_UINT(bRegion);
		SHOW_FIELD_VALUE_UINT(bIdc);
		SHOW_FIELD_VALUE_UINT(wPort);
		SHOW_FIELD_VALUE_INT(iWeightCur);
		SHOW_FIELD_VALUE_INT(iWeightConfig);
		SHOW_FIELD_VALUE_CSTR(szIpV4);
		SHOW_FIELD_VALUE_UINT_IP(dwIp);
		SHOW_FIELD_VALUE_UINT(dwServiceId);
		SHOW_FIELD_VALUE_UINT(dwCfgSeq);
		if(wType == SRV_TYPE_APP_LOG)
			SHOW_FIELD_VALUE_STR(stForAppList);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastReportTime);
		SHOW_FIELD_VALUE_UINT(dwRecvReqTotal);
		SHOW_FIELD_VALUE_UINT(dwRespOkTotal);
		SHOW_FIELD_VALUE_UINT(dwRespFailedTotal);
	}
}SLogServer; // primary : ip, type

#define MAX_SERVICE_PER_MACHINE 50

// 迁移云管系统全局配置项 -- 用于下发， 同 client 中的 mt_slog.h MtSystemConfig
typedef struct
{
    uint16_t wHelloRetryTimes; // hello 包重试次数
    uint16_t wHelloPerTimeSec; // hello 发送间隔
    uint16_t wCheckLogPerTimeSec; // 日志配置 check 时间
    uint16_t wCheckAppPerTimeSec; // app 配置 check 时间
    uint16_t wCheckServerPerTimeSec; // server 配置 check 时间
    uint16_t wCheckSysPerTimeSec; // system config 配置 check 时间
    uint32_t dwConfigSeq;
    uint8_t bAttrSendPerTimeSec; // attr 上报时间间隔
    uint8_t bLogSendPerTimeSec; // log 上报时间间隔
    uint8_t bReportCpuUseSec; // cpu 使用率多久上报一次
}MtSystemConfigClient;

// 迁移云管系统事件，相关服务必须尽快处理跟自己相关的事件, 事件一旦超时将被回收(slog_config)
// 事件过期事件不可太久否则会导致时间满
#define MAX_EVENT_COUNT 200
enum {
    // 一键部署插件事件 - slog_mtreport_server 处理, slog_config 生成
    EVENT_PREINSTALL_PLUGIN = 1, // 外网模式部署插件
    EVENT_INNER_MODE_PREINSTALL_PLUGIN = 11, // 内网模式部署插件
    EVENT_PREINSTALL_PLUGIN_EXPIRE_SEC = 30,
			    
	EVENT_MULTI_MACH_PLUGIN_CFG_MOD = 2,
	EVENT_MULTI_MACH_PLUGIN_REMOVE = 3,
	EVENT_MULTI_MACH_PLUGIN_ENABLE = 4,
	EVENT_MULTI_MACH_PLUGIN_DISABLE = 5,
	EVENT_MULTI_MACH_PLUGIN_OPR_EXPIRE_SEC = 30,
};

// 事件 EVENT_PREINSTALL_PLUGIN 处理的进度标记
enum {
    EV_PREINSTALL_INNER_DOWN_PACK = 0, // 内网部署模式，下载插件部署包
    EV_PREINSTALL_START = 1, // 开始-cgi 写入DB
    EV_PREINSTALL_DB_RECV = 2, // slog_config 设置到机器登录的服务器共享内存
    EV_PREINSTALL_TO_CLIENT_START = 3, // slog_mtreport_server 处理事件下发任务 
    EV_PREINSTALL_TO_CLIENT_OK = 4, // client 上报, slog_mtreport_server 收到client任务确认包
    EV_PREINSTALL_CLIENT_GET_DOWN_URL = 5, // client 上报, 获取到安装包下载地址
    EV_PREINSTALL_CLIENT_GET_PACKET = 6, // client 上报, 已下载插件部署包
    EV_PREINSTALL_CLIENT_START_PLUGIN = 7, // client 上报, 已部署并启动插件
    EV_PREINSTALL_SERVER_RECV_PLUGIN_MSG = 8, // client 上报, slog_mtreport_server 收到插件心跳
	EV_PREINSTALL_CLIENT_INSTALL_PLUGIN_OK = 9, // 插件安装成功, 等待访问才有数据上报
	EV_PREINSTALL_STATUS_MAX = 9,

    EV_PREINSTALL_ERR_MIN = 20,
    EV_PREINSTALL_ERR_GET_URL = 20, // 获取下载地址失败
    EV_PREINSTALL_ERR_GET_URL_RET = 21, // 获取下载地址返回错误码
    EV_PREINSTALL_ERR_GET_URL_PARSE_RET = 22, // 解析回包错误
    EV_PREINSTALL_ERR_DOWNLOAD_PACK = 23, // 下载部署包失败
    EV_PREINSTALL_ERR_UNPACK = 24, // 解压部署包失败
    EV_PREINSTALL_ERR_MKDIR = 25, // 创建部署目录失败，请检查agent 是否有权限
    EV_PREINSTALL_ERR_START_PLUGIN = 26, // 启动插件失败，可能是不兼容导致
	EV_PREINSTALL_ERR_DOWNLOAD_OPEN_CFG = 27, // 开源版配置文件下载失败
    EV_PREINSTALL_ERR_NOT_FIND = 27, // 未找到插件
    EV_PREINSTALL_ERR_DOWNLOAD_CFG = 28, // 配置文件下发失败
    EV_PREINSTALL_ERR_RESPONSE_TIMEOUT  = 29, // 响应超时
    EV_PREINSTALL_ERR_PARSE_JSON_FILE = 30, // 解析下载文件错误 
    EV_PREINSTALL_ERR_MAKE_PLUGIN_TAR = 31, // 生成插件部署包文件错误 
    EV_PREINSTALL_ERR_GET_LOCAL_URL = 32, // 获取本地下载地址错误
    EV_PREINSTALL_ERR_GET_KEY_FAILED = 33, // 获取加密key 失败
    EV_PREINSTALL_ERR_MAX = 50,
};

// 机器变更插件信息
enum {
	EV_MOP_OPR_START = 1, // 开始 
	EV_MOP_OPR_DB_RECV = 2, // db 接收
	EV_MOP_OPR_DOWNLOAD = 3, // agent 已下发
	EV_MOP_OPR_FAILED = 4, // agent 处理失败
	EV_MOP_OPR_SUCCESS = 5, // agent 处理成功
	EV_MOP_OPR_RESPONSE_TIMEOUT = 6, // agent 响应超时 
};

typedef struct _TEventMachinePluginOpr {
    int32_t iPluginId;
    int32_t iMachineId;
    int32_t iDbId;
    uint32_t dwUserMasterId;
    void Show() {
        SHOW_FIELD_VALUE_INT(iPluginId);
        SHOW_FIELD_VALUE_INT(iMachineId);
        SHOW_FIELD_VALUE_INT(iDbId);
        SHOW_FIELD_VALUE_UINT(dwUserMasterId);
    }
}TEventMachinePluginOpr;

typedef struct _TEventPreInstallPlugin {
    int32_t iPluginId;
    int32_t iMachineId;
    int32_t iDbId;
    void Show() {
        SHOW_FIELD_VALUE_INT(iPluginId);
        SHOW_FIELD_VALUE_INT(iMachineId);
        SHOW_FIELD_VALUE_INT(iDbId);
    }
}TEventPreInstallPlugin;

enum {
    EVENT_STATUS_INIT_SET = 1,
    EVENT_STATUS_FIN = 2
};

typedef struct _TEventInfo
{
    int iEventType; // 为 0 表示无事件
    uint32_t dwExpireTime; // 事件过期时间，过期后即可回收
    uint8_t bEventStatus; // 事件状态 
    union {
        TEventPreInstallPlugin stPreInstall;
		TEventMachinePluginOpr stMachPlugOpr;
        char sEvBuf[238];
    }ev;
	char sReserved[17];
    void Show() {
        SHOW_FIELD_VALUE_INT(iEventType);
        SHOW_FIELD_VALUE_UINT_TIME(dwExpireTime);
		SHOW_FIELD_VALUE_UINT(bEventStatus);
        if(iEventType == EVENT_PREINSTALL_PLUGIN)
            ev.stPreInstall.Show();
		else if(iEventType == EVENT_MULTI_MACH_PLUGIN_REMOVE
				|| iEventType == EVENT_MULTI_MACH_PLUGIN_CFG_MOD
				|| iEventType == EVENT_MULTI_MACH_PLUGIN_ENABLE
				|| iEventType == EVENT_MULTI_MACH_PLUGIN_DISABLE)
			ev.stMachPlugOpr.Show();
	}
	_TEventInfo() {
		iEventType = 0;
		dwExpireTime = 0;
    }
}TEventInfo;

// 上报时间超过统计时间 n(10) s 后表格被自动清除
#define PLUGIN_TABLE_OVER_STATIC_TIME_SEC 30
#define MAX_PLUGIN_REALTIME_LOG_COUNT 500

// 插件实时表格
typedef struct 
{
    int iPluginId;
    int iHostId;
    int iTableId;
    int iStaticTime;
    char sValue[256];
    uint32_t dwWriteTime;
    uint32_t dwStatId;
    char sReserved[12];
	void Show() {
        SHOW_FIELD_VALUE_INT(iPluginId);
        SHOW_FIELD_VALUE_INT(iHostId);
        SHOW_FIELD_VALUE_INT(iTableId);
        SHOW_FIELD_VALUE_INT(iStaticTime);
        SHOW_FIELD_VALUE_UINT_TIME(dwWriteTime);
        SHOW_FIELD_VALUE_UINT(dwStatId);
        if(sValue[255] == '\0')
            SHOW_FIELD_VALUE_CSTR(sValue);
    }
}TPlugRealTimeLog;


#define SYSTEM_FLAG_DAEMON 1 // 演示版标记

// 迁移云管系统全局配置项
typedef struct 
{
    // --- MtSystemConfigClient - start
    uint16_t wReserved;
    uint16_t wHelloRetryTimes; // hello 包重试次数
    uint16_t wHelloPerTimeSec; // hello 发送间隔
    uint16_t wCheckLogPerTimeSec; // 日志配置 check 时间
    uint16_t wCheckAppPerTimeSec; // app 配置 check 时间
    uint16_t wCheckServerPerTimeSec; // server 配置 check 时间
    uint16_t wCheckSysPerTimeSec; // system config 配置 check 时间
    uint8_t bReserved; 
    uint32_t dwConfigSeq;
    uint8_t bAttrSendPerTimeSec; // attr 上报时间间隔
    uint8_t bLogSendPerTimeSec; // log 上报时间间隔
    uint8_t bReserved_1;
    // --- MtSystemConfigClient - end

	// mysql -- 配置服务器配置信息
	char szUserName[32];
	char szDbHost[32];
	char szPass[32];
	char szDbName[32];
	int iDbPort;

	char szAgentAccessKey[33]; // slog_mtreport_client 接入访问 key
	int32_t iEmailVmemIndex;

	// attr 服务器分配管理
	uint32_t dwAttrSrvSeq;
	int32_t iAttrSrvIndex;
	uint32_t dwAttrSrvMasterIp;
	uint16_t wAttrSrvPort;

	// attr 监控列表 -- 由 slog_config 单进程管理更新操作
	uint16_t wAttrCount;
	int32_t iAttrIndexStart;
	int32_t iAttrIndexEnd;
	int32_t iAttrTypeTreeVmemIdx; // 监控点类型树vmem 存储索引

    // 插件实时表格数据，循环数组模式
    uint8_t bTableLogFlag; // 用于解决同时写 sRealTableLog 
    int32_t iLastWriteTableLogIdx;
    TPlugRealTimeLog sRealTableLog[MAX_PLUGIN_REALTIME_LOG_COUNT]; // 实时表格数据

	// 应用配置列表 -- 由 slog_config 单进程管理更新操作
	uint16_t wAppInfoCount;
	int32_t iAppInfoIndexStart;
	int32_t iAppInfoIndexEnd;

	// 日志配置列表 -- 由 slog_config 单进程管理更新操作
	uint16_t wLogConfigCount;
	int32_t iLogConfigIndexStart;
	int32_t iLogConfigIndexEnd;

	// 客户端列表 -- 由 mtreport_server 中的单进程管理更新操作
	uint16_t wCurClientCount;
	int32_t iMtClientListIndexStart;
	int32_t iMtClientListIndexEnd;

	// 监控机器列表 -- 由 slog_config 单进程管理更新操作
	uint16_t wMachineCount;
	int32_t iMachineListIndexStart;
	int32_t iMachineListIndexEnd;

	// 视图列表 -- 由 slog_config 单进程管理更新操作
	uint16_t wViewCount;
	int32_t iViewListIndexStart;
	int32_t iViewListIndexEnd;

	// 迁移提示列表 -- 最近200条, 其它的需要查询数据库(TWarnInfo)
	uint32_t dwTotalWarns;
	uint8_t bLastWarnCount;
	int32_t iWarnIndexStart;
	int32_t iWarnIndexEnd;

	// 对应 user::UserRemoteAppLogInfo 结构 -- 仅在 slog_server 程序中写
	char sUserRemoteAppLogInfo[300];

	// 以下动态设置
	int32_t iMachineId; // 本机的机器id
	int32_t iReserved; 
	uint32_t dwMonitorRecordsId; // 表更新本机当前读取到的 id 值

	uint32_t dwReserved1;
	uint32_t dwReserved2;
	uint32_t dwReserved3;
	uint32_t dwReserved4;
	int32_t iReserved1;
	int32_t iReserved2;
	int32_t iReserved3;

	uint32_t dwSystemFlag; // 系统标志位
	TOtherInfoShm stOtherInfoShm;

	// 迁移云管系统事件
	uint8_t bEventModFlag;
	TEventInfo stEvent[MAX_EVENT_COUNT];

	// 一键部署校验字符串
	char sPreInstallCheckStr[16];

    int32_t iDES-PWEUid; // 绑定的云账号id
    char szDES-PWEKey[16]; // 访问 key
    uint32_t dwDES-PWESetTime; // 云账号设置时间

	char sReserved[88];
	void Show() {
		SHOW_FIELD_VALUE_UINT(wHelloRetryTimes);
		SHOW_FIELD_VALUE_UINT(wHelloPerTimeSec);
		SHOW_FIELD_VALUE_UINT(wCheckLogPerTimeSec);
		SHOW_FIELD_VALUE_UINT(wCheckAppPerTimeSec);
		SHOW_FIELD_VALUE_UINT(wCheckServerPerTimeSec);
		SHOW_FIELD_VALUE_UINT(wCheckSysPerTimeSec);
		SHOW_FIELD_VALUE_UINT(dwConfigSeq);
		SHOW_FIELD_VALUE_UINT(bAttrSendPerTimeSec);
		SHOW_FIELD_VALUE_UINT(bLogSendPerTimeSec);

		SHOW_FIELD_VALUE_CSTR(szAgentAccessKey);
		SHOW_FIELD_VALUE_INT(iEmailVmemIndex);
		if(iEmailVmemIndex > 0)
			printf("\t email:%s\n", MtReport_GetFromVmem_Local(iEmailVmemIndex));
		SHOW_FIELD_VALUE_INT(iAttrSrvIndex);
		SHOW_FIELD_VALUE_UINT(dwAttrSrvSeq);
		SHOW_FIELD_VALUE_UINT_IP(dwAttrSrvMasterIp);
		SHOW_FIELD_VALUE_INT(wAttrSrvPort);
		SHOW_FIELD_VALUE_UINT(wAttrCount);
		SHOW_FIELD_VALUE_INT(iAttrIndexStart);
		SHOW_FIELD_VALUE_INT(iAttrIndexEnd);
		SHOW_FIELD_VALUE_INT(iAttrTypeTreeVmemIdx);

		SHOW_FIELD_VALUE_UINT(wAppInfoCount);
		SHOW_FIELD_VALUE_INT(iAppInfoIndexStart);
		SHOW_FIELD_VALUE_INT(iAppInfoIndexEnd);

		SHOW_FIELD_VALUE_UINT(wLogConfigCount);
		SHOW_FIELD_VALUE_INT(iLogConfigIndexStart);
		SHOW_FIELD_VALUE_INT(iLogConfigIndexEnd);

		SHOW_FIELD_VALUE_UINT(wCurClientCount);
		SHOW_FIELD_VALUE_INT(iMtClientListIndexStart);
		SHOW_FIELD_VALUE_INT(iMtClientListIndexEnd);

		SHOW_FIELD_VALUE_UINT(wMachineCount);
		SHOW_FIELD_VALUE_INT(iMachineListIndexStart);
		SHOW_FIELD_VALUE_INT(iMachineListIndexEnd);

		SHOW_FIELD_VALUE_UINT(wViewCount);
		SHOW_FIELD_VALUE_INT(iViewListIndexStart);
		SHOW_FIELD_VALUE_INT(iViewListIndexEnd);

		SHOW_FIELD_VALUE_UINT(dwTotalWarns);
		SHOW_FIELD_VALUE_UINT(bLastWarnCount);
		SHOW_FIELD_VALUE_INT(iWarnIndexStart);
		SHOW_FIELD_VALUE_INT(iWarnIndexEnd);

		SHOW_FIELD_VALUE_UINT(dwConfigSeq);
		SHOW_FIELD_VALUE_INT(iMachineId);
		SHOW_FIELD_VALUE_UINT(dwMonitorRecordsId);
		SHOW_FIELD_VALUE_UINT(dwSystemFlag);

        if(iDES-PWEUid != 0) {
            SHOW_FIELD_VALUE_INT(iDES-PWEUid);
		    SHOW_FIELD_VALUE_CSTR(szDES-PWEKey);
            SHOW_FIELD_VALUE_UINT(dwDES-PWESetTime);
        }

		printf("\nother info shm \n");
		stOtherInfoShm.Show();

		printf("\nevent info as followed: \n");
		SHOW_FIELD_VALUE_UINT(bEventModFlag);
		for(int i=0; i < MAX_EVENT_COUNT; i++) {
			if(stEvent[i].iEventType) {
				stEvent[i].Show();
				printf("\n");
			}
		}
		SHOW_FIELD_VALUE_CSTR(sPreInstallCheckStr);
	}
}MtSystemConfig;


typedef struct
{
	// 服务器相关, 由 slog_config , route_server 管理 --------------=
	// route server 用于路由服务
	uint8_t bModSrvListFlag; // for sync
	uint16_t wServerCount; // route server 用于路由
	SLogServer stServerList[MAX_SERVICE_COUNT]; // 全部server

	// app 日志配置列表
	uint32_t dwSLogConfigCount;
	SLogClientConfig stConfig[MAX_SLOG_CONFIG_COUNT];

	// 视图配置列表
	uint32_t dwViewConfigCount;
	TViewInfo stViewInfo[MAX_VIEW_INFO_COUNT];

	// system config 
	MtSystemConfig stSysCfg;

	char sReserved[64];
}SLogConfig;


// 字符串型监控点相关数据结构
#define MAX_STR_ATTR_ARRY_NODE_VAL_COUNT  15000 // 字符型监控点，字符串最大支持数量
#define MAX_STR_ATTR_STR_COUNT 20 // 字符串型监控点，最多保留的字符串数，超过则去掉最小的
#define STR_ATTR_COUNT_FOR_SELECT_STR 8 // 用于筛选上报字符串的缓冲节点数
typedef struct
{
	char szStrInfo[65]; // 字符串
	int32_t iStrVal;  // 上报值, 为 <= 0 时表示该节点未使用

	// 同一个字符串型监控点通过索引链接起来, 这个链表的写操作由 slog_monitor_server 进程管理
	int32_t iNextStrAttr; 
	void Show() {
		SHOW_FIELD_VALUE_CSTR(szStrInfo);
		SHOW_FIELD_VALUE_INT(iStrVal);
		SHOW_FIELD_VALUE_INT(iNextStrAttr);
	}
}StrAttrNodeVal;
typedef struct 
{
	int32_t iNodeUse; // 已使用节点数
	int32_t iWriteIdx; // 写索引，用于搜索空闲的 StrAttrNodeVal 节点
	StrAttrNodeVal stInfo[MAX_STR_ATTR_ARRY_NODE_VAL_COUNT];
	void Show() {
		SHOW_FIELD_VALUE_INT(iNodeUse);
		SHOW_FIELD_VALUE_INT(iWriteIdx);
	}
}StrAttrNodeValShmInfo;

#define STR_ATTR_NODE_VAL_SHM_DEF_KEY 2019060712 // StrAttrNodeValShmInfo 共享内存默认key

// 字符串属性上报存储内存结构
#define STR_ATTR_HASH_NODE_COUNT 4031 // 单机
#define DEF_STR_ATTR_SHM_KEY 2019101505
typedef struct _TStrAttrReportInfo
{
	int32_t iAttrId; // 上报属性 id
	uint8_t bAttrDataType;
	int32_t iMachineId; // 上报机器 id
	uint32_t dwLastReportIp; // 最后一次上报数据的远程IP
	uint32_t dwLastReportTime;
	uint8_t bStrCount; // 字符串数
	int32_t iReportIdx; // 上报数据索引，指向 StrAttrNodeVal 结构
	uint8_t bLastReportDayOfMonth; // 1-31
	uint32_t dwLastSaveDbTime; // 最后写入DB 的时间
	char sReserved[8];
	void Show() {
		SHOW_FIELD_VALUE_INT(iAttrId);
		SHOW_FIELD_VALUE_UINT(bAttrDataType);
		SHOW_FIELD_VALUE_INT(iMachineId);
		SHOW_FIELD_VALUE_UINT_IP(dwLastReportIp);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastReportTime);
		SHOW_FIELD_VALUE_UINT(bStrCount);
		SHOW_FIELD_VALUE_INT(iReportIdx);
		SHOW_FIELD_VALUE_UINT(bLastReportDayOfMonth);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastSaveDbTime);
	}
}TStrAttrReportInfo;

#define ATTR_HASH_NODE_COUNT 20011
#define DEF_ATTR_SHM_KEY 6633317 
typedef struct 
{
	int32_t id;
	int32_t iAttrType;
	uint32_t iDataType;
	int32_t iNameVmemIdx;
	uint32_t dwLastModTime; 
    uint8_t bValueType;
    uint8_t bChartType;
    int32_t iReserved;
    uint32_t dwReserved;

	// 用于 MtSystemConfig 中连接所有 attr
	int32_t iPreIndex;
	int32_t iNextIndex;

	// 用于 AttrTypeInfo 中连接相同监控点类型下的 attr
	int32_t iAttrTypePreIndex;
	int32_t iAttrTypeNextIndex;

	int32_t iStaticTime;
	char sReserved[4];
	void Show() {
		SHOW_FIELD_VALUE_INT(id);
		SHOW_FIELD_VALUE_INT(iAttrType);
		SHOW_FIELD_VALUE_UINT(iDataType);
		SHOW_FIELD_VALUE_INT(iNameVmemIdx);
		SHOW_FIELD_VALUE_UINT(bValueType);
		SHOW_FIELD_VALUE_UINT(bChartType);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastModTime);
		if(iNameVmemIdx > 0)
			printf("\t name:%s\n", MtReport_GetFromVmem_Local(iNameVmemIdx));
		SHOW_FIELD_VALUE_INT(iStaticTime);
		SHOW_FIELD_VALUE_INT(iPreIndex);
		SHOW_FIELD_VALUE_INT(iNextIndex);
		SHOW_FIELD_VALUE_INT(iAttrTypePreIndex);
		SHOW_FIELD_VALUE_INT(iAttrTypeNextIndex);
	}
}AttrInfoBin;

#define ATTRTYPE_HASH_NODE_COUNT 1711
#define DEF_ATTRTYPE_SHM_KEY 3377717 
typedef struct 
{
	int32_t id;
	int32_t iNameVmemIdx;
	uint32_t dwLastModTime; 

	// 用于链接该监控点类型下的所有监控点索引
	uint16_t wAttrCount;
	int32_t iAttrIndexStart;
	int32_t iAttrIndexEnd;

	char sReserved[8];

	void Show() {
		SHOW_FIELD_VALUE_INT(id);
		SHOW_FIELD_VALUE_INT(iNameVmemIdx);
		if(iNameVmemIdx > 0)
			printf("\t name:%s\n", MtReport_GetFromVmem_Local(iNameVmemIdx));
		SHOW_FIELD_VALUE_UINT_TIME(dwLastModTime);
		SHOW_FIELD_VALUE_UINT(wAttrCount);
		SHOW_FIELD_VALUE_INT(iAttrIndexStart);
		SHOW_FIELD_VALUE_INT(iAttrIndexEnd);
	}
}AttrTypeInfo;

#define MAX_REPORT_ATTR_COUNT_PER_MACHINE  1800 // 单机最大属性上报个数
#define ATTR_LOCAL_VMEM_SYNC_MEMCACHE_TIME 300 // 本地 vmem 同步到分布式 memcache 的最小时间间隔
typedef struct
{
	int32_t id;
	uint32_t ip1; // agent client ip
	uint32_t ip2; // reserved
	uint32_t ip3; // proxy ip 
	uint32_t ip4; // conn ip
	uint8_t bWarnFlag;
	uint8_t bModelId;
	uint32_t dwLastModTime;
	int32_t iNameVmemIdx;
	int32_t iDescVmemIdx;

	uint32_t dwLastHelloTime; // 最后一次 hello 时间, slog_mtreport_server 写入 
    uint32_t dwAgentStartTime; // agent 启动时间

	char sRandKey[17]; // 数据上报加密 key 16 字节带结尾 0

	char szAttrReportDay[15]; // 当天时间
	int32_t iReportAttrVmemIdx; // 当天有上报的属性列表
	uint32_t dwLastVmemSyncMemcacheTime; // 最后一次本地 vmem 同步到分布式 memcache 的时间

	uint32_t dwLastReportAttrTime;
	uint32_t dwLastReportLogTime;

	// 用于 MtSystemConfig 中连接所有 Machine, 由 slog_config 管理 
	int32_t iPreIndex;
	int32_t iNextIndex;

    uint32_t dwClientIp; // agent 自动获取到的ip
    uint32_t dwGwIp; // agent 所在机器连接 server 用的网关 ip 
    uint32_t dwConnectIp; // agent 接入server 的ip
    int32_t iTTL; 
    int32_t iResponseMs; //服务器hello 响应耗时(除2可认为是网络延迟)

    char sReserved[108];
	void Show() {
		SHOW_FIELD_VALUE_INT(id);
		SHOW_FIELD_VALUE_UINT_IP(ip1);
		SHOW_FIELD_VALUE_UINT_IP(ip2);
		SHOW_FIELD_VALUE_UINT_IP(ip3);
		SHOW_FIELD_VALUE_UINT_IP(ip4);
		SHOW_FIELD_VALUE_UINT(bWarnFlag);
		SHOW_FIELD_VALUE_UINT(bModelId);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastModTime);
		SHOW_FIELD_VALUE_INT(iNameVmemIdx);
		if(iNameVmemIdx > 0)
			printf("\t name:%s\n", MtReport_GetFromVmem_Local(iNameVmemIdx));

        SHOW_FIELD_VALUE_UINT_IP(dwClientIp);
        SHOW_FIELD_VALUE_UINT_IP(dwGwIp);
        SHOW_FIELD_VALUE_UINT_IP(dwConnectIp);
        SHOW_FIELD_VALUE_INT(iTTL);
        SHOW_FIELD_VALUE_INT(iResponseMs);

		SHOW_FIELD_VALUE_INT(iDescVmemIdx);
		if(iDescVmemIdx > 0)
			printf("\t desc:%s\n", MtReport_GetFromVmem_Local(iDescVmemIdx));
		SHOW_FIELD_VALUE_CSTR(szAttrReportDay);
		SHOW_FIELD_VALUE_CSTR(sRandKey);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastVmemSyncMemcacheTime);
		SHOW_FIELD_VALUE_INT(iReportAttrVmemIdx);

		int iLen = 0;
		int *pVmemAttrs = (int*)MtReport_GetFromVmem_Local2(iReportAttrVmemIdx, &iLen);
		if(pVmemAttrs!= NULL && iLen > 0) {
			iLen /= sizeof(int);
			SHOW_FIELD_VALUE_INT_COUNT(iLen, pVmemAttrs);
		}

		SHOW_FIELD_VALUE_UINT_TIME(dwLastReportAttrTime);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastReportLogTime);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastHelloTime);
		SHOW_FIELD_VALUE_UINT_TIME(dwAgentStartTime);
		SHOW_FIELD_VALUE_INT(iPreIndex);
		SHOW_FIELD_VALUE_INT(iNextIndex);
	}
}MachineInfo;

#define MACHINE_HASH_NODE_COUNT 3011
#define DEF_MACHINE_SHM_KEY 3333317 

#define MAX_MONITOR_MACHINE_COUNT 1000
#define DEF_MONITOR_MACHINE_SHM_KEY 2333317 
typedef struct
{
	int32_t id;
	uint32_t ip;
	uint16_t port;
	uint32_t weight;
	uint32_t cur_weight;
	char sReserved[8];
}MonitorMachineInfo;

typedef struct
{
	int count;
	MonitorMachineInfo stInfo[MAX_MONITOR_MACHINE_COUNT];
	char sReserved[16];
}MonitorMachineList;

#define SLOG_ERROR_LINE -__LINE__

// 日志缓冲记录数 --- 
#define BWORLD_MAX_SHM_SLOG_COUNT 10000 // 内存log 条数

#define SLOG_KEY_MAX_LENGTH 128 // 关键字的最大长度
#define SLOG_MAX_KEY_COUNT 10 // 关键字的最多数目

#define BWORLD_MEMLOG_BUF_LENGTH 256 // 常规的单条日志长度 , 定义至少大于 128 
#define BWORLD_SLOG_MAX_LINE_LEN 1024 // 单条日志的最大长度 
#define TOO_LONG_LOG_TRUNCATE_STR "  [slog too long truncate ...]"
#define LOG_TRUNCATE_STR_LENGTH strlen(TOO_LONG_LOG_TRUNCATE_STR)

#define BWORLD_SLOG_TYPE_LOCAL 1 // 本地文件 log, 生产进程直接写文件
#define BWORLD_SLOG_TYPE_NET 2 // 网络 log, 生产进程写 shm, 日志管理进程将 log 传输到集中管理服务器
#define BWORLD_SLOG_TYPE_TO_STD 4 // 标准输出
#define BWORLD_SLOG_TYPE_LOCAL_NET 3  // 本地和网络 log

#define SEC_USEC 1000000ULL // 1 秒包含的微秒数
#define IS_SEQ_BIGER(seq1, seq2) (seq1>seq2 || seq1+BWORLD_MAX_SHM_SLOG_COUNT<seq2) // seq 比较，测试回绕的情况
#define TIME_SEC_TO_USEC(sec) (sec*1000000ULL) // 秒转为微秒
#define TIME_USEC_TO_SEC(usec) (uint32_t)(usec/1000000) // 微妙转为秒

// exception deal list ----------
#define SLOG_WARN_SHM_WRITE_FULL MtReport_Attr_Add(86, 1); // 写 shm log 环形缓冲区满了

enum {
	SLOG_LEVEL_OTHER = 1,
	SLOG_LEVEL_DEBUG = 2,
	SLOG_LEVEL_INFO = 4,
	SLOG_LEVEL_WARNING = 8,
	SLOG_LEVEL_REQERROR = 16,
	SLOG_LEVEL_ERROR = 32,
	SLOG_LEVEL_FATAL = 64,
    SLOG_LEVEL_PLUGIN_TABLE = 128,
};

enum
{
	SLOG_TYPE_DEBUG = 1,
	SLOG_TYPE_INFO = 2,
	SLOG_TYPE_ERROR = 4,
	SLOG_TYPE_REQERR = 8,
};

// 每条内存日志都记录的基本信息
#define BWORLD_SLOG_BASE_FMT " [%s:%s:%d:%u:%d] "
#define BWORLD_SLOG_BASE_VAL __FILE__, __FUNCTION__, __LINE__, slog.GetLogSeq(), slog.GetPid()

// 本地磁盘日志
#define LOCAL_LOG_COND(logtype) (slog.CheckLogConfigChange() \
		&& (slog.m_iLogOutType&BWORLD_SLOG_TYPE_LOCAL) && (slog.m_bIsTestLog || \
		((slog.m_iLogType&logtype) && C2_Log_FreqControl(&slog.m_stLog, LL_FATAL, NULL))))

// 日志中心日志
#define NET_LOG_COND(logtype) (slog.CheckLogConfigChange() \
		&& (slog.m_iLogOutType&BWORLD_SLOG_TYPE_NET) && (slog.m_bIsTestLog || \
		((slog.m_iLogType&logtype) && C2_Log_FreqControl(&slog.m_stLogNet, LL_FATAL, NULL))))

#define ERR_LOG(fmt, ...) do { \
	if(slog.m_bInit) { \
		if(LOCAL_LOG_COND(SLOG_LEVEL_ERROR)) \
			C2_Log(&slog.m_stLog, LL_FATAL, (const char*)slog.m_szLogCommData, "err:" BWORLD_SLOG_BASE_FMT fmt, \
				BWORLD_SLOG_BASE_VAL, ##__VA_ARGS__); \
		if(NET_LOG_COND(SLOG_LEVEL_ERROR)) \
			slog.ShmLog(SLOG_LEVEL_ERROR, BWORLD_SLOG_BASE_FMT fmt, BWORLD_SLOG_BASE_VAL, ##__VA_ARGS__); \
	}else if(slog.m_stParam.iIsLogToStd == 1) \
		printf("(note:slog_not_init:%d) -- err: (%s:%s:%d) "fmt"\n", slog.m_bInit, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
}while(0)

#define REQERR_LOG(fmt, ...) do { \
	if(slog.m_bInit) { \
		if(LOCAL_LOG_COND(SLOG_LEVEL_REQERROR)) \
			C2_Log(&slog.m_stLog, LL_FATAL, (const char*)slog.m_szLogCommData, "reqerr: (%s:%s:%d:%u) "fmt, \
				__FILE__, __FUNCTION__, __LINE__, slog.GetLogSeq(), ##__VA_ARGS__); \
		if(NET_LOG_COND(SLOG_LEVEL_REQERROR)) \
			slog.ShmLog(SLOG_LEVEL_REQERROR, BWORLD_SLOG_BASE_FMT fmt, BWORLD_SLOG_BASE_VAL, ##__VA_ARGS__); \
	}else if(slog.m_stParam.iIsLogToStd == 1) \
		printf("(note:slog_not_init:%d) -- reqerr: (%s:%s:%d) "fmt"\n", slog.m_bInit, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
}while(0)

#define WARN_LOG(fmt, ...) do { \
	if(slog.m_bInit) { \
		if(LOCAL_LOG_COND(SLOG_LEVEL_WARNING)) \
			C2_Log(&slog.m_stLog, LL_FATAL, (const char*)slog.m_szLogCommData, "warn: (%s:%s:%d:%u) "fmt, \
				__FILE__, __FUNCTION__, __LINE__, slog.GetLogSeq(), ##__VA_ARGS__); \
		if(NET_LOG_COND(SLOG_LEVEL_WARNING)) \
			slog.ShmLog(SLOG_LEVEL_WARNING, BWORLD_SLOG_BASE_FMT fmt, BWORLD_SLOG_BASE_VAL, ##__VA_ARGS__); \
	}else if(slog.m_stParam.iIsLogToStd == 1) \
		printf("(note:slog_not_init:%d) -- warn: (%s:%s:%d) "fmt"\n", slog.m_bInit, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
}while(0)

#define FATAL_LOG(fmt, ...) do { \
	if(slog.m_bInit) { \
		if(LOCAL_LOG_COND(SLOG_LEVEL_FATAL)) \
			C2_Log(&slog.m_stLog, LL_FATAL, (const char*)slog.m_szLogCommData, "fatal: (%s:%s:%d:%u) "fmt, \
				__FILE__, __FUNCTION__, __LINE__, slog.GetLogSeq(), ##__VA_ARGS__); \
		if(NET_LOG_COND(SLOG_LEVEL_FATAL)) \
			slog.ShmLog(SLOG_LEVEL_FATAL, BWORLD_SLOG_BASE_FMT fmt, BWORLD_SLOG_BASE_VAL, ##__VA_ARGS__); \
	}else if(slog.m_stParam.iIsLogToStd == 1) \
		printf("(note:slog_not_init:%d) -- fatal: (%s:%s:%d) "fmt"\n", slog.m_bInit, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
}while(0)

#define INFO_LOG(fmt, ...) do { \
	if(slog.m_bInit) { \
		if(LOCAL_LOG_COND(SLOG_LEVEL_INFO)) \
			C2_Log(&slog.m_stLog, LL_FATAL, (const char*)slog.m_szLogCommData, "info: (%s:%s:%d:%u) "fmt, \
				__FILE__, __FUNCTION__, __LINE__, slog.GetLogSeq(), ##__VA_ARGS__); \
		if(NET_LOG_COND(SLOG_LEVEL_INFO)) \
			slog.ShmLog(SLOG_LEVEL_INFO, BWORLD_SLOG_BASE_FMT fmt, BWORLD_SLOG_BASE_VAL, ##__VA_ARGS__); \
	}else if(slog.m_stParam.iIsLogToStd == 1) \
		printf("(note:slog_not_init:%d) -- info: (%s:%s:%d) "fmt"\n", slog.m_bInit, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
}while(0)

#define DEBUG_LOG(fmt, ...) do { \
	if(slog.m_bInit) { \
		if(LOCAL_LOG_COND(SLOG_LEVEL_DEBUG)) \
			C2_Log(&slog.m_stLog, LL_FATAL, (const char*)slog.m_szLogCommData, "debug:" BWORLD_SLOG_BASE_FMT fmt, \
				BWORLD_SLOG_BASE_VAL, ##__VA_ARGS__); \
		if(NET_LOG_COND(SLOG_LEVEL_DEBUG)) \
			slog.ShmLog(SLOG_LEVEL_DEBUG, BWORLD_SLOG_BASE_FMT fmt, BWORLD_SLOG_BASE_VAL, ##__VA_ARGS__); \
	}else if(slog.m_stParam.iIsLogToStd == 1) \
		printf("(note:slog_not_init:%d) -- debug: (%s:%s:%d) "fmt"\n", slog.m_bInit, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
}while(0)

// 带日志对象的写 log 宏
#define BWORLD_SLOG_BASE_FMT_API " api - [%s:%s:%d] "
#define BWORLD_SLOG_BASE_VAL_API __FILE__, __FUNCTION__, __LINE__

// 本地磁盘日志
#define LOCAL_LOG_COND_API(pslog, logtype) ((pslog->m_iLogOutType&BWORLD_SLOG_TYPE_LOCAL) \
	&& (pslog->m_bIsTestLog || ((pslog->m_iLogType&logtype) && C2_Log_FreqControl(&pslog->m_stLog, LL_FATAL, NULL))))

// 日志中心日志
#define NET_LOG_COND_API(pslog, logtype) ((pslog->m_iLogOutType&BWORLD_SLOG_TYPE_NET) \
	&& (pslog->m_bIsTestLog || ((pslog->m_iLogType&logtype) && C2_Log_FreqControl(&pslog->m_stLogNet, LL_FATAL, NULL))))

#define ERR_LOG_API(pslog, fmt, ...) do { \
	if(pslog!=NULL && pslog->m_bInit) { \
		if(LOCAL_LOG_COND_API(pslog, SLOG_LEVEL_ERROR)) \
			C2_Log(&pslog->m_stLog, LL_FATAL, (const char*)slog.m_szLogCommData, "api - err: (%s:%s:%d) "fmt, \
				__FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		if(NET_LOG_COND_API(pslog, SLOG_LEVEL_ERROR)) \
			pslog->ShmLog(SLOG_LEVEL_ERROR, BWORLD_SLOG_BASE_FMT_API fmt, BWORLD_SLOG_BASE_VAL_API, ##__VA_ARGS__); \
	}else if(pslog->m_stParam.iIsLogToStd == 1) \
		printf("(note:slog_not_init:%d) -- api - err: (%s:%s:%d|%p|%d) "fmt"\n", (pslog!=NULL ? pslog->m_bInit : 0), \
			__FILE__, __FUNCTION__, __LINE__, pslog, (pslog!=NULL ? pslog->m_bInit : -1), ##__VA_ARGS__); \
}while(0)

#define REQERR_LOG_API(pslog, fmt, ...) do { \
	if(pslog!=NULL && pslog->m_bInit) { \
		if(LOCAL_LOG_COND_API(pslog, SLOG_LEVEL_REQERROR)) \
			C2_Log(&pslog->m_stLog, LL_FATAL, (const char*)slog.m_szLogCommData, "api - reqerr: (%s:%s:%d) "fmt, \
				__FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		if(NET_LOG_COND_API(pslog, SLOG_LEVEL_REQERROR)) \
			pslog->ShmLog(SLOG_LEVEL_REQERROR, BWORLD_SLOG_BASE_FMT_API fmt, BWORLD_SLOG_BASE_VAL_API, ##__VA_ARGS__); \
	}else if(pslog->m_stParam.iIsLogToStd == 1) \
		printf("(note:slog_not_init:%d) -- api - reqerr: (%s:%s:%d|%p|%d) "fmt"\n", (pslog!=NULL ? pslog->m_bInit : 0), \
			__FILE__, __FUNCTION__, __LINE__, pslog, (pslog!=NULL ? pslog->m_bInit : -1), ##__VA_ARGS__); \
}while(0)

#define WARN_LOG_API(pslog, fmt, ...) do { \
	if(pslog!=NULL && pslog->m_bInit) { \
		if(LOCAL_LOG_COND_API(pslog, SLOG_LEVEL_WARNING)) \
			C2_Log(&pslog->m_stLog, LL_FATAL, (const char*)slog.m_szLogCommData, "api - warn: (%s:%s:%d) "fmt, \
				__FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		if(NET_LOG_COND_API(pslog, SLOG_LEVEL_WARNING)) \
			pslog->ShmLog(SLOG_LEVEL_WARNING, BWORLD_SLOG_BASE_FMT_API fmt, BWORLD_SLOG_BASE_VAL_API, ##__VA_ARGS__); \
	}else if(pslog->m_stParam.iIsLogToStd == 1) \
		printf("(note:slog_not_init:%d) -- api - warn: (%s:%s:%d|%p|%d) "fmt"\n", (pslog!=NULL ? pslog->m_bInit : 0), \
			__FILE__, __FUNCTION__, __LINE__, pslog, (pslog!=NULL ? pslog->m_bInit : -1), ##__VA_ARGS__); \
}while(0)

#define FATAL_LOG_API(pslog, fmt, ...) do { \
	if(pslog!=NULL && pslog->m_bInit) { \
		if(LOCAL_LOG_COND_API(pslog, SLOG_LEVEL_FATAL)) \
			C2_Log(&pslog->m_stLog, LL_FATAL, (const char*)slog.m_szLogCommData, "api - fatal: (%s:%s:%d) "fmt, \
				__FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		if(NET_LOG_COND_API(pslog, SLOG_LEVEL_FATAL)) \
			pslog->ShmLog(SLOG_LEVEL_FATAL, BWORLD_SLOG_BASE_FMT_API fmt, BWORLD_SLOG_BASE_VAL_API, ##__VA_ARGS__); \
	}else if(pslog->m_stParam.iIsLogToStd == 1) \
		printf("(note:slog_not_init:%d) -- api - fatal: (%s:%s:%d|%p|%d) "fmt"\n", (pslog!=NULL ? pslog->m_bInit : 0), \
			__FILE__, __FUNCTION__, __LINE__, pslog, (pslog!=NULL ? pslog->m_bInit : -1), ##__VA_ARGS__); \
}while(0)

#define INFO_LOG_API(pslog, fmt, ...) do { \
	if(pslog!=NULL && pslog->m_bInit) { \
		if(LOCAL_LOG_COND_API(pslog, SLOG_LEVEL_INFO)) \
			C2_Log(&pslog->m_stLog, LL_FATAL, (const char*)slog.m_szLogCommData, "api - info: (%s:%s:%d) "fmt, \
				__FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		if(NET_LOG_COND_API(pslog, SLOG_LEVEL_INFO)) \
			pslog->ShmLog(SLOG_LEVEL_INFO, BWORLD_SLOG_BASE_FMT_API fmt, BWORLD_SLOG_BASE_VAL_API, ##__VA_ARGS__); \
	}else if(pslog->m_stParam.iIsLogToStd == 1) \
		printf("(note:slog_not_init:%d) -- api - info: (%s:%s:%d|%p|%d) "fmt"\n", (pslog!=NULL ? pslog->m_bInit : 0), \
			__FILE__, __FUNCTION__, __LINE__, pslog, (pslog!=NULL ? pslog->m_bInit : -1), ##__VA_ARGS__); \
}while(0)

#define DEBUG_LOG_API(pslog, fmt, ...) do { \
	if(pslog!=NULL && pslog->m_bInit) { \
		if(LOCAL_LOG_COND_API(pslog, SLOG_LEVEL_DEBUG)) \
			C2_Log(&pslog->m_stLog, LL_FATAL, (const char*)slog.m_szLogCommData, "api - debug: (%s:%s:%d) "fmt, \
				__FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		if(NET_LOG_COND_API(pslog, SLOG_LEVEL_DEBUG)) \
			pslog->ShmLog(SLOG_LEVEL_DEBUG, BWORLD_SLOG_BASE_FMT_API fmt, BWORLD_SLOG_BASE_VAL_API, ##__VA_ARGS__); \
	}else if(pslog->m_stParam.iIsLogToStd == 1) \
		printf("(note:slog_not_init:%d) -- api - debug: (%s:%s:%d|%p|%d) "fmt"\n", (pslog!=NULL ? pslog->m_bInit : 0), \
			__FILE__, __FUNCTION__, __LINE__, pslog, (pslog!=NULL ? pslog->m_bInit : -1), ##__VA_ARGS__); \
}while(0)


#define IS_TEST() (slog.m_bInit && (slog.m_bIsTestLog))

#define SLOG_CONTENT_INDEX_MAX (-1)

#define SLOG_LOG_FILES_COUNT_MAX 400 // 单个应用日志文件最大数目
#define SLOG_LOG_RECORDS_COUNT_MAX 200000 // 每个日志文件最多存的日志记录数目(可以改变)

#define SLOG_CHECK_BYTES_ORDER_NUM 1223 // 用于检测机器字节序的整数
#define SLOG_FILE_FLAG_INDEX 1
#define SLOG_FILE_FLAG_CONTENT 2
#define SLOG_LOG_SIZE_FILE "/tmp/_slog_size_" // 日志文件占用磁盘空间

// 日志字段定义
#define SLOG_FIELD_TIME 1
#define SLOG_FIELD_ADDR 2 
#define SLOG_FIELD_CUST_1 4
#define SLOG_FIELD_APP 8
#define SLOG_FIELD_MODULE 16
#define SLOG_FIELD_CUST_2 32 
#define SLOG_FIELD_CONTENT 64 
#define SLOG_FIELD_CONFIG 128 
#define SLOG_FIELD_CUST_3 256 
#define SLOG_FIELD_TYPE 512 
#define SLOG_FIELD_CUST_4 1024 
#define SLOG_FIELD_POS 2048 
#define SLOG_FIELD_CUST_5 4096 
#define SLOG_FIELD_CUST_6 8192 

#define SLOG_FILE_VERSION_CUR 2
#define SLOG_FILE_VERSION_MIN 1
#define SLOG_FILE_VERSION_MAX 2
#define SLOG_FILE_VERSION_1 1  // 单条日志记录对应结构: SLogFileLogIndex
#define SLOG_FILE_VERSION_2 2  // 单条日志记录对应结构: SLogFileLogIndex_ver2

typedef struct
{
	char szCheckDigit[40];
	uint16_t wCheckByteOrder;

	// 以下两个用于历史日志查询、确定日志先后顺序
	uint64_t qwLogTimeStart;  // 不一定等于 SLogFileInfo 中的 qwLogTimeStart
	uint64_t qwLogTimeEnd;

	int32_t iLogRecordsWrite; // 该文件当前写入的日志数
	int32_t iLogRecordsMax; // 该文件最大允许写入的日志记录数目
	uint8_t bLogFileVersion; 
	TLogStatInfo stLogStatInfo;
	uint32_t dwReserved1;
	uint32_t dwReserved2;
	int32_t iReserved1;
	int32_t iReserved2;
	char sReserved[220]; // 256 - stLogStatInfo = 220
}SLogFileHead; // slog 文件头部

typedef struct
{
	uint32_t dwCust_1;
	uint32_t dwCust_2;
	int32_t iCust_3;
	int32_t iCust_4;
	char szCust_5[16];
	char szCust_6[32];
	uint32_t dwLogConfigId; // 产生该条日志的配置编号
	uint32_t dwLogHost; // 产生该条日志的机器
	int32_t iAppId;
	int32_t iModuleId;
	uint16_t wLogType;
	uint32_t dwLogSeq;
	uint64_t qwLogTime;
	uint32_t dwLogContentLen;
	uint32_t dwLogContentPos;
}SLogFileLogIndex; // 单条日志的概要信息

typedef struct
{
	uint8_t bLogCustLen;
	uint32_t dwLogCustPos;

	uint32_t dwLogConfigId; // 产生该条日志的配置编号
	uint32_t dwLogHost; // 产生该条日志的机器
	int32_t iAppId;
	int32_t iModuleId;
	uint16_t wLogType;
	uint32_t dwLogSeq;
	uint64_t qwLogTime;
	uint32_t dwLogContentLen;
	uint32_t dwLogContentPos;
}SLogFileLogIndex_ver2; // 单条日志的概要信息 -- 对应 FILE_VERSION 为 2 的文件

typedef struct 
{
	char szAbsFileName[256];
	uint64_t qwLogTimeStart; // 通过文件名获取到的日志起始时间, 用于文件删除
	SLogFileHead stFileHead;
}SLogFileInfo; // 日志文件相关信息

typedef struct
{
	uint16_t wLogFileCount;
	int32_t iAppId;
	uint32_t dwReserved;

	SLogFileInfo stFiles[SLOG_LOG_FILES_COUNT_MAX];
	void Show() {
		SHOW_FIELD_VALUE_UINT(wLogFileCount);
		SHOW_FIELD_VALUE_INT(iAppId);
	}
}SLogFile;

#define MTLOG_CUST_FLAG_C1_SET 1 // 自定义字段1设置
#define MTLOG_CUST_FLAG_C2_SET 2
#define MTLOG_CUST_FLAG_C3_SET 4

#define MTLOG_CUST_FLAG_C4_SET 8
#define MTLOG_CUST_FLAG_C5_SET 16
#define MTLOG_CUST_FLAG_C6_SET 32
typedef struct
{
	uint8_t bCustFlag;
	uint32_t dwCust_1;
	uint32_t dwCust_2;
	int32_t iCust_3;
	int32_t iCust_4;
	char szCust_5[16];
	char szCust_6[32];

	uint32_t dwLogConfigId; // 产生该条日志的配置编号
	uint32_t dwLogHost; // 产生该条日志的机器编号
	uint32_t dwLogSeq; // 该条日志的 seq, seq 逐渐变大, 一定时间内唯一, 为 0 表示对应数组元素还没写入 log
	uint64_t qwLogTime; // 日志产生时间(单位:微秒), 可能不唯一
	int32_t iAppId; // wait delete ---
	int32_t iModuleId;
	uint16_t wLogType;
	int32_t  iContentIndex; // 内容超过 BWORLD_MEMLOG_BUF_LENGTH 部分存储在 vmem 中的位置索引
	uint32_t dwStartWriteLogTime;
	char cReserved[12];
	char sLogContent[BWORLD_MEMLOG_BUF_LENGTH];
}TSLog; // 内存日志

typedef struct
{
	uint8_t bCustFlag;
	uint32_t dwCust_1;
	uint32_t dwCust_2;
	int32_t iCust_3;
	int32_t iCust_4;
	char szCust_5[16];
	char szCust_6[32];
	uint32_t dwLogConfigId; // 产生该条日志的配置编号
	uint32_t dwLogHost; // 产生该条日志的机器
	uint32_t dwLogSeq;
	int32_t iAppId;
	int32_t iModuleId;
	uint16_t wLogType;
	uint64_t qwLogTime;
	char *pszLog;
}TSLogOut; // 日志查询的输出信息

typedef struct
{
	volatile uint32_t dwLogSeq;  // 下次写 log 的 seq, seq 必须不为 0
	volatile uint8_t bTryGetLogIndex; // 多进程写Log取索引时使用, 0 表示可取，1 表示需要等待

	// slog 有效日志起始索引 -- slog_client, slog_write 取log时会调整该字段
	// iLogStarIndex 小于0 表示日志数目为 0
	volatile int32_t iLogStarIndex; 
	volatile int32_t iWriteIndex; // 下次写 log 的写索引
	int32_t iAppId;
	int32_t iLogMaxCount; //  sLogList 数组大小

	uint32_t dwLogHourStartTime; // 最近一小时起始计算时间(用于展示最近一小时平均每分钟的日志量)
	int32_t iLogWriteTotalHour; // 最近一小时内总共写入的日志记录数(用于展示最近一小时平均每分钟的日志量)
	uint32_t dwLogFreqStartTime; // 日志频率限制开始计算时间(按每分钟计)
	int32_t iLogWriteCount; // 统计时间内已写入日志记录数

	char sLogReserved[236]; // 保留
	TSLog sLogList[0]; // 该数组用作环形数组（注意：)

	void Show() {
		SHOW_FIELD_VALUE_UINT(dwLogSeq);
		SHOW_FIELD_VALUE_UINT(bTryGetLogIndex);
		SHOW_FIELD_VALUE_INT(iLogStarIndex);
		SHOW_FIELD_VALUE_INT(iWriteIndex);
		SHOW_FIELD_VALUE_INT(iAppId);
		SHOW_FIELD_VALUE_INT(iLogMaxCount);
		SHOW_FIELD_VALUE_UINT_TIME(dwLogHourStartTime);
		SHOW_FIELD_VALUE_INT(iLogWriteTotalHour);
		SHOW_FIELD_VALUE_UINT_TIME(dwLogFreqStartTime);
		SHOW_FIELD_VALUE_INT(iLogWriteCount);
	}
}TSLogShm; // 内存日志结构头部 

// --- 索引双向链表  --- start
#define ILINK_SET_FIRST(head, s, e, node, p, n, nidx) \
	head->s = nidx; \
	head->e = nidx; \
	node->p = -1; \
	node->n = -1;
//
// s -- 第一个节点索引
// e -- 最后一个节点索引
// node -- 新节点指针
// nodeIdx -- 新节点索引
// p -- 前一个节点索引
// n -- 后一个节点索引
// first -- 当前第一个节点指针
#define ILINK_INSERT_FIRST(head, s, node, p, n, first, nodeIdx) \
	node->p = -1; \
	node->n = head->s; \
	first->p = nodeIdx; \
	head->s = nodeIdx; 

//
// e -- 最后一个节点索引
// node -- 新节点指针
// nodeIdx -- 新节点索引
// p -- 前一个节点索引
// n -- 后一个节点索引
// last -- 当前最后一个节点指针
#define ILINK_INSERT_LAST(head, e, node, p, n, last, nodeIdx) \
	node->p = head->e; \
	node->n = -1; \
	last->n = nodeIdx; \
	head->e = nodeIdx;

#define ILINK_DELETE_NODE(head, s, e, pNode, pPrev, pNext, p, n) \
	if(pPrev == NULL) \
		head->s = pNode->n; \
	else \
		pPrev->n = pNode->n; \
	if(pNext == NULL) \
		head->e = pNode->p; \
	else \
		pNext->p = pNode->p; \
	pNode->p = -1; \
	pNode->n = -1;

// --- 索引双向链表  --- end 

#define DEF_WARN_INFO_SHM_KEY 2019020113 
#define WARN_INFO_HASH_NODE_COUNT 10000 
#define MAX_WARN_INFO_IN_SHM_PER_USER 200
typedef struct 
{
	uint32_t id;
	int32_t iWarnTypeId;
	int32_t iAttrId;
	uint32_t dwWarnTime;
	uint32_t dwWarnFlag;
	uint32_t dwWarnVal;
	uint32_t dwWarnConfVal;
	int32_t iWarnTimes;
	int32_t iDealStatus;
	uint32_t dwLastWarnTime;

	// 用于 MtSystemConfig 中连接提示
	int32_t iPreIndex;
	int32_t iNextIndex;

	uint32_t dwLastUpdateTime;
	char sReserved[4];
	void Show() {
		SHOW_FIELD_VALUE_UINT(id);
		SHOW_FIELD_VALUE_INT(iWarnTypeId);
		SHOW_FIELD_VALUE_INT(iAttrId);
		SHOW_FIELD_VALUE_UINT_TIME(dwWarnTime);
		SHOW_FIELD_VALUE_UINT(dwWarnFlag);
		SHOW_FIELD_VALUE_UINT(dwWarnVal);
		SHOW_FIELD_VALUE_UINT(dwWarnConfVal);
		SHOW_FIELD_VALUE_INT(iWarnTimes);
		SHOW_FIELD_VALUE_INT(iDealStatus);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastWarnTime);
		SHOW_FIELD_VALUE_INT(iPreIndex);
		SHOW_FIELD_VALUE_INT(iNextIndex);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastUpdateTime);
	}
}TWarnInfo;

#define MTREPORT_CLIENT_ACCESS_KEY_LEN 32

// 用于　client , server 交互相关的数据结构 --------------- start
#define MT_CLIENT_NODE_COUNT 10000
#define MT_CLIENT_HASH_KEY 2014112942
#define MT_CLIENT_KEY_TIMEOUT_MIN 1440 // key 过期时间

// 用于 slog_mtreport_server 与 mtreport_client 交互
typedef struct 
{
	uint32_t dwFirstHelloTime;
	uint32_t dwLastHelloTime; // 最近一次接收到正确 hello 数据包的时间
	uint32_t dwAddress; // 客户端接入的 IPv4 地址
	uint16_t wBasePort; // hello 端口，还用于接收配置改变
	uint32_t dwAgentClientAddress; // mtreport_client 所在机器的监控 ip 
	int32_t iServerResponseTimeMs; // 服务器最大响应时间
	int32_t iClientResponseTimeMs; // 该客户端的最大响应时间(push)

	int32_t iMachineId; // client 所属的 machine id

	// 以下 key 在首个 hello 交互时设置
	uint8_t bEnableEncryptData;
	char sRandKey[17]; // 数据上报加密 key 16 字节带结尾 0

	int32_t iPreIndex;
	int32_t iNextIndex;

	char sReserved[32];
	void Show() {
		SHOW_FIELD_VALUE_UINT_TIME(dwFirstHelloTime);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastHelloTime);
		SHOW_FIELD_VALUE_UINT_IP(dwAddress);
		SHOW_FIELD_VALUE_UINT(wBasePort);
		SHOW_FIELD_VALUE_UINT_IP(dwAgentClientAddress);
		SHOW_FIELD_VALUE_INT(iServerResponseTimeMs);
		SHOW_FIELD_VALUE_INT(iClientResponseTimeMs);
		SHOW_FIELD_VALUE_INT(iMachineId);
		SHOW_FIELD_VALUE_UINT(bEnableEncryptData);
		SHOW_FIELD_VALUE_CSTR(sRandKey);
		SHOW_FIELD_VALUE_INT(iPreIndex);
		SHOW_FIELD_VALUE_INT(iNextIndex);
	}
}MtClientInfo;

#pragma pack()

class CLogTimeCur
{
	public:
		CLogTimeCur();
		uint64_t GetUsec() { return m_qwTime; }
		uint32_t GetSec() { return m_dwTimeSec; }

	private:
		uint32_t m_dwTimeSec;
		uint32_t m_dwTimeUsec;
		uint64_t m_qwTime;
};

// 日志记录类 ----- 用于将日志写入本地文件或者共享内存 - 用于日志生产模块写日志 ------------------------
#define COUNT_STATIC_TIMES_PER_DAY 1440
inline int GetDayOfMin(TIME_INFO *pinfo, int iStaticTime)
{
	if(iStaticTime <= 0)
		iStaticTime =1 ;
	int i = pinfo->hour*60+pinfo->min;
	i /= iStaticTime;
	if((i%iStaticTime) != 0 || pinfo->sec > 0)
		i++;
	i--; // 编号从 0 开始
	if(i < 0)
		i = 0;
	if(i >= COUNT_STATIC_TIMES_PER_DAY)
		i = COUNT_STATIC_TIMES_PER_DAY-1;
	return i;
}

inline bool IsValidStaticTime(int iStaticTime)
{
	if(iStaticTime != 1 && iStaticTime!=5 && iStaticTime!=10 && iStaticTime!=15 && iStaticTime!=30
			&& iStaticTime!=60 && iStaticTime!=120 && iStaticTime!=180) 
		return false;
	return true;
}

class CMemCacheClient;
class CSupperLog: public StdLog, public IError
{
	public:
		CSupperLog();
		~CSupperLog();

		void ShowSystemConfig(const char *parg=NULL);
		int UpdateConfig(const char *pkey, const char *pval);
        int TryUpdateSysOtherInfo(Query &qu, ::user::SystemOtherInfo &other);
        int GetSystemOtherInfo(user::SystemOtherInfo &stOther);

		int SetUserRemoteAppLogInfoPb(const user::UserRemoteAppLogInfo &stPb);
		int GetUserRemoteAppLogInfoPb(user::UserRemoteAppLogInfo &stPb);

		void ShowVarConfig();
		int CheckProcExist();

		bool IsShowVer(int argc, char *argv[]) {
			if(argc > 1 && !strcmp(argv[1], "-v")) {
				printf("version info - %s\n", g_version_info.c_str());
				return true;
			}
			return false;
		}

		// 本地日志接口
		int InitForUseLocalLog(const char *pszConfigFile);

		// 一般应用模块初始函数
		int InitCommon(const char *pszConfigFile);

		// 通过配置文件初始化日志参数
		int InitConfigByFile(const char *pszConfigFile, bool bCreateShm=false);
		
		// slog 网络日志初始化接口，相关参数使用数据库配置
		void SetLogAppId(int id) { m_iLogAppId = id; }
		void SetLogModuleId(int id) { m_iLogModuleId = id; }
		void SetConfigId(uint32_t id) { m_dwConfigId = id; } // 指定使用配置
		int Init();

		void Daemon(int nochdir, int noclose, int nopreFork=1);
		void ShowShmLogInfo(int iLogIndex=-1);
		void ShowVarmemInfo();
		void ShowAppShmLog(int iAppId=0, int iModuleId=0);

		// 日志参数设置接口
		void SetLogLevel(const char *plevel);
		void SetLogToStd(bool x) { m_stParam.iIsLogToStd = x; }
		void SetTestLog(bool x) { m_bIsTestLog = x; }
		void SetLogSpeed(int32_t iSpeed) { m_stParam.iWriteSpeed = iSpeed; }
		void SetLogTime(bool x) { m_stParam.iIsLogTime = x; }
		void SetLogNum(int32_t iNum) { m_stParam.dwMaxLogNum = iNum; }
		void SetLogFile(const char *pszLogFile){ 
			m_strLogFile = pszLogFile; 
			m_stParam.pszLogFullName = m_strLogFile.c_str();
		}
		void SetCommData() {
			int iWLen = 0;
			m_szLogCommData[0] = '\0';
			if(m_bCustFlag&MTLOG_CUST_FLAG_C1_SET)
				iWLen += sprintf(m_szLogCommData+iWLen, "%u ", m_dwCust_1);
			if(m_bCustFlag&MTLOG_CUST_FLAG_C2_SET)
				iWLen += sprintf(m_szLogCommData+iWLen, "%u ", m_dwCust_2);
			if(m_bCustFlag&MTLOG_CUST_FLAG_C3_SET)
				iWLen += sprintf(m_szLogCommData+iWLen, "%d ", m_iCust_3);
			if(m_bCustFlag&MTLOG_CUST_FLAG_C4_SET)
				iWLen += sprintf(m_szLogCommData+iWLen, "%d ", m_iCust_4);
			if(m_bCustFlag&MTLOG_CUST_FLAG_C5_SET)
				iWLen += sprintf(m_szLogCommData+iWLen, "%s ", m_szCust_5);
			if(m_bCustFlag&MTLOG_CUST_FLAG_C6_SET)
				iWLen += sprintf(m_szLogCommData+iWLen, "%s ", m_szCust_6);
		}
		void SetLogType(int iLogType) { m_iLogType = iLogType; }
		void SetLogOutType(int iLogOutType) { m_iLogOutType = iLogOutType; }
		void SetCust_1(uint32_t dwCust) {
			m_dwCust_1 = dwCust; m_bCustFlag |= MTLOG_CUST_FLAG_C1_SET; 
			SetCommData();
		}
		void SetCust_2(uint32_t dwCust) { 
			m_dwCust_2 = dwCust; m_bCustFlag |= MTLOG_CUST_FLAG_C2_SET; 
			SetCommData();
		}
		void SetCust_3(int32_t iCust) {
			m_iCust_3 = iCust; m_bCustFlag |= MTLOG_CUST_FLAG_C3_SET; 
			SetCommData();
		}
		void SetCust_4(int32_t iCust) {
			m_iCust_4 = iCust; m_bCustFlag |= MTLOG_CUST_FLAG_C4_SET; 
			SetCommData();
		}
		void SetCust_5(const char *pstrCust) {
			strncpy(m_szCust_5, pstrCust, sizeof(m_szCust_5)-1);
			m_bCustFlag |= MTLOG_CUST_FLAG_C5_SET; 
			SetCommData();
		}
		void SetCust_6(const char *pstrCust) {
			strncpy(m_szCust_6, pstrCust, sizeof(m_szCust_6)-1);
			m_bCustFlag |= MTLOG_CUST_FLAG_C6_SET;
			SetCommData();
		}
		void ClearAllCust() {
			m_dwCust_1=0; m_dwCust_2=0; m_iCust_3=0; m_iCust_4=0; m_szCust_5[0]='\0'; m_szCust_6[0]='\0';
			m_bCustFlag = 0;
		}

		// 在 slog_mtreport_server 上的请求终端信息 
		// 在 mtreport_server 上的请求终端信息 
		bool IsMtClientValid(MtClientInfo* pInfo) {
			if(!m_pShmConfig || !pInfo)
				return false;

			if(pInfo->iMachineId == 0
				|| pInfo->dwLastHelloTime + 3*m_pShmConfig->stSysCfg.wHelloRetryTimes
					*m_pShmConfig->stSysCfg.wHelloPerTimeSec <= m_stNow.tv_sec)
				return false;
			return true; 
		}

		MtClientInfo* GetMtClientInfo(int32_t iMachineId, uint32_t *piIsFind);
		MtClientInfo* GetMtClientInfo(int32_t iMachineId, int32_t index);
		MtClientInfo* GetMtClientInfo(int32_t index) {
			return (MtClientInfo*)NOLIST_HASH_INDEX_TO_NODE(&m_stHashMtClient, index);
		}
		int GetMtClientInfoIndex(MtClientInfo *pNode) { 
			return NOLIST_HASH_NODE_TO_INDEX(&m_stHashMtClient, pNode);
		}
		void FollowShowClientList(int iCount, int32_t iStartIdx);
		int InitMtClientInfo();

		// slog 配置获取接口 --- start
		AppInfo * GetAppInfoSelf() { return m_pAppInfo; }
		AppInfo * GetAppInfo(int32_t iAppId);
		AppInfo * GetAppInfoByIndex(int32_t idx) {
			if(NULL == m_pShmAppInfo || idx < 0 || idx > MAX_SLOG_APP_COUNT)
				return NULL;
			return m_pShmAppInfo->stInfo+idx;
		}
		void ShowApp(int32_t iAppId=0);
		int32_t GetAppInfo(int32_t iAppId, int32_t *piFirstFree);
		SLogAppInfo *GetAppInfo(){return m_pShmAppInfo;}
		const char *GetLocalIP(){ return m_strLocalIP.c_str(); }
		void FollowShowAppList(int iCount, int32_t iStartIdx);

		// 日志配置
		SLogClientConfig * GetSlogConfig(uint32_t iConfigId); 
		int32_t GetSlogConfig(uint32_t iConfigId, int32_t *piFirstFree);
		SLogConfig *GetSlogConfig(){return m_pShmConfig;}
		SLogClientConfig * GetLogConfigByIndex(int32_t idx) {
			if(NULL == m_pShmConfig || idx < 0 || idx > MAX_SLOG_CONFIG_COUNT)
				return NULL;
			return m_pShmConfig->stConfig+idx;
		}
		void ShowSlogConfig(uint32_t iConfigId=0);
		void FollowShowSlogConfigList(int iCount, int32_t iStartIdx);

		bool CheckLogConfigChange() {
			DealConfigChange();
			return true;
		}

		// ip 地址查询 操作
		int InitIpInfo(bool bCreate=false);
		TIpInfo * GetIpInfo(uint32_t dwIpAddr, T_FUN_IP_CMP pfun=IpInfoSearchCmp);
		TIpInfoShm * GetIpInfoShm() { return m_pIpInfoShm; }

		// warn info 操作
		TWarnInfo* GetWarnInfo(uint32_t id, uint32_t *piIsFind);
		int InitWarnInfo();
		SharedHashTableNoList* GetWarnInfoHash() { return &m_stHashWarnInfo; }
		void ShowWarnInfo(uint32_t dwWid=0);

		// view info -- start
		void AddViewInfoCount();
		void SubViewInfoCount();
		void AddViewBindAttr(int iView, int iAttr);
		void DelViewBindAttr(int iView, int iAttr);
		void AddViewBindMach(int iView, int iMachineId);
		int AddViewBindMach(TViewInfo *pView, int iMachineId);
		void DelViewBindMach(int iView, int iMachineId);
		void ShowViewInfo(int iView);
		int GetViewInfoIndex(int iViewId, int32_t *piFirstFree);
		int GetViewInfoIndex(int iViewId);
		TViewInfo *GetViewInfo(int idx) {
			if(NULL == m_pShmConfig || idx < 0 || idx >= MAX_VIEW_INFO_COUNT)
				return NULL;
			return m_pShmConfig->stViewInfo+idx;
		}
		const char *GetViewName(int id) {
			int t_idx = GetViewInfoIndex(id, NULL);
			TViewInfo *pViewinfo = GetViewInfo(t_idx);
			if(pViewinfo != NULL && pViewinfo->iViewNameVmemIdx > 0)
				return MtReport_GetFromVmem_Local(pViewinfo->iViewNameVmemIdx);
			return g_str_unknow.c_str();
		}
		void FollowShowViewList(int iCount, int32_t iStartIdx);
		// view info -- end 

		// warn attr --- 属性提示相关 start
		TWarnConfig* GetWarnConfigInfo(int32_t iWarnId, int32_t iAttrId, uint32_t *piIsFind);
		SharedHashTable* GetWarnConfigInfoHash() { return &m_stHashWarnConfig; }
		int InitwarnConfig();

		TMachineViewConfigInfo* GetMachineViewInfo(int32_t iMachineId, uint32_t *piIsFind);
		int InitMachineViewConfig();

		TAttrViewConfigInfo* GetAttrViewInfo(int32_t iAttrId, uint32_t *piIsFind);
		int InitAttrViewConfig();

		SharedHashTable & GetWarnAttrHash() { return m_stWarnHashAttr; }
		int InitWarnAttrList();
		int InitWarnAttrListForWrite();
		TWarnAttrReportInfo* GetWarnAttrInfo(int32_t iAttrId, int32_t iMachineId, uint32_t *piIsFind);
		// warn attr --- end

		// json 格式返回监控点 attr_id 的信息
		// 成功返回 0，其它表示查找失败
		int GetAttrInfoFromShm(int32_t attr_id, Json &js);
		const char * GetAttrNameFromShm(int32_t attr_id);

		// 字符串型监控点
		StrAttrNodeValShmInfo *GetStrAttrNodeValShm(bool bCreate);
		SharedHashTable & GetStrAttrShmHash() { return m_stStrAttrHash; }
		int InitStrAttrHash();
		int InitStrAttrHashForWrite();
		TStrAttrReportInfo* GetStrAttrShmInfo(int32_t iAttrId, int32_t iMachineId, uint32_t *piIsFind);

		// attr --- attr 操作
		AttrInfoBin* GetAttrInfo(int32_t id, uint32_t *piIsFind);
		AttrInfoBin* GetAttrInfo(int32_t idx) {
			return (AttrInfoBin*)NOLIST_HASH_INDEX_TO_NODE(&m_stHashAttr, idx); 
		}
		SharedHashTableNoList* GetAttrHash() { return &m_stHashAttr; }
		int InitAttrList();
		void ShowAttrList(int32_t id=0);
		void FollowShowAttrList(int iCount, int32_t iStartIdx);

		// attrtype --- attrtype 操作
		AttrTypeInfo* GetAttrTypeInfo(int32_t id, uint32_t *piIsFind);
		SharedHashTableNoList* GetAttrTypeHash() { return &m_stHashAttrType; }
		int InitAttrTypeList();
		void ShowAttrTypeList(int32_t id=0);

		MtSystemConfig *GetSystemCfg() { if(m_pShmConfig == NULL) return NULL; return &m_pShmConfig->stSysCfg; }

		// monitor --- machine 操作
		const char * GetMachineName(int id) {
			MachineInfo *pMachinfo = GetMachineInfo(id, NULL);
			if(pMachinfo != NULL && pMachinfo->iNameVmemIdx > 0)
				return MtReport_GetFromVmem_Local(pMachinfo->iNameVmemIdx);
			return g_str_unknow.c_str();
		}
		int GetLocalMachineId() { return (m_pShmConfig ? m_pShmConfig->stSysCfg.iMachineId : 0); }
		int IsIpMatchMachine(MachineInfo*pMach, uint32_t dwIp);
		int IsIpMatchLocalMachine(uint32_t dwIp);
		int IsIpMatchMachine(uint32_t dwIp, int32_t iMachineId);
		MachineInfo* GetMachineInfo(int32_t id, uint32_t *piIsFind);
		MachineInfo* GetMachineInfo(MtSystemConfig *pUmInfo, uint32_t dwClientIp);
		MachineInfo* GetMachineInfo(uint32_t dwClientIp);
		MachineInfo* GetMachineInfo(int idx) {
			return (MachineInfo*)NOLIST_HASH_INDEX_TO_NODE(&m_stHashMachine, idx);
		}
		SharedHashTableNoList* GetMachineHash() { return &m_stHashMachine; }
		MachineInfo* GetMachineInfoByIp(char *szIp) { return GetMachineInfo(inet_addr(szIp)); }
		int InitMachineList();
		void ShowMachineList(int32_t id=0);
		void FollowShowMachineList(int iCount, int32_t iStartIdx);

		// 服务器管理
		int32_t GetServerInfo(uint32_t srv_id, int32_t *piFirstFree);
		SLogServer* GetAttrServer(const char *psrvIp=NULL);
		SLogServer* GetAppLogServer(const char *psrvIp=NULL);
		SLogServer* GetAppMasterSrv(int32_t app_id, bool bNeedActive=true);
		SLogServer* GetAttrSrv();
		SLogServer* GetWebMasterSrv();
		SLogServer* GetServerByType(int iType, int *piStartIdx=NULL);
		SLogServer* GetValidServerByType(int iType, int *piStartIdx=NULL);
		SLogServer* GetServerInfo(int idx) {
			if(NULL == m_pShmConfig || idx < 0 || idx >= MAX_SERVICE_COUNT)
				return NULL;
			return m_pShmConfig->stServerList+idx;
		}
		void ShowServerInfo(uint32_t srv_id=0);
		int CheckAppLogServer(AppInfo *papp);
		int CheckAttrServer();

		// 迁移云管系统服务器接口
		MonitorMachineList * GetMonitorMachineList();
		void ShowMonitorMachineList();
		int32_t GetMonitorMachineInfo(int32_t id, int32_t *piFirstFree);

		int32_t GetAppModuleInfo(int32_t iAppIndex, int32_t iModuleId, int32_t *piFirstFree);
		int32_t GetTestKey(int iConfigIndex, uint8_t bKeyType, const char *pszKey, int32_t *piFirstFree);
		// slog 配置获取接口 --- end 

		uint32_t GetConfigId(){return m_dwConfigId;}

		int GetAppPort();

		// 设置染色 log 标志
		// isTest 返回1,表示命中染色,设置染色标记; isTest 返回 0,则清除染色标记
		void CheckTest(int (*isTest)(int iTestCfgCount, SLogTestKey *pstTestCfg, const void *pdata), 
			const void *pdata=NULL); 

		// 参数设置完后，初始化接口
		int InitSupperLog();
		bool IsExitSet() { return m_bExitProcess; }
		bool Run() {
			m_iRand = rand();
			gettimeofday(&m_stNow, 0);
			sv_SetCurTime(m_stNow.tv_sec);
			return !m_bExitProcess; 
		}

		// 平滑重启接口，重启时可以给进程一定时间用于回收资源
		bool TryRun() {
			m_iRand = rand();
			gettimeofday(&m_stNow, 0);
			sv_SetCurTime(m_stNow.tv_sec);
			if(m_bExitProcess) 
				return (m_stNow.tv_sec <= m_dwRecvExitSigTime+m_iMaxExitWaitTime);
			return true;
		}
		int GetPid() { return m_iPid; }

		// 写日志接口
		void ShmLog(int iLogLevel, const char *pszFmt, ...);
		int WriteAppLogToShm(TSLogShm* pShmLog, LogInfo *pLog, int32_t dwLogHost);
		void RemoteShmLog(::top::SlogLogInfo &stLog, TSLogShm* pShmLog=NULL);
		TSLogShm* GetAppLogShm(AppInfo *pAppShmInfo, bool bTryCreate=false);
		uint32_t GetLogSeq() { 
			if(NULL == m_pShmLog) return 0; 
			return (m_pShmLog->dwLogSeq != 0 ? m_pShmLog->dwLogSeq : 1); 
		}

		// 显示前 iCount 条日志
		void ShowShmLog(int iCount=10);
		TSLogShm* GetAppLogShmSelf() { return m_pShmLog; }

		// libSocket 库调用接口
		void error(ISocketHandler *,Socket *,const std::string& call,int err,const std::string& sys_err,loglevel_t);

		// supper class IError for libMysql 库调用接口
		void error(Database& db, const std::string& errmsg);
		void debug(Database& db, const std::string& errmsg);
		void info(Database& db, const std::string& errmsg);
		void error(Database& db, Query& qu, const std::string& errmsg);

		// memcache
		int InitMemcache();
		bool IsEnableMemcache() { return m_bEnableMemcache; }

		// realinfo
		int InitChangeOtherInfoShm();
		void EndChangeOtherInfoShm();

		static int InitGetShmLogIndex(TSLogShm *pShmLog); // 获取写 shmlog 索引前
		static void EndGetShmLogIndex(TSLogShm *pShmLog);  // 获取写 shmlog 索引后
		static void ModifyLogStartIndexCmpAndSwap(TSLogShm *pShmLog, int iOld, int iNew);

		// mailinfo
		TCommSendMailInfoShm *InitMailInfoShm(int iShmKey=DEFAULT_MAILINFO_SHM_KEY, bool bCreate=false);
		int AddMailToShm(TCommSendMailInfoShm *pshm, TCommSendMailInfo &stMail);
		int AddMailToShm(TCommSendMailInfo &stMail);	
		int InitGetMailShmLock(TCommSendMailInfoShm *pshm);
		void EndGetMailShmLock(TCommSendMailInfoShm *pshm) { pshm->bLockGetShm = 0; }

	private:
		int SetLocalIpInfo(char *szLocalIp);
		std::string GetProcNameByConfFile();
		int InitLocalLog();
		int InitShmLog();
		void DealConfigChange();

	public:
		int m_iCheckProcExist;
		bool m_bEnableMemcache;
		uint32_t m_dwConfigId; // 配置编号
		int32_t m_iConfigIndex; // 配置索引
		int32_t m_iConfigShmKey;
		int32_t m_iAppInfoShmKey;
		SLogConfig *m_pShmConfig;
		SLogAppInfo *m_pShmAppInfo;
		AppInfo * m_pAppInfo;
		uint32_t m_dwConfigSeq;
		int32_t m_bIsRemoteLog;
		int32_t m_iRemoteLogToStd;

		TSLogShm *m_pShmLog;
		int32_t m_iAttrShmKey;
		SharedHashTableNoList m_stHashAttr;

		int32_t m_iIpInfoShmKey;
		TIpInfoShm *m_pIpInfoShm;

		int32_t m_iWarnAttrShmKey;
		SharedHashTable m_stWarnHashAttr;

		int32_t m_iStrAttrShmKey;
		SharedHashTable m_stStrAttrHash;

		int32_t m_iMachineViewConfigShmKey;
		SharedHashTableNoList m_stHashMachineViewConfig;

		int32_t m_iAttrViewConfigShmKey;
		SharedHashTableNoList m_stHashAttrViewConfig;

		int32_t m_iWarnConfigShmKey;
		SharedHashTable m_stHashWarnConfig;

		int32_t m_iWarnInfoShmKey;
		SharedHashTableNoList m_stHashWarnInfo;

		int32_t m_iAttrTypeShmKey;
		SharedHashTableNoList m_stHashAttrType;

		int32_t m_iMachineShmKey;
		SharedHashTableNoList m_stHashMachine;

		int32_t m_iMonitorMachineShmKey;
		MonitorMachineList *m_pShmMonitorMachine;

		std::string m_strLocalIP;
		uint32_t m_dwIpAddr;

		int32_t m_iFastCgiHits;

		int m_bExitProcess;
		char m_szLogCommData[256];
		int32_t m_iProcessId;
		int32_t m_iProcessCount;
		volatile bool m_bInit;
		uint32_t m_dwCust_1;
		uint32_t m_dwCust_2;
		int32_t m_iCust_3;
		int32_t m_iCust_4;
		char m_szCust_5[16];
		char m_szCust_6[32];
		uint8_t m_bCustFlag;
		int32_t m_iLogOutType;
		int32_t m_iLocalLogType;
		int32_t m_iLogType;
		int32_t m_iLogAppId; // 应用 ID
		int32_t m_iLogModuleId; // 模块 ID，一个应用可以有多个模块
		int32_t m_bIsTestLog;
		std::string m_strLogFile;
		C2_LogFile m_stLog;
		C2_LogFile m_stLogNet; // 这个用于远程 log 的频率限制
		C2_LogFileParam m_stParam;
		int m_iVmemShmKey;

		// 64 用于存储长度超过 BWORLD_SLOG_MAX_LINE_LEN 的是提示信息
		char m_sLogBuf[BWORLD_SLOG_MAX_LINE_LEN+64];
		char m_szCoreFile[256];
		std::string m_strConfigFile;
		CMemCacheClient & memcache;

		// 柔性重启
		int32_t m_iMaxExitWaitTime;
		uint32_t m_dwRecvExitSigTime;

		int32_t m_iPid;

		// 当前时间
		struct timeval m_stNow;
		int32_t m_iRand;

		int32_t m_iMtClientInfoShmKey;
		SharedHashTableNoList m_stHashMtClient;
};


// 日志查询类 ----- 用于实时日志或者历史日志查询-- 用于日志服务端提供查询接口 --------------------------
class CSLogSearch
{
	public:
		// 构造函数 attache 上共享内存
		CSLogSearch();
		~CSLogSearch() {
			if(m_pShmLog != NULL)
				shmdt(m_pShmLog);
			if(m_pstLogFileList != NULL)
				shmdt(m_pstLogFileList);
		}
		CSLogSearch(int iFileShmKey);

		bool IsInit() { return m_bInit; }
		void InitDefaultSearch();
		int Init();

		// 查询历史记录专有参数
		void SetFileNo(int iFileNo) { m_wFileNo = iFileNo; }
		int GetFileNo() { return m_wFileNo; }
		void SetFilePos(int iFilePos) { m_dwFilePos = iFilePos; }
		int GetFilePos() { return m_dwFilePos; }
		void SetFileCount(int iFileCount) { m_wFileCount = iFileCount; }
		int GetFileCount() { return m_wFileCount; }
		void SetFileIndexStar(int iFileStar) { m_wFileIndexStar = iFileStar; }
		int GetFileIndexStar() { return m_wFileIndexStar; }
		void SetStartTime(uint32_t dwTime) { m_qwTimeStart = TIME_SEC_TO_USEC(dwTime); }
		void SetEndTime(uint32_t dwTime) { m_qwTimeEnd = TIME_SEC_TO_USEC(dwTime); }
		uint32_t GetStartTime() { return m_qwTimeStart/SEC_USEC; }
		uint32_t GetEndTime() { return m_qwTimeEnd/SEC_USEC; }
		
		// 返回历史记录扫描完成的百分比
		int GetSearchCurFilePercent();

		// 历史记录查询是否完成
		bool IsSearchHistoryComplete();

		// 设置查询的 app, module
		int SetAppId(int iAppId);
		int SetModuleId(int iModuleId);
		int SetAppModuleInfo(int iAppId, int iModuleIdCount, int *piModuleId);
		uint32_t GetLastLogSeq(){ return (m_bInit ? m_pShmLog->dwLogSeq : 0); }

		// 上报机器
		void SetLogPath(const char *plogPath) { strncpy(m_sLogPath, plogPath, sizeof(m_sLogPath)-1); }
		void SetMachine(int iMachineId) { m_iReportMachine = iMachineId; }

		// 设置查询的日志类型
		void AddLogType(int32_t iLogType);
		void SetLogType(uint32_t dwLogTypeFlag);
		void SetLogField(int iLogField);
		int GetLogField() { return m_iLogField; }
		static uint8_t GetLogType(int32_t iLogLevel);

		// 设置查询的关键字
		// 成功返回 0， 失败返回负错误码
		int SetSearchIncKey(int iIncKeyCount, char (*sIncKeyList)[SLOG_KEY_MAX_LENGTH]);
		int AddIncludeKey(const char *pszKey);
		int SetSearchExcpKey(int iExcpKeyCount, char (*sExcpKeyList)[SLOG_KEY_MAX_LENGTH]);
		int AddExceptKey(const char *pszKey);

		int ReadLogFileVersion_1(SLogFileHead &stFileHead, FILE *fp, TSLogOut &stLogOut, int i);
		int ReadLogFileVersion_2(SLogFileHead &stFileHead, FILE *fp, TSLogOut &stLogOut, int i);

		// 指定日志是否匹配查找参数
		bool IsLogMatch(TSLog *pstLog, const char *pszLog);
		bool IsLogMatch(TSLogOut *pstLog, const char *pszLog);

		// 获取历史日志
		// 调用该接口前请调用初始化查找相关函数，设置查找参数
		// 返回值: 获取到匹配日志则返回指向日志内容的指针，日志获取完时返回 NULL
		TSLogOut * HistoryLog();

		// 获取实时日志
		// 调用该接口前请调用初始化查找相关函数，设置查找参数
		// 参数: iLastLogIndex: 上次扫描的回环数组索引 
		//       seq: log seq, 除查询条件外，log 的 seq 还要大于该 seq 
		//	     qwTimeNow: 当前时间
		// 返回值: 获取到匹配日志则返回指向日志内容的指针，实时日志获取完时返回 NULL
		TSLogOut * RealTimeLog(int32_t & iLastLogIndex, uint32_t seq, uint64_t qwTimeNow);
		SLogFile * GetLogFileShm() { return m_pstLogFileList; }


		TSLogOut * RealTimeLogNew(int32_t & iLastLogIndex, uint32_t seq, uint64_t qwTimeNow);
		TSLogShm * GetLogShm() { return m_pShmLog; }

		void SetLogFile(const char *plogFile) { m_strLogFile = plogFile; }

	private:
		std::string m_strLogFile;
		TSLogShm *m_pShmLog;
		SLogFile * m_pstLogFileList; 
		uint32_t m_iLogTypeFlag;
		int32_t m_iLogField; // 查询结果需要的日志字段
		int32_t m_iLogAppId; // 查询的应用 ID
		int32_t m_iModuleIdCount;
		int32_t m_arrLogModuleId[SLOG_MODULE_COUNT_MAX_PER_APP]; // 模块 ID 列表，一个应用可以有多个模块

		char m_sLogPath[256];

		int  m_iIncKeyCount;
		char m_stIncKey[SLOG_MAX_KEY_COUNT][SLOG_KEY_MAX_LENGTH]; // 包含关键字
		int  m_stIncJmpTab[SLOG_MAX_KEY_COUNT][BMH_JUMP_TABLE_SIZE]; // 包含关键字 bmp 跳表

		int m_iExcpKeyCount;
		char m_stExcpKey[SLOG_MAX_KEY_COUNT][SLOG_KEY_MAX_LENGTH]; // 排除关键字
		int m_stExcpJmpTab[SLOG_MAX_KEY_COUNT][BMH_JUMP_TABLE_SIZE]; // 排除关键字 bmp 跳表

		uint64_t m_qwRealTimeUsec;

		int m_iReportMachine; // 上报机器 id
		uint64_t m_qwTimeStart;
		uint64_t m_qwTimeEnd;
		bool m_bInit;

		uint16_t m_wFileNo; // 文件编号索引，从 0 开始
		uint32_t m_dwFilePos; // 文件内日志记录索引
		uint16_t m_wFileCount; // 时间间隔内需要扫描的文件数目
		uint16_t m_wFileIndexStar; // 需要扫描的第一个文件在所有日志文件中的索引
		uint32_t m_dwCurFileHasRecords; // 当前扫描的文件中包含的日志数, 用于计算扫描完成百分比
};


// 日志读取类 ----- 用于日志客户端从共享内存中读取日志以便发送到日志服务器 ------------------------
// 一个配置对应一个类

class CSLogClient
{
	public:
		// 构造函数 attache 上共享内存
		CSLogClient(TSLogShm *pShmLog);
		~CSLogClient() {}
		bool IsInit() { return m_bInit; }
		TSLogOut * GetLog();
		void ShowShmInfo();

	private:
		TSLogShm *m_pShmLog;
		bool m_bInit;
};


// 日志记录类 ----- 将日志写入文件 - 用于日志服务器端将日志写入到磁盘文件 ------------------------
#define MAX_APP_LOG_SHM_COUNT 5000 // 单机最多应用日志共享内存数
#define DEF_SLOG_LOG_FILE_PATH "/home/mtreport/slog/" // slog 日志文件默认路径
#define SHOW_REALTIME_LOG_TIME_SEC 5*60 // 显示实时日志时，只显示前多少秒内的日志
#define SHOW_REALTIME_LAST_WRITE_LOG 10 // 显示最近写入的多少条日志，以便当客户端发送延迟的log时能够显示log

// 一个类管理一个 app log 的磁盘文件
class CSLogServerWriteFile
{
	public:
		// 构造函数 attache 上共享内存
		CSLogServerWriteFile(AppInfo *pAppInfo, const char *pszLogPath, int iScanAgain);
		~CSLogServerWriteFile();
		bool IsInit() { return m_bInit; }

		static SLogFile * GetAppLogFileShm(AppInfo *pAppInfo, bool bTryCreate=false);

		// 写指定数目的记录到文件, 返回实际写入文件的日志记录数目
		int WriteFile(int iWriteRecords, uint32_t dwCurTime);

		// 打印 shm 文件信息
		void ShowFileShmInfo(bool bLogContent);
		int GetWriteAppId() { return m_iAppId;}
		bool IsAppFileHeadReadAll();

	private:
		// 读取文件索引为 iLogFileIndex 的日志文件头部信息
		int ReadLogFileHead(int iLogFileIndex);

		int InitLogFiles(); // 读取所有的日志文件名
		int AddNewSlogFile(uint64_t qwLogTime); // 添加一个新的日志文件

		int Init();
		int AttachShm(); 
		int WriteLog(SLogFileHead &stFileHead, SLogFileLogIndex &stLogIndex, const char *pszLogTxt);
		int WriteLogVersion_1(SLogFileHead &stFileHead, TSLog *pShmLog, const char *pszLogTxt);

		uint32_t SaveLogCustData(char *pbuf, TSLog *pShmLog);
		int WriteLog(SLogFileHead &stFileHead, SLogFileLogIndex_ver2 &stLogIndex, const char *pszLogTxt, const char *pcustLog);
		int WriteLogVersion_2(SLogFileHead &stFileHead, TSLog *pShmLog, const char *pszLogTxt);

		int WriteLogRecord(int iLogIndex);
		void RemoveFileInfo(int iFileIndex);

		AppInfo * m_pAppInfo;
		int32_t m_iAppId;
		int32_t m_iFileShmKey;
		TSLogShm *m_pShmLog;
		SLogFile * m_pstLogFileList; 
		uint32_t m_iWriteLogBytes;

		bool m_bInit;
		int32_t m_iScanAgain;

		char m_szLogFilePath[256];
		uint16_t m_wLogFileCount;
		FILE *m_fpLogFile;
		uint16_t m_wCurFpLogFileIndex;
};

// 日志记录类 ----- 接收日志生产模块产生的日志并将日志写入共享内存, 作为日志服务器 ---------------
// 该类只能在服务器的一个进程中使用，作为唯一写共享内存的进程管理日志条目

class CSLogServer
{
	public:
		// 构造函数 attache 上共享内存
		CSLogServer();
		CSLogServer(int iShmKey, uint32_t dwVarNodeTotal, uint8_t bNodeLen);

		bool IsInit() { return m_bInit; }

	private:
		void InitDefault();
		int AttachShm(); 

		int32_t m_iShmKey;
		TSLogShm *m_pShmLog;

		bool m_bInit;
};

class CAutoFree {
    public:
        CAutoFree(char *pf): m_pf(pf) {}
        ~CAutoFree() {
            if(m_pf)
                free(m_pf);
        }
    private:
        char *m_pf;
};

bool IsVersionUp(const char *pbig_eq, const char *psmall);

// 本地回环地址
const std::string g_strLoopIP("127.0.0.1");
const uint32_t g_dwLoopIP = 16777343;
void StringReplaceAll(std::string &strSrc, const std::string &src, const std::string &dst);

extern CSupperLog slog;

#endif

