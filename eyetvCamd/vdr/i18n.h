/*
 * i18n.h: Internationalization
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: i18n.h 1.18 2006/03/26 09:08:00 kls Exp $
 */

#ifndef __I18N_H
#define __I18N_H

#include <stdio.h>

const int I18nNumLanguages = 21;

static inline const char *I18nTranslate(const char *s, const char *Plugin = NULL)
{
  return s;
}

#ifdef PLUGIN_NAME_I18N
#define tr(s)  I18nTranslate(s, PLUGIN_NAME_I18N)
#else
#define tr(s)  I18nTranslate(s)
#endif
#define trNOOP(s) (s)
#endif //__I18N_H
