PYTHONPREFIX=/fs/clip-software/user-supported/python+cython/python-current
CYTHON=$(PYTHONPREFIX)/bin/cython
PYTHON=$(PYTHONPREFIX)/bin/python

%.c: %.pyx
	$(CYTHON) $< -o $@

%.o: %.cc
	g++ -O6 -g -fPIC -c $<

all: cstrmap.c strmap.cc rule.c sym.c cdat.c cintlist.c cfloatlist.c calignment.c csuf.c clex.c rulefactory.c cveb.c lcp.c precomputation.c csyncon.c csynctx.c cbitlist.c
	$(PYTHON) setup.py build
	g++ extract_syncon.cpp -lz -o extract_syncon

clean:
	rm -f cdat.c cstrmap.c sym.c rule.c cintlist.c cfloatlist.c calignment.c csuf.c clex.c rulefactory.c cveb.c lcp.c precomputation.c csyncon.c csynctx.c cbitlist.c *.so *.o *.cxx *~ *.pyc
	rm -rf build
