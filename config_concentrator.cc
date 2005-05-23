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

/* section in config we get our values from */
static char *CONF_SEC="concentrator";

int configure_concentrator(struct v_objects *v)
{
        dictionary *conf=v->v_config;
        IpfixSender *ips=NULL;
        IpfixAggregator *ipa=NULL;
        IpfixReceiver *ipr=NULL;

        msg(MSG_DEBUG, "Config: now configuring the concentrator subsystem");

        v->conc_poll_ms=atoi(iniparser_getvalue(conf, CONF_SEC, "poll_interval"));


        /* Initialize concentrator subsystems reversely */

        /* make IPFIX exporter/sender */
        msg(MSG_DEBUG, "Config: now making IPFIX sender");
        initializeIpfixSenders();
        if(!(ips=createIpfixSender(
                              atoi(iniparser_getvalue(conf, CONF_SEC, "source_id")),
                              iniparser_getvalue(conf, CONF_SEC, "export_ip"),
                              atoi(iniparser_getvalue(conf, CONF_SEC, "export_port"))
                                  ))
          ) {
                goto out;
        }
        subsys_on(&(v->v_subsystems), SUBSYS_CONC_EXPORT);

        /* make IPFIX aggregator */
        msg(MSG_DEBUG, "Config: now making IPFIX aggregator");
        initializeAggregators();
        if(!(ipa=createAggregator(
                             iniparser_getvalue(conf, CONF_SEC, "rules"),
                             atoi(iniparser_getvalue(conf, CONF_SEC, "buffertime_min")),
                             atoi(iniparser_getvalue(conf, CONF_SEC, "buffertime_max"))
                                 ))
          ) {
                goto out1;
        }
        addAggregatorCallbacks(ipa, getIpfixSenderCallbackInfo(ips));
        subsys_on(&(v->v_subsystems), SUBSYS_CONC_ACCOUNT);

        /* make IPFIX receiver/collector */
        initializeIpfixReceivers();
        if(!(ipr=createIpfixReceiver(atoi(iniparser_getvalue(conf, CONF_SEC, "listen_port"))))) {
                goto out2;
        }
        addIpfixReceiverCallbacks(ipr, getAggregatorCallbackInfo(ipa));
        subsys_on(&(v->v_subsystems), SUBSYS_CONC_RECEIVE);

        v->conc_receiver=ipr;
        v->conc_exporter=ips;
        v->conc_aggregator=ipa;

        return 0;

out3:
        destroyIpfixReceiver(ipr);
out2:
        destroyAggregator(ipa);
out1:
        destroyIpfixSender(ips);
out:
        return 1;
}


/*
 this is the concentrator polling loop; this has to be done so flows expire
 and get exported etc
 */
void * concentrator_polling(void *arg)
{
        struct timespec req, rem;
        struct v_objects *v=(struct v_objects *)arg;

        IpfixReceiver *ipr=v->conc_receiver;
        IpfixAggregator *ipa=v->conc_aggregator;

        msg(MSG_DEBUG, "Aggregator: polling each %d ms", v->conc_poll_ms);
        /* break millisecond polltime into seconds and nanoseconds */
        req.tv_sec=(v->conc_poll_ms * 1000000) / 1000000000;
        req.tv_nsec=(v->conc_poll_ms * 1000000) % 1000000000;

        /*
        msg(MSG_DEBUG, "Concentrator: poll loop at %d seconds, %d nanoseconds (from %d ms)",
            req.tv_sec, req_tv_nsec, v->conc_poll_ms
           );
         */
        while(!v->conc_exitflag) {
                /* TODO: check for EINTR */
                nanosleep(&req, &rem);

                DPRINTF("Polling aggregator %p\n", ipa);
                stopIpfixReceiver(ipr);
		pollAggregator(ipa);
                startIpfixReceiver(ipr);


        }

        return (void *)1;
}
