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
#include <ifaddrs.h>
#include <linux/if.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>

#include "sv_str.h"

#ifndef MYSIZEOF
#define MYSIZEOF (unsigned)sizeof
#endif

#define bool int

static char s_sLogBuf[1024 * 4];

int IsFileExist(const char *file)
{
    FILE *fp = fopen(file, "r");
    if(fp != NULL) {
        fclose(fp);
        return 1;
    }
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

char *qwtoa(uint64_t qwVal)
{
	static char sbuf[40];
	sprintf(sbuf, "%" PRIu64 , qwVal);
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
	if(cNum <= 0)
		cNum = (int)strlen(pstr);
	cNum = (cNum >= (int)MYSIZEOF(s_sLogBuf) ? (int)MYSIZEOF(s_sLogBuf)-1 : cNum);
	memcpy(s_sLogBuf, pstr, cNum);
	s_sLogBuf[cNum] = '\0';
	int iMskNum = (cNum >= 5 ? (cNum*3/5) : (cNum >= 2 ? cNum/2 : cNum));
	int iMskStart = (cNum >= 5 ? (cNum/5) : (cNum >= 2 ? 1 : 0));
	for(; s_sLogBuf[iMskStart] != '\0' && iMskNum > 0; iMskNum--, iMskStart++)
		s_sLogBuf[iMskStart] = '*';
	return s_sLogBuf;
}

int GetLocalIPAndMac(uint32_t dwRemoteIp, char *pip, char *pmac, int iRemotePort)
{
	struct sockaddr_in stINETAddr;
	struct sockaddr_in stINETAddrLocal;
	int iCurrentFlag = 0;

	int iClientSockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if(iClientSockfd < 0 ) 
	    return -1;

	stINETAddr.sin_addr.s_addr = dwRemoteIp;
	stINETAddr.sin_family = AF_INET;
	stINETAddr.sin_port = htons(iRemotePort);
	iCurrentFlag = fcntl(iClientSockfd, F_GETFL, 0); 
	fcntl(iClientSockfd, F_SETFL, iCurrentFlag | FNDELAY);

	connect(iClientSockfd, (struct sockaddr *)&stINETAddr, MYSIZEOF(stINETAddr));
	socklen_t iAddrLenLocal = MYSIZEOF(stINETAddrLocal);
	if(getsockname(iClientSockfd, (struct sockaddr *)&stINETAddrLocal, &iAddrLenLocal) != 0)
    {
	    close(iClientSockfd);
        return -2;
    }
	const char *pLocalIP = inet_ntoa(stINETAddrLocal.sin_addr);
	strcpy(pip, pLocalIP);
    if(!pmac) {
	    close(iClientSockfd);
        return 0;
    }
   
    struct ifaddrs * ifAddrStruct=NULL, *ifFirst = NULL;
    if(getifaddrs(&ifFirst) != 0) {
	    close(iClientSockfd);
        return -3;
    }

    int iRet = 0;
    ifAddrStruct = ifFirst;
    while (ifAddrStruct!=NULL) {
        if (ifAddrStruct->ifa_addr->sa_family==AF_INET
            && stINETAddrLocal.sin_addr.s_addr == ((struct sockaddr_in*)(ifAddrStruct->ifa_addr))->sin_addr.s_addr)
        {
            struct sockaddr_ll *s = (struct sockaddr_ll*)(ifAddrStruct->ifa_addr);
            if(s->sll_halen == XRK_MAC_LEN) {
                memcpy(pmac, s->sll_addr, XRK_MAC_LEN);
            }
            else {
                struct ifreq ifr;
                strcpy(ifr.ifr_name, ifAddrStruct->ifa_name);
                if(ioctl(iClientSockfd, SIOCGIFHWADDR, (char *)&ifr) == 0) 
                    memcpy(pmac, ifr.ifr_ifru.ifru_hwaddr.sa_data, XRK_MAC_LEN);
                else 
                    iRet = -4;
            }
            break;
        }
        ifAddrStruct=ifAddrStruct->ifa_next;
    }
    if(ifFirst)
        freeifaddrs(ifFirst);
	close(iClientSockfd);
    return iRet;
}

const char * GetLocalIP(const char *pszRemoteIp)
{
	static char s_szLocalIP[32];

	char *pLocalIP;
	struct sockaddr_in stINETAddr;
	struct sockaddr_in stINETAddrLocal;
	int iCurrentFlag = 0;
	int iClientSockfd = socket(AF_INET, SOCK_DGRAM, 0); 
	if(iClientSockfd < 0 ) 
		return NULL;

	stINETAddr.sin_addr.s_addr = inet_addr(pszRemoteIp);
	stINETAddr.sin_family = AF_INET;
	stINETAddr.sin_port = htons(80);

	iCurrentFlag = fcntl(iClientSockfd, F_GETFL, 0); 
	fcntl(iClientSockfd, F_SETFL, iCurrentFlag | FNDELAY);
	connect(iClientSockfd, (struct sockaddr *)&stINETAddr, MYSIZEOF(stINETAddr));
	socklen_t iAddrLenLocal = MYSIZEOF(stINETAddrLocal);
	if(getsockname(iClientSockfd, (struct sockaddr *)&stINETAddrLocal, &iAddrLenLocal) != 0)
    {
	    close(iClientSockfd);
        return NULL;
    }
	pLocalIP = inet_ntoa(stINETAddrLocal.sin_addr);
	strncpy(s_szLocalIP, pLocalIP, MYSIZEOF(s_szLocalIP)-1);
	close(iClientSockfd);
	return s_szLocalIP;
}

static char EscapedChars[] = "$&+,/:;=?@ \"<>#%{}|\\^~[]`'";
static int is_reserved_char(char c)
{
    int i = 0; 
    if (c < 32 || c > 122) {
        return 1;
    } else {
        while (EscapedChars[i]) {
            if (c == EscapedChars[i]) {
                return 1;
            }    
            ++i; 
        }    
    }    
    return 0;
}

const char * url_escape(const char *in) 
{
    int nl = 0; 
    int l = 0; 
    unsigned char *buf = (unsigned char *)in;
    unsigned char *s = (unsigned char *) s_sLogBuf;
    nl = 0; l = 0; 
    while (buf[l] && nl < (int)sizeof(s_sLogBuf))
    {    
        if (buf[l] == ' ') 
        {    
            s[nl++] = '+';
            l++;
        }
        else
        {
            if (is_reserved_char(buf[l]))
            {
                s[nl++] = '%';
                s[nl++] = "0123456789ABCDEF"[buf[l] / 16];
                s[nl++] = "0123456789ABCDEF"[buf[l] % 16];
                l++;
            }
            else
            {
                s[nl++] = buf[l++];
            }
        }
    }
    s[nl] = '\0';
    return (const char *)s;
}

