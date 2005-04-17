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
 this TAB has to corespond to the SUBSYS_ defines
 where name[i] == log2(SUBSYS_DEFINE)
 */
static char *TAB[]={
	"config",
	"conc-receiv", "conc-account", "conc-export",
	"samp-capture", "samp-filter", "samp-sink",
	0 };
/*
 should print all running subsystems
 this could be a lot more elaborate, e.g. dumping parameters
 and so forth, if you like
 */
void subsys_dump(unsigned int flags)
{
        int sys=0;
	int size=(8*sizeof(flags));

	msg(MSG_DIALOG, "subsystems running: 0x%x", flags);

	/* do some fancy dumping */
	while(size--) {
		if(flags & 0x1) {
                        msg(MSG_DIALOG, "[X] %s", TAB[sys]);
		}
		flags >>= 1;
		sys++;
	}
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
