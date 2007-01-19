#ifndef IPFIXDBWRITER_H
#define IPFIXDBWRITER_H

#include "IpfixDbCommon.h"
#include "rcvIpfix.h"
#include "ipfix.h"
#include "ipfixlolib/ipfixlolib.h"
#include <mysql.h>
#include <netinet/in.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXPORTERID 0

/**
 * MAX_TABLE    : count of buffered tablenames 
 * MAX_EXP_TABLE: count of buffered exporters
 * MAX_STATMENT : count of insertstatement to buffer before they store to database
 */
#define MAX_TABLE         3
#define MAX_EXP_TABLE     3 // increase this value if you use more exporters in parallel


/**
 * Struct stores for each BufEntry TableBuffer[maxTable]
 *  start-, endtime and tablename for the different tables
 */
typedef struct {
        uint64_t startTableTime;
        uint64_t endTableTime;                          
        char TableName[TABLE_WIDTH];
} BufEntry;

/**
 * Store for each expTable ExporterBuffer[maxExpTable]
 * exporterID,srcID and expIP for the different exporters
 */
typedef struct {
        int Id;          /** Id entry of sourcID and expIP in the ExporterTable */
        // TODO: rename this into observationDomainId
        uint32_t observationDomainId;  /** SourceID of  the exporter monitor */
        uint32_t  expIp; /** IP of the exporter */
} ExpTable;

/** 
 * Store the single statements for insert in a buffer until statemReceived is equal maxstatemt   
 */
typedef struct {
        int statemReceived;                /**counter of insert into statements*/
        char** statemBuffer;               /**buffer  of char pointers to store the insert statements*/
	int maxStatements;
} Statement;

/** 
 * makes a buffer for the different tables and the different exporters
 */
typedef struct {
        int countCol;                            /**counter of columns*/
        int countBuffTable;                      /**counter of buffered table names*/
        BufEntry tableBuffer[MAX_TABLE];         /**buffer to store struct BufEntry*/             
        int countExpTable;                       /**counter of buffered exporter*/
        ExpTable exporterBuffer[MAX_EXP_TABLE];  /**buffer to store struct expTable*/
        Statement* statement;                    /**pointer to struct Statement*/
} Table;        

/**
 * IpfixDbWriter powered the communication to the database server
 * also between the other structs
 */
typedef struct {
        const char* hostName;        /** Hostname*/
        const char* dbName;          /**Name of the database*/
        const char* userName;        /**Username (default: Standarduser) */
        const char* password ;       /** Password (default: none) */
        unsigned int portNum;        /** Portnumber (use default) */
        const char* socketName;      /** Socketname (use default) */
        unsigned int flags;          /** Connectionflags (none) */
        MYSQL* conn;                 /** pointer to connection handle */       
        Table* table;                /**pointer to struct Table*/
        SourceID srcId;              /**Exporter default SourceID */
} IpfixDbWriter;


int initializeIpfixDbWriters();
int deinitializeIpfixDbWriters();

int startIpfixDbWriter(IpfixDbWriter* ipfixDbWriter);
int stopIpfixDbWriter(IpfixDbWriter* ipfixDbWriter);
int destroyIpfixDbWriter(IpfixDbWriter*  ipfixDbWriter);

IpfixDbWriter* createIpfixDbWriter(const char* hostName, const char* dbName,
                                   const char* userName, const char* password,
                                   unsigned int port, uint16_t observationDomainId,
                                   int maxStatements);

CallbackInfo getIpfixDbWriterCallbackInfo(IpfixDbWriter* ipfixDbWriter);

#ifdef __cplusplus
}
#endif

#endif


