CPPFLAGS = -I ../../lib/udt/udt4/src

tpg:\
 tpg_common.o \
 tpg_log.o \
 tpg_ctrl.o \
 tpg_udt.o \
 tpg_tcp.o \
 tpg_udp.o \
 tpg_conn.o \
 tpg_protocol.o \
 tpg_stream.o \
 tpg_profile.o \
 tpg_client.o \
 tpg_server.o
 
	g++ -o tpg \
	tpg_main.cpp \
	tpg_common.cpp \
	tpg_log.cpp \
	tpg_ctrl.cpp \
	tpg_udt.cpp \
	tpg_tcp.cpp \
	tpg_udp.cpp \
	tpg_conn.cpp \
	tpg_protocol.cpp \
	tpg_stream.cpp \
	tpg_profile.cpp \
	tpg_client.cpp \
	tpg_server.cpp \
	-Wall -I../../lib/udt/udt4/src \
	-static -lstdc++ -lpthread \
	../../lib/udt/udt4/src/libudt.a

clean:
	rm *.o tpg