/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert

 */
#include <stdio.h>
#include "subsystems.h"
#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 should print all running subsystems
 by now only dump flags
 */
void subsys_dump(unsigned int flags)
{
        int size=(8*sizeof(flags));

	msg(MSG_DIALOG, "subsystems running: 0x%x", flags);

}


void subsys_on(unsigned int *flag, unsigned int sys)
{
        *flag |= sys;
}


void subsys_off(unsigned int *flag, unsigned int sys)
{
        *flag &= ~sys;
}


#ifdef __cplusplus
}
#endif
