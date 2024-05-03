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

#ifndef __SV_STRUCT__ 
#define __SV_STRUCT__ 1

#include <inttypes.h>

#define ntohll(n64) (((uint64_t)ntohl(n64)) << 32) | htonl(n64 >> 32)
#define htonll(h64) (((uint64_t)ntohl(h64)) << 32) | htonl(h64 >> 32)

// set bit b in value n
#define SET_BIT(n, b) n|=b 
// check is set bit b in value n
#define IS_SET_BIT(n, b) n&b
// clear bit b in value n
#define CLEAR_BIT(n, b) n&=~b

#define SPKG '['   // tcp, udp packet data start
#define EPKG ']'   // tcp, udp packet data start

#define MAX_SIGNATURE_LEN 128 

// ------------- cmd define start --------------------
// monitor system cmd: 201-400 , 与 c_comm 中的 sv_struct.h 中定义一致
#define CMD_MONI_SEND_HELLO_FIRST 201 // 首个 hello 命令
#define CMD_MONI_SEND_HELLO 202 // 普通 hello 命令，用于探测服务器是否可用
#define CMD_MONI_SEND_ATTR 203
#define CMD_MONI_CHECK_LOG_CONFIG 206
#define CMD_MONI_CHECK_APP_CONFIG 207
#define CMD_MONI_SEND_LOG 209
#define CMD_MONI_CHECK_SYSTEM_CONFIG 213
#define CMD_MONI_SEND_STR_ATTR 214 // client send str attr
#define CMD_CGI_SEND_ATTR 215 
#define CMD_CGI_SEND_STR_ATTR 216 
#define CMD_CGI_SEND_LOG 217 
#define CMD_MONI_SEND_PLUGIN_INFO 218 // client send plugin info 
#define CMD_MONI_PREINSTALL_REPORT 219 // client send 一键部署进度 
#define CMD_SEND_PLUGIN_CONFIG 220

// monitor system s2c cmd: 701-999
#define CMD_MONI_S2C_LOG_CONFIG_NOTIFY 701 
#define CMD_MONI_S2C_APP_CONFIG_NOTIFY 702 
#define CMD_MONI_S2C_PRE_INSTALL_NOTIFY 703 
#define CMD_MONI_S2C_MACH_ORP_PLUGIN_REMOVE 704 
#define CMD_MONI_S2C_MACH_ORP_PLUGIN_ENABLE 705
#define CMD_MONI_S2C_MACH_ORP_PLUGIN_DISABLE 706
#define CMD_MONI_S2C_MACH_ORP_PLUGIN_MOD_CFG 707
// ------------- cmd define end --------------------

// ------------------------------ tlv define start -------------------
#define TLV_ACK_ERROR 5

// monitor system tlv : 201 --- 1000
#define TLV_MONI_SEND_LOG 201
#define TLV_MONI_SEND_ATTR 202
#define TLV_MONI_COMM_INFO 204
#define TLV_MONI_CONFIG_ADD 205
#define TLV_MONI_CONFIG_MOD 206
#define TLV_MONI_CONFIG_DEL 207
// ------------------------------ tlv define end -------------------


// ------------------------------ signature start  -------------------
// monitor signatur 1-64
#define MT_SIGNATURE_TYPE_HELLO_FIRST 1 
#define MT_SIGNATURE_TYPE_COMMON 101
// ------------------------------ signature end -------------------


#pragma pack(1)

// ---------------------------- comm struct start 

typedef struct
{
	uint32_t dwSeq;
	uint32_t dwCmd;
	uint8_t bEnableEncryptData;
	char szReserved[16];
}MonitorCommSig; // monitor 通用签名结构

#define MAGIC_RESPONSE_NUM 1117250
#define REQ_PKG_HEAD_VERSION 1
typedef struct
{
	uint32_t dwCmd;
	uint32_t dwSeq;
	uint16_t wPkgLen;
	uint16_t wToPkgBody;
	uint8_t bRetCode;
	uint8_t bResendTimes;
	uint16_t wVersion;
	uint32_t dwRespMagicNum;
	char sEchoBuf[32];
	uint64_t qwSessionId;
	char sReserved[8];
}ReqPkgHead;

typedef struct
{
	uint8_t bSigType;
	uint16_t wSigLen;
	char sSigValue[0];
}TSignature;

typedef struct
{
	uint16_t wType;
	uint16_t wLen;
	uint8_t sValue[0];
}TWTlv;

typedef struct
{
	uint8_t bTlvNum;
	TWTlv stTlv[0];
}TPkgBody;

// ---------------------------- comm struct end



// for sys monitor  --- start
// udp req packet is : SPKG + ReqPkgHead + TSignature + {monitor} + TPkgBody + EPKG
//  req tlv: TLV_MONI_SEND_LOG --- for cmd: CMD_MONI_SEND_LOG 
//  req tlv: TLV_MONI_SEND_ATTR --- for cmd:CMD_MONI_SEND_ATTR 
// cmd first hello sig ---
typedef struct
{
	uint8_t bEnableEncryptData;
	uint32_t dwPkgSeq;
	char sRespEncKey[16+1];
	uint32_t dwAgentClientIp;
}MonitorHelloSig;

typedef struct
{
	int32_t iMtClientIndex;
	int32_t iMachineId;
	uint32_t dwReserved_1;
	uint16_t wReserved_1;
	uint32_t dwReserved_2;
	uint16_t wReserved_2;
	char sReserved[32];
}TlvMoniCommInfo; // for TLV_MONI_COMM_INFO
// for sys monitor  --- end 

#pragma pack()

int SetWTlv(TWTlv *ptlv, uint16_t wType, uint16_t wLen, const char *pValue);
TWTlv * GetWTlvByType(uint16_t wType, uint16_t wTlvNum, TWTlv *ptlv);
TWTlv * GetWTlvByType2(uint16_t wType, TPkgBody *pstTlv);
int CheckPkgBody(TPkgBody *pstBody, uint16_t wBodyLen);
TWTlv * GetWTlvType2_list(TPkgBody *pstTlv, int *piStartNum);
int InitSignature(TSignature *psig, void *pdata, const char *pKey, int bSigType);

#endif

