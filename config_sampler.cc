
int config_sampler(struct v_objects *v, dictionary *d)
{
	/* now configure the template */
	if(!(v->templ=configure_template(
					 atoi(iniparser_getvalue(conf, "sampler", "template_id")),
					 iniparser_getvalue(conf, "sampler", "template")
					)
	    )) {
		msg(MSG_FATAL, "Config: could not configure a template");
		return(1);
	}


	/* FIXME: handle interface = off */
	if(!(v->observer=configure_observer(
					    iniparser_getvalue(conf, "sampler", "interface"),
                                            atoi(iniparser_getvalue(conf, "sampler", "capturelen"))
					   )
	    )) {
		msg(MSG_FATAL, "Config: could not configure an observer(pcap)");
		return(1);
	}


	if(!(v->filter=configure_filter())) {


	}



	/* attach the successfully used config into the main config struct */
	v->v_config=conf;

	return 0;
}


/*
 configure an observer, listening at interface with capturelen
 capturelen 0 means use Observer's default
 */
static Observer * configure_observer(char *interface, int snaplen)
{
	Observer *o=new Observer(interface);

	if(snaplen) {
		if(! o->setCaptureLen(snaplen)) {
			msg(MSG_FATAL, "Observer: wrong snaplen specified, using %d", o->getCaptureLen());
		}
	}
}

/* configure the sampler template from a "," separated list */
static Template * configure_template(uint16_t template_id, char *list)
{
	Template *t;
	char *l, *token;
        int tmpid;
        const ipfix_identifier *id;
	
	/* violating the original string is not nice, so copy */
	if(!(l=strdup(list))) {
		return NULL;
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

	return t;
}

