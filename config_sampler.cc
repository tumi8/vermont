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

static bool configure_observer(struct v_objects *v, char *interface, int snaplen);
static bool configure_template(struct v_objects *v, uint16_t template_id, char *list);
static bool configure_filter(struct v_objects *v, char *list);

/* FIXME: careful freeing of previously allocated resources in case of error */
int configure_sampler(struct v_objects *v)
{
	dictionary *conf=v->v_config;

	/* now configure the template */
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
			    ) ) {

		msg(MSG_FATAL, "Config: could not configure the filter");
                return 1;
	}



	/* attach the successfully used config into the main config struct */
	v->v_config=conf;

	return 0;
}


/* configure the sampler template from a "," separated list */
static bool configure_template(struct v_objects *v, uint16_t template_id, char *list)
{
	Template *t;
	char *l, *token;
        int tmpid;
        const ipfix_identifier *id;
	
	/* violating the original string is not nice, so copy */
	if(!(l=strdup(list))) {
		return false;
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

	return true;
}


/*
 configure an observer, listening at interface with capturelen
 capturelen 0 means use Observer's default
 */
static bool configure_observer(struct v_objects *v, char *interface, int snaplen)
{
	Observer *o=new Observer(interface);

	if(snaplen) {
		if(! o->setCaptureLen(snaplen)) {
			msg(MSG_FATAL, "Observer: wrong snaplen specified, using %d", o->getCaptureLen());
		}
	}

	v->observer=o;

        return true;
}


/*
 configure a complete filtering process with all sub-packetprocessors
 list is a simple char * retrieved from the config
 */
static bool configure_filter(struct v_objects *v, char *list)
{
	char *l, *token;
        PacketProcessor *p;
	Filter *f=new Filter();

	/* violating the original string is not nice, so copy */
	if(!(l=strdup(list))) {
		return false;
	}
	
	while((token=strsep(&l, ","))) {

                /* lookup setting for this particular filter in config */

                /* make new sub-filter object */

		/* and add to filtering process */
                f->addProcessor(p);
	}

        free(l);
	return true;
}
