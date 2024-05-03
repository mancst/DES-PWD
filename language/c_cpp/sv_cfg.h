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


   开发库 mtagent_api_open 说明:
        itc云框架系统内部公共库，提供各种公共的基础函数调用

****/

#ifndef _SV_CFG_H
#define _SV_CFG_H

#define CFG_STRING		((int)1)
#define CFG_INT			((int)2)
#define CFG_LONG		((int)3)
#define CFG_DOUBLE		((int)4)
#define CFG_LINE		((int)5)
#define CFG_UINT32		((int)6)
#define CFG_UINT64		((int)7)

#ifdef __cplusplus
extern "C" {
#endif

int CheckPluginConfig(const char *pszConfigFilePath);
int IsPluginVersionOk(const char *pbig_eq, const char *psmall);
int LoadConfig(const char *pszConfigFilePath, ...);
int GetConfigFile(const char *pAppStr, char **pconfOut);
int GetLogTypeByStr(const char *pstrType);

char * get_val(char* desc, char* src);
void get_config_val(char* desc, char* src); 

#ifdef __cplusplus
}
#endif

#endif // _SV_CFG_H

