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

#ifndef _SV_STR_H_
#define _SV_STR_H_

#include <inttypes.h>

    int IsFileExist(const char *file);
	int file_lockw(const char *pfile);
	int file_lock(const char *pfile);
	int file_lockw_fd(int fd);
	int file_lock_fd(int fd);
	int file_unlock(const char *pfile);
	int file_unlock_fd(int fd);

	char *OI_RandStrURandom(char *buffer, int len);
	char *ipv4_addr_str(uint32_t dwAddr);
	const char *GetLocalIP(const char *pszRemoteIp);
#define XRK_MAC_LEN 6
    int GetLocalIPAndMac(uint32_t dwRemoteIp, char *pip, char *pmac, int iRemotePort=80);

	char *strchr_n(char *pstr, char c, int n);
	char *itoa(int i);
	char *uitoa(unsigned int i);
	char *qwtoa(uint64_t qwVal);
    const char *OI_DumpHex_Lower(void *pMem, size_t uDumpOffset, size_t uDumpLen);
    const char *OI_DumpHex(void *pMem, size_t uDumpOffset, size_t uDumpLen);
	const char *DumpStrByMask(const char *pstr, int cNum);
    const char * url_escape (const char *in);

#define MY_STRLEN(pstr) (pstr==NULL ? 0 : strlen(pstr))

#endif

