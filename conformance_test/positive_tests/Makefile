##############################################################################
# Copyright (c) 2000-2016 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Kirjak, Adrien
#
# not ready
##############################################################################
#TOPDIR := ../..
#include $(TOPDIR)/Makefile.regression

#ifdef LCOV
#COVERAGE_FLAG := -C
#endif

MAKE := make

MAKECLEAN := make clean

run:
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -f -t dummy.tpd && \
	cd bin && \
	$(MAKE) && \
	$(TTCN3_DIR)/bin/ttcn3_start dummy dummy.cfg && \
	cd ..

clean:
	cd bin && \
	$(MAKECLEAN) && \
	cd .. 

.PHONY: all clean run 



