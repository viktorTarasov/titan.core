##############################################################################
# Copyright (c) 2000-2016 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Baji, Laszlo
#   Balasko, Jeno
#   Baranyi, Botond
#   Forstner, Matyas
#   Kovacs, Ferenc
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Janos Zoltan – initial implementation
#
##############################################################################
TOPDIR := ..
include $(TOPDIR)/Makefile.regression

CODIRS := dynamicTemplate styleGuide topLevelPdu \
	circularImport circularImport2 typeInstantiation \
	compareImported compoundif \
	centralstorage mfgen-tpd mfgen-xsd \
	openType optionalAssignCompare portConstructor \
	isbound namedActualParameters assignmentNotation \
	attribQualif HT48786 selectCase openTypeNames \
	defaultParamUsageBeforeDecl deterministic readFromFile

all dep clean distclean:
	for dir in $(CODIRS); do $(MAKE) -C $$dir $@ || exit; done
