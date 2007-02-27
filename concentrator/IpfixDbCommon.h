/* Some constants that are common to IpfixDbWriter and IpfixDbReader */
/**
 * STARTLEN     : Length of statement for INSERT IN.., CREATE TABL.., CREATE DATA.., SELECT FROM...
 * TABLE_WIDTH  : Length of table name string
 * COL_WIDTH    : Length of the string denotes the name of the single columns
 *                and datatype to store in table
 * INS_WIDTH    : Length of the string for insert statement in depency of count columns
 */

#ifdef DB_SUPPORT_ENABLED

#define STARTLEN         60             
#define TABLE_WIDTH      16
#define COL_WIDTH        40
#define INS_WIDTH        25

#endif
