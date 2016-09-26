##############################################################################
# Copyright (c) 2000-2016 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Szabo, Bence Janos
#
##############################################################################
TOPDIR := ../..
include $(TOPDIR)/Makefile.regression

ifdef LCOV
COVERAGE_FLAG := -C
endif

MFGEN := $(TTCN3_DIR)/bin/ttcn3_makefilegen
MFGEN_FLAGS := -sf $(RT2_FLAG) $(SPLIT_FLAG)

# ${MAKEPROG} has the same content as the built-in ${MAKE},
# except the special handling of ${MAKE} does not apply.
# If we used ${MAKE} in the rules below, 'make -n' would succeed once,
# then fail on every subsequent invocation until a 'make clean' is done. 
MAKEPROG := ${MAKE}

all: normal centralstorage centralstorage_with_normal recursive_tpd improved_linking


NORMAL_NORMAL_FILES := MainNormal.ttcn

NORMAL_EXT_FILES := MainExtNormal.ttcn

NORMAL_FILES := Main.ttcn Definitions.xsd $(NORMAL_NORMAL_FILES)

EXT_FILES := MainExt.ttcn DefinitionsExt.xsd $(NORMAL_EXT_FILES)

EXT_FILES2 := MainExtSingle.ttcn DefinitionsExt.xsd $(NORMAL_EXT_FILES)

EXT_NORMAL_FILES := $(addprefix ../normal/, $(NORMAL_FILES))

EXT_NORMAL_NORMAL_FILES := $(addprefix ../normal/, $(NORMAL_NORMAL_FILES))


normal:
	#Normal tests with various switches
	cd normal && $(MFGEN) $(MFGEN_FLAGS) $(NORMAL_FILES) && $(MAKEPROG) \
	&& ./Main normal.cfg && make clean && make port && make clean && make check \
	&& make clean

	cd normal && $(MFGEN) $(MFGEN_FLAGS) -p $(NORMAL_FILES) && $(MAKEPROG) \
	&& ./Main normal.cfg && make clean && make port && make clean && make check \
	&& make clean

	cd normal && $(MFGEN) $(MFGEN_FLAGS) -g $(NORMAL_FILES) && $(MAKEPROG) \
	&& ./Main normal.cfg && make clean && make port && make clean && make check \
	&& make clean

	cd normal && $(MFGEN) $(MFGEN_FLAGS) -U type $(NORMAL_FILES) && $(MAKEPROG) \
	&& ./Main normal.cfg && make clean && make port && make clean && make check \
	&& make clean

	cd normal && $(MFGEN) $(MFGEN_FLAGS) -U 5 $(NORMAL_FILES) && $(MAKEPROG) \
	&& ./Main normal.cfg && make clean && make port && make clean && make check \
	&& make clean

	cd normal && $(MFGEN) $(MFGEN_FLAGS) -l $(NORMAL_FILES) && $(MAKEPROG) \
	&& ./Main normal.cfg && make clean && make port && make clean && make check \
	&& make clean && make shared_objects && make clean

	cd normal && $(MFGEN) $(MFGEN_FLAGS) -L $(NORMAL_FILES) && $(MAKEPROG) \
	&& make clean && make port && make clean && make check \
	&& make clean

	cd normal && $(MFGEN) $(MFGEN_FLAGS) -pg $(NORMAL_FILES) && $(MAKEPROG) \
	&& ./Main normal.cfg && make clean && make port && make clean && make check \
	&& make clean

	cd normal && $(MFGEN) $(MFGEN_FLAGS) -pl -U type $(NORMAL_FILES) && $(MAKEPROG) \
	&& ./Main normal.cfg && make clean && make port && make clean && make check \
	&& make clean && make shared_objects && make clean

	cd normal && $(MFGEN) $(MFGEN_FLAGS) -pgl $(NORMAL_FILES) && $(MAKEPROG) \
	&& ./Main normal.cfg && make clean && make port && make clean && make check \
	&& make clean && make shared_objects && make clean

	cd normal && $(MFGEN) $(MFGEN_FLAGS) -pgl -U type $(NORMAL_FILES) && $(MAKEPROG) \
	&& ./Main normal.cfg && make clean && make port && make clean && make check \
	&& make clean && make shared_objects && make clean

	cd normal && rm -rf Makefile


centralstorage:
	#Centralstorage tests with various switches
	#First build normal
	cd normal && $(MFGEN) $(MFGEN_FLAGS) $(NORMAL_FILES) && $(MAKEPROG)

	cd centralstorage && $(MFGEN) $(MFGEN_FLAGS) -c $(EXT_FILES) $(EXT_NORMAL_FILES) \
	&& $(MAKEPROG) && ./MainExt config.cfg \
	&& make clean && make port && make clean && make check && make clean

	cd centralstorage && $(MFGEN) $(MFGEN_FLAGS) -cp $(EXT_FILES) $(EXT_NORMAL_FILES) \
	&& $(MAKEPROG) && ./MainExt config.cfg \
	&& make clean && make port && make clean && make check && make clean

	cd centralstorage && $(MFGEN) $(MFGEN_FLAGS) -cg $(EXT_FILES) $(EXT_NORMAL_FILES) \
	&& $(MAKEPROG) && ./MainExt config.cfg \
	&& make clean && make port && make clean && make check && make clean

	cd centralstorage && $(MFGEN) $(MFGEN_FLAGS) -cl $(EXT_FILES) $(EXT_NORMAL_FILES) \
	&& $(MAKEPROG) && ./MainExt config.cfg \
	&& make clean && make port && make clean && make check && make clean \
	&& make shared_objects && make clean

	cd centralstorage && $(MFGEN) $(MFGEN_FLAGS) -cL $(EXT_FILES) $(EXT_NORMAL_FILES) \
	&& $(MAKEPROG) && make clean && make port && make clean && make check && make clean

	cd centralstorage && $(MFGEN) $(MFGEN_FLAGS) -cpg $(EXT_FILES) $(EXT_NORMAL_FILES) \
	&& $(MAKEPROG) && ./MainExt config.cfg \
	&& make clean && make port && make clean && make check && make clean

	cd centralstorage && $(MFGEN) $(MFGEN_FLAGS) -cpgl $(EXT_FILES) $(EXT_NORMAL_FILES) \
	&& $(MAKEPROG) && ./MainExt config.cfg \
	&& make clean && make port && make clean && make check && make clean \
	&& make shared_objects && make clean

	# For the tests with -U type the 'base' should be compiled with -U type
	cd normal && make clean && $(MFGEN) $(MFGEN_FLAGS) -U type $(NORMAL_FILES) && $(MAKEPROG)

	cd centralstorage && $(MFGEN) $(MFGEN_FLAGS) -c -U type $(EXT_FILES) $(EXT_NORMAL_FILES) \
	&& $(MAKEPROG) && ./MainExt config.cfg \
	&& make clean && make port && make clean && make check && make clean

	cd centralstorage && $(MFGEN) $(MFGEN_FLAGS) -cpl -U type $(EXT_FILES) $(EXT_NORMAL_FILES) \
	&& $(MAKEPROG) && ./MainExt config.cfg \
	&& make clean && make port && make clean && make check && make clean \
	&& make shared_objects && make clean

	cd centralstorage && $(MFGEN) $(MFGEN_FLAGS) -cpgl -U type $(EXT_FILES) $(EXT_NORMAL_FILES) \
	&& $(MAKEPROG) && ./MainExt config.cfg \
	&& make clean && make port && make clean && make check && make clean \
	&& make shared_objects && make clean

	cd normal && make clean && $(MFGEN) $(MFGEN_FLAGS) -U 5 $(NORMAL_FILES) && $(MAKEPROG)

	cd centralstorage && $(MFGEN) $(MFGEN_FLAGS) -c -U 5 $(EXT_FILES) $(EXT_NORMAL_FILES) \
	&& $(MAKEPROG) && ./MainExt config.cfg \
	&& make clean && make port && make clean && make check && make clean

	cd normal && make clean && rm -rf *.so Makefile && cd ../centralstorage && rm -rf Makefile *.cc *.hh # Makefilegen error???


centralstorage_with_normal:
	# Tests that the makefilegen is good if the centralstorage has XSD and the
	# external project does not have XSD files.
	cd normal && $(MFGEN) $(MFGEN_FLAGS) $(NORMAL_FILES) && $(MAKEPROG)

	cd centralstorage && $(MFGEN) $(MFGEN_FLAGS) -c $(NORMAL_EXT_FILES) $(EXT_NORMAL_FILES) \
	&& $(MAKEPROG) && ./MainExtNormal configNormal.cfg \
	&& make clean && make port && make clean && make check && make clean

	cd centralstorage && $(MFGEN) $(MFGEN_FLAGS) -cpg $(NORMAL_EXT_FILES) $(EXT_NORMAL_FILES) \
	&& $(MAKEPROG) && ./MainExtNormal configNormal.cfg \
	&& make clean && make port && make clean && make check && make clean

	cd normal && make clean

	cd normal && $(MFGEN) $(MFGEN_FLAGS) $(NORMAL_NORMAL_FILES) && $(MAKEPROG)

	cd centralstorage && $(MFGEN) $(MFGEN_FLAGS) -c $(EXT_NORMAL_NORMAL_FILES) $(EXT_FILES2) \
	&& $(MAKEPROG) && ./MainNormal configNormal2.cfg \
	&& make clean && make port && make clean && make check && make clean

	cd centralstorage && $(MFGEN) $(MFGEN_FLAGS) -cpg $(EXT_NORMAL_NORMAL_FILES) $(EXT_FILES2) \
	&& $(MAKEPROG) && ./MainNormal configNormal2.cfg \
	&& make clean && make port && make clean && make check && make clean

	cd normal && make clean && rm -rf Makefile && cd ../centralstorage && rm -rf Makefile *.cc *.hh


recursive_tpd:
	# Makefile generation from tpd tests
	$(MFGEN) $(MFGEN_FLAGS) -t normal.tpd && cd bin && $(MAKEPROG) \
	&& ./bin ../normal/normal.cfg && make clean && make port && make check && cd .. \
	&& rm -rf bin

	# Recursive makefile generation
	$(MFGEN) $(MFGEN_FLAGS) -FrcW -t top.tpd && cd top_bin && $(MAKEPROG) \
	&& ./bin ../config.cfg && make clean && make port && make check && cd .. \
	&& rm -rf top_bin centralstorage_bin normal_bin

	$(MFGEN) $(MFGEN_FLAGS) -FrcWp -t top.tpd && cd top_bin && $(MAKEPROG) \
	&& ./bin ../config.cfg && make clean && make port && make check && cd .. \
	&& rm -rf top_bin centralstorage_bin normal_bin

	$(MFGEN) $(MFGEN_FLAGS) -FrcWg -t top.tpd && cd top_bin && $(MAKEPROG) \
	&& ./bin ../config.cfg && make clean && make port && make check && cd .. \
	&& rm -rf top_bin centralstorage_bin normal_bin

	$(MFGEN) $(MFGEN_FLAGS) -FrcWpg -t top.tpd && cd top_bin && $(MAKEPROG) \
	&& ./bin ../config.cfg && make clean && make port && make check && cd .. \
	&& rm -rf top_bin centralstorage_bin normal_bin

	
improved_linking:
	#Improved linking
	$(MFGEN) $(MFGEN_FLAGS) -FrWZ -t top.tpd -b Library && cd top_bin && $(MAKEPROG) \
	&& ./bin ../config.cfg && make clean && make port && make check && cd .. \
	&& rm -rf top_bin centralstorage_bin normal_bin

	$(MFGEN) $(MFGEN_FLAGS) -FrWZgp -t top.tpd -b Library && cd top_bin && $(MAKEPROG) \
	&& ./bin ../config.cfg && make clean && make port && make check && cd .. \
	&& rm -rf top_bin centralstorage_bin normal_bin

	$(MFGEN) $(MFGEN_FLAGS) -FrWZH -t top.tpd -b Library && cd top_bin && $(MAKEPROG) \
	&& ./bin ../config.cfg && make clean && make port && make check && cd .. \
	&& rm -rf top_bin centralstorage_bin normal_bin

	$(MFGEN) $(MFGEN_FLAGS) -FrWZHc -t top.tpd -b Library && cd top_bin && $(MAKEPROG) \
	&& ./bin ../config.cfg && make clean && make port && make check && cd .. \
	&& rm -rf top_bin centralstorage_bin normal_bin

	$(MFGEN) $(MFGEN_FLAGS) -FrWZHcgp -t top.tpd -b Library && cd top_bin && $(MAKEPROG) \
	&& ./bin ../config.cfg && make clean && make port && make check && cd .. \
	&& rm -rf top_bin centralstorage_bin normal_bin

	$(MFGEN) $(MFGEN_FLAGS) -FrWZHclgp -t top.tpd -b Library && cd top_bin && $(MAKEPROG) \
	&& ./bin ../config.cfg && make clean && make port && make check && cd .. \
	&& rm -rf top_bin centralstorage_bin normal_bin


.PHONY: all clean distclean run normal centralstorage centralstorage_with_normal recursive_tpd improved_linking

