/*	$Id: YSM_Wrappers.c,v 1.69 2005/11/14 01:58:28 rad2k Exp $	*/
/*
-======================== ysmICQ client ============================-
		Having fun with a boring Protocol
-======================== YSM_Wrappers.c ===========================-

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
__RCSID("$Id: YSM_Wrappers.c,v 1.69 2005/11/14 01:58:28 rad2k Exp $");

#include "YSM_Lists.h"
#include "YSM_Wrappers.h"
#include "YSM_ToolBox.h"
#include "YSM_Win32.h"
#include "YSM_Slaves.h"
#include "YSM_Main.h"

#if defined(YSM_WITH_THREADS) && defined(WIN32)
extern	HANDLE	_thHandle[THREADS_COUNT];
extern	DWORD	_thid[THREADS_COUNT];
extern	HANDLE g_hSuicideEvent, g_hThreadDiedEvent;
#endif


char	YSM_Reconnecting = FALSE;

#ifdef YSM_TRACE_MEMLEAK
/* Debugging purposes */
int unfreed_blocks = 0;
#endif

void
YSM_Reconnect ( void )
{
YSM_SLAVE	*Firstnode = plist_firstSLAVE;
u_int32_t	z = 0;

/* Starting time is 10 seconds */
u_int32_t	x = 10;
int32_t		y = 0;

	g_sinfo.flags &= ~FL_LOGGEDIN;
	YSM_Reconnecting = TRUE;

	/* put them all offline */
	g_sinfo.onlineslaves = 0;

	/* Reset slaves status */
	for ( z = 0; z < List_amountSLAVE; z++ ) {

		if(!Firstnode) break;
		Firstnode->status = STATUS_OFFLINE;
		Firstnode = Firstnode->next;
	}


	while (y <= 0) {

#if defined(WIN32) || defined(OS2)
		YSM_WindowAlert();
#endif

		PRINTF(VERBOSE_BASE,
		"\n" RED "Disconnection detected. "
		"Reconnecting in %d seconds.\n" NORMAL, x );

		YSM_Thread_Sleep( x, 0 );

		close( YSM_USER.network.rSocket );

		if ((y = YSM_SignIn()) < 0) {
 			if (x < 300) x += 5;
			else {
				PRINTF(VERBOSE_BASE,
				"\n" RED "Maximum reconnects reached. "
				"Network must be down..\n" NORMAL);
				YSM_Error(ERROR_NETWORK, __FILE__, __LINE__, 0);
			}
		}
	}
	
}


int
YSM_READ( int32_t	sock,
	void		*buf,
	int		read_len,
	char		priority )
{
int32_t	r = 0, x = 0, rlen = 0;

	rlen = read_len;

	while(read_len > r && ((YSM_Reconnecting && priority) 
	|| !YSM_Reconnecting)) {

		x = SOCK_READ(sock, (char *)buf+r, rlen);
	
		if(x >= 0) {
			if (!x) break;
			r += x;
			rlen -= x;
		} else {
			return -1;
		}
	}


	/* Only negotiation functions take precedence */
	/* since we have multiple threads, we stop them this way */
	if (!priority && YSM_Reconnecting) {
		/* make the thread sleep a little dont consume 100% ! */
		YSM_Thread_Sleep(0, 100);
		return -1;
	}

	return r;
}


int
YSM_WRITE( int32_t sock, void *data, int32_t data_len )
{
int32_t	r = 0, y = 0;

	do {
		y = SOCK_WRITE(sock, data, data_len);
		if (y) r += y;
	} while (y >= 0 && r != data_len);

	if (y < 0) YSM_Reconnect();

	return r;
}

int32_t
YSM_WRITE_DC(YSM_SLAVE *victim, int32_t sock, void *data, int32_t data_len)
{
	/* checks on DC, open a DC if it doesn't exist! */
	if (victim->d_con.flags & DC_CONNECTED)
		return SOCK_WRITE(sock , data, data_len);
	else {
		PRINTF( VERBOSE_BASE,
			"No open DC session found with slave.\n"
			"Use the 'opendc' command to open a DC session.\n ");
	
		return -1;	
	}
}

/* reads a single line until \r\n is met.
 * returns the read amount of bytes in a size_t (without \r\n)
 */

size_t
YSM_READ_LN(int32_t sock, int8_t *obuf, size_t maxsize)
{
int8_t	ch = 0;
size_t	bread = 0;

	while (ch != '\n' && bread < maxsize) {
		if (SOCK_READ(sock, &ch, 1) <= 0)
			break;

		obuf[bread++] = ch;
	}

	/* we make sure we dont return \r\n in our read size */
	if (bread >= 2 && obuf[bread-2] == '\r' && obuf[bread-1] == '\n')
		return bread - 2;

	/* we might have had a single \n */
	if (bread >= 1 && obuf[bread-1] == '\n')
		return bread - 1;

	return bread;
}

void *
YSM_Malloc( size_t size, char *file, int line)
{
char	*memory;

	memory = malloc(size);

	if( (size <= 0) || (memory == NULL)) 
	{
		PRINTF(VERBOSE_BASE,
		"\rYSM_Malloc: Error in block. Probably size error.\n");

		PRINTF(VERBOSE_BASE,
		"Inform the author! File: %s Line: %d\n", file, line);

		YSM_Exit( -1, 1 );
	}

#ifdef YSM_TRACE_MEMLEAK
	unfreed_blocks++;
#endif

	return memory;
}

void *
YSM_Calloc(size_t nmemb, size_t size, char *file, int line)
{
	char *p;

	p = YSM_Malloc(nmemb * size, file, line);
	memset(p, 0, nmemb * size);
	return (p);
}

void
YSM_Free(void *what, char *file, int line)
{

	if( what == NULL )
	{
		PRINTF(VERBOSE_BASE,
		"\rYSM_Free: NULL Block . Probably double free?.\n");

		PRINTF(VERBOSE_BASE,
		"Inform the author! File: %s Line: %d\n", file, line);

		YSM_Exit( -1, 1 );	
	}

#ifdef YSM_TRACE_MEMLEAK
	unfreed_blocks--;
#endif

	free(what);
	what = NULL;
}


int
YSM_IsInvalidPtr(void *ptr)
{
	if (ptr == NULL)
		return TRUE;

	return FALSE;
}


/*	This is the function that should be called instead of directly	*/
/*	using the exit() syscall. It does some garbage collection and	*/
/*	allows the addition of pre-leaving procedures.			*/

void
YSM_Exit( int32_t status, int8_t ask )
{
#ifdef OS2
	os2_deinit();	/* restore window title and kill message queue */
#endif
#ifdef WIN32
	HWND	YSM_HWND = NULL;
	int	x = 0;
	HANDLE	hInputRead;
#endif
	int8_t	fl_fishgui = 0;

	if (g_sinfo.blgroupsid != NULL) {
		YSM_Free( g_sinfo.blgroupsid, __FILE__, __LINE__ );
		g_sinfo.blgroupsid = NULL;
	}

	if (g_sinfo.blusersid != NULL) {
		YSM_Free( g_sinfo.blusersid, __FILE__, __LINE__ );
		g_sinfo.blusersid = NULL;
	}

	/* Logging off event */
	YSM_Event( EVENT_LOGOFF, 
		YSM_USER.Uin,
		YSM_USER.info.NickName,
		0,
		NULL,
		0 );

#if !defined(WIN32) && !defined(__OpenBSD__) && !defined(OS2)
	/* Clear the terminal title */
	printf( "%c]0;%c", '\033', '\007' );
#endif

	/* close the network socket */
	close(YSM_USER.network.rSocket);

	/* close the FishGUI socket */
	if (YSM_USER.fishgui.socket > 0)
	{
		close(YSM_USER.fishgui.socket);
		YSM_USER.fishgui.socket = -1;
		fl_fishgui = 1;
	}


#if defined(WIN32) && defined(YSM_WITH_THREADS)
	/* close STDIN to cause any reads() to fail */
	hInputRead = GetStdHandle(STD_INPUT_HANDLE);
	if (hInputRead != INVALID_HANDLE_VALUE) {
		CloseHandle(hInputRead);
	}

	/* Start a mass-suicide for all Threads */
	SetEvent( g_hSuicideEvent );

	YSM_HWND = getConsoleWindow();
	UnregisterHotKey(
			YSM_HWND,
			0x1337);

	for ( x = 0; x < THREADS_COUNT; x++ ) {
		WaitForSingleObject( g_hThreadDiedEvent, 500 );
		ResetEvent( g_hThreadDiedEvent );
	}

#if 0
	/* we've been told killing ourselves is a bad idea..
	 * commenting this out.
	 */

	/* Kill those who fear my evil */
	for ( x = 0; x < THREADS_COUNT; x++ ) {
		/* is it still alive? */
		TerminateThread(_thHandle[x], 0x0);
	}
#endif
	/* In Win32 most people run YSM directly without opening */
	/* a new console. Hence ask for any key so it wont close */
	/* without revealing the output to the user.		 */
	if (ask && !fl_fishgui) {
		PRINTF( VERBOSE_BASE, 
			"\nPress any key to close the application.");
		_getch();
	}
#endif

	

	/* Free all those nodes! */
	List_freelist();

#ifdef WIN32
	ExitProcess(status);
#elif OS2
	exit(0);
#else
	/* the following iteration through child processes could be
	 * done in a tidy manner by waiting for the SIGCHLD signal
	 * in order to call waitpid. meanwhile..this does the job. 
	 */
	while (waitpid(-1, NULL, WNOHANG) > 0);

	/* now exit without zombies */
	exit(status);
#endif
}


FILE *
YSM_fopen(const char *path, const char *mode)
{
FILE		*fd = NULL;
u_int32_t	i = 0;
pFileMap	node = plist_firstFILEMAP;

	/* call the real fopen */
	fd = fopen(path, mode);
	if (fd == NULL) 
		return fd;

	/* do we already have an entry for this fd? weird..could happen */
	for (i = 1; node != NULL; i++) {
		if (node->fd == fd) {
			/* yep, does exist. */
			return fd;
		}
		node = node->next;
	}

	/* no entry exists, create one */
	node = YSM_Calloc(1, sizeof(FileMap), __FILE__, __LINE__);
	node->fd = fd;	

	/* get the file size */
	if (fseek(node->fd, 0L, SEEK_END) != 0) {
		fclose(fd);
		YSM_Free(node, __FILE__, __LINE__);
		node = NULL;
		return NULL;
	}

	node->size = ftell(node->fd);
	if (node->size < 0) {
		fclose(fd);
		YSM_Free(node, __FILE__, __LINE__);
		node = NULL;
		return NULL;
	}		

	/* rewind the file descriptor */
	if (fseek(node->fd, 0L, SEEK_SET) != 0) {
		fclose(fd);
		YSM_Free(node, __FILE__, __LINE__);
		node = NULL;
		return NULL;
	}

	/* load the file in memory */
	if (node->size > 0) {
		node->data = YSM_Malloc(node->size+1, __FILE__, __LINE__);
		if (node->data == NULL) {
			/* ERR_MEMORY */
			fclose(fd);
			YSM_Free(node, __FILE__, __LINE__);
			node = NULL;
			return NULL;
		}

		/* update the size with the new real read size */
		node->size = fread(node->data, 1, node->size, node->fd);
		if (node->size < 0) {
			/* ERRO_WOOPS */
			fclose(fd);
			YSM_Free(node->data, __FILE__, __LINE__);
			YSM_Free(node, __FILE__, __LINE__);
			node = NULL;
			return NULL;
		}
	}

	/* rewind the file descriptor again */
	if (fseek(node->fd, 0L, SEEK_SET) != 0) {
		fclose(fd);
		if (node->data != NULL)
			YSM_Free(node->data, __FILE__, __LINE__);

		YSM_Free(node, __FILE__, __LINE__);
		node = NULL;
		return NULL;
	}

	/* add the filemap to the list */
	List_addFILEMAP(node);

	return fd;
}

int
YSM_fclose(FILE *stream)
{
pFileMap	node = NULL;
u_int32_t	i = 0;

	/* do we have an entry for this fd? */
	for (i = 1; node != NULL; i++) {
		if (node->fd == stream) {
			List_delFILEMAP(node);
		}
		node = node->next;
	}

	return fclose(stream);
}
