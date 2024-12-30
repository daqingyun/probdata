CXX = g++
CPPFLAGS = -I../../lib/iperf3 -I../../lib/tpg -I../../lib/udt
CXXFLAGS = -Wall -static
LDFLAGS = -lpthread
LIBS = ../../lib/iperf3/libiperf.a ../../lib/tpg/libtpg.a ../../lib/udt/libudt.a

FASTPROF_SRCS = \
    fastprof.cpp \
    fastprof_log.cpp \
    fastprof_error.cpp \
    fastprof_tcp.cpp \
    fastprof_udt.cpp \
    fastprof_main.cpp

OBJS = $(FASTPROF_SRCS:.cpp=.o)

TARGET = fastprof

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
