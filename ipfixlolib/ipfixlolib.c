/*
 This file is part of the ipfixlolib.
 Release under LGPL.

 Header for encoding functions suitable for IPFIX

 Changes by Gerhard Münz, 2006-02-01
   Changed and debugged sendbuffer structure and Co
   Added new function for canceling data sets and deleting fields

 Changes by Christoph Sommer, 2005-04-14
 Modified ipfixlolib-nothread Rev. 80
 Added support for DataTemplates (SetID 4)

 Changes by Ronny T. Lampert, 2005-01
 Changed 03-2005: Had to add a lot of casts for malloc() and friends
 because of stricter C++ checking

 Based upon the original
 by Jan Petranek, University of Tuebingen
 2004-11-12
 jan@petranek.de
 */
#include "ipfixlolib.h"
#include <netinet/in.h>

/* foreign systems */
#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define bit_set(data, bits) ((data & bits) == bits)

static int init_send_udp_socket(char *serv_ip4_addr, int serv_port);
static int ipfix_find_template(ipfix_exporter *exporter, uint16_t template_id, enum ipfix_validity cleanness);
static int ipfix_prepend_header(ipfix_exporter *p_exporter, int data_length, ipfix_sendbuffer *sendbuf);
static int ipfix_init_sendbuffer(ipfix_sendbuffer **sendbufn);
static int ipfix_reset_sendbuffer(ipfix_sendbuffer *sendbuf);
static int ipfix_deinit_sendbuffer(ipfix_sendbuffer **sendbuf);
static int ipfix_init_collector_array(ipfix_receiving_collector **col, int col_capacity);
static int ipfix_deinit_collector_array(ipfix_receiving_collector **col);
static int ipfix_init_send_socket(char *serv_ip4_addr, int serv_port, enum ipfix_transport_protocol protocol);
static int ipfix_init_template_array(ipfix_exporter *exporter, int template_capacity);
static int ipfix_deinit_template_array(ipfix_exporter *exporter);
static int ipfix_update_template_sendbuffer(ipfix_exporter *exporter);
static int ipfix_send_templates(ipfix_exporter* exporter);
static int ipfix_send_data(ipfix_exporter* exporter);

#if 0
static int init_rcv_udp_socket(int lport);
/* NOT USED */

/*
 * Initializes a UDP-socket to listen to.
 * Parameters: lport the UDP-portnumber to listen to.
 * Returns: a socket to read from. -1 on failure.
 */
static int init_rcv_udp_socket(int lport)
{
        int s;
        struct sockaddr_in serv_addr;

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(lport);
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        // create socket
        if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                fprintf(stderr, "error opening socket\n");
                return -1;
        }

        // bind to socket
        if(bind(s, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                perror ("bind failed");
                return -1;
        }
        return s;
}
#endif

/*
 * Initializes a UDP-socket to send data to.
 * Parameters:
 * char* serv_ip4_addr IP-Address of the recipient (e.g. "123.123.123.123")
 * serv_port the UDP-portnumber of the server.
 * Returns: a socket to write to. -1 on failure
 */
static int init_send_udp_socket(char *serv_ip4_addr, int serv_port){

        int s;
        struct sockaddr_in serv_addr;

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons (serv_port);
        serv_addr.sin_addr.s_addr = inet_addr(serv_ip4_addr);

        // create socket
        if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
                msg(MSG_FATAL, "IPFIX: error opening socket, %s", strerror(errno));
                return -1;
        }

        // connect to server
        if(connect(s, (struct sockaddr*)&serv_addr, sizeof(serv_addr) ) < 0) {
                msg(MSG_FATAL, "IPFIX: connect failed, %s", strerror(errno));
                /* clean up */
                close(s);
                return -1;
        }

        return s;
}


/*
 * Initialize an exporter process
 * Allocates all memory necessary.
 * Parameters:
 * sourceID The source ID, to which the exporter will be initialized to.
 * exporter an ipfix_exporter* to be initialized
 */
int ipfix_init_exporter(uint32_t source_id, ipfix_exporter **exporter)
{
        ipfix_exporter *tmp;
        int ret;

        if(!(tmp=(ipfix_exporter *)malloc(sizeof(ipfix_exporter)))) {
                goto out;
        }

        tmp->source_id=source_id;
        tmp->sequence_number = 0;
        tmp->collector_num = 0; // valgrind kindly asked me to inititalize this value JanP
        tmp->collector_max_num = 0;


        // initialize the sendbuffers
        ret=ipfix_init_sendbuffer(&(tmp->data_sendbuffer));
        if (ret != 0) {
                msg(MSG_FATAL, "IPFIX: initializing data sendbuffer failed");
                goto out1;
        }

        ret=ipfix_init_sendbuffer(&(tmp->template_sendbuffer));
        if (ret != 0) {
                msg(MSG_FATAL, "IPFIX: initializing template sendbuffer failed");
                goto out2;
        }

        // intialize the collectors to zero
        ret=ipfix_init_collector_array( &(tmp->collector_arr), IPFIX_MAX_COLLECTORS);
        if (ret !=0) {
                msg(MSG_FATAL, "IPFIX: initializing collectors failed");
                goto out3;
        }

        tmp->collector_max_num = IPFIX_MAX_COLLECTORS;

        // initialize an array to hold the templates.
        // TODO: own function:
        if(ipfix_init_template_array(tmp, IPFIX_MAX_TEMPLATES)) {
                goto out4;
        }
        /*   (**exporter).ipfix_lo_template_maxsize  = IPFIX_MAX_TEMPLATES; */
        /*   (**exporter).ipfix_lo_template_current_count = 0 ; */
        /*   (**exporter).template_arr =  (ipfix_lo_template*) malloc (IPFIX_MAX_TEMPLATES * sizeof(ipfix_lo_template) ); */
        // we have not transmitted any templates yet!
        tmp->last_template_transmission_time=0;
        tmp->template_transmission_timer=IPFIX_DEFAULT_TEMPLATE_TIMER;

        /* finally attach new exporter to the pointer we were given */
        *exporter=tmp;

        return 0;

out4:
        ipfix_deinit_collector_array(&(tmp->collector_arr));
out3:
        ipfix_deinit_sendbuffer(&(tmp->data_sendbuffer));
out2:
        ipfix_deinit_sendbuffer(&(tmp->template_sendbuffer));
out1:
        free(tmp);
out:
        /* we have nothing to free */
        return -1;
}


/*
 * cleanup an exporter process
 */
int ipfix_deinit_exporter(ipfix_exporter *exporter)
{
        // cleanup processes
        int ret;
        // close sockets etc.
        // (currently, nothing to do)

        // free all children

        // deinitialize array to hold the templates.
        ret=ipfix_deinit_template_array(exporter);

        /*   free ( (**exporter).template_arr); */
        /*   (**exporter).template_arr = NULL; */

        // deinitialize the sendbuffers
        ret=ipfix_deinit_sendbuffer(&(exporter->data_sendbuffer));
        ret=ipfix_deinit_sendbuffer(&(exporter->template_sendbuffer));

        // deinitialize the collectors
        ret=ipfix_deinit_collector_array(&(exporter->collector_arr));

        // free own memory
        free(exporter);
        exporter=NULL;

        return 0;
}


/*
 * Add a collector to the exporting process
 * Parameters:
 *  exporter: The exporting process, to which the collector shall be attached
 *  coll_ip4_addr : the collector's ipv4 address (in dot notation, e.g. "123.123.123.123")
 *  coll_port: port number of the collector
 *  proto: transport protocol to use
 * NOTE: this is subject to change, as a collector might use SCTP, which would require 2 connections!
 * Returns: 0 on success or -1 on failure
 */
int ipfix_add_collector(ipfix_exporter *exporter, char *coll_ip4_addr, int coll_port, enum ipfix_transport_protocol proto)
{
        int i=0;
        int searching = TRUE;

        DPRINTF("ipfix_add_collector start\n");

        // check, if exporter is valid
        if(exporter == NULL) {
                msg(MSG_FATAL, "IPFIX: add_collector, exporter is NULL");
                return -1;
        }

        // check if there is a free slot at all:
        if(exporter->collector_num >= exporter->collector_max_num ) {
                msg(MSG_FATAL, "IPFIX: no more free slots for new collectors available, limit %d reached",
                    exporter->collector_max_num
                   );
                return -1;
        }
        DPRINTF("ipfix_add_collector searching\n");

        while (searching && ( i< exporter->collector_max_num) ) {

                DPRINTF("ipfix_add_collector searching %i, i %i \n", searching, i);
                if(exporter->collector_arr[i].valid == FALSE) {
                        // we have found a free slot:
                        /*
                         FIXME: only a quick fix to make that work
                         Must be copied, else pointered data must be around forever
                         Better use binary/u_int32_t representation
                         */
                        strncpy(exporter->collector_arr[i].ipv4address, coll_ip4_addr, 16);
                        exporter->collector_arr[i].port_number = coll_port;
                        exporter->collector_arr[i].protocol = proto;

                        // open the socket: call an own function.
                        exporter->collector_arr[i].data_socket = ipfix_init_send_socket( coll_ip4_addr, coll_port, proto);

                        // error handling, in case we were unable to open the port:
                        if(exporter->collector_arr[i].data_socket < 0 ) {
                                msg(MSG_ERROR, "IPFIX: add collector %s:%i, initializing socket failed",
                                    coll_ip4_addr, coll_port
                                   );
                                return -1;
                        }
                        // currently, the data socket and the template socket are the same.
                        // TODO, when SCTP is added!
                        exporter->collector_arr[i].template_socket = exporter->collector_arr[i].data_socket;

                        // now, we may set the collector to valid;
                        exporter->collector_arr[i].valid = UNCLEAN;

                        // increase total number of collectors.
                        exporter->collector_num++;
                        searching = FALSE;
                }
                i++;
        }

        return 0;
}

/*
 * Remove a collector from the exporting process
 * Parameters:
 *  coll_id: the ID of the collector to remove
 * Returns: 0 on success, -1 on failure
 */
int ipfix_remove_collector(ipfix_exporter *exporter, char *coll_ip4_addr, int coll_port)
{
        // find the collector in the exporter
        int i=0;
        int searching = TRUE;

        while(searching && ( i < exporter->collector_max_num) ) {
                if( ( strcmp( exporter->collector_arr[i].ipv4address, coll_ip4_addr) == 0 )
                   && exporter->collector_arr[i].port_number == coll_port
                  )  {

                        // are template and data socket the same?
                        if( exporter->collector_arr[i].template_socket ==  exporter->collector_arr[i].data_socket ) {
                                close ( exporter->collector_arr[i].data_socket );
                        } else { //close both connections:
                                close (exporter->collector_arr[i].data_socket );
                                close (exporter->collector_arr[i].template_socket );
                        }

                        exporter->collector_arr[i].valid = FALSE;
                        searching = FALSE;
                }
                i++;
        }
        if (searching) {
                msg(MSG_ERROR, "IPFIX: remove_collector, exporter %s not found", coll_ip4_addr);
                return -1;
        }

        exporter->collector_num--;
        return 0;
}

/************************************************************************************/
/* Template management                                                              */
/************************************************************************************/



/*
 * Helper function: Finds a template in the exporter
 * Parmeters:
 * exporter: Exporter to search for the template
 * template_id: ID of the template we search
 * cleanness: search for COMMITED or UNCLEAN templates? May even search for UNUSED templates
 * Returns: the index of the template in the exporter or -1 on failure.
 */

static int ipfix_find_template(ipfix_exporter *exporter, uint16_t template_id, enum ipfix_validity cleanness)
{
        int i=0;
        int searching;

        // first, some safety checks:
        if(exporter == NULL) {
                msg(MSG_ERROR, "IPFIX: find_template, NULL exporter given");
                return -1;
        }
        if(exporter->template_arr == NULL) {
                msg(MSG_ERROR, "IPFIX: find_template, template array not initialized, cannot search for %d", template_id);
                return -1;
        }

        // do we already have a template with this ID?
        // -> update it!
        searching = TRUE;
        while(searching && ( i< exporter->ipfix_lo_template_maxsize) ) {

                if( exporter->template_arr[i].valid == cleanness) {
                        // if we are searching for an existing template, compare the template_id too:
                        if( (cleanness == COMMITED) || (cleanness = UNCLEAN) ) {

                                if(exporter->template_arr[i].template_id == template_id) {
                                        return i;
                                        searching = FALSE;
                                }

                        } else {
                                // we found an unused slot; return the index:
                                return i;
                        }
                }
                i++;
        }

        return -1;
}


/*
 * Remove a template from the exporting process
 * Parameters:
 * exporter: the exporter
 * template_id: ID of the template to remove
 * Returns: 0 on success, -1 on failure
 * This will free the templates data store!
 */
int ipfix_remove_template_set(ipfix_exporter *exporter, uint16_t template_id)
{
        int ret = 0;

        // TODO: maybe, we have to clean up unclean templates too:
        int found_index = ipfix_find_template(exporter,template_id, COMMITED);

        if (found_index >= 0) {
                ret=ipfix_deinit_template_set(exporter, &(exporter->template_arr[found_index]));
        }else {
                msg(MSG_ERROR, "IPFIX: remove_template ID %u not found", template_id);
                return -1;
        }

        return ret;
}

/************************************************************************************/
/* End of Template management                                                       */
/************************************************************************************/



/*
 * Prepends an ipfix message header to the sendbuffer
 *
 * The ipfix message header is set according to:
 * - the exporter ( Source ID and sequence number)
 * - the length of the contained data
 * - the current system time
 * - the ipfix version number
 *
 * Note: the first HEADER_USED_IOVEC_COUNT  iovec struct are reserved for the header! These will be overwritten!
 */
static int ipfix_prepend_header(ipfix_exporter *p_exporter, int data_length, ipfix_sendbuffer *sendbuf)
{

        time_t export_time;
        int ret = 0;
        uint16_t total_length = 0;

        // did the user set the data_length field?
        if (data_length != 0) {
                total_length = data_length + sizeof(ipfix_header);
        } else {
                // compute it on our own:
                // sum up all lengths in the iovecs:
                int i;

                // start the loop with 1, as 0 is reserved for the header!
                for (i = 1; i< sendbuf->current;  i++) {
                        total_length += sendbuf->entries[i].iov_len;
                }

                // add the header lenght to the total length:
                total_length += sizeof(ipfix_header);
        }

        // write the length into the header
        (sendbuf->packet_header).length = htons(total_length);

        // write version number and source ID and sequence number
        (sendbuf->packet_header).version = htons(IPFIX_VERSION_NUMBER);
        (sendbuf->packet_header).source_id = htonl(p_exporter->source_id);
        (sendbuf->packet_header).sequence_number = htonl(p_exporter->sequence_number);

        // get the export time:
        export_time = time(NULL);
        if(export_time == (time_t)-1) {
                // survive
                export_time=0;
                msg(MSG_DEBUG,"IPFIX: prepend_header, time() failed, using %d", export_time);
        }
        //  global_last_export_time = (uint32_t) export_time;
        (sendbuf->packet_header).export_time = htonl((uint32_t)export_time);

        return ret;
}



/*
 * Create and initialize an ipfix_sendbuffer for at most maxelements
 * Parameters: ipfix_sendbuffer** sendbuf pointerpointer to an ipfix-sendbuffer
 */
static int ipfix_init_sendbuffer(ipfix_sendbuffer **sendbuf)
{
        ipfix_sendbuffer *tmp;

        // mallocate memory for the sendbuffer
        if(!(tmp=(ipfix_sendbuffer *)malloc(sizeof(ipfix_sendbuffer)))) {
                goto out;
        }

        tmp->current = HEADER_USED_IOVEC_COUNT; // leave the 0th field blank for the header
        tmp->committed = HEADER_USED_IOVEC_COUNT;
        tmp->marker = HEADER_USED_IOVEC_COUNT;
        tmp->committed_data_length = 0;

        // init and link packet header
        memset(&(tmp->packet_header), 0, sizeof(ipfix_header));
        tmp->entries[0].iov_len = sizeof(ipfix_header);
        tmp->entries[0].iov_base = &(tmp->packet_header);

        // initialize an ipfix_set_manager
	(tmp->set_manager).set_counter = 0;
        (tmp->set_manager).header_iovec = NULL;
        memset(&(tmp->set_manager).set_header_store, 0, sizeof((tmp->set_manager).set_header_store));
        (tmp->set_manager).data_length = 0;

        *sendbuf=tmp;
        return 0;

out1:
        free(tmp);
out:
        return -1;
}

/*
 * reset ipfix_sendbuffer
 * Resets the contents of an ipfix_sendbuffer, so the sendbuffer can again
 * be filled with data.
 * (Present headers are also purged).
 */
static int ipfix_reset_sendbuffer(ipfix_sendbuffer *sendbuf)
{
        if(sendbuf == NULL ) {
                msg(MSG_DEBUG, "IPFIX: trying to reset NULL sendbuf");
                return -1;
        }

        sendbuf->current = HEADER_USED_IOVEC_COUNT;
        sendbuf->committed = HEADER_USED_IOVEC_COUNT;
        sendbuf->marker = HEADER_USED_IOVEC_COUNT;
        sendbuf->committed_data_length = 0;

        memset(&(sendbuf->packet_header), 0, sizeof(ipfix_header));

        // also reset the set_manager!
	(sendbuf->set_manager).set_counter = 0;
        (sendbuf->set_manager).header_iovec = NULL;
        memset(&(sendbuf->set_manager).set_header_store, 0, sizeof((sendbuf->set_manager).set_header_store));
        (sendbuf->set_manager).data_length = 0;

        return 0;
}


/*
 * Deinitialize (free) an ipfix_sendbuffer
 */
static int ipfix_deinit_sendbuffer(ipfix_sendbuffer **sendbuf)
{
        // free the sendbuffer itself:
        free(*sendbuf);
        *sendbuf = NULL;

        return 0;
}


/*
 * initialize array of collectors
 * Allocates memory for an array of collectors
 * Parameters:
 * col: collector array to initialize
 * col_capacity: maximum amount of collectors to store in this array
 */
static int ipfix_init_collector_array(ipfix_receiving_collector **col, int col_capacity)
{
        int i;
        ipfix_receiving_collector *tmp;

        tmp=(ipfix_receiving_collector *)malloc((sizeof(ipfix_receiving_collector) * col_capacity));
        if(!tmp) {
                return -1;
        }

        for (i = 0; i< col_capacity; i++) {
                (tmp[i]).valid = FALSE;
        }

        *col=tmp;
        return 0;
}


/*
 * deinitialize an array of collectors
 * Parameters:
 * col: collector array to clean up
 */
static int ipfix_deinit_collector_array(ipfix_receiving_collector **col)
{
        free(*col);
        *col=NULL;

        return 0;
}


/*
 * Initializes a send socket
 * Parameters:
 * serv_ip4_addr of the recipient in dot notation
 * serv_port: port
 * protocol: transport protocol
 */
static int ipfix_init_send_socket(char *serv_ip4_addr, int serv_port, enum ipfix_transport_protocol protocol)
{
        int sock = -1;

        switch(protocol) {
        case UDP:
                sock= init_send_udp_socket( serv_ip4_addr, serv_port);
                break;

        case TCP:
                msg(MSG_FATAL, "IPFIX: Transport Protocol TCP not implemented");
                break;

        case SCTP:
                msg(MSG_FATAL, "IPFIX: Transport Protocol SCTP not implemented");
                break;

        default:
                msg(MSG_FATAL, "IPFIX: Transport Protocol not supported");
                return -1;
        }

        return sock;
}



/*
 * initialize array of templates
 * Allocates memory for an array of templates
 * Parameters:
 * exporter: exporter, whose template array we'll initialize
 * template_capacity: maximum amount of templates to store in this array
 */
static int ipfix_init_template_array(ipfix_exporter *exporter, int template_capacity)
{
        int i;

        DPRINTF("IPFIX - ipfix_init_template_array with elem %d\n", template_capacity);
        // allocate the memory for template_capacity elements:
        exporter->ipfix_lo_template_maxsize  = template_capacity;
        exporter->ipfix_lo_template_current_count = 0 ;
        exporter->template_arr =  (ipfix_lo_template*) malloc (template_capacity * sizeof(ipfix_lo_template) );

        for(i = 0; i< template_capacity; i++) {
                exporter->template_arr[i].valid = UNUSED;
        }

        return 0;
}


/*
 * deinitialize an array of templates
 * Parameters:
 * exporter: exporter, whose template store will be purged
 */
static int ipfix_deinit_template_array(ipfix_exporter *exporter)
{
        /* FIXME: free all templates in the array!
         This was our memory leak.
         JanP, 2005-21-1
         */
        int i=0;
        int ret = 0;
        
	for(i=0; i< exporter->ipfix_lo_template_maxsize; i++) {
                // try to free all templates:
                ret = ipfix_deinit_template_set(exporter, &(exporter->template_arr[i]) );
                // for debugging:
                DPRINTF("ipfix_deinit_template_array deinitialized template %i with success %i \n", i, ret);
                // end debugging
        }
        free(exporter->template_arr);

        exporter->template_arr = NULL;
        exporter->ipfix_lo_template_maxsize = 0;
        exporter->ipfix_lo_template_current_count = 0;

        return 0;
}


/*
 * Updates the template sendbuffer
 * will be called, after a template has been added or removed
 */
static int ipfix_update_template_sendbuffer (ipfix_exporter *exporter)
{
        int ret;
        int i;

        // first, some safety checks:
        if (exporter == NULL) {
                msg(MSG_DEBUG, "IPFIX: trying to update NULL template sendbuffer");
                return -1;
        }
        if (exporter->template_arr == NULL) {
                msg(MSG_DEBUG,  "IPFIX: update_template_sendbuffer, template store not initialized");
                return -1;

        }

        ipfix_sendbuffer* t_sendbuf = exporter->template_sendbuffer;
	
        // clean the template sendbuffer
        ret=ipfix_reset_sendbuffer(t_sendbuf);

        // place all valid templates to the template sendbuffer
        // could be done just like put_data_field:

        for (i = 0; i < exporter->ipfix_lo_template_maxsize; i++ )  {
                // is the current template valid?
                if(exporter->template_arr[i].valid==COMMITED) {
                        // link the data to the sendbuffer:
                        if (t_sendbuf->current >= IPFIX_MAX_SENDBUFSIZE-2 ) {
                                msg(MSG_ERROR, "IPFIX: template sendbuffer too small to handle more than %i entries", t_sendbuf->current);
                                return -1;
                        }

                        t_sendbuf->entries[ t_sendbuf->current ].iov_base = exporter->template_arr[i].template_fields;
                        t_sendbuf->entries[ t_sendbuf->current ].iov_len =  exporter->template_arr[i].fields_length;
                        t_sendbuf->current++;
                        // total_length += (*exporter).template_arr[i].fields_length;
                        t_sendbuf->committed_data_length +=  exporter->template_arr[i].fields_length;
                }
        } // end loop over all templates

        // that's it!
        return 0;
}


/*******************************************************************/
/* Transmission                                                    */
/*******************************************************************/
/*
 * If necessary, sends all associated templates
 * Parameters:
 *  exporter sending exporting process
 * Return value: 1 on success, -1 on failure, 0 on no need to send.
 */
static int ipfix_send_templates(ipfix_exporter* exporter)
{
        int i;
        int ret=0;
        // determine, if we need to send the template data:
        time_t time_now = time(NULL);

        // has the timer expired?
        if ( (time_now - exporter->last_template_transmission_time) >  exporter->template_transmission_timer) {

                // send the template date

                // hope, the template sendbuffer is valid.

                // update the sendbuffer header, as we must set the export time & sequence number!
                ret = ipfix_prepend_header(exporter,
                                           exporter->template_sendbuffer->committed_data_length,
                                           exporter->template_sendbuffer
                                          );

                if(ret != 0 ) {
                        msg(MSG_ERROR, "IPFIX: sending templates failed");
                        return -1;
                }

                exporter->last_template_transmission_time = time_now;

                // send the sendbuffer to all collectors
                for (i = 0; i < exporter->collector_max_num; i++) {
                        // is the collector a valid target?
                        if ((*exporter).collector_arr[i].valid) {
                                DPRINTF("Sending template to exporter %s:%d\n",
                                        exporter->collector_arr[i].ipv4address,
                                        exporter->collector_arr[i].port_number
                                       );
                                ret=writev(exporter->collector_arr[i].data_socket,
                                           exporter->template_sendbuffer->entries,
                                           exporter->template_sendbuffer->current
                                          );
                                // TODO: we should also check, what writev returned. NO ERROR HANDLING IMPLEMENTED YET!

                        }
                } // end exporter loop

                return 1;
        } // end if export template.
        return 0;
}

/*
 Send data to collectors
 Sends all data committed via ipfix_put_data_field to this exporter.
 Parameters:
 exporter sending exporting process

 Return value:
 on success: 0
 on failure: -1
 */
static int ipfix_send_data(ipfix_exporter* exporter)
{
        int i;
        int ret=0;
        // send the current data_sendbuffer:
        int data_length=0;

        // is there data to send?
        if (exporter->data_sendbuffer->committed_data_length > 0 ) {
                data_length = exporter->data_sendbuffer->committed_data_length;

                // prepend a header to the sendbuffer
                ret = ipfix_prepend_header (exporter, data_length, exporter->data_sendbuffer);
                if (ret != 0) {
                        msg(MSG_ERROR, "IPFIX: ipfix_send_data failed");
                        return -1;
                }

                // send the sendbuffer to all collectors
                for (i = 0; i < exporter->collector_max_num; i++) {

                        // is the collector a valid target?
                        if(exporter->collector_arr[i].valid) {
#ifdef DEBUG
                                DPRINTF("IPFIX: Sending to exporter %s\n", exporter->collector_arr[i].ipv4address);

                                // debugging output of data buffer:
                                DPRINTF("Sendbuffer contains %u bytes\n",  exporter->data_sendbuffer->committed_data_length );
                                DPRINTF("Sendbuffer contains %u fields\n",  exporter->data_sendbuffer->committed );
                                int tested_length = 0;
                                int j;
                                int k;
                                for (j =0; j <  exporter->data_sendbuffer->committed; j++) {
                                        if(exporter->data_sendbuffer->entries[j].iov_len > 0 ) {
                                                tested_length += exporter->data_sendbuffer->entries[j].iov_len;
                                                DPRINTF ("Data Buffer [%i] has %u bytes\n", j, exporter->data_sendbuffer->entries[j].iov_len);

                                                for (k=0; k < exporter->data_sendbuffer->entries[j].iov_len; k++) {
                                                        DPRINTF ("Data at  buf_vector[%i] pos %i is 0x%hx \n", j,k,   *(  (char*) ( (*(*exporter).data_sendbuffer).entries[j].iov_base+k) ) );
                                                }
                                        }
                                }
                                DPRINTF("IPFIX: Sendbuffer really contains %u bytes!\n", tested_length )#;
#endif
				
                                ret=writev( exporter->collector_arr[i].data_socket,
                                           exporter->data_sendbuffer->entries,
                                           exporter->data_sendbuffer->committed
                                          );
                                // TODO: we should also check, what writev returned. NO ERROR HANDLING IMPLEMENTED YET!
                        }
                } // end exporter loop
                ret = 1;
        }  // end if

        // reset the sendbuffer
        ipfix_reset_sendbuffer(exporter->data_sendbuffer);

        // actually, this should return some error handling from writev.
        // TODO
        return ret;
}

/*
 Send data to collectors
 Sends all data committed via ipfix_put_data_field to this exporter.
 If necessary, sends all associated templates.
 Increment sequence number(sequence_number) only here.

 Parameters:
 exporter sending exporting process
 Return value: 0 on success, -1 on failure.
 */
int ipfix_send(ipfix_exporter *exporter)
{
        int ret_templates, ret_data;
        int ret = 0;

        if((ret_templates=ipfix_send_templates(exporter)) > 0) {
                exporter->sequence_number++;
        }

        if((ret_data=ipfix_send_data(exporter)) > 0) {
                exporter->sequence_number++;
        }

        if(ret_templates < 0) {
                msg(MSG_ERROR, "IPFIX: sending templates failed");
                ret = -1;
        }
        if(ret_data < 0) {
                msg(MSG_ERROR, "IPFIX: sending data failed");
                ret = -1;
        }

        return ret;
}

/*******************************************************************/
/* Generation of a data set                                        */
/*******************************************************************/

/*
 * Marks the beginning of a data set
 * Parameters:
 *  exporter: exporting process to send data to
 *   data_length: total length of data put into this set  (network byte order)
 *  template_id: ID of the used template (in network byte order)
 * Note: the set ID MUST match a previously sent template ID! This is the user's responsibility, as the
 * library will not perform any checks.
 */
// parameter data_length will be deprecated soon!!!
// calculate via put datafield.
int ipfix_start_data_set(ipfix_exporter *exporter, uint16_t template_id)
{
	ipfix_set_manager *manager = &(exporter->data_sendbuffer->set_manager);
	unsigned current = manager->set_counter;
    
	// security check
	if(exporter->data_sendbuffer->current != exporter->data_sendbuffer->committed) {
                msg(MSG_ERROR, "IPFIX: start_data_set called twice.");
                goto out;
        }
    
        // check, if there is enough space in the data set buffer
        // the -1 is because, we expect, we want to add at least one data field.
        if(exporter->data_sendbuffer->current >= IPFIX_MAX_SENDBUFSIZE-1 ) {
                msg(MSG_ERROR, "IPFIX: start_data_set sendbuffer too small to handle more than %i entries",
                    exporter->data_sendbuffer->current
                   );
                goto out;
        }

	// check if we do have space for another set header
        if((current + 1) >= IPFIX_MAX_SETS_PER_PACKET ) {
                msg(MSG_ERROR, "IPFIX: start_data_set set_header_store too small to handle more than %i entries",
                    current + 1
                   );
                goto out;
        }

        // write the set id (=template id) to the data set buffer (length will be added by ipfix_end_data_set):
	manager->set_header_store[current].set_id = template_id;

        // link current set header in entries
        exporter->data_sendbuffer->entries[exporter->data_sendbuffer->current].iov_base = &(manager->set_header_store[current]);
        exporter->data_sendbuffer->entries[exporter->data_sendbuffer->current].iov_len = sizeof(ipfix_set_header);

        exporter->data_sendbuffer->current++;
        DPRINTF("start_data_set: exporter->data_sendbuffer->current %i\n", exporter->data_sendbuffer->current);

	// set marker to current in order to avoid deletion of set header with ipfix_cancel_data_fields_upto_marker()
	exporter->data_sendbuffer->marker = exporter->data_sendbuffer->current;

        // initialize the counting of the record's data:
        manager->data_length = 0;

        return 0;

out:
        return -1;
}


/*
 * Marks the end of a data set
 * Parameters:
 *   exporter: exporting process to send data to
 */
int ipfix_end_data_set(ipfix_exporter *exporter)
{
	ipfix_set_manager *manager = &(exporter->data_sendbuffer->set_manager);
	unsigned current = manager->set_counter;
	uint16_t record_length;

        // calculate and store the total length of the record:
        record_length = manager->data_length + sizeof(ipfix_set_header);
	manager->set_header_store[current].length = htons(record_length);

        // update the sendbuffer
        exporter->data_sendbuffer->committed_data_length += record_length;

	// now as we are finished with this set, increase set_counter
	manager->set_counter++;
	
	// update committed 
	exporter->data_sendbuffer->committed = exporter->data_sendbuffer->current;
	exporter->data_sendbuffer->marker = exporter->data_sendbuffer->current;
	    
        return 0;
}


/*
 * Cancel a previously started data set
 * Parameters:
 *   exporter: exporting process to send data to
 */
int ipfix_cancel_data_set(ipfix_exporter *exporter)
{
	ipfix_set_manager *manager = &(exporter->data_sendbuffer->set_manager);
	unsigned current = manager->set_counter;
	int i;

	// security check
	if(exporter->data_sendbuffer->current == exporter->data_sendbuffer->committed) {
                msg(MSG_ERROR, "IPFIX: cancel_data_set called but there is no set to cancel.");
                goto out;
        }
    
        // clean set id and length:
	manager->set_header_store[current].set_id = 0;
        manager->data_length = 0;

        // clean up entries
	for(i=exporter->data_sendbuffer->committed; i<exporter->data_sendbuffer->current; i++) {
	    exporter->data_sendbuffer->entries[i].iov_base = NULL;
	    exporter->data_sendbuffer->entries[i].iov_len = 0;
	}

        exporter->data_sendbuffer->current = exporter->data_sendbuffer->committed;
        exporter->data_sendbuffer->marker = exporter->data_sendbuffer->committed;

        return 0;

out:
        return -1;
}

/*
 * Sets the data field marker to the current position in order to allow deletion of newly added fields
 * Parameters:
 *   exporter: exporting process to send data to
 */
int ipfix_set_data_field_marker(ipfix_exporter *exporter)
{
    exporter->data_sendbuffer->marker = exporter->data_sendbuffer->current;
    return 0;
}

/*
 * Delete recently added fields up to the marker
 * Parameters:
 *   exporter: exporting process to send data to
 */
int ipfix_delete_data_fields_upto_marker(ipfix_exporter *exporter)
{
	ipfix_set_manager *manager = &(exporter->data_sendbuffer->set_manager);
	int i;

	// security check
	if(exporter->data_sendbuffer->current == exporter->data_sendbuffer->committed) {
                msg(MSG_ERROR, "IPFIX: delete_data_fields_upto_marker called but there is no set.");
                goto out;
        }

	// if marker is before current, clean the entries and set current back 
	if(exporter->data_sendbuffer->marker < exporter->data_sendbuffer->current) {
	    for(i=exporter->data_sendbuffer->marker; i<exporter->data_sendbuffer->current; i++) {
		// alse decrease data_length
		manager->data_length -= exporter->data_sendbuffer->entries[i].iov_len;
		exporter->data_sendbuffer->entries[i].iov_base = NULL;
		exporter->data_sendbuffer->entries[i].iov_len = 0;
	    }
	    exporter->data_sendbuffer->current = exporter->data_sendbuffer->marker;
	}

        return 0;

out:
        return -1;
}

/*******************************************************************/
/* Generation of a data template set and option template set       */
/*******************************************************************/

/*
 * Marks the beginning of a data template set
 * Parameters:
 *  exporter: exporting process to associate the template with
 *  template_id: the template's ID (in host byte order)
 *  field_count: number of template fields in this template (in host byte order)
 */
// length not ommited; need it to allocate buffer for template
// template length changes depending on vendor specific stuff.

// gerhard: bei Template alles in Host-Byte-Order. Hier koeen wir auf IOVecs verzichten und die
// Felder direkt hintereinander in den Buffer schreiben. Dabei wandeln wir jeweils in Network-Byte-Order
// um.


/*
 * Will allocate memory and stuff for a new template
 * End_data_template set will add this template to the exporter
 */
int ipfix_start_datatemplate_set (ipfix_exporter *exporter, uint16_t template_id, uint16_t preceding, uint16_t field_count, uint16_t fixedfield_count)
{
        // are we updating an existing template?
        int i;
        int searching;
        int found_index = -1;
	int datatemplate=(fixedfield_count || preceding) ? 1 : 0;

        DPRINTF("ipfix_start_template_set: start\n");
        found_index = ipfix_find_template(exporter, template_id, COMMITED);

        // have we found a template?
        if(found_index >= 0) {
                DPRINTF("ipfix_start_template_set: template found at index %i\n", found_index);
                // we must overwrite the old template.
                // first, clean up the old template:

                // remove the old template from memory!
                exporter->template_arr[found_index].valid = UNUSED;
                free(exporter->template_arr[found_index].template_fields) ;
                exporter->template_arr[found_index].template_fields = NULL;
        } else {
                /* allocate a new, free slot */
                DPRINTF("ipfix_start_template_set: making new template\n");

                searching = TRUE;

                // check if there is a free slot at all:
                if(exporter->ipfix_lo_template_current_count >= exporter->ipfix_lo_template_maxsize ) {
                        msg(MSG_ERROR, "IPFIX: start_template_set has no more free slots for new templates available, not added");
                        // do error handling:
                        found_index = -1;
                        searching = FALSE;
                        return -1;
                }

                DPRINTF("ipfix_start_template_set: found_index: %i,  searching: %i, maxsize: %i\n",
                        found_index, searching, exporter->ipfix_lo_template_maxsize
                       );
                i = 0;

                // search for a free slot:
                while(searching && i < exporter->ipfix_lo_template_maxsize) {

                        if(exporter->template_arr[i].valid == UNUSED ) {
                                // we have found a free slot:

                                // increase total number of templates.
                                exporter->ipfix_lo_template_current_count ++;
                                searching = FALSE;
                                found_index = i;
                                exporter->template_arr[i].template_fields = NULL;
                                // TODO: maybe check, if this field is not null. Might only happen, when
                                // asynchronous threads change the template fields.
                                DPRINTF("ipfix_start_template_set: free slot found at %i \n", found_index);
                        }
                        i++;
                }
        }

        /*
         initialize the slot
         test for a valid slot
         */
        if( (found_index >= 0 ) && ( found_index < exporter->ipfix_lo_template_maxsize ) ) {
                //  int ret;
                char *p_pos;
                char *p_end;

                DPRINTF("ipfix_start_template_set: initializing new slot\n");
                // allocate memory for the template's fields:
                // maximum length of the data: 8 bytes for each field, as one field contains:
                // field type, field length (2*2bytes)
                // and may contain an Enterprise Number (4 bytes)
                // also, reserve 8 bytes space for the header!

                exporter->template_arr[found_index].max_fields_length = 8 * (field_count + fixedfield_count) + (datatemplate ? 12 : 8);
		exporter->template_arr[found_index].fields_length = (datatemplate ? 12 : 8);

                exporter->template_arr[found_index].template_fields = (char*)malloc(exporter->template_arr[found_index].max_fields_length );

                // initialize the rest:
                exporter->template_arr[found_index].valid = UNCLEAN;
                // TODO FIXME : only TRUE and FALSE don't seem to suffice!
                //    (*exporter).template_arr[found_index].valid = FALSE;
                exporter->template_arr[found_index].template_id = template_id;
                exporter->template_arr[found_index].field_count = field_count;

                // also, write the template header fields into the buffer (except the lenght field);

                // beginning of the buffer
                p_pos = exporter->template_arr[found_index].template_fields;
                // end of the buffer
                p_end = p_pos + exporter->template_arr[found_index].max_fields_length;
                // add offset to the buffer's beginning: this is, where we will write to.
                //  p_pos +=  (*exporter).template_arr[found_index].fields_length;

		// set ID is 2 for a template, 4 for a template with fixed fields:
		write_unsigned16 (&p_pos, p_end, datatemplate ? 4 : 2);
                // write 0 to the lenght field; this will be overwritten with end_template
                write_unsigned16 (&p_pos, p_end, 0);
                // write the template ID:
                write_unsigned16 (&p_pos, p_end, template_id); 
                // write the field count:
                write_unsigned16 (&p_pos, p_end, field_count);

                if (datatemplate) {
                        // write the fixedfield count:
                        write_unsigned16 (&p_pos, p_end, fixedfield_count);
                        // write the preceding:
                        write_unsigned16 (&p_pos, p_end, preceding);
                }

                // does this work?
                // (*exporter).template_arr[found_index].fields_length += 8;
                DPRINTF("ipfix_start_template_set: max_fields_len %u \n", exporter->template_arr[found_index].max_fields_length);
                DPRINTF("ipfix_start_template_set: fieldss_len %u \n", exporter->template_arr[found_index].fields_length);
        } else return -1;

        return 0;
}
/*
 * Marks the beginning of an option template set
 * Parameters:
 *  exporter: exporting process to associate the template with
 *  template_id: the template's ID (in host byte order)
 *  scope_length: the option scope length (in host byte oder)
 *  option_length: the option scope length (in host byte oder)
 */
int ipfix_start_optionstemplate_set(ipfix_exporter *exporter, uint16_t template_id, uint16_t scope_length, uint16_t option_length)
{
        msg(MSG_FATAL, "IPFIX: start_optionstemplate_set() not implemented");
        return -1;
}

/*
 * Append field to the exporter's current template set
 * Parameters:
 *  template_id: the id specified at ipfix_start_template_set()
 *  type: field or scope type (in host byte order)
 *        Note: The enterprise id will only be used, if type has the enterprise bit set.
 *  length: length of the field or scope (in host byte order)
 *  enterprise: enterprise type (in host byte order)
 * Note: This function is called after ipfix_start_data_template_set or ipfix_start_option_template_set.
 * Note: This function MAY be replaced by a macro in future versions.
 */
int ipfix_put_template_field(ipfix_exporter *exporter, uint16_t template_id, uint16_t type, uint16_t length, uint32_t enterprise_id)
{
        int found_index;
        /* set pointers to the buffer */
        int ret;
        char *p_pos;
        char *p_end;
	int enterprise_bit_set = ipfix_enterprise_flag_set(type);

        found_index=ipfix_find_template(exporter, template_id,  UNCLEAN);

        /* test for a valid slot */
        if( found_index < 0 || found_index >= exporter->ipfix_lo_template_maxsize ) {
                msg(MSG_DEBUG, "IPFIX: put_template_field,  template ID %d not found", template_id);
                return -1;
        }

        /* beginning of the buffer */
        p_pos = exporter->template_arr[found_index].template_fields;
        // end of the buffer
        p_end = p_pos + exporter->template_arr[found_index].max_fields_length;

        DPRINTF("ipfix_put_template_field: template found at %d\n", found_index);
        DPRINTF("ipfix_put_template_field: A p_pos %p, p_end %p\n", p_pos, p_end);
        DPRINTF("ipfix_put_template_field: max_fields_len %d\n", exporter->template_arr[found_index].max_fields_length);
        DPRINTF("ipfix_put_template_field: fieldss_len %d\n", exporter->template_arr[found_index].fields_length);

        // add offset to the buffer's beginning: this is, where we will write to.
        p_pos += exporter->template_arr[found_index].fields_length;

        DPRINTF("ipfix_put_template_field: B p_pos %p, p_end %p\n", p_pos, p_end);

        if(enterprise_bit_set) {
                DPRINTF("Notice: using enterprise ID %d with data %d\n", template_id, enterprise_id);
        }

        // now write the fields to the buffer:
        ret = write_extension_and_fieldID(&p_pos, p_end, type);
        // write the field length
        ret = write_unsigned16(&p_pos, p_end, length);

        // add the 4 bytes to the written length:
        exporter->template_arr[found_index].fields_length += 4;

        // write the vendor specific id
        if (enterprise_bit_set) {
                ret = write_unsigned32(&p_pos, p_end, enterprise_id);
                exporter->template_arr[found_index].fields_length += 4;
        }

        return 0;
}


/*
   ipfix_start_template_set
   Starts a new template, see ipfix_start_datatemplate_set
*/
int ipfix_start_template_set (ipfix_exporter *exporter, uint16_t template_id,  uint16_t field_count) {
        return ipfix_start_datatemplate_set(exporter, template_id, 0, field_count, 0);
}


/*
   ipfix_put_template_fixedfield
   Append fixed-value data type field to the exporter's current data template set, see ipfix_put_template_field
*/
int ipfix_put_template_fixedfield(ipfix_exporter *exporter, uint16_t template_id, uint16_t type, uint16_t length, uint32_t enterprise_id) {
        return ipfix_put_template_field(exporter, template_id, type, length, enterprise_id);
}


/*
   ipfix_put_template_data
   Append fixed-value data to the exporter's current data template set
*/
int ipfix_put_template_data(ipfix_exporter *exporter, uint16_t template_id, void* data, uint16_t data_length) {
        int found_index;
        /* set pointers to the buffer */
        int ret;
        char *p_pos;
        char *p_end;
	
        int i;

        found_index=ipfix_find_template(exporter, template_id,  UNCLEAN);

        /* test for a valid slot */
        if ( found_index < 0 || found_index >= exporter->ipfix_lo_template_maxsize ) {
                fprintf (stderr, "Template not found. \n");
                return -1;
        }

        ipfix_lo_template *templ=(&(*exporter).template_arr[found_index]);
        templ->max_fields_length += data_length;
        templ->template_fields=(char *)realloc(templ->template_fields, templ->max_fields_length);

        /* beginning of the buffer */
        p_pos =  (*exporter).template_arr[found_index].template_fields;
        // end of the buffer
        p_end = p_pos +  (*exporter).template_arr[found_index].max_fields_length;

        DPRINTF("ipfix_put_template_data: template found at %i\n", found_index);
        DPRINTF("ipfix_put_template_data: A p_pos %u, p_end %u\n", p_pos, p_end);
        DPRINTF("ipfix_put_template_data: max_fields_len %u \n", (*exporter).template_arr[found_index].max_fields_length);
        DPRINTF("ipfix_put_template_data: fieldss_len %u \n", (*exporter).template_arr[found_index].fields_length);

        // add offset to the buffer's beginning: this is, where we will write to.
        p_pos += (*exporter).template_arr[found_index].fields_length;

        DPRINTF("ipfix_put_template_data: B p_pos %u, p_end %u\n", p_pos, p_end);

	for(i = 0; i < data_length; i++) {
		ret = write_octet(&p_pos, p_end, *(((uint8_t*)data)+i) );
	}

        // add to the written length:
        (*exporter).template_arr[found_index].fields_length += data_length;

        return 0;
}

/*
 * Marks the end of a template set
 * Parameters:
 *   exporter: exporting process to send the template to
 * Note: the generated template will be stored within the exporter
 */
int ipfix_end_template_set(ipfix_exporter *exporter, uint16_t template_id)
{
        int found_index;
        char *p_pos;
        char *p_end;

        found_index=ipfix_find_template(exporter, template_id, UNCLEAN);

        // test for a valid slot:
        if ( (found_index < 0 ) || ( found_index >= exporter->ipfix_lo_template_maxsize ) ) {
                msg(MSG_ERROR, "IPFIX: template %d not found", template_id);
                return -1;
        }

        // reallocate the memory , i.e. free superfluous memory, as we allocated enough memory to hold
        // all possible vendor specific IDs.
        ipfix_lo_template *templ=(&exporter->template_arr[found_index]);
        /* sometime I'll fuck C++ with a serious chainsaw - casting malloc() et al is DUMB */
        templ->template_fields=(char *)realloc(templ->template_fields, templ->fields_length);

        /*
         write the real length field:
         set pointers:
         beginning of the buffer
         */
        p_pos = exporter->template_arr[found_index].template_fields;
        // end of the buffer
        p_end = p_pos + exporter->template_arr[found_index].max_fields_length;
        // add offset of 2 bytes to the buffer's beginning: this is, where we will write to.
        p_pos += 2;

        // write the lenght field
        write_unsigned16 (&p_pos, p_end, templ->fields_length);
        // call the template valid
        templ->valid = COMMITED;

        // commit the template buffer to the sendbuffer
        ipfix_update_template_sendbuffer(exporter);

        return 0;
}

/*
 * removes a template set from the exporter
 * Checks, if the template is in use, before trying to free it.
 * Parameters:
 *  exporter: exporting process to associate the template with
 *  template* : pointer to the template to be freed
 * Returns: 0  on success, -1 on failure
 * This is an internal function.
 */
int ipfix_deinit_template_set(ipfix_exporter *exporter, ipfix_lo_template *templ)
{
        // note: ipfix_deinit_template_array tries to free all possible templates, many of them
        // won't be initialized. So you'll get a lot of warning messages, which are just fine...

        if(templ == NULL) {
                DPRINTF("ipfix_deinit_template_set: Cannot free template. Template is already NULL\n!");
                return -1;
        }

        // FIXME: make sure, we get a mutex lock on the templates, or else an other process
        // might try to write to an unclean template!!!

        // first test, if we can free this template
        if ( (templ->valid == COMMITED) || (templ->valid == UNCLEAN )) {
                templ->valid = UNUSED;
                free(templ->template_fields);
                exporter->ipfix_lo_template_current_count--;

        } else {
                DPRINTF("ipfix_deinit_template_set: Cannot free template. Template is UNUSED\n");
                return -1;
        }

        return 0;
}


/* check if the enterprise bit in an ID is set */
int ipfix_enterprise_flag_set(uint16_t id)
{
        return bit_set(id, IPFIX_ENTERPRISE_FLAG);
}


#ifdef __cplusplus
}
#endif
