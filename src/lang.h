/*	$Id: YSM_Lang.h,v 1.4 2004/10/24 23:52:28 rad2k Exp $	*/
/* 

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

Take any of these definitions out of the comments.
#define SPANISH
#define ENGLISH
#define RUSSIAN
#define SWEDISH
#define ITALIAN
#define GERMAN
#define BR_PORTUGUESE
#define FRENCH_ONE
#define FRENCH_TWO
#define CROATIAN
*/

#if defined (SPANISH)
#include "YSMLangs/spanish.h"
#elif defined (ENGLISH)
#include "YSMLangs/english.h"
#elif defined (RUSSIAN)
#include "YSMLangs/russian.h"
#elif defined (SWEDISH)
#include "YSMLangs/swedish.h"
#elif defined (ITALIAN)
#include "YSMLangs/italian.h"
#elif defined (GERMAN)
#include "YSMLangs/german.h"
#elif defined (BR_PORTUGUESE)
#include "YSMLangs/portuguesebr.h"
#elif defined (FRENCH_ONE)
#include "YSMLangs/french_1.h"
#elif defined (FRENCH_TWO)
#include "YSMLangs/french_2.h"
#elif defined (CROATIAN)
#include "YSMLangs/croatian.h"
#elif defined (CZ)
#include "YSMLangs/czech.h"
#else
#include "YSMLangs/english.h"
#endif
