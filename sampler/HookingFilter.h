/*
 Hooking Filter
 (c) by Ronny T. Lampert

 This filter serves as the interface between the
 Sampler part, which deals with Packets, and the
 Concentrator part, which deals with Flows.

 */

#ifndef HOOKING_FILTER_H
#define HOOKING_FILTER_H

#include "PacketProcessor.h"
#include "concentrator/FlowSink.hpp"

class HookingFilter : public PacketProcessor {

	public:
		IpfixRecord::TemplateInfo* ip_traffic_template;
		IpfixRecord::TemplateInfo* icmp_traffic_template;
		IpfixRecord::TemplateInfo* udp_traffic_template;
		IpfixRecord::TemplateInfo* tcp_traffic_template;

		HookingFilter(FlowSink *flowSink = 0);
		virtual ~HookingFilter();

		/*
		   the hook function may need additional context.
		   in our case we need the instance of the aggregator that gets data
		   */
		void setFlowSink(FlowSink *flowSink) {
			this->flowSink=flowSink;
		}

		virtual bool processPacket(const Packet *p);

	protected:
		FlowSink *flowSink;

		IpfixRecord::FieldInfo* ip_traffic_fi;
		IpfixRecord::FieldInfo* icmp_traffic_fi;
		IpfixRecord::FieldInfo* udp_traffic_fi;
		IpfixRecord::FieldInfo* tcp_traffic_fi;
};




#endif
