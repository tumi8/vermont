#ifndef CONFIG_SAMPLER_H
#define CONFIG_SAMPLER_H

/* static-sized variables suck, but well */
#define PROCESSOR_MAX_PARAM 16

/*
 PacketProcessor IDs as used in the config file for type
 mainly used in make_filter_processor()
 */
#define PROCESSOR_INTERNAL 0
#define PROCESSOR_SYSTEMATIC 1
#define PROCESSOR_RANDOM 2
#define PROCESSOR_IPHEADER 3

int configure_sampler(struct v_objects *v);

#endif
