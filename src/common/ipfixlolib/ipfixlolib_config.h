#ifndef IPFIXLOLIB_CONFIG_H
#define IPFIXLOLIB_CONFIG_H

#include <stdint.h>

typedef struct {
    uint16_t mtu; /*!< Maximum transmission unit (MTU).
		     If set to 0, PMTU discovery will be used.
		     (Only available on the Linux platform)
		     Applies to UDP and DTLS over UDP only. */
} ipfix_aux_config_udp;

#endif
