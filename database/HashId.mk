CXX = g++ -std=gnu++0x
CXXFLAGS = -Wall -g -D HASHID_UNIT_TEST
INCLUDES = 
LIBS = -lcrypto -lboost_serialization -lboost_system
OBJS = HashId.o
PROGRAM = HashId.out

all:$(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(INCLUDES) $(LIBS) -o $(PROGRAM)

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBS) -c $<

.PHONY: clean
clean:
	rm -f *o $(PROGRAM)
