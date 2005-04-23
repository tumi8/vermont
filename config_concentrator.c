/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert

 */


/* own systems */
#include "vermont.h"
#include "iniparser.h"
#include "msg.h"
#include "subsystems.h"
#include "config_concentrator.h"

int configure_concentrator(struct v_objects *v)
{
        dictionary *conf=v->v_config;

        msg(MSG_DEBUG, "Config: now configuring the concentrator subsystem");



}

void * concentrator_polling(void *arg)
{
        struct timespec req, rem;
        req.tv_sec=1;
        req.tv_nsec=0;

        while(!conc_exit) {
                /* TODO: check for EINTR */
                nanosleep(&req, &rem);
                pollAggregator();
        }


}
