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

#ifndef SV_TIME_H_
#define SV_TIME_H_

char * time_to_ISO8601(time_t t);

// 计算两个时间点耗时
// GetTimeDiffMs(1)
//  --- do something ---
// GetTimeDiffMs(0)
int32_t GetTimeDiffMs(char cIsStart);

// 判断时间 t 是否是每天的 h 时, h 取值 0 - 23
int IsTimeInDayHour(time_t t, int h);

void sv_SetCurTime(time_t now);
time_t sv_GetCurTime();

#endif

