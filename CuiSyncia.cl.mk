CXX = clang++ -std=gnu++0x
CXXFLAGS = -Wall -g -D CUISYNCIA_UNIT_TEST -D BOOST_ASIO_ENABLE_HANDLER_TRACKING -D__STRICT_ANSI__
INCLUDES = 
LIBS = -lboost_filesystem -lboost_date_time -lboost_iostreams -lboost_serialization -lcrypto -lboost_thread -ldl -lpthread -lboost_system
OBJS = CuiSyncia.o
PROGRAM = CuiSyncia.out

all:$(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(INCLUDES) $(LIBS) -o $(PROGRAM)

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBS) -c $<

.PHONY: clean
clean:
	rm -f *o $(PROGRAM)
