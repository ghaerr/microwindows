#!/usr/bin/perl
# elkspatch.pl
#
# Perl script to hack AllocReq macros for ELKS bcc compiler
#
# This script rewrites the AllocReq macro which uses ANSI C's '##'
# token pasting operator to an input compatible with the bcc compiler
#
# This is only required if desiring to build client/server Nano-X for ELKS
# To run:
#	mv client.c client.dist
#	./elkspatch.pl < client.dist > client.c
#

while (<>) {
	if (/^(.*)AllocReq\(([A-Za-z]+)\)(.*)$/) {
		print("#if ELKS /* KLUDGE */\n");
		printf("%s((nx%sReq *)nxAllocReq(GrNum%s,(long)sizeof(nx%sReq), 0L))%s /* KLUDGE */\n",
			$1, $2, $2, $2, $3);
		print("#else /* KLUDGE */\n");
		print;
		print("#endif /* KLUDGE */\n");
	} elsif (/^(.*)AllocReqExtra\(([A-Za-z]+) *, *(.*)\)(;)$/) {
		print("#if ELKS /* KLUDGE */\n");
		printf("%s((nx%sReq *)nxAllocReq(GrNum%s,(long)sizeof(nx%sReq), (long)%s))%s /* KLUDGE */\n",
			$1, $2, $2, $2, $3, $4);
		print("#else /* KLUDGE */\n");
		print;
		print("#endif /* KLUDGE */\n");
	} else {
		print;
	}
}
