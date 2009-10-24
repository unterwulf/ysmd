/*	$Id: YSM_Config.h,v 1.3 2005/11/14 01:59:11 rad2k Exp $ 
 *
 * YSM pre-compiling configuration.
 * ********************************
 * You may switch from an #undef to a #define in order to ACTIVATE a feature.
 * You may as well switch from a #define to an #undef in order to DISABLE it.
 */
#ifndef _CONFIG_H_
#define _CONFIG_H_

#define YSM_INFORMATION        "ysmd"
#define YSM_INFORMATION2       "0.0.1"

#define YSM_CFGDIRECTORY       ".ysmd"
#define YSM_CFGFILENAME        "ysmd.conf"
#define YSM_INCOMINGDIRECTORY  "incoming"
#define YSM_DEFAULTSRV         "login.icq.com"
#define YSM_DEFAULTPORT        5190
#define YSM_KEEPALIVETIME      90
#define YSM_COMMANDSTIME       15
#define YSM_FIFO               "/tmp/ysm"

/* YSM_SILENT_SLAVES_STATUS:
 * if active, only 'x to offline' and 'offline to x' status changes are shown.
 *********************************************************/
#define YSM_SILENT_SLAVES_STATUS

/* COMPACT_DISPLAY:
 * if active, YSM output (status changes, listings, etc) is more compact.
 *********************************************************/
#undef COMPACT_DISPLAY

/* YSM_WAR_MODE:
 * Undocumented 
 *********************************************************/
#undef YSM_WAR_MODE

/* YSM_TRACE_MEMLEAK:
 * enables the showleak command to help trace memory leaks.
 *********************************************************/
#undef YSM_TRACE_MEMLEAK

#endif /* _CONFIG_H_ */
