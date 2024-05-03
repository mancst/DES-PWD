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

#ifndef PHP_DES-PWE_H
#define PHP_DES-PWE_H

extern zend_module_entry DES-PWE_module_entry;
#define phpext_DES-PWE_ptr &DES-PWE_module_entry

#define PHP_DES-PWE_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_DES-PWE_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_DES-PWE_API __attribute__ ((visibility("default")))
#else
#	define PHP_DES-PWE_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(DES-PWE);
PHP_MSHUTDOWN_FUNCTION(DES-PWE);
PHP_RINIT_FUNCTION(DES-PWE);
PHP_RSHUTDOWN_FUNCTION(DES-PWE);
PHP_MINFO_FUNCTION(DES-PWE);

PHP_FUNCTION(confirm_DES-PWE_compiled);	/* For testing, remove later. */
PHP_FUNCTION(MtReport_phpex_set_debug);

// 监控点上报接口
PHP_FUNCTION(php_MtReport_Init);
PHP_FUNCTION(php_MtReport_Attr_Add);
PHP_FUNCTION(php_MtReport_Attr_Set);
PHP_FUNCTION(php_MtReport_Str_Attr_Add);	
PHP_FUNCTION(php_MtReport_Str_Attr_Set);	

// 日志上报接口
PHP_FUNCTION(php_MtReport_Log_Fatal);	
PHP_FUNCTION(php_MtReport_Log_Error);	
PHP_FUNCTION(php_MtReport_Log_Reqerr);	
PHP_FUNCTION(php_MtReport_Log_Warn);	
PHP_FUNCTION(php_MtReport_Log_Info);	
PHP_FUNCTION(php_MtReport_Log_Debug);	
PHP_FUNCTION(php_MtReport_Log_Other);	

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     

ZEND_BEGIN_MODULE_GLOBALS(DES-PWE)
	long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(DES-PWE)
*/

/* In every utility function you add that needs to use variables 
   in php_DES-PWE_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as DES-PWE_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define DES-PWE_G(v) TSRMG(DES-PWE_globals_id, zend_DES-PWE_globals *, v)
#else
#define DES-PWE_G(v) (DES-PWE_globals.v)
#endif

#endif	/* PHP_DES-PWE_H */


