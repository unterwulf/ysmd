/* $Id: YSM_os2.c,v 1.3 2004/03/26 01:17:53 rad2k Exp $ */
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-========================== YSM_os2.c ==============================-


YSM (YouSickMe) ICQ Client. An Original Multi-Platform ICQ client.
Copyright (C) 2002 rad2k Argentina.

YSM is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

For Contact information read the AUTHORS file.

*/

#include "YSM.h"
__RCSID("$Id: YSM_os2.c,v 1.3 2004/03/26 01:17:53 rad2k Exp $");
#ifdef OS2 /* os2 related code will be placed in this file */

/* based on the   Sergey I. Yevtushenko code `*/
#include "YSM_ToolBox.h"
static PPIB pib = 0;
static PTIB tib = 0;
static ULONG ulType = 0;
HWND hwndFrame;
HAB hab;
HMQ hmq;
HSWITCH hSw;
SWCNTRL swOldData = {0};
char cOldTitle[257];

void
os2_readkbd(struct KeyPacket *event) 
{

KBDKEYINFO	CharData;	/* Pointer to character data */

 for(;;){
   KbdCharIn(&CharData, IO_WAIT, 0);
   event->key=CharData.chChar;
   event->scancode=CharData.chScan;
   if(CharData.chChar==0 && CharData.chScan==0) continue;
   else break;
  }

  event->ext_key=((CharData.fbStatus & 0x2) && (CharData.chChar==0 || CharData.chChar==0xE0));
  event->shift_key=((CharData.fsState & 0x2) || (CharData.fsState & 0x1));
  return;
}

void os2_set_edit_cursor(int mode){
  VIOCURSORINFO    os2cursor;  /*  Cursor characteristics. */
  if(mode) os2cursor.yStart=-50;
  else  os2cursor.yStart=-90;
  os2cursor.cEnd  =-100;
  os2cursor.cx=1;
  os2cursor.attr=0;
  VioSetCurType(&os2cursor, 0);
}

void
YSM_WindowAlert(void)
{
  if(hmq){
 /* fishgui doesn't want us popping up */
 if (YSM_USER.fishgui.port) return;
 switch( YSM_SETTING_WINALERT )
 {
  case 0x1:
   os2_showin();
   break;
  case 0x3:
   os2_showin();
  case 0x2:
    _beginthread( (void *)&os2_flashtitle, NULL, THREADSTACKSIZE, NULL);
   break;
  default:
   break;
 }
}
 return;
}


void os2_flashtitle(void){
  if (hmq){
    os2_flashchange(TRUE);
    sleep(5);
    os2_flashchange(FALSE);
    return;
  }
}


int os2_showin(void){
  return(WinSwitchToProgram(hSw));
}

int os2_flashchange(int State)
{
 if (!hmq) return 0;
 WinPostMsg(hwndFrame, WM_FLASHWINDOW, (MPARAM)State, (MPARAM)0);
 return 1;
}


int os2_init(void){

 APIRET     rc;
 int dst[2];char buf[100];
 KBDINFO kbInfo;
 kbInfo.cb = sizeof(KBDINFO);
 _scrsize (dst); /* Setting COLUMNS and ROWS variable for the getline()*/
 snprintf(buf,sizeof(buf),"COLUMNS=%d",dst[0]);
 putenv(buf);
 snprintf(buf,sizeof(buf),"ROWS=%d",dst[1]);
 putenv(buf);

 rc = KbdGetStatus(&kbInfo, 0); /* switching keyboard to binary mode */
 if (!rc){
  kbInfo.fsMask = KEYBOARD_BINARY_MODE | KEYBOARD_SHIFT_REPORT ;
  rc = KbdSetStatus(&kbInfo, 0);
 }
 VioSetAnsi(1,0); /* turning ansi on */
 os2_set_edit_cursor(0); /* reset cursor to normal */

	if(DosGetInfoBlocks(&tib, &pib)) return 1;
    ulType = pib->pib_ultype;
    pib->pib_ultype = 3;	/* switching app type to PM */
    hab = WinInitialize(0);
    hmq = WinCreateMsgQueue(hab, 0);
	hSw = WinQuerySwitchHandle(0, pib->pib_ulpid);
	if(hSw)
	{
		WinQuerySwitchEntry(hSw, &swOldData);
		hwndFrame = swOldData.hwnd;
		if(hwndFrame)
			WinQueryWindowText(hwndFrame, sizeof(cOldTitle), (PCH)cOldTitle);
	}
    return 0;
}


void os2_minimize_console()
{
 if(hmq) {
	 WinPostMsg(hwndFrame, WM_SYSCOMMAND, (MPARAM)SC_MINIMIZE, MPFROM2SHORT(CMDSRC_MENU, FALSE));
 }
}


void os2_set_title(char *title)
{
    if(hmq)
    {
        static char vTitle[MAXPATH]="";
        if(strcmp(vTitle, title))
        {
            SWCNTRL swData = swOldData;
            strcpy(vTitle, title);
            strncpy(swData.szSwtitle, title, MAXNAMEL);
            WinChangeSwitchEntry(hSw, &swData);
            WinSetWindowText(hwndFrame, (PCH)title);
        }
    }
}

void os2_deinit(void)
{
    if(hwndFrame)
        os2_set_title(cOldTitle);
    WinChangeSwitchEntry(hSw, &swOldData);
	
    if(hmq)
        WinDestroyMsgQueue(hmq);
    if(hab)
        WinTerminate(hab);
	
    if(pib && ulType)
        pib->pib_ultype = ulType;
}

void os2_set_clip(char *text)
{
    char *pByte = 0;
    int sz = strlen(text) + 1;
    if(!hab || !text) return;
    WinOpenClipbrd(hab);
    WinEmptyClipbrd(hab);
	
    if (!DosAllocSharedMem((PPVOID)&pByte, 0, sz,
        PAG_WRITE | PAG_COMMIT | OBJ_GIVEABLE | OBJ_GETTABLE))
    {
        memcpy(pByte, text, sz);
        WinSetClipbrdData(hab, (ULONG) pByte, CF_TEXT, CFI_POINTER);
    }
    WinCloseClipbrd(hab);
}

char* os2_get_clip(void)
{
    char *ClipData;
    char *str;
    ULONG ulFormat;
    int sz;
    if(!hab)
        return 0;
    WinQueryClipbrdFmtInfo(hab, CF_TEXT, &ulFormat);
    if(ulFormat != CFI_POINTER)
        return 0;
    WinOpenClipbrd(hab);
    ClipData = (char *)WinQueryClipbrdData(hab, CF_TEXT);
    if(!ClipData)
        return 0;
    sz = strlen(ClipData) + 1;
    str = malloc(sz);
    memcpy(str, ClipData, sz);
    WinCloseClipbrd(hab);
    return str;
}

int os2_startsession(char ** argv,int argc){
	int x=1;
	char os2_cmd[MAX_DATA_LEN+1];
    while (argv[x] != NULL && x < argc ) {
		strcat(os2_cmd,argv[x]);
		x++;
	}
	
	STARTDATA  sd;
	PID        pidProcess;
	CHAR       szBuf[CCHMAXPATH];
	ULONG      ulSessionID;
	APIRET     rc;
	
	sd.Length = sizeof(sd);                    /* Length of the structure */
	sd.Related = SSF_RELATED_INDEPENDENT;      /* Unrelated session       */
	sd.FgBg = SSF_FGBG_FORE;                   /* In the foreground       */
	sd.TraceOpt = SSF_TRACEOPT_NONE;           /* No tracing              */
	sd.PgmTitle = (PSZ) NULL;                  /* Title is PgmName        */
	sd.PgmName = argv[0];                      /* Address of szPgmName    */
	sd.PgmInputs =  os2_cmd;                   /* command line args    */
	sd.TermQ = (PBYTE) NULL;                   /* No terminal queue       */
	sd.Environment = (PBYTE) NULL;             /* Inherits environment    */
	sd.InheritOpt = SSF_INHERTOPT_PARENT;      /* Uses parent environment */
	sd.SessionType = SSF_TYPE_DEFAULT;         /* def session              */
	sd.IconFile = (PSZ) NULL;                  /* Uses default icon       */
	sd.PgmHandle = 0;                          /* Used by Win calls       */
	sd.PgmControl = SSF_CONTROL_MAXIMIZE;      /* Starts app maximized    */
	sd.InitXPos = 0;                           /* Lower left corner       */
	sd.InitYPos = 0;                           /* Lower left corner       */
	sd.InitXSize = 0;                          /* Ignored for maximized   */
	sd.InitYSize = 0;                          /* Ignored for maximized   */
	sd.ObjectBuffer = szBuf;                   /* Fail-name buffer        */
	sd.ObjectBuffLen = sizeof(szBuf);          /* Buffer length           */
	
	char drive[_MAX_DRIVE], dir[_MAX_DIR];
	char fname[_MAX_FNAME], ext[_MAX_EXT];
	
	char path_tmp[_MAX_PATH],curdir[_MAX_PATH];
	_splitpath (argv[0], drive, dir, fname, ext);
	snprintf(path_tmp,sizeof(path_tmp)-1,"%s\%s",drive,dir);
	getcwd (curdir,_MAX_PATH-1);
	if (strlen(path_tmp)){
		_chdir2(path_tmp);
	}
	rc = DosStartSession(&sd, &ulSessionID, &pidProcess);
	_chdir2(curdir);
	if (rc) {
		DosBeep(750,250);
		printf("Error starting new session,rc=%d\n%s\n",(int)rc,szBuf);
	}
	return rc;
}

#endif
