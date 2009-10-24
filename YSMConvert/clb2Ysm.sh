#!/bin/bash
# $Id: clb2Ysm.sh,v 1.4 2002/02/07 05:53:50 ccas Exp $
###########################################################
# Clb to Ysm convertion shellscript.
# aweil [aweil@mail.ru]
############################################################
# Use:  ./clb2Ysm.sh file.clb > dumpfile
# output goes to stdout. 
############################################################

LISTA=`cat "$*"`
C=
I=0
MAXL=`echo "$LISTA" | wc -l`

while [ "$I" != "$MAXL" ]; do
#Get the line
    I=`echo $C | wc -c`
    L=`echo "$LISTA" | head -n $I | tail -n 1`
#    echo \[Line=$I\] $L
    C=$C+

#line proccessing..
    G=`echo "$L" | sed 's/\;.*//g'`
    H=`echo "$L" | sed "s/$G\;//g" | sed 's/\;.*//g'`
    J=`echo "$L" | sed "s/$G\;//g;s/$H\;//g" | sed 's/\;.*//g;s/ //g;s/[^a-z,A-Z]//g'`
#    echo Id=$H 
#    echo Nick=$J
#    echo Group=$G
    echo $J:$H
done
