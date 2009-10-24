#/bin/sh
# $Id: catlicquser.sh,v 1.1 2002/02/07 05:57:57 ccas Exp $

OUTFILE=$2
FILE=$1
UIN=`echo $1 | sed 's/.*\///g;s/\..*//g'` 
ALIAS=`grep -i alias $FILE | sed 's/.* //g'`
echo $ALIAS:$UIN >> $2
