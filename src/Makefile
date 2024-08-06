CXXFLAGS=-std=c++20 -stdlib=libc++ -fexperimental-library -g
CXX=clang++-18

chibicc: 
	$(CXX) $(CXXFLAGS) -o chibicc main.cc $(LDFLAGS)

test: chibicc
	./test.sh

clean:
	rm -f chibicc *.o *~ tmp*

.PHONY: test clean
