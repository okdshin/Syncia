CXX = g++ -std=gnu++0x
CXXFLAGS = -Wall -g -D CUISYNCIA_UNIT_TEST -D BBOOST_ASIO_ENABLE_HANDLER_TRACKING
INCLUDES = -I./include/
LIBS = -lboost_filesystem -lboost_iostreams -lboost_serialization -lcrypto -lboost_thread -ldl -lpthread -lboost_system -lboost_date_time
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
