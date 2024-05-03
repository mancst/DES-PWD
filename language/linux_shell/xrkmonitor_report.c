
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>

#include <mt_report.h>
#include <sv_cfg.h>
#include <mt_log.h>

const char *g_pVersion = "open-version-v2.0-@2021-05-24";

void Help()
{
	printf("DES-PWE report data for linux shell。 \n");
    printf("%s\n\n", g_pVersion);
	printf("./DES-PWE_report help\n");
    printf("./DES-PWE_report version\n");

	// 监控点数据上报
	printf("./DES-PWE_report attr add id number\n");
	printf("./DES-PWE_report attr set id number\n");
	printf("./DES-PWE_report strattr add id string number\n");
	printf("./DES-PWE_report strattr set id string number\n");

	// 日志数据上报 log_type 可以是: debug,info,warn,reqerr,error,fatal
	printf("./DES-PWE_report log config_id log_type log\n");

	// 带配置文件 - 监控点数据上报
	printf("./DES-PWE_report file config_file attr add id_string number\n");
	printf("./DES-PWE_report file config_file attr set id_string number\n");
	printf("./DES-PWE_report file config_file strattr add id_string string number\n");
	printf("./DES-PWE_report file config_file strattr set id_string string number\n");

	// 带配置文件 - 日志数据上报
	printf("./DES-PWE_report file config_file log log_type log\n");
    printf("./DES-PWE_report file config_file table table_id_str static_time stat_id log\n");
}


int no_file_report(int argc, char *argv[])
{
	int iRet = 0;
	if(argc == 5) {
		if(!strcmp(argv[1], "attr")) {
			iRet=MtReport_Init(0, NULL, 0, 0);
			if(iRet != 0) {
				fprintf(stderr, "MtReport_Init failed\n");
				return iRet;
			}
	
			if(!strcmp(argv[2], "add")) {
				iRet=MtReport_Attr_Add(atoi(argv[3]), atoi(argv[4]));
			}
			else if(!strcmp(argv[2], "set")) {
				iRet=MtReport_Attr_Set(atoi(argv[3]), atoi(argv[4]));
			}
			else {
				fprintf(stderr, "invalid parameter, try ./DES-PWE_report help\n");
				return ERROR_LINE;
			}
		}
		else if(!strcmp(argv[1], "log")) {
			iRet=MtReport_Init(strtoul(argv[2], NULL, 10), NULL, 0, 0);
			if(iRet != 0) {
				fprintf(stderr, "MtReport_Init failed, log config id:%s\n", argv[2]);
				return iRet;
			}
			if(!strcmp(argv[3], "debug"))
				iRet=MtReport_Log_Debug("%s", argv[4]);
			else if(!strcmp(argv[3], "info"))
				iRet=MtReport_Log_Info("%s", argv[4]);
			else if(!strcmp(argv[3], "warn"))
				iRet=MtReport_Log_Warn("%s", argv[4]);
			else if(!strcmp(argv[3], "reqerr"))
				iRet=MtReport_Log_Reqerr("%s", argv[4]);
			else if(!strcmp(argv[3], "error"))
				iRet=MtReport_Log_Error("%s", argv[4]);
			else if(!strcmp(argv[3], "fatal"))
				iRet=MtReport_Log_Fatal("%s", argv[4]);
			else {
				fprintf(stderr, "invalid parameter, try ./DES-PWE_report help\n");
				return ERROR_LINE;
			}
		}
		else {
			fprintf(stderr, "invalid parameter, try ./DES-PWE_report help\n");
			return ERROR_LINE;
		}
	}
	else if(argc == 6) {
		if(!strcmp(argv[1], "strattr")) {
			iRet=MtReport_Init(0, NULL, 0, 0);
			if(iRet != 0) {
				fprintf(stderr, "MtReport_Init failed\n");
				return iRet;
			}
			if(!strcmp(argv[2], "add")) {
				iRet=MtReport_Str_Attr_Add(atoi(argv[3]), argv[4], atoi(argv[5]));
			}
			else if(!strcmp(argv[2], "set")) {
				iRet=MtReport_Str_Attr_Set(atoi(argv[3]), argv[4], atoi(argv[5]));
			}
			else {
				fprintf(stderr, "invalid parameter, try ./DES-PWE_report help\n");
				return ERROR_LINE;
			}
		}
		else {
			fprintf(stderr, "invalid parameter, try ./DES-PWE_report help\n");
			return ERROR_LINE;
		}
	}
	else {
		fprintf(stderr, "invalid parameter, try ./DES-PWE_report help\n");
		return ERROR_LINE;
	}
	return iRet;
}

int file_report(int argc, char *argv[])
{
	int iRet = 0;
	int iAttrId = 0;
	int iPluginId = 0;
    char szPluginName[64] = {0};
    char szPluginVer[16] = {0};
	char *ptmp = NULL;
	char *pself = strdup(argv[0]);
	if((ptmp=strrchr(pself, '/')) != NULL) {
		*ptmp = '\0';
		chdir(pself);
		free(pself);
	}

	if(argc < 3) {
		fprintf(stderr, "invalid parameter, try ./DES-PWE_report help\n");
		return ERROR_LINE;
	}

    // 工具程序特殊逻辑，需要先从配置文件里获取基础信息
    if((iRet=LoadConfig(argv[2],
        "XRK_PLUGIN_ID", CFG_INT, &iPluginId, 0,
        "XRK_PLUGIN_NAME", CFG_STRING, szPluginName, "", MYSIZEOF(szPluginName),
        "XRK_PLUGIN_HEADER_FILE_VER", CFG_STRING, szPluginVer, "", MYSIZEOF(szPluginVer),
        NULL)) < 0)
    {
        fprintf(stderr, "loadconfig from:%s failed, msg:%s !\n", argv[2], strerror(errno));
        return ERROR_LINE;
    }
    if(iPluginId <= 0 || szPluginName[0] == '\0' || szPluginVer[0] == '\0') {
        fprintf(stderr, "read config from file:%s failed, plugin (id, name, version) !\n", argv[2]);
        return ERROR_LINE;
    }
    
    iRet = MtReport_Plus_Init(argv[2], iPluginId, szPluginName, szPluginVer);
    if(iRet < 0 || g_mtReport.pMtShm == NULL) {
        return ERROR_LINE;
    }      

	if(argc == 7) {
        if(!strcmp(argv[3], "attr")) {
			iRet=LoadConfig(argv[2], argv[5], CFG_INT, &iAttrId, 0, (void*)NULL);
			if(iRet < 0 || iAttrId <= 0) {
				fprintf(stderr, "load attr:%s failed or invalid file(%s), attr id(%d), ret:%d\n",
					argv[5], argv[2], iAttrId, iRet);
				return ERROR_LINE;
			}

			if(!strcmp(argv[4], "add")) {
				iRet=MtReport_Attr_Add(iAttrId, atoi(argv[6]));
			}
			else if(!strcmp(argv[4], "set")) {
				iRet=MtReport_Attr_Set(iAttrId, atoi(argv[4]));
			}
			else {
				fprintf(stderr, "invalid parameter, try ./DES-PWE_report help\n");
				return ERROR_LINE;
			}
		}
	}
	else if(argc == 8) {
        if(!strcmp(argv[3], "table")) {
            int iTableId = 0;
            LoadConfig(argv[2], argv[4], CFG_INT, &iTableId, 0, NULL);
            if(iTableId != 0) {
                if((iRet=MtReport_Plugin_Table(iPluginId, iTableId, atoi(argv[5]), atoi(argv[6]), "%s", argv[7])) != 0) {
                    fprintf(stderr, "MtReport_Plugin_Table failed, ret:%d\n", iRet);
                    return ERROR_LINE;
                }
                return 0;
            }
            else {
                fprintf(stderr, "get table id:%d failed, config file:%s\n", iTableId, argv[2]);
                return ERROR_LINE;
            }
        }

		if(!strcmp(argv[3], "strattr")) {
			iRet=LoadConfig(argv[2], argv[5], CFG_INT, &iAttrId, 0, (void*)NULL);
			if(iRet < 0 || iAttrId <= 0) {
				fprintf(stderr, "load attr:%s failed or invalid file(%s), attr id(%d), ret:%d\n",
					argv[5], argv[2], iAttrId, iRet);
				return ERROR_LINE;
			}

			if(!strcmp(argv[4], "add")) {
				iRet=MtReport_Str_Attr_Add(iAttrId, argv[6], atoi(argv[7]));
			}
			else if(!strcmp(argv[4], "set")) {
				iRet=MtReport_Str_Attr_Set(iAttrId, argv[6], atoi(argv[7]));
			}
			else {
				fprintf(stderr, "invalid parameter, try ./DES-PWE_report help\n");
				return ERROR_LINE;
			}
		}
		else {
			fprintf(stderr, "invalid parameter, try ./DES-PWE_report help\n");
			return ERROR_LINE;
		}
	}
	else if(argc == 6) {
		if(!strcmp(argv[3], "log")) {
			if(!strcmp(argv[4], "debug"))
				iRet=MtReport_Log_Debug("%s", argv[5]);
			else if(!strcmp(argv[4], "info"))
				iRet=MtReport_Log_Info("%s", argv[5]);
			else if(!strcmp(argv[4], "warn"))
				iRet=MtReport_Log_Warn("%s", argv[5]);
			else if(!strcmp(argv[4], "reqerr"))
				iRet=MtReport_Log_Reqerr("%s", argv[5]);
			else if(!strcmp(argv[4], "error"))
				iRet=MtReport_Log_Error("%s", argv[5]);
			else if(!strcmp(argv[4], "fatal"))
				iRet=MtReport_Log_Fatal("%s", argv[5]);
			else {
				fprintf(stderr, "invalid parameter, try ./DES-PWE_report help\n");
				return ERROR_LINE;
			}
		}
		else {
			fprintf(stderr, "invalid parameter, try ./DES-PWE_report help\n");
			return ERROR_LINE;
		}
	}
	else {
		fprintf(stderr, "invalid parameter, try ./DES-PWE_report help\n");
		return ERROR_LINE;
	}
	return iRet;
}


int main(int argc, char *argv[])
{
	int iRet = 0;

    if(argc == 2 && !strcasecmp(argv[1], "help")) {
		Help();
		return 0;
	}
    if(argc == 2 && !strcasecmp(argv[1], "version")) {
        printf("%s\n", g_pVersion);
        return 0;
    }
	else if(argc < 2) {
		fprintf(stderr, "invalid parameter, try ./DES-PWE_report help\n");
		return ERROR_LINE;
	}

    srand(time(NULL));
	if(!strcmp(argv[1], "file"))
		iRet = file_report(argc, argv);
	else
		iRet = no_file_report(argc, argv);
	return iRet;
}


