# the command is like this
# g++ -g -o main main.cpp -I. -I$SYSTEMC/include -I$SCML/include -I$SYSTEMC/src/tlm_utils -L$SYSTEMC/lib-linux64 -L$SCML/lib-linux64 -lsystemc -lscml -lm -frtti

CXX=g++
CXXFLAGS=-g -I. -I$(SYSTEMC)/include -I$(SCML)/include -I$(SYSTEMC)/src/tlm_utils
LDFLAGS=-L$(SYSTEMC)/lib-linux64 -L$(SCML)/lib-linux64
LDLIBS=-lsystemc -lscml -lm -frtti

main: main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f main