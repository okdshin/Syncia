CXX = g++ -std=gnu++0x
CXXFLAGS = -Wall -g -D LINKDB_UNIT_TEST
INCLUDES = 
LIBS = -lboost_system -lboost_thread -lpthread
OBJS = LinkDb.o
PROGRAM = LinkDb.out

all:$(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(INCLUDES) $(LIBS) -o $(PROGRAM)

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBS) -c $<

.PHONY: clean
clean:
	rm -f *o $(PROGRAM)
