#!/bin/sh
# $Id: licq2Ysm.sh,v 1.1 2002/02/07 05:57:57 ccas Exp $

USERSDIR=~/.licq/users
OUTPUTFILE=APPEND-IT-TO-YSM-Config

find $USERSDIR -type f -exec grep -iq alias {} \; -exec ./catlicquser.sh {} $OUTPUTFILE \;
