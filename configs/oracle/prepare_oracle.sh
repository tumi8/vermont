#!/bin/bash

# Need to run as root to create datafiles
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

# Setup oracle env
export ORACLE_BASE=/usr/lib/oracle/xe/app/oracle/product
export ORACLE_HOME=$ORACLE_BASE/10.2.0/server
export LD_LIBRARY_PATH=$ORACLE_HOME/lib:/usr/lib
export PATH=$ORACLE_HOME/bin:/usr/local/bin:/bin:/usr/bin:/usr/X11R6/bin:~/bin

# SQLplus Options needed
OPTS="-S"

# !!! Ajust to need !!!
# <username>/<password>@<connect_identifier>
LOGON="system/ph1lf3hr3@xe"
USER="vermont"
PASS="vermont"
DB="oex"

# Create directory for Datafiles 
mkdir -p /u/oracle/
chmod -R a+rw /u/oracle/

# Setup OracleDB
sqlplus $OPTS $LOGON << EOF
	CREATE TABLESPACE $DB DATAFILE '/u/oracle/$DB.dbf' SIZE 10m AUTOEXTEND ON NEXT 10m MAXSIZE UNLIMITED;
	CREATE USER $USER IDENTIFIED BY $PASS DEFAULT TABLESPACE $DB QUOTA UNLIMITED ON $DB;
	GRANT CREATE SESSION, CREATE TABLE, CREATE VIEW TO $USER;
	GRANT CREATE ANY SEQUENCE TO $USER;	
	GRANT CREATE ANY TRIGGER TO $USER;
	EXIT;
EOF
	
