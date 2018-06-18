##############################################################################
# Copyright (c) 2018 Semihalf sp. z o.o.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v2.0
# which accompanies this distribution, and is available at
# https://www.eclipse.org/org/documents/epl-2.0/EPL-2.0.html
#
# Contributors:
#   Klimkowicz, Jacek
#
##############################################################################
TOPDIR := ../../..
include $(TOPDIR)/Makefile.regression

ifdef LCOV
COVERAGE_FLAG := -C
endif

# ${MAKEPROG} has the same content as the built-in ${MAKE},
# except the special handling of ${MAKE} does not apply.
# If we used ${MAKE} in the rules below, 'make -n' would succeed once,
# then fail on every subsequent invocation until a 'make clean' is done.

MAKEPROG := ${MAKE}

all: CheckTpd CheckTpdTrue CheckTpdFalse

BuildTpd:
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd

CheckTpd: BuildTpd
	if [ ! -f "./bin/Makefile" ] \
	|| [ `grep -c "COMPILER_FLAGS =.*-D" ./bin/Makefile` -ne 0 ]; \
	then echo "Makefilegen disableUserInformation test: Overall verdict: fail" && exit 1; fi

BuildTpdTrue:
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -b True

CheckTpdTrue: BuildTpdTrue
	if [ ! -f "./bin2/Makefile" ] \
	|| [ `grep -c "COMPILER_FLAGS =.*-D" ./bin2/Makefile` -ne 1 ]; \
	then echo "Makefilegen disableUserInformation test: Overall verdict: fail" && exit 1; fi

BuildTpdFalse:
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -b False

CheckTpdFalse: BuildTpdFalse
	if [ ! -f "./bin3/Makefile" ] \
	|| [ `grep -c "COMPILER_FLAGS =.*-D" ./bin3/Makefile` -ne 0 ]; \
	then echo "Makefilegen disableUserInformation test: Overall verdict: fail" && exit 1; fi



clean:
	-rm -rf bin bin2 bin3

distclean: clean
	-rm -f *.out

.PHONY: all clean distclean CheckTpd CheckTpdTrue CheckTpdFalse BuildTpd BuildTpdTrue BuildTpdFalse

