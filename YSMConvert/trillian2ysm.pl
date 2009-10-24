#!/usr/bin/perl
#
# $Id: trillian2ysm.pl,v 1.2 2003/05/23 14:21:38 rad2k Exp $
#
# trillian2ysm.pl
# Trillian to You Sick Me! contact list converter
# By GasFed
#
# Just paste that output in your YSM Config file, below the [SLAVES] line.
#
# Usage: cat Buddies.xml |perl trillian2ysm.pl

while(<>) {
	chomp;
	if(/ICQ/) {
		($null,$line) = split (/\"/,$_);
		$line =~ s/%(..)/pack("c",hex($1))/ge;
		($null,$null,$icqnum,$name) = split (/:/,$line);
		$name =~ tr/áéíóúñÁÉÍÓÚÑ/aeiounAEIOUN/;
		$name =~ s/[^a-zA-Z0-9\-\_]//g;
		print "$name:$icqnum:\n";
		}
}
