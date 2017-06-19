# VERMONT - VERsatile MONitoring Tool

[![Build Status](https://travis-ci.org/tumi8/vermont.svg?branch=master)](https://travis-ci.org/tumi8/vermont)

Vermont is an open-source software toolkit for the creation and processing of network flow data, based on monitored Internet packet data.
See the [Wiki](https://github.com/tumi8/vermont/wiki) for full details of all its features.

## REQUIREMENTS

VERMONT has been tested on Linux and FreeBSD systems.

For compilation, GNU C/C++ compiler and standard libraries are required,
as well as the following Ubuntu/Debian packages (or equivalent packages
of other Linux distributions):
 - cmake 
 - libboost-filesystem-dev
 - libboost-regex-dev
 - libboost-test-dev
 - libboost-thread-dev
 - libxml2-dev 
 - libpcap-dev 
 - libsctp-dev (if not available, disable cmake option SUPPORT_SCTP)

The following packages are optional:
 - cmake-curses-gui (ccmake, interactive user interface of cmake)
 - libpq-dev (for PostgreSQL support)
    ==> cmake option SUPPORT_POSTGRESQL
 - libmysqlclient-dev (for MySQL support)
    ==> cmake option SUPPORT_MYSQL
 - libgsl-dev (for connection-based sampling with Bloom filters)
    ==> cmake option USE_GSL
 - libczmq-dev (for receiving IPFIX reports over ZMQ)
    ==> cmake option SUPPORT_ZMQ

For DTLS support, OpenSSL 1.0.0 is required.

## BUILDING AND INSTALLATION

This project uses cmake for setting platform- and user-specific compile 
options. In order to generate the Makefile for actual compilation, you 
need to call in the root of the source directory:
``` shell
$ cmake .
```

In order to change the default compile options, use:
``` shell
$ cmake -D OPTION1=value1 -D OPTION2=value2 ...
```

To get a list of the most important options, call:
``` shell
$ cmake -LH
```

As a user-friendly alternative, you can use the interactive user interface.
Please note that this requires the package cmake-curses-gui, if you are using
Ubuntu/Debian.
``` shell
$ ccmake .
```

If some libraries are installed in custom directories, use:
``` shell
$ cmake -D CMAKE_PREFIX_PATH=/custom/directory1:/custom/directory2

```

After successfully generating the Makefile with cmake, start the 
compilation with:
``` shell
$ make
```

Although not strictly necessary, VERMONT binaries and data files can be 
copied to the usual install location by running:
``` shell
$ make install 
```


### BUILDING WITH DTLS-OVER-UDP SUPPORT

VERMONT's DTLS support is based on OpenSSL version 1.0.0. OpenSSL 1.1.0 is not currently supported.

In order to compile VERMONT with DTLS-over-UDP support set the following option:
``` shell
$ cmake -DSUPPORT_DTLS=YES
```

If CMake does not find OPENSSL you can explicitly specify the include and library paths:
```
cmake -DSUPPORT_DTLS=YES -DCMAKE_INCLUDE_PATH=/path/to/openssl/include -DCMAKE_LIBRARY_PATH=/path/to/openssl/lib
```

### BUILDING WITH DTLS-OVER-SCTP SUPPORT

At the time of writing (July 2010), DTLS over SCTP can be used on FreeBSD only!
This is due to the fact that FreeBSD is currently the only OS which supports 
the SCTP-AUTH extension (see RFC 4895) which is required by DTLS.

The current version of OpenSSL (1.0.0a) has no native support for SCTP. You 
have to download additional patches from

http://sctp.fh-muenster.de/

and apply them to the OpenSSL sourcese before building OpenSSL. Make sure that
the patches fit to your local version of OpenSSL. Otherwise, you might need to
manually adapt the patch files.

Also, make sure to add the command line argument "sctp" when running OpenSSL's
./config to build SCTP support into OpenSSL.

In order to compile VERMONT with DTLS-over-SCTP support, you need to run cmake
with the following three options:

`-DSUPPORT_SCTP -DSUPPORT_DTLS -DSUPPORT_DTLS_OVER_SCTP`

In addition, you need to indicate the include and library paths to your patched
version of OpenSSL as explained for DTLS-over-UDP.


## USAGE AND CONFIGURATION

In order to run VERMONT, a configuration file is needed which specifies the 
modules to be used and their parameters:
``` shell
$ ./vermont -f <config-file>
```

Example configuration files can be found in [`configs/`](config).
Documentation of the available modules and their configuration parameters
can be found at https://github.com/tumi8/vermont/wiki/Moduleconfiguration

Use `Ctrl-C` to stop VERMONT. If VERMONT does not exit properly, enter Ctrl-C
for a second time.


## OPERATION AS COLLECTOR: TUNING SOCKET RECEIVE BUFFER

VERMONT can be used as an IPFIX/PSAMP and NetFlow.v9 collector. As the 
incoming IPFIX/PSAMP/NetFlow messages usually arrive in bursts, losses 
may occur due to insufficient buffer space.

As a solution, the socket receive buffer can be increased. To check the
current settings, use the following system calls on Linux systems with 
`/proc` file system:
``` shell
$ cat /proc/sys/net/core/rmem_default
$ cat /proc/sys/net/core/rmem_max
```

In order to configure a new value X (in bytes), call:
``` shell
$ sysctl -w net/core/rmem_default=X
$ sysctl -w net/core/rmem_max=X
```

If you want Vermont to use a different buffer size than the default one,
you can specify it using the `<buffer>` directive in the `<listener>` section.


## OPTIMIZED PACKET CAPTURING WITH PCAP

To reduce the number of times packets need to be copied on their way from
the network interface card to the user space (i.e., VERMONT), we recommend 
the utilization of pcap library 1.0.0 or higher.

For earlier versions of pcap, the pcap-mmap patch can be applied to 
improve the performance: http://public.lanl.gov/cpw/


## EFFECTS OF RECEIVE OFFLOAD MECHANISMS

Several mechanisms have been implemented in modern network interface cards,
drivers, and kernels to offload common functions from the protocol stack and 
the application. One particular focus is on TCP segmentation and reassembly.

Receive offload mechanisms aim at reassembling subsequent TCP segments into
a single large packet before passing it to the IP/TCP protocol stack and 
finally to the application. In the Linux kernel, this is done by generic 
receive offload (GRO) if the network interface card and the driver support 
NAPI. Latest Intel 10GE controllers (e.g., 82599) support receive side 
coalescing (RSC) which performs TCP reassembly in hardware.

If any receive offload mechanism is enabled, VERMONT (like any other 
pcap-based application) does not observe the actually captured TCP packets 
but the reassembled ones. One consequence is that packet counts of flows will 
be smaller than the true number of packets.

In order to avoid such distortions, all receive offload mechanisms need to 
be disabled. In the case of GRO (and the older LRO), this can be done with 
ethtool. The following call returns a list of the current status of all 
offload mechanisms for interface <dev>:
``` shell
$ ethtool -k <dev>
```

If GRO is not shown, you probably need to install a newer version of ethtool.
To disable GRO (and LRO), execute:
``` shell
$ ethtool -K <dev> gro off
$ ethtool -K <dev> lro off
```

Hardware-based RSC can be deactivated at compile time of the driver as 
explained here:
http://downloadmirror.intel.com/19004/eng/README-2.0.72.4.txt

