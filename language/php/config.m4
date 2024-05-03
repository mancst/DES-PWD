dnl $Id$
dnl config.m4 for extension DES-PWE

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(DES-PWE, for DES-PWE support,
dnl Make sure that the comment is aligned:
dnl [  --with-DES-PWE             Include DES-PWE support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(DES-PWE, whether to enable DES-PWE support,
dnl Make sure that the comment is aligned:
[  --enable-DES-PWE           Enable DES-PWE support])

if test "$PHP_DES-PWE" != "no"; then
  PHP_ADD_INCLUDE(/usr/include/mtreport_api/)
  PHP_ADD_LIBRARY_WITH_PATH(mtreport_api, /usr/lib64, DES-PWE_SHARED_LIBADD)

  PHP_SUBST(DES-PWE_SHARED_LIBADD)
  PHP_NEW_EXTENSION(DES-PWE, DES-PWE.c, $ext_shared)
fi
