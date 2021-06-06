CPPFLAGS = -I ../../lib/udt/udt4/src/ \
-I ../../lib/iperf3/iperf-master/src

probdata:\
 ../tpg/tpg_common.o \
 ../tpg/tpg_log.o \
 probdata_log.o \
 probdata_error.o \
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
 ../fastprof/fastprof.o \
 ../fastprof/fastprof_log.o \
 ../fastprof/fastprof_error.o \
 ../fastprof/fastprof_tcp.o \
 ../fastprof/fastprof_udt.o \
 probdata.o \
 probdata_control.o \
 probdata_log.o \
 probdata_error.o \
 probdata_client.o \
 probdata_server.o
 
	g++ -o probdata \
	probdata_main.cpp \
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
	../fastprof/fastprof.cpp \
	../fastprof/fastprof_log.cpp \
	../fastprof/fastprof_error.cpp \
	../fastprof/fastprof_tcp.cpp \
	../fastprof/fastprof_udt.cpp \
	probdata.cpp \
	probdata_control.cpp \
	probdata_log.cpp \
	probdata_error.cpp \
	probdata_client.cpp \
	probdata_server.cpp \
	-Wall -I../../lib/udt/udt4/src \
	-I../../lib/iperf3/iperf-master/src \
	-static -lstdc++ -lpthread \
	../../lib/udt/udt4/src/libudt.a \
	../../lib/iperf3/iperf-master/src/.libs/libiperf.a

clean:
	rm *.o probdata