CXX = g++ -std=gnu++0x
CXXFLAGS = -Wall -g -D FILEKEYHASHREMOVER_UNIT_TEST
INCLUDES = 
LIBS =
OBJS = FileKeyHashRemover.o
PROGRAM = FileKeyHashRemover.out

all:$(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(INCLUDES) $(LIBS) -o $(PROGRAM)

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBS) -c $<

.PHONY: clean
clean:
	rm -f *o $(PROGRAM)
