#!/usr/bin/perl
#
# $Id: centericq2ysm.pl,v 1.2 2002/02/07 05:53:50 ccas Exp $
#
# centericq2ysm.pl
# CenterICQ to You Sick Me! contact list converter
# By SToRM NiGHT <storm@malditainternet.com>
#
# Just paste that output in your YSM Config file, below the [SLAVES] line.
#

$|=0;
use File::stat;
$userHome=$ENV{'HOME'};
$centericqdir="$userHome/.centericq";

if(-e $centericqdir) {
} else {
	die ("FATAL ERROR: $centericqdir not found! can't continue!.\n");
}

@archivos=`ls $centericqdir`;
$i=0;
for($i=0;$i<=$#archivos;$i++) {
        chomp($archivos[$i]);
	if (-d "$centericqdir/$archivos[$i]") {
		if ($archivos[$i]=~/[1234567890]/) {
		 	unless (open(LOGFILE,"<$centericqdir/$archivos[$i]/info")) {
				die ("\n\nError: Can't open 'info' file for UIN: [$archivos[$i]] !\n\n");
			} 
			$nickName=<LOGFILE>;
			chomp($nickName);
			if ($nickName eq "") {
				$nickName="Unknown";
			}
			$nickName =~ s/\s+//g;
			if ($archivos[$i] ne "0") {
				print "$nickName:$archivos[$i]\n";
			}
	        } else {
			print ("Error: [$archivos[$i]] is not a valid UIN (may be a MSG/Yahoo contact?)\n");
		}
	}
}
