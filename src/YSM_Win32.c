/*	$Id: YSM_Win32.c,v 1.66 2005/09/04 01:36:48 rad2k Exp $	*/
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-========================= YSM_Win32.c =============================-


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
__RCSID("$Id: YSM_Win32.c,v 1.66 2005/09/04 01:36:48 rad2k Exp $");

#include "YSM_ToolBox.h"
#include "YSM_Wrappers.h"
#include "YSM_Prompt.h"
#include "YSM_Win32.h"

#if defined (WIN32) /* WINDOWS */
HWND		g_hYSMHiddenWnd = NULL;

/* I HAD to do this because Threads in Win32
 * drove crazy with global variables.
 * hence they now call this function
 */

HWND
getConsoleWindowHandlebyTitle(void)
{
char	pszNewWindowTitle[1024];

	(void)wsprintf( pszNewWindowTitle, "YSM ICQ [%ld]", YSM_USER.Uin);

	/* Look for NewWindowTitle. */
	return FindWindow(NULL, pszNewWindowTitle); 
 
}

/*	Port of the WinNT GetConsoleWindow function		*/
/*	This should work pretty well with YSM.			*/
/*	It sets the console's Title to a unique YSM title	*/

HWND
getConsoleWindow(void) 
{ 
	return getConsoleWindowHandlebyTitle();
} 

void
ScrollConsoleVertical( short n ) 
{
    SMALL_RECT w1;
    CHAR_INFO ci;
	HANDLE hConsole;
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo; 

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    ci.Attributes = 0;

	if (! GetConsoleScreenBufferInfo(hConsole, &csbiInfo)) 
		return;


	w1 = csbiInfo.srWindow;

	if ( w1.Top > 0 || w1.Top <= 0 && n < 0 ) { 
		w1.Top -= n;      // move top up by one row 
		w1.Bottom -= n;   // move bottom up by one row 

		if (! SetConsoleWindowInfo( 
               hConsole,      // screen buffer handle 
               TRUE,         // absolute coordinates 
               &w1)) // specifies new location 
        {

		}
 	} 
}


void
YSM_ScrollConsole( int8_t fl )
{
	if (fl) ScrollConsoleVertical( 1 );
	else ScrollConsoleVertical( -1 );
}

int32_t
CtrlHandler( int32_t fdwCtrlType )
{
	switch (fdwCtrlType) {
		/* Handle all exit signals to clean, then pass them */
		case CTRL_C_EVENT:
			PRINTF(VERBOSE_BASE,
				"\nUse the 'quit' command to exit ysm.\n");
			YSM_ConsoleRedrawPrompt(0);
			return TRUE;
		case CTRL_CLOSE_EVENT:
		case CTRL_BREAK_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			YSM_Exit(0, 0);
			return TRUE;
		default:
			return FALSE;
	}
}

int
getkey()
{
#ifndef YSM_WITH_THREADS

	static HANDLE handle;
	int read;
	INPUT_RECORD r;
			
	if (!handle)
		handle = GetStdHandle(STD_INPUT_HANDLE);

	do {
		if (!GetNumberOfConsoleInputEvents(handle, &read) || !read)
			return (0);
		if (!ReadConsoleInput(handle,&r,1,&read) || !read)
			return (0);
	} while (r.EventType != KEY_EVENT || !r.Event.KeyEvent.bKeyDown);


	return (r.Event.KeyEvent.uChar.AsciiChar);

#else		/* we need it to be blocking */
	return _getch();

#endif
}


void
do_backspace()
{
HANDLE hStdout;
CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 

	if (! GetConsoleScreenBufferInfo(hStdout, &csbiInfo)) 
		return;


	putchar(0x08);
	putchar(0x20);
	putchar(0x08);
	
	
	if (csbiInfo.dwCursorPosition.X <= 0)
	{
		
		/* Clear the last char on the upper row */
		/* and move the cursor up */
		csbiInfo.dwCursorPosition.Y --; 
		csbiInfo.dwCursorPosition.X = csbiInfo.dwSize.X - 1;
		SetConsoleCursorPosition(hStdout, csbiInfo.dwCursorPosition);
		putchar(0x20);
		SetConsoleCursorPosition(hStdout, csbiInfo.dwCursorPosition);
	}


}


char *
YSM_fgets(char *str, int size, char hide)
{
	/* XXX sorry, no cool fgets on windoze yet */
	return (fgets(str, size, stdin));
}

int8_t *
YSM_getpass(int8_t *text)
{
	return getpass(text);
}

int
PRINTF(int verbose_level, char *fmt,...)
{
	static HANDLE handle;
	int written;
	char ibuf[4096], *p, *q;
	va_list argptr;

	if ( verbose_level > YSM_SETTING_VERBOSE ) 
			return -1;

	while (g_promptstatus.flags & FL_BUSYDISPLAY)
		YSM_Thread_Sleep( 0, 100 );

	if (!handle)
		handle = GetStdHandle(STD_OUTPUT_HANDLE);

	va_start(argptr, fmt);
	vsprintf(ibuf, fmt, argptr);
	va_end(argptr);

	p = ibuf;
	while ((q = strchr(p, 0x1b)) != NULL) {
		WriteConsole(handle, p, q-p, (DWORD *)&written, 0);
		p = q;
		while (*p && *p != ';')
			p++;
		if (*p) {
			short color;
			p++;
			color = atoi(p);
			while (*p && *p != 'm')
				p++;
			if (*p)
				p++;

			switch (color) {
			case 30:
				color = 0;
				break;
			case 31:
				color = FOREGROUND_RED | FOREGROUND_INTENSITY;
				break;
			case 32:
				color = FOREGROUND_GREEN;
				break;
			case 33:
				color = FOREGROUND_RED | FOREGROUND_GREEN;
				break;
			case 34:
				color = FOREGROUND_RED | FOREGROUND_GREEN;
				break;
			case 35:
				color = FOREGROUND_RED | FOREGROUND_BLUE;
				break;
			case 36:
				color = FOREGROUND_GREEN | FOREGROUND_BLUE |
				    FOREGROUND_INTENSITY;
				break;
			case 37:
			default:
				color = FOREGROUND_RED | FOREGROUND_GREEN |
				    FOREGROUND_BLUE | FOREGROUND_INTENSITY;
				break;
			}
			SetConsoleTextAttribute(handle, color);
		}
	}

	if (*p)
		WriteConsole(handle, p, strlen(p), (DWORD *)&written, 0);

	return 0;
}


int
YSM_ReadWindowsProxy (void)
{
DWORD	dwType;
DWORD	dwSize = 1024;
long	res;
HKEY	hKey;
/* Don't worry in case its longer than 1024 (duh!) it won't overflow */
char	Proxy_buf[1024], *aux = NULL;


	res = RegOpenKeyEx( HKEY_CURRENT_USER, YSM_PROXYKEY,
						0, KEY_QUERY_VALUE,
						&hKey );
	if (res == ERROR_SUCCESS) {

		memset(Proxy_buf, 0, sizeof(Proxy_buf));

		res = RegQueryValueEx( hKey, YSM_PROXYVALUE, 0, &dwType,
								&Proxy_buf[0],
								&dwSize );
		if (res == ERROR_SUCCESS) {

			aux = strtok(Proxy_buf, ":");
			if (aux != NULL) {

				memset(YSM_USER.proxy.proxy_host, 0, 
					sizeof(YSM_USER.proxy.proxy_host));

				strncpy( YSM_USER.proxy.proxy_host,
					aux,
					sizeof(YSM_USER.proxy.proxy_host) - 1);
		
				aux = strtok(NULL, "");
				if(aux != NULL)
					YSM_USER.proxy.proxy_port = atoi(aux);

				RegCloseKey( hKey );
				return 1;
			}
		}
	
		RegCloseKey( hKey );
	}

	return 0;
}

void
YSM_WindowAlert(void)
{
	HWND YSM_HWND = getConsoleWindowHandlebyTitle();

	/* Probably because its a Win95/98/ME	*/
	if( YSM_HWND == NULL ) return;

	/* fishgui doesn't want us popping up */
	if (YSM_USER.fishgui.port && YSM_USER.fishgui.socket > 0
		&& YSM_USER.fishgui.hide_console) return;

	switch( YSM_SETTING_WINALERT )
	{
		case 0x1:
			ShowWindow( YSM_HWND, SW_SHOWNORMAL );
			break;
		case 0x3:
			ShowWindow( YSM_HWND, SW_SHOWNORMAL );
		case 0x2:
			FlashWindow( YSM_HWND, TRUE );	
			break;
		
		default:
			break;
	}

	return;
}

void
YSM_WindowHide( HWND handle )
{
	if( handle == NULL ) return;

	ShowWindow( handle, SW_MINIMIZE );
	return;
}

void
YSM_WindowShow( HWND handle )
{
	if (handle == NULL || 
		(YSM_USER.fishgui.port
		&& YSM_USER.fishgui.socket > 0
		&& YSM_USER.fishgui.hide_console)) return;

	ShowWindow( handle, SW_RESTORE );
	ShowWindow( handle, SW_SHOW );
	SetForegroundWindow( handle );

	return;
}


void
YSM_WindowRegisterHotKey(void)
{
		
	if(RegisterHotKey(
		g_hYSMHiddenWnd,
		0x1337,
		MOD_ALT | MOD_CONTROL,
		VkKeyScan((char)YSM_SETTING_HOT_KEY_MAXIMIZE)) != TRUE)
			return;
	
	return;
}


int32_t
WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch (message) {
		case WM_HOTKEY:
			YSM_WindowShow(getConsoleWindowHandlebyTitle());
			break;

		default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

return 0;
}

int32_t
YSM_CreateHotKeyWindow(void)
{
#if (WINVER <= 0x0400)
	return -1;
#else
	WNDCLASS	wc;
	LPCTSTR		szClassName = "YSMHiddenWindow", szAppName = "YSMHiddenWindow";
	WNDPROC		oldcallback = NULL;
	

	wc.style         = CS_VREDRAW | CS_HREDRAW;		//  Basically repaint the window anytime its size changes
	wc.lpfnWndProc   = (WNDPROC)WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = (HANDLE)GetModuleHandle(NULL);
	wc.hIcon         = 0;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);	//  Plain, default cursor
	wc.hbrBackground = (HBRUSH)(COLOR_SCROLLBAR+1);	//  Grey background
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szClassName;

	RegisterClass(&wc);

	g_hYSMHiddenWnd = CreateWindow( szClassName,
					szAppName,
					WS_POPUPWINDOW,
					CW_USEDEFAULT, 
					CW_USEDEFAULT, 
					CW_USEDEFAULT, 
					0,
					NULL,
					NULL, 
					(HANDLE)GetModuleHandle(NULL),	// hInstance
					NULL );

	if (g_hYSMHiddenWnd == NULL)
		return -1;

	oldcallback = (WNDPROC)GetWindowLong(g_hYSMHiddenWnd, GWL_WNDPROC);
	SetWindowLong(g_hYSMHiddenWnd, GWL_WNDPROC, (LONG)(WNDPROC)WndProc );

	return 0;
#endif
}


#else		/* UNIX */

#ifndef OS2
/* OS2 has its own WindowAlert rutine in YSM_os2.c */
void
YSM_WindowAlert(void)
{
	if (YSM_USER.fishgui.port && YSM_USER.fishgui.socket > 0
		&& YSM_USER.fishgui.hide_console) return;

	switch( YSM_SETTING_WINALERT )
	{
		case 0x1:
		case 0x3:
			PRINTF(VERBOSE_BASE, DECONIFY);
			break;
		case 0x2:
		default:
			break;
	}

	return;
}
#endif


void
CtrlHandler(int32_t sig)
{
	/* don't do a thing. depending on the OS we get
	 * called by process or thread, and unless we 
	 * spend more time on this function, its better
	 * to do nothing on ctrl+c 
	 */
}

int
getkey()
{
	unsigned char c;

#ifdef OS2
	c = _read_kbd(0, 1, 0);
	if (c == 0) {
		c = _read_kbd(0, 1, 0);
		c = 7;
	}

	if (c == 13) c = 10;
#else
	if (read(STDIN_FILENO, &c, sizeof(unsigned char)) <= 0) {
		fprintf(stderr, "Can't read: %s.\n", strerror(errno));
		YSM_Error(ERROR_CRITICAL, __FILE__, __LINE__, 1);
		/* NOTREACHED */
	}

#endif
	return ((int)c);
}


int
putch(int c)
{
	putchar(c);
	fflush(stdout);
	return (0);
}

int
gettxy(int *x, int *y)
{
	printf("\x1B[6n");
	
	/* Trash unrequired characters */
	getchar(); getchar();

	/* row */
	*y = 10 * (getchar() - '0');
	*y = *y + (getchar() - '0');

	/* Trash unrequired characters */
	getchar();

	*x = 10 * (getchar() - '0');
	*x = *x + (getchar() - '0');

	/* Trash unrequired characters */
	getchar();
	getchar();

	return (1);
}


int
getxy(int32_t *x, int32_t *y)
{
	char buf[20], *p, *q;
	ssize_t r = 0, i = 0;

	fprintf(stdout, "\x1B[6n");
	fflush(stdout);

#ifndef BEOS
	usleep(50000);
#endif
	if (YSM_USER.fishgui.socket <= 0 || !YSM_USER.fishgui.hide_console)
		if ((r = read(STDIN_FILENO, buf, sizeof(buf) - 1)) < 0)
			return (0);

	buf[r] = '\0';

	for (--r, i = 0; i < r && buf[i] != '\x1B' && buf[i+1] != '['; i++);
	if (i >= r)
		return (0);

	for (p = &buf[i]; *p != '\0' && *p != 'R'; p++);
	for (q = &buf[i]; *q != '\0' && *q != ';'; q++);
	if (*p == '\0' || *q == '\0')
		return (0);

	*p = '\0';
	*q = '\0';
	*x = atoi(q + 1);
	*y = atoi(&buf[i+2]);

	return (1);
}

void
gotoxy( int8_t X, int8_t Y )
{
	fprintf( stdout, "\33[%d;%dH", Y, X );
}

void
do_backspace()
{
	putchar('\b');
	putchar(0x20);
	putchar('\b');
	
	fflush(stdout);
}

char *
YSM_fgets(char *str, int size, char hide)
{
	unsigned char c;
	int i;

	if (size <= 0) {
		errno = EINVAL;
		return (NULL);
	}

	i = 0;
	str[i] = '\0';
	while (i < size) {
		c = getkey();
		switch (c) {
		case 0x08:
		case 0x7F:
			if (i) {
				str[--i] = '\0';
				do_backspace();
			}
			break;

		case '\n':
			putch(c);
			str[i++] = '\n';

		case '\0':
			str[i] = '\0';
			return (str);

		default:
			str[i++] = c;
			str[i] = '\0';
			if (!hide)
				putch(c);
		}
	}

	return (str);
}

int8_t *
YSM_getpass(int8_t *text)
{
struct termios t;
static int8_t	buf[MAX_PWD_LEN+1], *aux = NULL;
int8_t	restore = 0;

	PRINTF(VERBOSE_BASE, "%s", text);

	if (tcgetattr(STDIN_FILENO, &t) == 0) {

		t.c_lflag &= ~ECHO;

		if (tcsetattr(STDIN_FILENO, TCSANOW, &t)) {
			/* if it fails..we have to proceed */
		}

		restore = 1;
	}

	memset(buf, 0, sizeof(buf));	
	YSM_fgets(buf, sizeof(buf)-1, 1);
	buf[sizeof(buf)-1] = 0x00;
	aux = strchr(buf, '\n');
	if (aux != NULL) *aux = 0x00;
	PRINTF(VERBOSE_BASE, "\r\n", text);

	if (restore) {
		t.c_lflag |= ECHO;
		if (tcsetattr(STDIN_FILENO, TCSANOW, &t)) {
			/* if it fails..we have to proceed */
		}
	}

	return (int8_t *) &buf;
}

int
PRINTF(int verbose_level, char *fmt, ...)
{
	va_list argptr;
	int st;

	if ( verbose_level > YSM_SETTING_VERBOSE ) 
		return -1;

	while (g_promptstatus.flags & FL_BUSYDISPLAY)
		YSM_Thread_Sleep( 0, 100 );

	va_start(argptr, fmt);
	st = vprintf(fmt, argptr);
	va_end(argptr);

	fflush(stdout);

	return (st);
}

#endif	/* WIN32 */


/*
 * port for the getpass(3) function used in *nix.
 */

#if defined (WIN32) || defined (BEOS)

char *
getpass (const char *prompt)
{

static char password[MAX_PWD_LEN+1];
int n = 0;

	memset(password,'\0',sizeof(password));

	fputs(prompt, stderr);

#ifndef BEOS
	while ( ((password[n] = _getch()) != '\r') && n <= MAX_PWD_LEN )
#else
	while ( ((password[n] = getchar()) != '\r') && n <= MAX_PWD_LEN )
#endif
	{
		if( password[n] >= ' ' && password[n] <= '~' )
		{
			n++;
			PRINTF(VERBOSE_BASE,"*");
		}
		else
		{
			if( (password[n] == 0x08) && (n > 0) )
			{
				do_backspace();
				n--;
			}
	
			else
			{
				PRINTF(VERBOSE_BASE,"\n");
				fputs(prompt, stderr);
				n = 0;
			}
		}
	}

	if( password[n] == '\r' || password[n] == '\n' ) password[n] = '\0';
	password[n+1] = '\0';

	PRINTF( VERBOSE_BASE,"\n");

	return (char *) &password;
}

#endif /* WIN32 || BEOS */

int32_t
OutputCharacter( void *handle,
		int8_t	*ch,
		int32_t	len,
		COORD	coord,
		int32_t	*out,
		int8_t	*color )
{
#ifdef WIN32
	return WriteConsoleOutputCharacter(handle,
					ch,
					len,
					coord,
					(DWORD *)out);
#else
	gotoxy( coord.X, coord.Y );
	(*out) = 0;

	if (color != NULL) printf( color );	
	while (len--) {
		putch(*(ch+(*out)));
		(*out)++;
	}
	if (color != NULL) printf( NORMAL );
	return 0;
#endif

}

void
DrawBox( void *hOut, SMALL_RECT rect, int8_t *color )
{
COORD	pt;
int8_t	chBox[6];
int32_t	x = 0, out;

	chBox[0] = (char)0xda; chBox[1] = (char)0xbf;
	chBox[2] = (char)0xc0; chBox[3] = (char)0xd9;
	chBox[4] = (char)0xc4; chBox[5] = (char)0xb3;
	
	pt.Y = rect.Top;

	/* Top Left char */
	pt.X = rect.Left;
	OutputCharacter(hOut, &chBox[0], 1, pt, &out, color);
	pt.X++;

	/* Draw Top Horizontal line */
	for (x = 0; x < (rect.Right - rect.Left - 1); x++) {
		OutputCharacter(hOut, &chBox[4], 1, pt, &out, color);
		pt.X++;
	}

	/* Top Right char */
	pt.X = rect.Right;
	OutputCharacter(hOut, &chBox[1], 1, pt, &out, color);
	
	/* Draw vertical Left and Right line */
	pt.Y = rect.Top;
	pt.Y ++;
	
	for (x = 0; x < (rect.Bottom - rect.Top - 1); x++) {
		pt.X = rect.Left;
		OutputCharacter(hOut, &chBox[5], 1, pt, &out, color);
		pt.X = rect.Right;
		OutputCharacter(hOut, &chBox[5], 1, pt, &out, color);
		pt.Y++;
	}

	pt.Y = rect.Bottom;

	/* Bottom left bottom char */
	pt.X = rect.Left;
	OutputCharacter(hOut, &chBox[2], 1, pt, &out, color);
	pt.X++;	

	/* Draw Bottom Horizontal line */
	for (x = 0; x < (rect.Right - rect.Left - 1); x++) {
		OutputCharacter(hOut, &chBox[4], 1, pt, &out, color);
		pt.X++;
	}

	/* Bottom Right char */
	pt.X = rect.Right;
	OutputCharacter(hOut, &chBox[3], 1, pt, &out, color);
} 

void
BoxString( int8_t *string,
	int16_t	color,
	int32_t	top,
	int32_t	bottom,
	int8_t	*ansicol1,
	int8_t	*ansicol2 )
{
#ifdef WIN32
CONSOLE_SCREEN_BUFFER_INFO	bInfo;
#endif
COORD				postext;
SMALL_RECT			rc;
void				*hOut = 0;
int32_t				i = 0, out = 0, x, y = 0, x1, x2, y1, y2;

#ifdef WIN32
	hOut = GetStdHandle(STD_OUTPUT_HANDLE); 

	if (!GetConsoleScreenBufferInfo( hOut, &bInfo ))
		return;

	x = bInfo.dwSize.X;
	y = bInfo.dwCursorPosition.Y;
#elif OS2
	int dst[2];
	_scrsize(dst);
	VioGetCurPos((PSHORT)&y, (PSHORT)&x, 0);
	x = dst[0];
	y++;
#else
	if (!getxy(&x, &y)) return;
	x = 80;			/* use a default value for x */
#endif

	x1 = (x - strlen(string))/2 - 2;
	y1 = y + top;
	x2 = x1 + strlen(string) + 4;
	y2 = y1 + bottom;

	postext.X = x1;
	postext.Y = y1;

	for (i = 0; i < 5; i++) {
#ifdef WIN32
		if (!FillConsoleOutputAttribute( hOut,
						color,
						strlen(string) + 4,
						postext,
						(DWORD *)&out )) return;
#endif
		postext.Y++;
	} 

	postext.X = x1 + 2;
	postext.Y = y1 + 2 + top;	

	OutputCharacter(hOut, string, strlen(string), postext, &out, ansicol1);

	rc.Left	= x1;
	rc.Top	= y1;
	rc.Right = x2 - 1;
	rc.Bottom = y2 - 1;

	DrawBox( hOut, rc, ansicol2 );
}

void
YSM_PrintGreetingBox( void )
{
#ifdef WIN32
	BoxString( "Welcome to ysmICQ client for Windows. "
			"Version " YSM_INFORMATION2,
			FOREGROUND_RED | FOREGROUND_GREEN |
			FOREGROUND_BLUE | FOREGROUND_INTENSITY |
			BACKGROUND_RED | BACKGROUND_BLUE,
			0,
			5,
			0,
			0 );

	fprintf( stdout, "\n\n\n\n\n" );

	BoxString( "HAVING FUN WITH A BORING PROTOCOL",
			FOREGROUND_RED | FOREGROUND_GREEN |
			FOREGROUND_BLUE | FOREGROUND_INTENSITY,
			-1,
			3,
			0,
			0 );

	fprintf( stdout, "\n\n\n\n\n" );

#else

# ifndef COMPACT_DISPLAY
	PRINTF( VERBOSE_BASE, "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
	    "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
	    "\n\n\n\n\n\n\n\n\n\n\n\n\n");
# endif

	PRINTF( VERBOSE_BASE, CLRSCR );
	gotoxy( 0, 0 );

	BoxString( "Welcome to ysmICQ client. Version " YSM_INFORMATION2,
			0,
			0,
			5,
			BRIGHT_BLUE,
			BRIGHT_GREEN);

	fprintf( stdout, "\n\n" );

	BoxString( "HAVING FUN WITH A BORING PROTOCOL" ,
			0,
			-1,
			3,
			BRIGHT_BLUE,
			NORMAL);

	fprintf( stdout, "\n" );

#endif
		
}

void
YSM_PrintWizardBox( int8_t *string )
{
#ifdef WIN32
	system("cls");
	BoxString( string,
			FOREGROUND_RED | FOREGROUND_GREEN |
			FOREGROUND_BLUE | FOREGROUND_INTENSITY |
			BACKGROUND_RED | BACKGROUND_BLUE,
			0,
			5,
			0,
			0 );

	fprintf( stdout, "\n\n\n\n\n\n" );

#else

	PRINTF( VERBOSE_BASE, CLRSCR );
	gotoxy( 0, 0 );

	BoxString( string,
			0,
			0,
			5,
			BRIGHT_BLUE,
			BRIGHT_GREEN);

	fprintf( stdout, "\n\n" );

#endif
		
}


void
YSM_SetConsoleTitle(void)
{
char	pszNewWindowTitle[1024];

	/* Format a "unique" NewWindowTitle. */
	(void)snprintf( pszNewWindowTitle,
			sizeof(pszNewWindowTitle),
			"YSM ICQ [%d]",
			(int)YSM_USER.Uin );

#ifdef WIN32
	/* Change current window title. */
	SetConsoleTitle(pszNewWindowTitle); 
 
	/* Ensure window title has been updated. */
	Sleep(40); 
#elif OS2
	os2_set_title(pszNewWindowTitle);
#elif !defined(__OpenBSD__)
	printf( "%c]0;%s%c", '\033', pszNewWindowTitle, '\007' );
#endif
}


