#  +----------------------------------------------------------------------+
#  | PHP version 4.0                                                      |
#  +----------------------------------------------------------------------+
#  | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
#  +----------------------------------------------------------------------+
#  | This source file is subject to version 2.02 of the PHP license,      |
#  | that is bundled with this package in the file LICENSE, and is        |
#  | available at through the world-wide-web at                           |
#  | http://www.php.net/license/2_02.txt.                                 |
#  | If you did not receive a copy of the PHP license and are unable to   |
#  | obtain it through the world-wide-web, please send a note to          |
#  | license@php.net so we can mail you a copy immediately.               |
#  +----------------------------------------------------------------------+
#  | Authors: Sascha Schumann <sascha@schumann.cx>                        |
#  +----------------------------------------------------------------------+
#
# $Id: build.mk,v 1.7 2001/04/03 21:16:07 wsanchez Exp $ 
#
#
# Makefile to generate build tools
#

SUBDIRS = Zend TSRM

STAMP = buildmk.stamp

ALWAYS = generated_lists

all: $(STAMP) $(ALWAYS)
	@$(MAKE) AMFLAGS=$(AMFLAGS) -s -f build/build2.mk

generated_lists:
	@echo makefile_am_files = Zend/Makefile.am \
		TSRM/Makefile.am > $@
	@echo config_h_files = Zend/acconfig.h TSRM/acconfig.h >> $@
	@echo config_m4_files = Zend/Zend.m4 TSRM/tsrm.m4 \
		Zend/acinclude.m4 ext/*/config.m4 sapi/*/config.m4 >> $@

$(STAMP): build/buildcheck.sh
	@build/buildcheck.sh && touch $(STAMP)

snapshot:
	distname='$(DISTNAME)'; \
	if test -z "$$distname"; then \
		distname='php4-snapshot'; \
	fi; \
	myname=`basename \`pwd\`` ; \
	cd .. && cp -rp $$myname $$distname; \
	cd $$distname; \
	rm -f $(SUBDIRS) 2>/dev/null || true; \
	for i in $(SUBDIRS); do \
		test -d $$i || (test -d ../$$i && cp -rp ../$$i $$i); \
	done; \
	find . -type l -exec rm {} \; ; \
	$(MAKE) AMFLAGS=--copy -f build/build.mk; \
	cd ..; \
	tar cf $$distname.tar $$distname; \
	rm -rf $$distname $$distname.tar.*; \
	bzip2 -9 $$distname.tar; \
	md5sum $$distname.tar.bz2; \
	sync; sleep 2; \
	md5sum $$distname.tar.bz2; \
	bzip2 -t $$distname.tar.bz2

cvsclean:
	@for i in `find . -name .cvsignore`; do \
		(cd `dirname $$i` 2>/dev/null && rm -rf `cat .cvsignore` *.o *.a || true); \
	done
	@rm -f $(SUBDIRS) 2>/dev/null || true

.PHONY: $(ALWAYS) snapshot cvsclean
