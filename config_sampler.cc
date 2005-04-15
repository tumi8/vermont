/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert

 */
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
static int make_filter_processor(PacketProcessor **p, char *setting);

/* FIXME: careful freeing of previously allocated resources in case of error */
int configure_sampler(struct v_objects *v)
{
	dictionary *conf=v->v_config;

        msg(MSG_INFO, "Config: now configuring the sampler subsystem");

	if(!configure_template(
			       v,
			       atoi(iniparser_getvalue(conf, "sampler", "template_id")),
			       iniparser_getvalue(conf, "sampler", "template")
			      )) {
		msg(MSG_FATAL, "Config: could not configure a template");
		return 1;
	}

	if(!configure_observer(
			       v,
			       iniparser_getvalue(conf, "sampler", "interface"),
			       atoi(iniparser_getvalue(conf, "sampler", "capturelen"))
			      )) {
		msg(MSG_FATAL, "Config: could not configure an observer(pcap)");
		return 1;
	}

	if(!configure_filter(
			     v,
			     iniparser_getvalue(conf, "sampler", "filters")
			    )) {

		msg(MSG_FATAL, "Config: could not configure the filter");
                return 1;
	}

        /* attach the successfully used config into the main config struct */
	v->v_config=conf;

	return 0;
}


/* configure the sampler template from a "," separated list */
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
	
        /* assemble the Template */
	msg(MSG_INFO, "Template: using ID %d", template_id);
	t=new Template(template_id);

	while((token=strsep(&l, ","))) {

		/*
		 lookup field
		 name_lookup returns -1 on error, id_lookup NULL
                 make use of short circuit in C for ||
		 */
		if(isalpha((int)*token)) {
			tmpid=ipfix_name_lookup(token);
		} else {
                        tmpid=atoi(token);
		}

		if( (tmpid == -1) || ((id=ipfix_id_lookup(tmpid)) == NULL) ) {
			msg(MSG_ERROR, "Ignoring unknown template field %s", token);
                        continue;
		}

		msg(MSG_INFO, "Template: adding %s -> ID %d", token, id->id);
		t->addField((uint16_t)id->id, (uint16_t)id->length);
	}

	free(l);
	msg(MSG_INFO, "Template: %d fields", t->getFieldCount());

        v->templ=t;

	return 0;
}


/*
 configure an observer, listening at interface with capturelen
 capturelen 0 means use Observer's default
 */
static int configure_observer(struct v_objects *v, char *interface, int snaplen)
{
	Observer *o=new Observer(interface);

	if(snaplen) {
                if(! o->setCaptureLen(snaplen)) {
			msg(MSG_FATAL, "Observer: wrong snaplen specified, using %d", o->getCaptureLen());
		}
        }
	msg(MSG_INFO, "Observer: using snaplen %d", o->getCaptureLen());
	v->observer=o;

        return 0;
}


/*
 configure a complete filtering process with all sub-packetprocessors
 list is a simple char * retrieved from the config
 */
static int configure_filter(struct v_objects *v, char *list)
{
        return true;

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
	
	routing=iniparser_getvalue(conf, "main", "routing");
	
	while((token=strsep(&l, ","))) {

                /* lookup setting for this particular filter in config */
		if(!(p_settings=iniparser_getvalue(conf, "sampler", token))) {
			msg(MSG_ERROR, "Filter: ignoring filter %s, has no config", token);
			continue;
		}
		
		
                /* make new sub-filter object */
		make_filter_processor(&p, p_settings);
		
		/* and add to filtering process */
               	f->addProcessor(p);
		msg(MSG_INFO, "Filter: adding filter %s with settings %s", token, p_settings);
	}

        v->filter=f;
        free(l);

        return 0;
}

static int make_filter_processor(PacketProcessor **p, char *setting)
{
	PacketProcessor *n;
	char *l, *token;
	int id;
	
	if(!(l=strdup(setting))) {
		return 1;
	}
	
	/* processor type is the first number */
	token=strsep(&l, ",");
		
	switch(id) {
	
	case 0:
		//n=new HookingFilter();
		return 1;
	case 1:
		n=new SystematicSampler();
		break;
	case 2:
		n=new RandomSampler();
		break;
	case 3:
		n=new IPHeaderFilter();
		break;
	default:
		msg(MSG_FATAL, "Filter: cannot make PacketProcessor with ID %d, settings %s", id, setting);
		return 1;
	}

	free(l);
	*p=n;

        return 0;
}
