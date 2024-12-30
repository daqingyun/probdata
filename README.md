PRofiling Optimization Based DAta Transfer Advisor (ProbData)
================================================================

Overview
-------

probdata is a data transfer advisor to help end users easily
determine what data transfer methods to use and what control
parameter values to set for achieving high-speed data transfer
in high-performance networks.


Obtaining probdata
----------------

probdata is available at:

	https://github.com/daqingyun/probdata


Building probdata
---------------

### Prerequisites: ###

probdata needs the following libraries:

	1. UDT (https://udt.sourceforge.io/)	
	2. ESnet iperf3 (https://software.es.net/iperf/)
	3. TPG (inlcluded)

### Building/Updating libraries (optional) ###

	1. Download the UDT package to ./lib/udt
	2. Download the iperf3 package to ./lib/iperf3
	3. Unpack and compile them
		
### Building probdata ###

	1. Go to ./src/probdata
	2. Issue make (tested on FC-23)

Invoking probdata
---------------

	Usage: probdata [-s|-c host] [options]
	       probdata [-h|--help] [-v|--version]

Example test
---------------

Between the following two hosts:

	Client: 10.0.0.1
	Server: 10.0.0.2

### At server site ###
	./probdata -s

### At client site ###
	./probdata -c 10.0.0.2

### Sample output ###
	ProbData version 1.0 (November 20, 2019)
	Linux X.X.X-XX-generic ... GNU/Linux
	...
	+++++++++++++++++++++++++++++++++++++++++++++++++
	*** Estimating RTT...
		Measured RTT: 0.120 ms
	+++++++++++++++++++++++++++++++++++++++++++++++++
	*** SEARCH IN MEM-MEM HISTORICAL PROFILING DATA
	...
		Below is what we've found in the historical data:
		-----------------------------------------
		SrcIP:        10.0.0.1
		DstIP:        10.0.0.2
		...
		AvgPerf:      8563.356130 Mbps
		-----------------------------------------

	+++++++++++++++++++++++++++++++++++++++++++++++++
	*** FASTPROF TCP-BASED CLIENT IS STARTED
		...
	*** FASTPROF TCP IS DONE WITH BELOW RESULT:
		-----------------------------------------
		...
		SrcIP:    10.0.0.1
		DstIP:    10.0.0.2
		Protocol: TCP
		...
		MaxPerf:  2940.410 Mbps
		-----------------------------------------
		FASTPROF TCP-BASED CLIENT IS DONE
	+++++++++++++++++++++++++++++++++++++++++++++++++

	+++++++++++++++++++++++++++++++++++++++++++++++++
	*** FASTPROF UDT-BASED CLIENT IS STARTED
		...
	*** FASTPROF UDT IS DONE WITH BELOW RESULT:
		-----------------------------------------
		...
		SrcIP:    10.0.0.1
		DstIP:    10.0.0.2
		Protocol: UDT
		...
		MaxPerf:  4223.755 Mbps
		-----------------------------------------
		FASTPROF UDT-BASED CLIENT IS DONE
	+++++++++++++++++++++++++++++++++++++++++++++++++

	+++++++++++++++++++++++++++++++++++++++++++++++++
	*** RECOMMENDED CONTROL PARAMETER VALUES:
		-----------------------------------------
		SrcIP: 10.0.0.1
		DstIP: 10.0.0.2
		Protocol: TCP
		...
		MaxPerf: 8563.356130 Mbps
		-----------------------------------------
	+++++++++++++++++++++++++++++++++++++++++++++++++
	...
	Time elapsed: x hours x minutes xx seconds


Reference
-----------

* D. Yun, C.Q. Wu, N.S.V. Rao, and R. Kettimuthu.
Advising Big Data Transfer Over Dedicated Connections Based on Profiling Optimization.
IEEE/ACM Transactions on Networking, vol. 27, no. 6, pp. 2280-2293, December 2019.
DOI: 10.1109/TNET.2019.2943884.


Copyright
---------

Copyright (c) All Rights Reserved - Daqing Yun (daqingyun@gmail.com)
