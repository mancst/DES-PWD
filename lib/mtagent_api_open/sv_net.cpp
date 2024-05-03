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

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <ifaddrs.h>

#include "sv_net.h"
#include "sv_str.h"

int GetIf(IfDesc_ipv4 * pstDesc)
{
	int fd = 0;
	struct ifreq req;

	if(NULL == pstDesc){
		return -1;
	}
	if(strlen(pstDesc->name) > IFNAMSIZ - 1){
		return -1;
	};

	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		SetApiErrorMsg("socket failed ret:%d, msg:%s", fd, strerror(errno));
		return -1;
	}

	strncpy(req.ifr_name, pstDesc->name, IFNAMSIZ-1);

	if(ioctl(fd, SIOCGIFADDR, (char *)&req) < 0){
		SetApiErrorMsg("ioctl failed, msg:%s", strerror(errno));
		return -1;
	}
	if(req.ifr_addr.sa_family != AF_INET){
		return -1;
	}

	memcpy(&(pstDesc->addr), &(req.ifr_addr), sizeof(pstDesc->addr));

	if (ioctl(fd, SIOCGIFFLAGS, (char *)&req) < 0){
		SetApiErrorMsg("ioctl failed, msg:%s", strerror(errno));
		return -1;
	}
	pstDesc->flags = req.ifr_flags;

	close(fd);
	return 0;
}


void GetCustLocalIP(char *pszLocalIp)
{
	struct ifaddrs *addr = NULL;
	struct ifaddrs *temp_addr = NULL;
	const char *pip = NULL;

	if( getifaddrs(&addr) == 0) {
		temp_addr = addr;
		while(temp_addr != NULL) {
			if(temp_addr->ifa_addr->sa_family == AF_INET) { 
				pip = inet_ntoa(((struct sockaddr_in *)temp_addr->ifa_addr)->sin_addr);
				if(strcmp(pip, "127.0.0.1")){
					strcpy(pszLocalIp, pip);
					break;
				}
			} 
			temp_addr = temp_addr->ifa_next;        
		}    
	}    
	freeifaddrs(addr);
}

int SetNBlock(int iSock)
{
	int iFlags;
	iFlags = fcntl(iSock, F_GETFL, 0); 
	iFlags |= O_NONBLOCK;
	iFlags |= O_NDELAY;
	fcntl(iSock, F_SETFL, iFlags);
	return 0;
}

int SetSocketBuffer(int sockfd, int nRecvBuf, int nSendBuf)
{
	int iUdpSockRecvBufLen = nRecvBuf;
	int iUdpSockSendBufLen = nSendBuf;
	socklen_t optlen = sizeof(int);
	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &iUdpSockRecvBufLen, optlen) < 0
		|| setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &iUdpSockSendBufLen, optlen) < 0)
	{
		return -1;
	}
	return 0;
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
	connect(iClientSockfd, (struct sockaddr *)&stINETAddr, sizeof(stINETAddr));
	socklen_t iAddrLenLocal = sizeof(stINETAddrLocal);
	getsockname(iClientSockfd, (struct sockaddr *)&stINETAddrLocal, &iAddrLenLocal);
	pLocalIP = inet_ntoa(stINETAddrLocal.sin_addr);
	strncpy(s_szLocalIP, pLocalIP, sizeof(s_szLocalIP)-1);
	close(iClientSockfd);
	return s_szLocalIP;
}

