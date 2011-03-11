import socket
import struct

def IP2String(int_ip):
	return socket.inet_ntoa(struct.pack('>L',int_ip))

def String2IP(dotted_quad_ip):
	return struct.unpack('>L', socket.inet_aton(dotted_quad_ip))


class FlowRecord:
	field_map = { "sourceIPv4Address" : "_src_ip",
				  "destinationIPv4Address" : "_dst_ip",
				  "sourceTransportPort" : "_src_port",
				  "destinationTransportPort" : "_dst_port",
				  "protocolIdentifier" : "_prot",
				  "flowStartMilliSeconds" : "_start_time",
				  "flowEndMilliSeconds" : "_end_time",
				  "revFlowStartMilliSeconds" : "_rev_start_time",
				  "revFlowEndMilliSeconds" : "_rev_end_time",
				}
	def __init__(self, fields):
		for f in fields.keys():
			attrname = f
			if f in self.field_map: 
				attrname = self.field_map[f]
			setattr(self, attrname, fields[f])
