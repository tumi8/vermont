/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert

 */
#ifndef SUBSYSTEMS_H
#define SUBSYSTEMS_H

#ifdef __cplusplus
extern "C" {
#endif

void subsys_dump(unsigned int flags);
void subsys_on(unsigned int *flag, unsigned int sys);
void subsys_off(unsigned int *flag, unsigned int sys);


#define SUBSYS_CONFIG 0x1

#define SUBSYS_CONC_RECEIVE 0x2
#define SUBSYS_CONC_ACCOUNT 0x4
#define SUBSYS_CONC_EXPORT 0x8

#define SUBSYS_SAMP_CAPTURE 0x10
#define SUBSYS_SAMP_FILTER 0x20
#define SUBSYS_SAMP_SINK 0x40

#ifdef __cplusplus
}
#endif

#endif
