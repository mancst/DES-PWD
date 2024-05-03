/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_DES-PWE.h"
#include "mt_report.h"

// 调试开关
static int s_debug = 0;
static const char *s_logFilePath = "/tmp/php_ex/ex_pay.log";

#define PHP_EX_DEBUG(fmt, ...) { \
    if (s_debug) { \
        FILE *file = fopen(s_logFilePath, "a+"); \
        if (file != NULL) { \
            time_t rawtime; \
            struct tm *timeinfo; \
            time(&rawtime); \
            timeinfo = localtime(&rawtime); \
            fprintf(file, "%d-%02d-%02d %02d:%02d:%02d\t %s:%s:%d " fmt "\n", \
                1900+timeinfo->tm_year, 1+timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, \
                timeinfo->tm_min, timeinfo->tm_sec, __FILE__, __FUNCTION__, __LINE__, \
                ##__VA_ARGS__); \
            fclose(file); \
        } else { \
            printf("open debug file %s faild\n", s_logFilePath); \
        } \
    } \
}

/* If you declare any globals in php_DES-PWE.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(DES-PWE)
*/

/* True global resources - no need for thread safety here */
static int le_DES-PWE;

/* {{{ DES-PWE_functions[]
 *
 * Every user visible function must have an entry in DES-PWE_functions[].
 */
const zend_function_entry DES-PWE_functions[] = {
	PHP_FE(confirm_DES-PWE_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE(MtReport_phpex_set_debug,	NULL)
	PHP_FE(php_MtReport_Init,	NULL)
	PHP_FE(php_MtReport_Attr_Add,	NULL)
	PHP_FE(php_MtReport_Attr_Set,	NULL)
	PHP_FE(php_MtReport_Str_Attr_Add,	NULL)
	PHP_FE(php_MtReport_Str_Attr_Set,	NULL)

	PHP_FE(php_MtReport_Log_Fatal,	NULL)
	PHP_FE(php_MtReport_Log_Error,	NULL)
	PHP_FE(php_MtReport_Log_Reqerr,	NULL)
	PHP_FE(php_MtReport_Log_Warn,	NULL)
	PHP_FE(php_MtReport_Log_Info,	NULL)
	PHP_FE(php_MtReport_Log_Debug,	NULL)
	PHP_FE(php_MtReport_Log_Other,	NULL)

	PHP_FE_END	/* Must be the last line in DES-PWE_functions[] */
};
/* }}} */

/* {{{ DES-PWE_module_entry
 */
zend_module_entry DES-PWE_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"DES-PWE",
	DES-PWE_functions,
	PHP_MINIT(DES-PWE),
	PHP_MSHUTDOWN(DES-PWE),
	PHP_RINIT(DES-PWE),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(DES-PWE),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(DES-PWE),
#if ZEND_MODULE_API_NO >= 20010901
	PHP_DES-PWE_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_DES-PWE
ZEND_GET_MODULE(DES-PWE)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("DES-PWE.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_DES-PWE_globals, DES-PWE_globals)
    STD_PHP_INI_ENTRY("DES-PWE.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_DES-PWE_globals, DES-PWE_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_DES-PWE_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_DES-PWE_init_globals(zend_DES-PWE_globals *DES-PWE_globals)
{
	DES-PWE_globals->global_value = 0;
	DES-PWE_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(DES-PWE)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(DES-PWE)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(DES-PWE)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(DES-PWE)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(DES-PWE)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "DES-PWE support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_DES-PWE_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_DES-PWE_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "DES-PWE", arg);
	RETURN_STRINGL(strg, len, 0);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

/* 
   *
   * 设置 php 扩展调试开关
   *
 */ 
PHP_FUNCTION(MtReport_phpex_set_debug)
{
	long int log_debug = 0;
	zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &log_debug);
	s_debug = log_debug;
}


/*
   *
   * api 接口初始化函数
   *
 */
PHP_FUNCTION(php_MtReport_Init)
{
	long int config_id = 0;
	char *local_log_file = NULL;
	int file_len = 0;
	long int local_log_type = 0;
	long int shm_key = MT_REPORT_DEF_SHM_KEY;

	array_init(return_value);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|lsll", 
		&config_id, &local_log_file, &file_len, &local_log_type, &shm_key) == FAILURE) 
	{
		add_assoc_long(return_value, "ret_code", -1);
		add_assoc_string(return_value, "ret_msg", "read parameters failed", 1);
		return;
	}

	if(MtReport_Init(config_id, local_log_file, local_log_type, shm_key) != 0)
	{
		add_assoc_long(return_value, "ret_code", -2);
		add_assoc_string(return_value, "ret_msg", "MtReport_Init failed", 1);
		return;
	}

	add_assoc_long(return_value, "ret_code", 0);
	add_assoc_string(return_value, "ret_msg", "ok", 1);
}

PHP_FUNCTION(php_MtReport_Attr_Add)
{
	long int attr_id = 0;
	long int attr_val = 0;
	int err_len = 0, ret = 0;
	char *err_msg = NULL;

	array_init(return_value);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll", &attr_id, &attr_val) == FAILURE)
	{
		add_assoc_long(return_value, "ret_code", -1);
		add_assoc_string(return_value, "ret_msg", "read parameters failed", 1);
		return;
	}

	if(attr_id == 0 || attr_val <= 0)
	{
		add_assoc_long(return_value, "ret_code", -2);
		err_len = spprintf(&err_msg, 0, "invalid parameters, attr:%ld, val:%ld", attr_id, attr_val);
		add_assoc_stringl(return_value, "ret_msg", err_msg, err_len, 0);
		return;
	}

	if((ret=MtReport_Attr_Add(attr_id, attr_val)) != 0)
	{
		add_assoc_long(return_value, "ret_code", -3);
		err_len = spprintf(&err_msg, 0, "MtReport_Attr_Add failed, ret:%d", ret);
		add_assoc_stringl(return_value, "ret_msg", err_msg, err_len, 0);
		return;
	}

	PHP_EX_DEBUG("MtReport_Attr_Add, attr:%ld, val:%ld", attr_id, attr_val);
	add_assoc_long(return_value, "ret_code", 0);
	add_assoc_string(return_value, "ret_msg", "ok", 1);
}

PHP_FUNCTION(php_MtReport_Attr_Set)
{
	long int attr_id = 0;
	long int attr_val = 0;
	int err_len = 0, ret = 0;
	char *err_msg = NULL;

	array_init(return_value);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll", &attr_id, &attr_val) == FAILURE)
	{
		add_assoc_long(return_value, "ret_code", -1);
		add_assoc_string(return_value, "ret_msg", "read parameters failed", 1);
		return;
	}

	if(attr_id == 0 || attr_val <= 0)
	{
		add_assoc_long(return_value, "ret_code", -2);
		err_len = spprintf(&err_msg, 0, "invalid parameters, attr:%ld, val:%ld", attr_id, attr_val);
		add_assoc_stringl(return_value, "ret_msg", err_msg, err_len, 0);
		return;
	}

	if((ret=MtReport_Attr_Set(attr_id, attr_val)) != 0)
	{
		add_assoc_long(return_value, "ret_code", -3);
		err_len = spprintf(&err_msg, 0, "MtReport_Attr_Set failed, ret:%d", ret);
		add_assoc_stringl(return_value, "ret_msg", err_msg, err_len, 0);
		return;
	}

	PHP_EX_DEBUG("MtReport_Attr_Set, attr:%ld, val:%ld", attr_id, attr_val);
	add_assoc_long(return_value, "ret_code", 0);
	add_assoc_string(return_value, "ret_msg", "ok", 1);
}

PHP_FUNCTION(php_MtReport_Str_Attr_Add)
{
	long int attr_id = 0;
	long int attr_val = 0;
	char *str = NULL;
	int str_len = 0;
	int err_len = 0, ret = 0;
	char *err_msg = NULL;

	array_init(return_value);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsl", &attr_id, &str, &str_len, &attr_val) == FAILURE)
	{
		add_assoc_long(return_value, "ret_code", -1);
		add_assoc_string(return_value, "ret_msg", "read parameters failed", 1);
		return;
	}

	if(attr_id == 0 || attr_val <= 0 || str_len <= 0 || str_len > 64)
	{
		add_assoc_long(return_value, "ret_code", -2);
		err_len = spprintf(&err_msg, 0, "invalid parameters, attr:%ld, str:%s, str_len:%d, val:%ld",
			attr_id, attr_val, str, str_len);
		add_assoc_stringl(return_value, "ret_msg", err_msg, err_len, 0);
		return;
	}

	if((ret=MtReport_Str_Attr_Add(attr_id, str, attr_val)) != 0)
	{
		add_assoc_long(return_value, "ret_code", -3);
		err_len = spprintf(&err_msg, 0, "MtReport_Str_Attr_Add failed, ret:%d", ret);
		add_assoc_stringl(return_value, "ret_msg", err_msg, err_len, 0);
		return;
	}

	PHP_EX_DEBUG("MtReport_Str_Attr_Add, attr:%ld, str:%s, val:%ld", attr_id, str, attr_val);
	add_assoc_long(return_value, "ret_code", 0);
	add_assoc_string(return_value, "ret_msg", "ok", 1);
}

PHP_FUNCTION(php_MtReport_Str_Attr_Set)
{
	long int attr_id = 0;
	long int attr_val = 0;
	char *str = NULL;
	int str_len = 0;
	int err_len = 0, ret = 0;
	char *err_msg = NULL;

	array_init(return_value);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsl", &attr_id, &str, &str_len, &attr_val) == FAILURE)
	{
		add_assoc_long(return_value, "ret_code", -1);
		add_assoc_string(return_value, "ret_msg", "read parameters failed", 1);
		return;
	}

	if(attr_id == 0 || attr_val <= 0 || str_len <= 0 || str_len > 64)
	{
		add_assoc_long(return_value, "ret_code", -2);
		err_len = spprintf(&err_msg, 0, "invalid parameters, attr:%ld, str:%s, str_len:%d, val:%ld",
			attr_id, attr_val, str, str_len);
		add_assoc_stringl(return_value, "ret_msg", err_msg, err_len, 0);
		return;
	}

	if((ret=MtReport_Str_Attr_Set(attr_id, str, attr_val)) != 0)
	{
		add_assoc_long(return_value, "ret_code", -3);
		err_len = spprintf(&err_msg, 0, "MtReport_Str_Attr_Set failed, ret:%d", ret);
		add_assoc_stringl(return_value, "ret_msg", err_msg, err_len, 0);
		return;
	}

	PHP_EX_DEBUG("php_MtReport_Str_Attr_Set, attr:%ld, str:%s, val:%ld", attr_id, str, attr_val);
	add_assoc_long(return_value, "ret_code", 0);
	add_assoc_string(return_value, "ret_msg", "ok", 1);
}

PHP_FUNCTION(php_MtReport_Log_Fatal)
{
	char *str_file = NULL;
	int str_file_len = 0;
	char *str_fun = NULL;
	int str_fun_len = 0;
	long int src_line_num = 0;
	char *str = NULL;
	int str_len = 0;
	int err_len = 0, ret = 0;
	char *err_msg = NULL;

	array_init(return_value);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssls", 
		&str_file, &str_file_len, &str_fun, &str_fun_len, &src_line_num, &str, &str_len) == FAILURE)
	{
		add_assoc_long(return_value, "ret_code", -1);
		add_assoc_string(return_value, "ret_msg", "read parameters failed", 1);
		return;
	}

	if((ret=MtReport_Log(MTLOG_TYPE_FATAL, " [%s:%s:%d] %s", str_file, str_fun, src_line_num, str)) != 0)
	{
		add_assoc_long(return_value, "ret_code", -2);
		err_len = spprintf(&err_msg, 0, "MtReport_Log failed, ret:%d", ret);
		add_assoc_stringl(return_value, "ret_msg", err_msg, err_len, 0);
		return;
	}

	PHP_EX_DEBUG("write fatal log info : [%s:%s:%d] %s", str_file, str_fun, src_line_num, str);
	add_assoc_long(return_value, "ret_code", 0);
	add_assoc_string(return_value, "ret_msg", "ok", 1);
}

PHP_FUNCTION(php_MtReport_Log_Error)
{
	char *str_file = NULL;
	int str_file_len = 0;
	char *str_fun = NULL;
	int str_fun_len = 0;
	long int src_line_num = 0;
	char *str = NULL;
	int str_len = 0;
	int err_len = 0, ret = 0;
	char *err_msg = NULL;

	array_init(return_value);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssls", 
		&str_file, &str_file_len, &str_fun, &str_fun_len, &src_line_num, &str, &str_len) == FAILURE)
	{
		add_assoc_long(return_value, "ret_code", -1);
		add_assoc_string(return_value, "ret_msg", "read parameters failed", 1);
		return;
	}

	if((ret=MtReport_Log(MTLOG_TYPE_ERROR, " [%s:%s:%d] %s", str_file, str_fun, src_line_num, str)) != 0)
	{
		add_assoc_long(return_value, "ret_code", -2);
		err_len = spprintf(&err_msg, 0, "MtReport_Log failed, ret:%d", ret);
		add_assoc_stringl(return_value, "ret_msg", err_msg, err_len, 0);
		return;
	}

	PHP_EX_DEBUG("write error log info : [%s:%s:%d] %s", str_file, str_fun, src_line_num, str);
	add_assoc_long(return_value, "ret_code", 0);
	add_assoc_string(return_value, "ret_msg", "ok", 1);
}

PHP_FUNCTION(php_MtReport_Log_Reqerr)
{
	char *str_file = NULL;
	int str_file_len = 0;
	char *str_fun = NULL;
	int str_fun_len = 0;
	long int src_line_num = 0;
	char *str = NULL;
	int str_len = 0;
	int err_len = 0, ret = 0;
	char *err_msg = NULL;

	array_init(return_value);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssls", 
		&str_file, &str_file_len, &str_fun, &str_fun_len, &src_line_num, &str, &str_len) == FAILURE)
	{
		add_assoc_long(return_value, "ret_code", -1);
		add_assoc_string(return_value, "ret_msg", "read parameters failed", 1);
		return;
	}

	if((ret=MtReport_Log(MTLOG_TYPE_REQERR, " [%s:%s:%d] %s", str_file, str_fun, src_line_num, str)) != 0)
	{
		add_assoc_long(return_value, "ret_code", -2);
		err_len = spprintf(&err_msg, 0, "MtReport_Log failed, ret:%d", ret);
		add_assoc_stringl(return_value, "ret_msg", err_msg, err_len, 0);
		return;
	}

	PHP_EX_DEBUG("write reqerr log info : [%s:%s:%d] %s", str_file, str_fun, src_line_num, str);
	add_assoc_long(return_value, "ret_code", 0);
	add_assoc_string(return_value, "ret_msg", "ok", 1);
}

PHP_FUNCTION(php_MtReport_Log_Warn)
{
	char *str_file = NULL;
	int str_file_len = 0;
	char *str_fun = NULL;
	int str_fun_len = 0;
	long int src_line_num = 0;
	char *str = NULL;
	int str_len = 0;
	int err_len = 0, ret = 0;
	char *err_msg = NULL;

	array_init(return_value);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssls", 
		&str_file, &str_file_len, &str_fun, &str_fun_len, &src_line_num, &str, &str_len) == FAILURE)
	{
		add_assoc_long(return_value, "ret_code", -1);
		add_assoc_string(return_value, "ret_msg", "read parameters failed", 1);
		return;
	}

	if((ret=MtReport_Log(MTLOG_TYPE_WARN, " [%s:%s:%d] %s", str_file, str_fun, src_line_num, str)) != 0)
	{
		add_assoc_long(return_value, "ret_code", -2);
		err_len = spprintf(&err_msg, 0, "MtReport_Log failed, ret:%d", ret);
		add_assoc_stringl(return_value, "ret_msg", err_msg, err_len, 0);
		return;
	}

	PHP_EX_DEBUG("write warn log info : [%s:%s:%d] %s", str_file, str_fun, src_line_num, str);
	add_assoc_long(return_value, "ret_code", 0);
	add_assoc_string(return_value, "ret_msg", "ok", 1);
}

PHP_FUNCTION(php_MtReport_Log_Info)
{
	char *str_file = NULL;
	int str_file_len = 0;
	char *str_fun = NULL;
	int str_fun_len = 0;
	long int src_line_num = 0;
	char *str = NULL;
	int str_len = 0;
	int err_len = 0, ret = 0;
	char *err_msg = NULL;

	array_init(return_value);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssls", 
		&str_file, &str_file_len, &str_fun, &str_fun_len, &src_line_num, &str, &str_len) == FAILURE)
	{
		add_assoc_long(return_value, "ret_code", -1);
		add_assoc_string(return_value, "ret_msg", "read parameters failed", 1);
		return;
	}

	if((ret=MtReport_Log(MTLOG_TYPE_INFO, " [%s:%s:%d] %s", str_file, str_fun, src_line_num, str)) != 0)
	{
		add_assoc_long(return_value, "ret_code", -2);
		err_len = spprintf(&err_msg, 0, "MtReport_Log failed, ret:%d", ret);
		add_assoc_stringl(return_value, "ret_msg", err_msg, err_len, 0);
		return;
	}

	PHP_EX_DEBUG("write info log info : [%s:%s:%d] %s", str_file, str_fun, src_line_num, str);
	add_assoc_long(return_value, "ret_code", 0);
	add_assoc_string(return_value, "ret_msg", "ok", 1);
}

PHP_FUNCTION(php_MtReport_Log_Debug)
{
	char *str_file = NULL;
	int str_file_len = 0;
	char *str_fun = NULL;
	int str_fun_len = 0;
	long int src_line_num = 0;
	char *str = NULL;
	int str_len = 0;
	int err_len = 0, ret = 0;
	char *err_msg = NULL;

	array_init(return_value);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssls", 
		&str_file, &str_file_len, &str_fun, &str_fun_len, &src_line_num, &str, &str_len) == FAILURE)
	{
		add_assoc_long(return_value, "ret_code", -1);
		add_assoc_string(return_value, "ret_msg", "read parameters failed", 1);
		return;
	}

	if((ret=MtReport_Log(MTLOG_TYPE_DEBUG, " [%s:%s:%d] %s", str_file, str_fun, src_line_num, str)) != 0)
	{
		add_assoc_long(return_value, "ret_code", -2);
		err_len = spprintf(&err_msg, 0, "MtReport_Log failed, ret:%d", ret);
		add_assoc_stringl(return_value, "ret_msg", err_msg, err_len, 0);
		return;
	}

	PHP_EX_DEBUG("write debug log info : [%s:%s:%d] %s", str_file, str_fun, src_line_num, str);
	add_assoc_long(return_value, "ret_code", 0);
	add_assoc_string(return_value, "ret_msg", "ok", 1);
}

PHP_FUNCTION(php_MtReport_Log_Other)
{
	char *str_file = NULL;
	int str_file_len = 0;
	char *str_fun = NULL;
	int str_fun_len = 0;
	long int src_line_num = 0;
	char *str = NULL;
	int str_len = 0;
	int err_len = 0, ret = 0;
	char *err_msg = NULL;

	array_init(return_value);
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssls", 
		&str_file, &str_file_len, &str_fun, &str_fun_len, &src_line_num, &str, &str_len) == FAILURE)
	{
		add_assoc_long(return_value, "ret_code", -1);
		add_assoc_string(return_value, "ret_msg", "read parameters failed", 1);
		return;
	}

	if((ret=MtReport_Log(MTLOG_TYPE_OTHER, " [%s:%s:%d] %s", str_file, str_fun, src_line_num, str)) != 0)
	{
		add_assoc_long(return_value, "ret_code", -2);
		err_len = spprintf(&err_msg, 0, "MtReport_Log failed, ret:%d", ret);
		add_assoc_stringl(return_value, "ret_msg", err_msg, err_len, 0);
		return;
	}

	PHP_EX_DEBUG("write other log info : [%s:%s:%d] %s", str_file, str_fun, src_line_num, str);
	add_assoc_long(return_value, "ret_code", 0);
	add_assoc_string(return_value, "ret_msg", "ok", 1);
}

