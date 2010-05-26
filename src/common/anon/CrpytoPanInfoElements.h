#ifndef _CRYPTOPANINFOELEMENTS_H_
#define _CRYPTOPANINFOELEMENTS_H_


#include <string>
#include <stdint.h>


typedef struct {
    std::string fromNet;
    std::string toNet;
    std::string cidr;
} map_info;

typedef struct {
    uint32_t fromNet;
    uint32_t toNet;
    uint8_t cidr;
} cpan_net_info;


#endif
