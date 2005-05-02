/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert

 */
#include <assert.h>
#include "ipfixlolib/ipfixlolib.h"

#include "sampler/Packet.h"
#include "sampler/Filter.h"
#include "sampler/Observer.h"
#include "sampler/PacketSink.h"
#include "sampler/ExporterSink.h"
#include "sampler/PacketProcessor.h"
#include "sampler/Template.h"
#include "sampler/IPHeaderFilter.h"

/* collector */


/* own systems */
#include "vermont.h"
#include "iniparser.h"
#include "msg.h"
#include "subsystems.h"
#include "config_sampler.h"


static int configure_observer(struct v_objects *v, char *interface, int snaplen);
static int configure_template(struct v_objects *v, uint16_t template_id, char *list);
static int configure_filter(struct v_objects *v, char *list);
static int configure_sink(struct v_objects *v, char *list);
static PacketProcessor * make_filter_processor(char *name, char *setting);

/*
 main entry-point to configure the sampler subsystem

 read config from struct v_objects v->v_config
 attach stuff to stuct v_objects v

 do all necessary steps one-by-one thru small subfunctions

 FIXME: careful freeing of previously allocated resources in case of error
 */
int configure_sampler(struct v_objects *v)
{
	dictionary *conf=v->v_config;

        msg(MSG_DEBUG, "Config: now configuring the sampler subsystem");

        /* dont configure a template if simply using PacketSink -- export_ip=off */
        if(strcasecmp("off", iniparser_getvalue(conf, "sampler", "export_ip")) == 0) {
                msg(MSG_INFO, "Config: Template will not be build, because export_ip=off");

        } else if(configure_template(
                                     v,
                                     atoi(iniparser_getvalue(conf, "sampler", "template_id")),
                                     iniparser_getvalue(conf, "sampler", "template")
                                    )) {
                msg(MSG_FATAL, "Config: could not configure a template");
                return 1;
        }

        /*
         configure an observer
         will caputure packets
         */
        if(configure_observer(
			       v,
			       iniparser_getvalue(conf, "sampler", "interface"),
			       atoi(iniparser_getvalue(conf, "sampler", "capturelen"))
			      )) {
		msg(MSG_FATAL, "Config: could not configure an observer(pcap)");
		return 1;
	}
        subsys_on(&(v->v_subsystems), SUBSYS_SAMP_CAPTURE);

        /*
         configure a filter process
         make main filter object
         attach packetprocessors
         */

        if(configure_filter(
			     v,
			     iniparser_getvalue(conf, "sampler", "filters")
			    )) {

		msg(MSG_FATAL, "Config: could not configure the filter");
                return 1;
	}
        subsys_on(&(v->v_subsystems), SUBSYS_SAMP_FILTER);

        /*
         we MUST have a sink, else we will memleak like hell

         configure_sink() takes care to allocate one of:
         ExporterSink() -> export IPFIX data
         PacketSink() -> just free packets
         */
        /*
         BIG FIXME: FIX PARAMETERS!
         */
        if(configure_sink(
                          v,
                          iniparser_getvalue(conf, "sampler", "export_ip")
                         )) {
		msg(MSG_FATAL, "Config: could not configure a packet sink (important)");
                return 1;
        }
        subsys_on(&(v->v_subsystems), SUBSYS_SAMP_SINK);

        /*
	 now do virtual wiring:
	 observer --> filter --> sink

	 FIXME: HARDCODED WITH ALL 3 SYSTEMS
	 */
	msg(MSG_INFO, "Config: now doing wiring: observer->filter->sink");
	assert(v->observer != 0);
	assert(v->filter != 0);
	assert(v->sink != 0);

	v->observer->addReceiver(v->filter);
        v->filter->setReceiver(v->sink);

        msg(MSG_DEBUG, "Config: sampler subsystem successfully configured");
        return 0;
}


/*
 configure the sampler template from a "," separated list
 list can have numerical and/or string fields
 */
static int configure_template(struct v_objects *v, uint16_t template_id, char *list)
{
        Template *t;
	char *l, *token;
        int tmpid;
        const ipfix_identifier *id;
	
        /* violating the original string is not nice, so copy */
	if(!(l=strdup(list))) {
		return 1;
	}
	
	msg(MSG_INFO, "Template: using ID %d", template_id);
	t=new Template(template_id);

        /*
         loop over all fields
         accept numerical or string input
         lookup id if string input
         add to Template
         */
        while((token=strsep(&l, ","))) {

		/*
		 lookup field
                 name_lookup returns -1 on error, id_lookup NULL
                 range-check id if numerical id was given in list
		 */
		if(isalpha((int)*token)) {
			tmpid=ipfix_name_lookup(token);
		} else {
                        tmpid=atoi(token);
                        if(!ipfix_id_rangecheck(tmpid)) {
                                msg(MSG_ERROR, "Template: ignoring template field %s -> %d - rangecheck not ok", token, tmpid);
                                continue;
                        }
		}

		if( (tmpid == -1) || ((id=ipfix_id_lookup(tmpid)) == NULL) ) {
                        msg(MSG_ERROR, "Template: ignoring unknown template field %s", token);
                        continue;
		}

		msg(MSG_INFO, "Template: adding %s -> ID %d with size %d", token, id->id, id->length);
		t->addField((uint16_t)id->id, (uint16_t)id->length);
	}

	free(l);
        msg(MSG_DEBUG, "Template: got %d fields (given: %s)", t->getFieldCount(), list);

        v->templ=t;

	return 0;
}


/*
 configure an observer, listening at interface with capturelen
 capturelen 0 means use Observer's default
 */
static int configure_observer(struct v_objects *v, char *interface, int snaplen)
{
        dictionary *conf=v->v_config;
        char *pcap_filter=iniparser_getvalue(conf, "sampler", "pcap_filter");
        Observer *o;

        o=new Observer(interface);

        if(!o->prepare(pcap_filter)) {
                msg(MSG_FATAL, "Observer: preparing failed");
                goto out;
	}

        if(snaplen) {
                if(!o->setCaptureLen(snaplen)) {
			msg(MSG_FATAL, "Observer: wrong snaplen specified - using %d", o->getCaptureLen());
		}
        }
	msg(MSG_DEBUG, "Observer: using interface %s, snaplen %d", o->captureInterface, o->getCaptureLen());
	v->observer=o;

        return 0;

out:
	delete o;
        return 1;
}


/*
 configure a complete filtering process with all sub-packetprocessors
 list is a simple char * retrieved from the config
 */
static int configure_filter(struct v_objects *v, char *list)
{
        char *l, *token;
	char *routing;
	
        PacketProcessor *p;
	char *p_settings;
	int p_id;

	dictionary *conf=v->v_config;
	
        Filter *f=new Filter();

        /* violating the original string is not nice, so copy */
        if(!(l=strdup(list))) {
		return 1;
        }

        /* FIXME: add sampler->concentrator hook! */
        routing=iniparser_getvalue(conf, "main", "routing");

        /*
         loop over all processors specified in list
         look up setting for this processor
         make new packetprocessor via make_filter_processor()
         add to main filter
         */
        while((token=strsep(&l, ","))) {

                /* lookup setting for this particular filter in config */
		if(!(p_settings=iniparser_getvalue(conf, "sampler", token))) {
			msg(MSG_ERROR, "Filter: ignoring processor %s - has no config", token);
			continue;
		}
		
                /* make new sub-filter object */
                if(!(p=make_filter_processor(token, p_settings))) {
                        msg(MSG_ERROR, "Filter: ignoring processor %s(%s) - make_filter_process() error", token, p_settings);
                        continue;
                }
		
		/* and add to filtering process */
                msg(MSG_INFO, "Filter: adding processor %s<%s> to main filter", token, p_settings);
               	f->addProcessor(p);
	}

        v->filter=f;
        free(l);

        return 0;
}

/*
 name: just a name for the printing (eg from config file)
 setting: make a new PacketProcessor out of a configuration line
 mapping ID <-> object-type is done here

 0: internal, may be HookingFilter
 1: SystematicSampler, takes 3 arguments
 2: RandomSampler, takes 2 arguments
 3: IPHeaderFilter, takes 4 arguments

 */
static PacketProcessor * make_filter_processor(char *name, char *setting)
{
        PacketProcessor *n;
	char *l, *token;
        int id;

        /*
         the following is helluva dirty
         keywords for grep: FUCK, DIRTY, MORON, SUCKS, SUCKZ

         we don't accept filters with more than PROCESSOR_MAX_PARAM parameters
         */
        int p_conf[PROCESSOR_MAX_PARAM];
        int p_conf_nr=0, i=0;
	
        /* really do not violate original string */
        if(!(l=strdup(setting))) {
		return NULL;
	}
	
	/* processor type is the first number */
	token=strsep(&l, ",");
        id=atoi(token);

        msg(MSG_DEBUG, "Filter: new PacketProcessor %s - type %d, full params %s", name, id, setting);
        while((token=strsep(&l, ",")) && p_conf_nr < PROCESSOR_MAX_PARAM) {
                p_conf[p_conf_nr]=atoi(token);
                /* include hardcode debuggin in case of */
                DPRINTF("PacketProcessor: token %s parsed as %d\n", token, p_conf[p_conf_nr]);
                p_conf_nr++;
        }

        /* just dump the settings if one is interested */
        while(i < p_conf_nr) {
                msg(MSG_INFO, "Processor %s param #%d: %d", name, i, p_conf[i]);
                i++;
        }

        /*
         the following is helluva dirty, too
         keywords for grep: FUCK, DIRTY, MORON, SUCKS, SUCKZ

         PROCESSOR_* are defined in config_sampler.h
         hardcoded parameters!
         */
        switch(id) {
	case PROCESSOR_INTERNAL:
                n=NULL;
                break;
	case PROCESSOR_SYSTEMATIC:
		n=new SystematicSampler(p_conf[0], p_conf[1], p_conf[2]);
		break;
	case PROCESSOR_RANDOM:
		n=new RandomSampler(p_conf[0], p_conf[1]);
		break;
	case PROCESSOR_IPHEADER:
		n=new IPHeaderFilter(p_conf[0], p_conf[1], p_conf[2], p_conf[3]);
		break;
	default:
                msg(MSG_FATAL, "Filter: cannot make PacketProcessor with ID %d, settings %s", id, setting);
                n=NULL;
        }
out:
        free(l);

        return n;
}


/*
 configure a sink; char *list are the parameters

 FIXME: HARDCODED PROTO UDP

 POTENTIAL FIXME: we could have more of them, e.g. 2 exporterSinks
 will be a bit more work to figure out the Packet-free()ing then!
 */
static int configure_sink(struct v_objects *v, char *list)
{
        dictionary *conf;
        Sink *s;
        char *dst_port, *dst_ip, *source_id;

        conf=v->v_config;

        source_id=iniparser_getvalue(conf, "sampler", "source_id");
        dst_port=iniparser_getvalue(conf, "sampler", "export_port");
        dst_ip=iniparser_getvalue(conf, "sampler", "export_ip");

        /*
         dont want to export ? so the sink is a simple PacketSink
         we need a sink, or else no packets will be free()d
         */
        if(strcasecmp(list, "off") == 0) {
                s=new PacketSink();
                msg(MSG_DEBUG, "Sink: using plain PacketSink()");
        } else {
                int sID;
                ExporterSink *e;
		/* check parameters */
		if(!source_id || !dst_port || !dst_ip) {
			msg(MSG_FATAL,
			    "Sink: not all parameters in config: source_id: %s, dst_port: %s, dst_ip: %s",
			    source_id, dst_port, dst_ip
			   );
			return 1;
		}

                sID=atoi(source_id);
		/* do a real exporter sink */
                msg(MSG_DEBUG, "Sink: using ExporterSink(), own source_ID %d, dst %s:%s",
		    sID,
                    dst_ip,
                    dst_port
                   );
		e=new ExporterSink(v->templ, sID);
		/* FIXME: alias PacketProcessor to ExportSink, so we can use subfunction */
                e->AddCollector(dst_ip, (uint16_t)atoi(dst_port), "UDP");
                s=e;
        }

        v->sink=s;

        return 0;
}
