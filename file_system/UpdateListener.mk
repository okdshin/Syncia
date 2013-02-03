CXX = g++ -std=gnu++0x
CXXFLAGS = -Wall -g -D UPDATELISTENER_UNIT_TEST
INCLUDES = 
LIBS = -I../includes/
OBJS = UpdateListener.o
PROGRAM = UpdateListener.out

all:$(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(INCLUDES) $(LIBS) -o $(PROGRAM)

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBS) -c $<

.PHONY: clean
clean:
	rm -f *o $(PROGRAM)
