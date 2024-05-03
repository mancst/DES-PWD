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

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "mt_report.h"
#include "sv_str.h"

#ifndef MYSIZEOF
#define MYSIZEOF (unsigned)sizeof
#endif

#define bool int

static char s_sLogBuf[4096 * 4];
char s_szCommApiLastError[1024] = {0};

int get_cmd_result(const char *cmd, std::string &strResult)
{
    strResult = "";
    FILE *fp = popen(cmd, "r");
    if(!fp) {
        return ERROR_LINE;
    }

    if(fgets(s_sLogBuf, sizeof(s_sLogBuf), fp)) {
        strResult = s_sLogBuf;
        size_t pos = strResult.find("\n");
        if(pos != std::string::npos)
            strResult.replace(pos, 1, "");
        pos = strResult.find("\r");
        if(pos != std::string::npos)
            strResult.replace(pos, 1, "");
        pos = strResult.find("\r\n");
        if(pos != std::string::npos)
            strResult.replace(pos, 2, "");
    }
    pclose(fp);
    return 0;
}

int file_lockw(const char *pfile)
{
	int oldmask = 0;
	int s_iFileLockFd = 0;

	oldmask = umask(0);
	s_iFileLockFd = open(pfile, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	umask(oldmask);

	if(s_iFileLockFd <= 0)
		return -1;
	return file_lockw_fd(s_iFileLockFd);
}

int file_lock(const char *pfile)
{
	int oldmask = 0;
	int s_iFileLockFd = 0;

	oldmask = umask(0);
	s_iFileLockFd = open(pfile, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	umask(oldmask);

	if(s_iFileLockFd <= 0)
		return -1;
	return file_lock_fd(s_iFileLockFd);
}

int file_unlock(const char *pfile)
{
	int oldmask = 0;
	int s_iFileLockFd = 0;

	oldmask = umask(0);
	s_iFileLockFd = open(pfile, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	umask(oldmask);

	if(s_iFileLockFd <= 0)
		return -1;
	return file_unlock_fd(s_iFileLockFd);
}

int file_lockw_fd(int fd)
{
	struct flock lock;
	lock.l_type = F_WRLCK;   /* F_RDLCK, F_WRLCK, F_UNLCK */
	lock.l_start = 0;    /* byte offset, relative to l_whence */
	lock.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
	lock.l_len = 0;     /* #bytes (0 means to EOF) */
	return fcntl(fd, F_SETLKW, &lock);
}

int file_lock_fd(int fd)
{
	struct flock lock;
	lock.l_type = F_WRLCK;   /* F_RDLCK, F_WRLCK, F_UNLCK */
	lock.l_start = 0;    /* byte offset, relative to l_whence */
	lock.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
	lock.l_len = 0;     /* #bytes (0 means to EOF) */
	return fcntl(fd, F_SETLK, &lock);
}

int file_unlock_fd(int fd)
{
	struct flock lock;
	lock.l_type = F_UNLCK;   /* F_RDLCK, F_WRLCK, F_UNLCK */
	lock.l_start = 0;    /* byte offset, relative to l_whence */
	lock.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
	lock.l_len = 0;     /* #bytes (0 means to EOF) */
	return fcntl(fd, F_SETLK, &lock);
}

char *strchr_n(char *pstr, char c, int n)
{
	int i=0;
	for(; i < n; i++)
	{
		if(pstr[i] == c)
			return pstr+i;
	}
	return NULL;
}

char * ipv4_addr_str(uint32_t dwAddr)
{
    static char s_ip[18];
	struct in_addr in;
	in.s_addr = dwAddr;
	strncpy(s_ip, inet_ntoa(in), sizeof(s_ip)-1);
    return s_ip;
}

uint32_t datetoui(const char *pdate)
{
	struct tm t;
	memset(&t, 0, MYSIZEOF(t));
	if(sscanf(pdate, "%d-%d-%d %d:%d:%d",
		&t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) != 6)
		return 0;

	t.tm_year -= 1900;
	t.tm_mon -= 1;
	return mktime(&t);
}

void uitotime_info(uint32_t dwTimeSec, TIME_INFO *pinfo)
{
    memset(pinfo, 0, sizeof(*pinfo));
	struct tm stTm;
	time_t tmnew = dwTimeSec;
	localtime_r(&tmnew, &stTm);
    pinfo->y = stTm.tm_year+1900;
    pinfo->m = stTm.tm_mday;
    pinfo->d = stTm.tm_mon+1;
    pinfo->hour = stTm.tm_hour;
    pinfo->min = stTm.tm_min;
    pinfo->sec = stTm.tm_sec;
}

char *uitodate(uint32_t dwTimeSec)
{
	static char sBuf[64];
	struct tm stTm;
	time_t tmnew = dwTimeSec;
	localtime_r(&tmnew, &stTm);
	strftime(sBuf, MYSIZEOF(sBuf), "%Y-%m-%d %H:%M:%S", &stTm);
	return sBuf;
}

char *qwtoa(uint64_t qwVal)
{
	static char sbuf[40];
	sprintf(sbuf, "%" PRIu64, qwVal);
	return sbuf;
}

char *uitoa(unsigned int i)
{
	static char sbuf[20];
	sprintf(sbuf, "%u", i);
	return sbuf;
}

char *itoa(int i)
{
	static char sbuf[20];
	sprintf(sbuf, "%d", i);
	return sbuf;
}

static int GetURandBuf(char * sBuf, int iLen)
{
	static int iFD = -1;
	if (0 > iFD)
	{
		iFD = open("/dev/urandom", O_RDONLY);
		if(0 > iFD)
		{
			return -1;
		}
	}
	if(read(iFD, sBuf, iLen) != iLen)
	{
		close(iFD);
		iFD = -1;
		return -1;
	}
	return 0;
}

static char *GenRandStrURandom(const char *sCharBuf, const int iCharBufLen, char *sBuf, int iLen)
{
	int i;
	if(0 > GetURandBuf(sBuf, iLen))
	{
		static uint32_t ulSeed = 0;
		if (ulSeed == 0) {
			struct timeval tv;
			gettimeofday(&tv, 0);
			ulSeed = (getpid() << 16) ^ getuid() ^ tv.tv_sec ^ tv.tv_usec;
			srand(ulSeed);
		}
		for(i = 0; i < iLen; i++)
		{
			sBuf[i] = sCharBuf[(int)((double)iCharBufLen * rand() / (RAND_MAX + 1.0))];
		}
		sBuf[iLen] = '\0';
		return sBuf;
	}
	for(i = 0; i < iLen; i++)
	{
		sBuf[i] = sCharBuf[(int)((double)iCharBufLen * ((unsigned char)(sBuf[i])) / (255 + 1.0))];
	}
	sBuf[iLen] = '\0';
	return sBuf;
}

char *OI_RandStrURandom(char *buffer, int len)
{
	const char *chars = "ABCDEFGHIJKMNPQRSTUVWXYZabcdefghijkmnpqrstuvwxyz23456789;:',<.>?[{]}`~!@#$%^*()_-+="; 
	int chars_len = strlen(chars);
	
	return GenRandStrURandom(chars, chars_len, buffer, len);
}

char *OI_randstr(char* buffer, int len)
{
  const char *chars="ABCDEFGHIJKMNPQRSTUVWXYZabcdefghijkmnpqrstuvwxyz23456789"; 
  int chars_len = strlen(chars);

  return GenRandStrURandom(chars, chars_len, buffer, len);
} 

char *OI_randstr2(char* buffer, int len)
{
  const char *chars="abcdefghijkmnpqrstuvwxyz23456789"; 
  int chars_len = strlen(chars);

  return GenRandStrURandom(chars, chars_len, buffer, len);
}

char *OI_randstr_number(char* buffer, int len)
{
  const char *chars="0123456789"; 
  int chars_len = strlen(chars);
  return GenRandStrURandom(chars, chars_len, buffer, len);
}

const char * OI_DumpHex_Lower(void *pMem, size_t uDumpOffset, size_t uDumpLen)
{
    int inc; 
    size_t i;
    char *pDst = s_sLogBuf, *pSrc = (char*)pMem + uDumpOffset;
    s_sLogBuf[0] = '\0';
    for(i = uDumpOffset; i < uDumpOffset + uDumpLen; i++, pSrc++)
    {    
        inc = snprintf(pDst, s_sLogBuf + sizeof(s_sLogBuf) - pDst, "%02x", (unsigned char) *pSrc);
        if(inc < 0) 
        {    
            break;
        }    
        pDst += inc; 
        if(pDst >= s_sLogBuf + sizeof(s_sLogBuf))
        {    
            break;
        }    
    }    
    return s_sLogBuf;
}

const char * OI_DumpHex(void *pMem, size_t uDumpOffset, size_t uDumpLen)
{
    int inc;
    size_t i;
    char *pDst = s_sLogBuf, *pSrc = (char*)pMem + uDumpOffset;

    s_sLogBuf[0] = '\0';
    for(i = uDumpOffset; i < uDumpOffset + uDumpLen; i++, pSrc++)
    {
        inc = snprintf(pDst, s_sLogBuf + MYSIZEOF(s_sLogBuf) - pDst, "%02X", (unsigned char) *pSrc);
        if(inc < 0)
        {
            break;
        }
        pDst += inc;
        if(pDst >= s_sLogBuf + MYSIZEOF(s_sLogBuf))
        {
            break;
        }
    }
    return s_sLogBuf;
}

const char *DumpStrByMask(const char *pstr, int cNum)
{
	static char sLocalBuf[1024];
	if(cNum <= 0)
		cNum = (int)strlen(pstr);
	cNum = (cNum >= (int)MYSIZEOF(sLocalBuf) ? (int)MYSIZEOF(sLocalBuf)-1 : cNum);
	memcpy(sLocalBuf, pstr, cNum);
	sLocalBuf[cNum] = '\0';
	int iMskNum = (cNum >= 5 ? (cNum*3/5) : (cNum >= 2 ? cNum/2 : cNum));
	int iMskStart = (cNum >= 5 ? (cNum/5) : (cNum >= 2 ? 1 : 0));
	for(; sLocalBuf[iMskStart] != '\0' && iMskNum > 0; iMskNum--, iMskStart++)
		sLocalBuf[iMskStart] = '*';
	return sLocalBuf;
}

void SetApiErrorMsg(const char *pszFmt, ...) 
{
    va_list ap;
    va_start(ap, pszFmt);
    vsnprintf(s_szCommApiLastError, sizeof(s_szCommApiLastError)-1, pszFmt, ap); 
    va_end(ap);
}

const char * GetApiLastError() 
{ 
    return s_szCommApiLastError; 
}

// db 输入敏感字符检查
int CheckDbString(const char *pstr)
{
    while(*pstr != '\0')
    {
        if(*pstr == '\'')
            return 1;
        pstr++;
    }
    return 0;
}

const char *DumpStrByMask2(const char *pstr)
{
	int iLen = (int)strlen(pstr);
	if(iLen > 6) {
		strncpy(s_sLogBuf, pstr, 3);
		s_sLogBuf[3] = '\0';
		strcat(s_sLogBuf, "***...***");
		strcat(s_sLogBuf, pstr+iLen-3);
	}
	else {
		strcpy(s_sLogBuf, pstr);
		s_sLogBuf[iLen-1] = '*';
	}
	return s_sLogBuf;
}

int isnumber(const char *pstr)
{       
	while(pstr != NULL && *pstr != '\0')
	{ 
		if(*pstr < '0' || *pstr > '9')
			return 0;
		pstr++;
	}
	return 1;
}

char *uitodate2(uint32_t dwTimeSec) 
{   
	static char sBuf[64];
	struct tm stTm;
	time_t tmnew = dwTimeSec;
	localtime_r(&tmnew, &stTm);
	strftime(sBuf, sizeof(sBuf), "%Y-%m-%d", &stTm);
	return sBuf;
}   

uint32_t set_bit(uint32_t n, int b)
{
	return (n|b);
}

uint32_t clear_bit(uint32_t n, int b)
{
	return (n & (~b));
}

int check_bit(uint32_t n, int b)
{
	return (n & b);
}

char *Str_Trim_Char(char *s, char c)
{
	char *pb;
	char *pe;
	char *ps;
	char *pd;

	if (strcmp(s, "") == 0) 
		return s;
	
	pb = s;
		 
	while (*pb == c)
	{
		pb ++;
	}
	
	pe = s;
	while (*pe != 0)
	{
		pe ++;
	}
	pe --;
	while ((pe >= s) && (*pe == c))
	{
		pe --;
	}
	
	ps = pb;
	pd = s;
	while (ps <= pe)
	{
		*pd = *ps;
		ps ++;
		pd ++;
	}
	*pd = 0;
	
	return s;
}

char *Str_Trim_Local(char *s)
{
	static char s_str[1024] = {0};
	if(strlen(s) >= sizeof(s_str))
		return s;
	strcpy(s_str, s);
	return Str_Trim(s_str);
}

char *Str_Trim(char *s)
{
	char *pb;
	char *pe;
	char *ps;
	char *pd;

	if (strcmp(s, "") == 0) 
		return s;
	
	pb = s;
		 
	while ((*pb == ' ') || (*pb == '\t') || (*pb == '\n') || (*pb == '\r'))
	{
		pb ++;
	}
	
	pe = s;
	while (*pe != 0)
	{
		pe ++;
	}
	pe --;
	while ((pe >= s) && ((*pe == ' ') || (*pe == '\t') || (*pe == '\n') || (*pe == '\r')))
	{
		pe --;
	}
	
	ps = pb;
	pd = s;
	while (ps <= pe)
	{
		*pd = *ps;
		ps ++;
		pd ++;
	}
	*pd = 0;
	
	return s;
}

void ReplaceAllChar(char *pstr, char src, char dst)
{
    while(pstr && *pstr != '\0') {
        if(*pstr == src)
            *pstr = dst;
        pstr++;
    }
}

int get_char_count(const char *pstr, char c)
{   
    int i = 0;
    while(*pstr != '\0') {
        if(*pstr == c)
            i++;
        pstr++;
    }
    return i;
}  

uint32_t datetoui2(const char *pdate)
{
	struct tm t;
	memset(&t, 0, sizeof(t));
	if(sscanf(pdate, "%d-%d-%d", &t.tm_year, &t.tm_mon, &t.tm_mday) != 3)
		return 0;

	if(t.tm_year == 0)
		return 0;

	t.tm_year -= 1900;
	t.tm_mon -= 1;
	return mktime(&t);
}

