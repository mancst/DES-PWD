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
#include <stdarg.h>
#include <iostream>

typedef struct {
	int y;
	int m;
	int d;
	int hour;
	int min;
	int sec;
}TIME_INFO;

void uitotime_info(uint32_t dwTimeSec, TIME_INFO *pinfo);
int get_cmd_result(const char *cmd, std::string &strResult);
int get_char_count(const char *pstr, char c);

int file_lockw(const char *pfile);
int file_lock(const char *pfile);
int file_lockw_fd(int fd);
int file_lock_fd(int fd);
int file_unlock(const char *pfile);
int file_unlock_fd(int fd);
const char * GetApiLastError();
void SetApiErrorMsg(const char *pszFmt, ...);
uint32_t set_bit(uint32_t n, int b);
uint32_t clear_bit(uint32_t n, int b);
int check_bit(uint32_t n, int b);
char *OI_RandStrURandom(char *buffer, int len);
char *ipv4_addr_str(uint32_t dwAddr);
char *uitodate(uint32_t dwTimeSec);
uint32_t datetoui(const char *pdate);
uint32_t datetoui2(const char *pdate);
char *strchr_n(char *pstr, char c, int n);
char *itoa(int i);
char *uitoa(unsigned int i);
char *qwtoa(uint64_t qwVal);
const char * OI_DumpHex_Lower(void *pMem, size_t uDumpOffset, size_t uDumpLen);
const char *OI_DumpHex(void *pMem, size_t uDumpOffset, size_t uDumpLen);
const char *DumpStrByMask2(const char *pstr);
const char *DumpStrByMask(const char *pstr, int cNum);
int CheckDbString(const char *pstr);
char *OI_randstr(char* buffer, int len);
char *OI_randstr_number(char* buffer, int len);
int isnumber(const char *pstr);
char *uitodate2(uint32_t dwTimeSec);
char *Str_Trim(char *s);
char *Str_Trim_Char(char *s, char c);
char *Str_Trim_Local(char *s);
void ReplaceAllChar(char *pstr, char src, char dst);

#define MY_STRLEN(pstr) (pstr==NULL ? 0 : strlen(pstr))

#endif

