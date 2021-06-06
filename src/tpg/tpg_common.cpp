/***************************************************************************
 * TPG - Transport Profile Generator
 * All rights reserved - Daqing Yun <daqingyun@gmail.com>
 ***************************************************************************/

#include "tpg_common.h"
#include <udt.h>

int t_errno = -1;

const char tpg_program_version[] =
    "TPG version " 
    T_PROGRAM_VERSION 
    " (" 
    T_PROGRAM_VERSION_DATE 
    ")";

const char tpg_bug_report_contact[] = 
    "Net2013 Team <" T_BUG_REPORT_EMAIL ">";

const char tpg_usage_shortstr[] =
    "\nUsage: tpg [-s|-c host] [options]\n"
    "Try `tpg --help' for more information.\n";

const char tpg_usage_longstr[] =
"Usage: tpg [-s|-c host] [options]\n"
"       tpg [-h|--help] [-v|--version]\n\n"
"Server/Client:\n"
" -p, --port <p1:p2>   # port# default <ctrl_port:data_port> <%d:%d>\n"
" -b, --bind <ip:port> # client binds to an <ip:port> port=0 will use default\n"
" -a, --affinity       # set CPU affinity\n"
" -x, --FastProf       # enable/disable automatic FastProf algorithm\n"
" -v, --version        # show version information and quit\n"
"\n"
"Server specific:\n"
" -s, --server         # run in server mode\n"
"\n"
"Client specific:\n"
" -c, --client <host>  # run in client mode, connecting to <host>\n"
" -t, --udt            # use UDT rather than TCP (default %s)\n"
" -d, --time duration  # test duration (seconds) by default %d seconds\n"
" -l, --len [bytes]    # length of the buffer to read or write\n"
"                        by default TCP (%d B), UDT (%d B), UDP: (%d B)\n"
"                        UDP is not supported currently\n"
" -i, --interval       # seconds between periodic reports default %.1f\n"
" -j, --output         # print interval results default %s\n"
" -P, --parallel       # # of parallel client streams to run (default %d)\n"
" -m, --Multi-NIC      # if specified do multiple NIC-to-NIC by default %s\n"
" -M, --set-mss        # set TCP/UDT maximum segment size (Bytes)\n"
"                        MTU for UDT, (MTU-40) for TCP\n"
" -w, --window         # TCP setsockopt() (socket buffer size, Bytes)\n"
" -f, --UDT send buf   # UDT send buffer size\n"
" -F, --UDP send buf   # UDP send buffer size (UDT option, Bytes)\n"
" -r, --UDT recv buf   # UDT recv buffer size\n"
" -R, --UDP recv buf   # UDP recv buffer size\n"
" -B, --UDT Bandwidth  # UDT_MAXBW, -1 is no limit, also used in FastProf\n"
"\n"
"FastProf specific:\n"
" -x  --fastprof       # Enable/disable FastProf\n"
" -y  --capacity       # Link capacity\n"
" -T  --RTT            # Round trip time in milliseconds\n"
" -C  --ratio          # Performance gain ratio\n"
" -L  --limit          # Maximal number of iterations with no improvement\n"
" -N  --Max limit      # Maximal number of the total iterations\n"
"\n"
"ProbData specific:\n"
" -z  --probdata       # Enable/disable ProbData\n"
" -S  --src file       # Source file (including path)\n"
" -V  --dst file       # Destination file (including path)\n"
"\n"
"TPG version %s\n"
"Please report bugs to %s.\n"
"\n";

enum
{
    IN_UNITBYTE = 0,
    IN_KILOBYTE = 1,
    IN_MEGABYTE = 2,
    IN_GIGABYTE = 3
};

const double tpg_byte_conv[] =
{
    1.0,
    1.0 / 1024,
    1.0 / 1024 / 1024,
    1.0 / 1024 / 1024 / 1024
};

const char* tpg_byte_label[] =
{
    " B",
    "KB",
    "MB",
    "GB"
};

const double tpg_bit_conv[] =
{
    1.0,
    1.0 / 1000,
    1.0 / 1000 / 1000,
    1.0 / 1000 / 1000 / 1000
};

const char* tpg_bit_label[] =
{
    " b",
    "Kb",
    "Mb",
    "Gb"
};

void t_print_short_usage()
{
    fputs(tpg_usage_shortstr, stderr);
}

void t_print_long_usage()
{
    fprintf(stderr, tpg_usage_longstr,
            T_DEFAULT_PORT,
            T_DEFAULT_UDT_PORT,
            "TCP",
            T_DEFAULT_DURATION,
            T_DEFAULT_TCP_BLKSIZE,
            T_DEFAULT_UDT_BLKSIZE,
            T_DEFAULT_UDP_BLKSIZE,
            T_REPT_INTERVAL,
            "NO",
            1,
            "NO",
            tpg_program_version,
            tpg_bug_report_contact);
}

const char* t_errstr(int err_no)
{
    static std::string errstr("");
    char intstr[32];
    int perr = 0;
    
    switch (err_no)
    {
        case T_ERR_NONE:
            errstr = "No error";
            break;
        case T_ERR_SERV_CLIENT:
            errstr = "Cannot be both server and client";
            break;
        case T_ERR_NO_ROLE:
            errstr = "Must either be a client (-c) or server (-s)";
            break;
        case T_ERR_SERVER_ONLY:
            errstr = "Some of the options are server only";
            break;
        case T_ERR_CLIENT_ONLY:
            errstr = "Some of the option are client only";
            break;
        case T_ERR_DURATION:
            sprintf(intstr, "%d", T_MAX_DURATION);
            errstr = "Profiling duration is too long (maximum = " + std::string(intstr) + " seconds)";
            break;
	case T_ERR_NUM_STREAMS:
            sprintf(intstr, "%d", T_MAX_STREAM_NUM);
            errstr = "Number of parallel streams is too large (maximum = " + std::string(intstr) + "%d)";
            break;
	case T_ERR_MULTI_NIC:
            errstr = "streams# must be > 1 if multiple NIC-to-NIC is enabled";
            break;
        case T_ERR_BIND_CONFLICT:
            errstr = "Cannot bind to a single address when multiple is enabled";
            break;
        case T_ERR_BW_NEEDED:
            errstr = "Value is needed by FastProf (-x) with RTT (-D) specified";
            break;
        case T_ERR_BLOCK_SIZE:
            sprintf(intstr, "%d", T_MAX_BLOCKSIZE);
            errstr = "Block size is too large (maximum = " + std::string(intstr) + " bytes)";
            sprintf(intstr, "%d", T_MIN_BLOCKSIZE);
            errstr += " or too small (min = " + std::string(intstr) + " bytes)";
            break;
        case T_ERR_BUF_SIZE:
            sprintf(intstr, "%d", T_MAX_TCP_BUFFER);
            errstr = "TCP socket buffer size is too large (maximum = " + std::string(intstr) + " bytes)";
            break;
        case T_ERR_INTERVAL:
            sprintf(intstr, "%d", T_MIN_DURATION);
            errstr = "Invalid report interval (min = " + std::string(intstr) + ", max = duration <-d option 10s by default>)";
            break;
        case T_ERR_MSS:
            sprintf(intstr, "%d", T_MAX_MSS);
            errstr = "TCP MSS is too large (maximum = " + std::string(intstr) + " bytes)";
            break;
        case T_ERR_FILE:
            errstr = "Unable to open file";
            perr = 1;
            break;
        case T_ERR_INIT_PROFILE:
            errstr = "Profile initialization failed";
            perr = 1;
            break;
        case T_ERR_LISTEN:
            errstr = "Unable to start listener for connections";
            perr = 1;
            break;
        case T_ERR_CONNECT:
            errstr = "Unable to connect to server";
            perr = 1;
            break;
        case T_ERR_ACCEPT:
            errstr = "Unable to accept connection from client";
            perr = 1;
            break;
        case T_ERR_SEND_COOKIE:
            errstr = "Unable to send cookie to server";
            perr = 1;
            break;
        case T_ERR_RECV_COOKIE:
            errstr = "Unable to receive cookie at server";
            perr = 1;
            break;
        case T_ERR_CTRL_WRITE:
            errstr = "Unable to write to the control socket";
            perr = 1;
            break;
        case T_ERR_CTRL_READ:
            errstr = "Unable to read from the control socket";
            perr = 1;
            break;
        case T_ERR_CTRL_CLOSE:
            errstr = "Control socket has closed unexpectedly";
            break;
        case T_ERR_MESSAGE:
            errstr = "Received an unknown control message";
            break;
        case T_ERR_SEND_MESSAGE:
            errstr = "Unable to send control message";
            perr = 1;
            break;
        case T_ERR_RECV_MESSAGE:
            errstr = "Unable to receive control message";
            perr = 1;
            break;
        case T_ERR_SEND_PARAMS:
            errstr = "Unable to send parameters to server";
            perr = 1;
            break;
        case T_ERR_RECV_PARAMS:
            errstr = "Unable to receive parameters from client";
            perr = 1;
            break;
        case T_ERR_PACKAGE_RESULTS:
            errstr = "Unable to package results";
            perr = 1;
            break;
	case T_ERR_SEND_RESULTS:
            errstr = "Unable to send results";
            perr = 1;
            break;
        case T_ERR_RECV_RESULTS:
            errstr = "Unable to receive results";
            perr = 1;
            break;
        case T_ERR_SELECT:
            errstr = "Select failed";
            perr = 1;
            break;
        case T_ERR_CLIENT_TERM:
            errstr = "Client has terminated";
            break;
        case T_ERR_SERVER_TERM:
            errstr = "Server has terminated";
            break;
        case T_ERR_ACCESS_DENIED:
            errstr = "Server is busy running a profiling, try again later";
            break;
        case T_ERR_SET_NODELAY:
            errstr = "Unable to set TCP NODELAY";
            perr = 1;
            break;
        case T_ERR_SET_MSS:
            errstr = "Unable to set TCP MSS";
            perr = 1;
            break;
        case T_ERR_SET_BUF:
            errstr = "Unable to set TCP socket buffer size";
            perr = 1;
            break;
        case T_ERR_REUSEADDR:
            errstr = "Unable to reuse address on socket";
            perr = 1;
            break;
	case T_ERR_NON_BLOCKING:
            errstr = "Unable to set socket to non-blocking";
            perr = 1;
            break;
        case T_ERR_SET_WIN_SIZE:
            errstr = "Unable to set socket window size";
            perr = 1;
            break;
        case T_ERR_PROTOCOL:
            errstr = "Protocol does not exist";
            break;
        case T_ERR_AFFINITY:
            errstr = "Unable to set CPU affinity";
            perr = 1;
            break;
        case T_ERR_CREATE_STREAM:
            errstr = "Unable to create a new stream";
            break;
        case T_ERR_INIT_STREAM:
            errstr = "Unable to initialize stream";
            perr = 1;
            break;
        case T_ERR_STREAM_LISTEN:
            errstr = "Unable to start stream listener";
            perr = 1;
            break;
        case T_ERR_STREAM_CONNECT:
            errstr = "Unable to connect stream";
            perr = 1;
            break;
        case T_ERR_STREAM_ACCEPT:
            errstr = "Unable to accept stream connection";
            perr = 1;
            break;
        case T_ERR_STREAM_WRITE:
            errstr = "Unable to write to stream socket";
            perr = 1;
            break;
        case T_ERR_STREAM_READ:
            errstr = "Unable to read from stream socket";
            perr = 1;
            break;
        case T_ERR_STREAM_CLOSE:
            errstr = "Stream socket has closed unexpectedly";
            break;
        case T_ERR_STREAM_INIT:
            errstr = "Stream cannot be initialized";
            break;
        case T_ERR_SET_CONGESTION:
            errstr = "Unable to set TCP_CONGESTION: supplied cong ctrl alg not supported on this host";
            break;
        case T_ERR_UDT_ERROR:
            errstr = "UDT Error: " + std::string(UDT::getlasterror().getErrorMessage());
            break;
        case T_ERR_THREAD_ERROR:
            perr = 1;
            errstr = "Threading error";
            break;
        case T_ERR_UDT_SEND_ERROR:
            errstr = "UDT send Error: " + std::string(UDT::getlasterror().getErrorMessage());
            break;
        case T_ERR_UDT_RECV_ERROR:
            errstr = "UDT recv Error: " + std::string(UDT::getlasterror().getErrorMessage());
            break;
        case T_ERR_UDT_OPT_ERROR:
            errstr = "UDT Set Option Error: " + std::string(UDT::getlasterror().getErrorMessage());
            break;
        case T_ERR_UDT_PERF_ERROR:
            errstr = "UDT PerfMon Error: " + std::string(UDT::getlasterror().getErrorMessage());
            break;
        case T_ERR_CLIENT_PACK:
            errstr = "Pack sending packets error";
            break;
        case T_ERR_SOCK_CLOSE_ERROR:
            errstr = "Socket close errors";
            break;
        case T_ERR_PHY_CONN_INIT:
            errstr = "IP-IP connection initialization error";
            break;
        case T_ERR_DSTFILE_ONLY:
            errstr = "You only specify a destination file a source file is required";
            break;
        case T_ERR_DISABLED_PROBDATA:
            errstr = "probdata is disabled, cannot specify any file names";
		break;
    }
    
    if (perr) {
        errstr += ": ";
    }
    if (errno && perr) {
        errstr += strerror(errno);
    }
    return (errstr.c_str());
}

const char* t_status_to_str(signed char tpgstate)
{
    switch (tpgstate)
    {
        case T_PROGRAM_START:
            return "T_PROGRAM_START";
            break;
        case T_TWOWAY_HANDSHAKE:
            return "T_TWOWAY_HANDSHAKE";
            break;
        case T_CTRLCHAN_EST:
            return "T_CTRLCHAN_EST";
            break;
        case T_CREATE_DATA_STREAMS:
            return "T_CREATE_DATA_STREAMS";
            break;
        case T_PROFILING_START:
            return "T_PROFILING_START";
            break;
        case T_PROFILING_RUNNING:
            return "T_PROFILING_RUNNING";
            break;
        case T_PROFILING_END:
            return "T_PROFILING_END";
            break;
        case T_EXCHANGE_PROFILE:
            return "T_EXCHANGE_PROFILE";
            break;
        case T_PROGRAM_DONE:
            return "T_PROGRAM_DONE";
            break;
        case T_SERVER_ERROR:
            return "T_SERVER_ERROR";
            break;
        case T_ACCESS_DENIED:
            return "T_ACCESS_DENIED";
            break;
        case T_SERVER_TERM:
            return "T_SERVER_TERM";
            break;
        case T_CLIENT_TERM:
            return "T_CLIENT_TERM";
            break;
        default:
            return "BAD_STATE";
            break;
    }
}

double t_unit_atof(const char *str)
{
	double var;
	char unit = '\0';

	assert(str != NULL);

	sscanf(str, "%lf%c", &var, &unit);

	switch (unit)
	{
	case 'g':
	case 'G':
		{
			var *= T_GB;
			break;
		}
	case 'm':
	case 'M':
		{
			var *= T_MB;
			break;
		}
	case 'k':
	case 'K':
		{
			var *= T_KB;
			break;
		}
	default:
		break;
	}

	return var;
}

int t_unit_atoi(const char *str)
{
    double var;
    char unit = '\0';
    
    assert(str != NULL);
    
    sscanf(str, "%lf%c", &var, &unit);
    
    switch (unit)
    {
        case 'g':
        case 'G':
        {
            var *= T_GB;
            break;
        }
        case 'm':
        case 'M':
        {
            var *= T_MB;
            break;
        }
        case 'k':
        case 'K':
        {
            var *= T_KB;
            break;
        }
        default:
            break;
    }
    return (int) var;
}

long long int t_get_currtime_ms()
{
    struct timeb t;
    ftime(&t);
    return (long long int)(1000 * t.time + t.millitm);
}

long long int t_get_currtime_us()
{
#ifndef WIN32
    timeval t;
    gettimeofday(&t, 0);
    return (long long int)(t.tv_sec * 1000000ULL + t.tv_usec);
#else
    LARGE_INTEGER ccf;
    HANDLE hCurThread = GetCurrentThread();
    DWORD_PTR dwOldMask = SetThreadAffinityMask(hCurThread, 1);
    if (QueryPerformanceFrequency(&ccf))
    {
        LARGE_INTEGER cc;
        if (QueryPerformanceCounter(&cc))
        {
            SetThreadAffinityMask(hCurThread, dwOldMask);
            return (cc.QuadPart * 1000000ULL / ccf.QuadPart);
        }
    }
    SetThreadAffinityMask(hCurThread, dwOldMask);
    return GetTickCount() * 1000ULL;
#endif
}

void t_unit_format(char* s, int len, double bytes, char flag)
{
    assert(len >= 11);
    
    const char* output_format;
    const char* output_suffix;
    double conv_size = 0.0;
    double conv_size_temp = 0.0;
    
    int converion_index = IN_UNITBYTE;
    
    if (isupper(flag)) {
        conv_size = bytes;
        conv_size_temp = conv_size;
        while (conv_size_temp >= 1024.0 && converion_index <= IN_GIGABYTE) {
            conv_size_temp /= 1024.0;
            converion_index ++;
        }
        conv_size *= tpg_byte_conv[converion_index];
        output_suffix = tpg_byte_label[converion_index];
    } else {
        conv_size = bytes * 8.0;
        conv_size_temp = conv_size;
        while (conv_size_temp >= 1000.0 && converion_index <= IN_GIGABYTE) {
            conv_size_temp /= 1000.0;
            converion_index ++;
        }
        conv_size *= tpg_bit_conv[converion_index];
        output_suffix = tpg_bit_label[converion_index];
    }
    
    if (conv_size < 9.995) {
        output_format = "%4.3f %s";
    } else if (conv_size < 99.95) {
        output_format = "%4.2f %s";
    } else if (conv_size < 999.5) {
        output_format = "%4.1f %s";
    } else {
        output_format = "%5.0f %s";
    }
    snprintf(s, len, output_format, conv_size, output_suffix);
}