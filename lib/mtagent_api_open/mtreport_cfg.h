#ifndef _mtreport_cfg_h 
#define _mtreport_cfg_h 1

#include <iostream>
#include <list>

typedef struct {
    std::string strComment;
    std::string strConfigName;
    std::string strConfigValue;
}TConfigItem;
typedef std::list<TConfigItem*> TConfigItemList;

int UpdateConfigFile(const char *path, const char *pfile, TConfigItemList & list);
void ReleaseConfigList(TConfigItemList & list);

#endif

