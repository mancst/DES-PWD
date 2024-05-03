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

#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <sv_cfg.h>
#include <sstream>
#include <errno.h>

#include "supper_log.h"
#include "mtreport_cfg.h"

#define MAX_CONFIG_LINE_LEN (5 * 1024 - 1)

#ifndef ERROR_LINE 
#define ERROR_LINE -__LINE__
#endif

int GetConfigItemList(const char *pfile, TConfigItemList & list);
int UpdateConfigFile(const char *path, const char *pfile, TConfigItemList & list)
{
    std::string strAbsFile(path);
	TConfigItemList list_cfg;
    strAbsFile += "/";
    strAbsFile += pfile;
	if(GetConfigItemList(strAbsFile.c_str(), list_cfg) < 0)
		return ERROR_LINE;

	TConfigItemList::iterator it = list.begin();
	TConfigItem *pitem = NULL, *pitem_cfg = NULL;
	for(; it != list.end(); it++)
	{
		pitem = *it;
		TConfigItemList::iterator it_cfg = list_cfg.begin();
		for(; it_cfg != list_cfg.end(); it_cfg++)
		{
			pitem_cfg = *it_cfg;
			if(pitem->strConfigName == pitem_cfg->strConfigName){
				pitem_cfg->strConfigValue = pitem->strConfigValue;
				pitem_cfg->strConfigValue += "\n";
				break;
			}
		}
		if(it_cfg == list_cfg.end())
		{
			TConfigItem *pitem_tmp = new TConfigItem;
			pitem_tmp->strConfigName = pitem->strConfigName;
			pitem_tmp->strConfigValue = pitem->strConfigValue;;
			pitem_tmp->strConfigValue += "\n";
			list_cfg.push_back(pitem_tmp);
		}
	}

	
	FILE *pstFile = NULL;
	if ((pstFile = fopen(strAbsFile.c_str(), "w+")) == NULL)
    {
		ERR_LOG("open file : %s failed, msg:%s", pfile, strerror(errno));
        return ERROR_LINE;
    }

	TConfigItemList::iterator it_cfg = list_cfg.begin();
	for(; it_cfg != list_cfg.end(); it_cfg++)
	{
		pitem_cfg = *it_cfg;
        if(pitem_cfg->strComment.size() > 0 || pitem_cfg->strConfigName == "" || pitem_cfg->strConfigValue == "")
		{
			std::string::reverse_iterator rit = pitem_cfg->strComment.rbegin();
            if(*rit == '\n')
            	fprintf(pstFile, "%s", pitem_cfg->strComment.c_str());
			else
            	fprintf(pstFile, "%s\n", pitem_cfg->strComment.c_str());
        }
        else {
        	fprintf(pstFile, "%s %s", pitem_cfg->strConfigName.c_str(), pitem_cfg->strConfigValue.c_str());
        }
	}
	fclose(pstFile);
	ReleaseConfigList(list_cfg);
	return 0;
}

static int GetParamVal(char *sLine, char *sParam, char *sVal)
{
	if (sLine[0] == '#')
		return 1;

	get_val(sParam, sLine);
	strcpy(sVal, sLine);
	if (sParam[0] == '#')
		return 1;
		
	return 0;
}

int GetConfigItemList(const char *pfile, TConfigItemList & list)
{
	FILE *pstFile = NULL;
	char sLine[MAX_CONFIG_LINE_LEN+1], sParam[MAX_CONFIG_LINE_LEN+1], sVal[MAX_CONFIG_LINE_LEN+1];

	if ((pstFile = fopen(pfile, "rb")) == NULL)
	{
		ERR_LOG("open file : %s failed, msg:%s", pfile, strerror(errno));
		return -1;
	}

	while (1)
	{
		fgets(sLine, sizeof(sLine), pstFile);
		if (feof(pstFile))
			break; 
		
		TConfigItem *pitem = new TConfigItem;
		if (GetParamVal(sLine, sParam, sVal) == 0)
		{
			pitem->strConfigName = sParam;
			pitem->strConfigValue = sVal;
			list.push_back(pitem);
		}
        else {
            pitem->strComment = sLine;
            list.push_back(pitem);
        }
	}
	fclose(pstFile);
	return 0;
}

void ReleaseConfigList(TConfigItemList & list)
{
    TConfigItemList::iterator it = list.begin();
    TConfigItem *pitem = NULL;
    for(; it != list.end();)
    {    
        pitem = *it; 
        delete pitem;
        it = list.erase(it);
    }    
    list.clear();
}


