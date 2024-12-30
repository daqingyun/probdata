CXX = g++
CPPFLAGS = -I../../lib/tpg/ -I../../lib/udt/ -I../../lib/iperf3/
CXXFLAGS = -Wall -static
LDFLAGS = -lpthread
LIBS = ../../lib/tpg/libtpg.a ../../lib/udt/libudt.a ../../lib/iperf3/libiperf.a

FASTPROF_SRCS = \
    ../fastprof/fastprof.cpp \
    ../fastprof/fastprof_log.cpp \
    ../fastprof/fastprof_error.cpp \
    ../fastprof/fastprof_tcp.cpp \
    ../fastprof/fastprof_udt.cpp

PROBDATA_SRCS = \
    probdata.cpp \
    probdata_control.cpp \
    probdata_log.cpp \
    probdata_error.cpp \
    probdata_client.cpp \
    probdata_server.cpp \
    probdata_main.cpp

OBJS = $(TPG_SRCS:.cpp=.o) $(FASTPROF_SRCS:.cpp=.o) $(PROBDATA_SRCS:.cpp=.o)

TARGET = probdata

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
