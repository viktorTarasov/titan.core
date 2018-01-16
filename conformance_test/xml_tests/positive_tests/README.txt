// README to positive xml conformance tests

How to run:

A. (Makefile)
	1. Build and run tests using Makefile in positive tests folder:
		make
	2. Clean everything in bin folder:
		make clean


B. (Without the Makefile) 
	1. Generate a Makefile from XML_tests.tpd in positive_tests folder:
		ttcn3_makefilegen -f -t XML_tests.tpd
	2. Compile Makefile in bin folder:
		make
	3. Run tests in bin folder:
		ttcn3_start XML_tests ../XML_tests.cfg
	4. Clean everything in bin folder:
		make clean
