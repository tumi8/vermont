import socket
import struct

def IP2String(int_ip):
	return socket.inet_ntoa(struct.pack('>L',int_ip))

def String2IP(dotted_quad_ip):
	return struct.unpack('>L', socket.inet_aton(dotted_quad_ip))


class FlowRecord:
	def __init__(self, sip, dip, sport, dport):
		self._src_ip = sip
		self._dst_ip = dip
		self._src_port = sport
		self._dst_port = dport
