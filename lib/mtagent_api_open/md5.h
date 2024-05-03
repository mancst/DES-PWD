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

#ifndef _MD5_H 
#define _MD5_H

#include <stdint.h>

#ifndef MD5LEN
#define MD5LEN          16
#endif

#define MD5_DIGEST_LENGTH	16

void Md5HashBuffer(unsigned char *pOutBuffer, unsigned char *pInBuffer, uint32_t dwLength);

#endif 
