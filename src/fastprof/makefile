CPPFLAGS = -I ../../lib/iperf3/iperf-master/src \
-I ../../lib/udt/udt4/src

fastprof:\
 ../tpg/tpg_common.o \
 ../tpg/tpg_log.o \
 ../tpg/tpg_ctrl.o \
 ../tpg/tpg_udt.o \
 ../tpg/tpg_tcp.o \
 ../tpg/tpg_udp.o \
 ../tpg/tpg_conn.o \
 ../tpg/tpg_protocol.o \
 ../tpg/tpg_stream.o \
 ../tpg/tpg_profile.o \
 ../tpg/tpg_client.o \
 ../tpg/tpg_server.o \
 fastprof.o \
 fastprof_log.o \
 fastprof_error.o \
 fastprof_tcp.o \
 fastprof_udt.o
 
	g++ -o fastprof \
	../tpg/tpg_common.cpp \
	../tpg/tpg_log.cpp \
	../tpg/tpg_ctrl.cpp \
	../tpg/tpg_udt.cpp \
	../tpg/tpg_tcp.cpp \
	../tpg/tpg_udp.cpp \
	../tpg/tpg_conn.cpp \
	../tpg/tpg_protocol.cpp \
	../tpg/tpg_stream.cpp \
	../tpg/tpg_profile.cpp \
	../tpg/tpg_client.cpp \
	../tpg/tpg_server.cpp \
	fastprof.cpp \
	fastprof_log.cpp \
	fastprof_error.cpp \
	fastprof_tcp.cpp \
	fastprof_udt.cpp \
	fastprof_main.cpp \
	-Wall -I../../lib/iperf3/iperf-master/src \
	-I../../lib/udt/udt4/src \
	-static -lstdc++ -lpthread \
	../../lib/iperf3/iperf-master/src/.libs/libiperf.a \
	../../lib/udt/udt4/src/libudt.a

clean:
	rm *.o fastprof