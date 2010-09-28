/*
 * VERMONT
 * Copyright (C) 2009 Informatik 7 Uni-Erlangen <limmer@cs.fau.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include "Utils.h"
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>

#define MAX_SIZE_OF_FILENAME 256

/** Returns the double value of a string that holds a double,
 * either as as comma separated value or separated by a period */
double get_double(char string[]){
	double ret = 0;
	if(strchr(string, '.') != NULL){
		// '.' found
		ret = atof(string);
	}else{
		int 	pre_comma_i = -1.0,
			post_comma_i = -1.0;
		char * 	tokens[2];
		char 	separator[] = ",. ";
		strtoken(string, separator, tokens);
		//printf("STRINGS: %s  ,  %s\n", tokens[0], tokens[1]);

		//sscanf(string, "%d %d", &pre_comma, &post_comma);

		pre_comma_i = atoi(tokens[0]);
		post_comma_i = atoi(tokens[1]);

		//printf("INTS: %i  ,  %i\n", pre_comma_i, post_comma_i);
		int sizeOfToken1 = 0;
		int i = 0;
		while(tokens[1][i] != 0){
			sizeOfToken1++;
			i++;
		}

		double 	pre_comma_d = (double)pre_comma_i,
			post_comma_d = (double)post_comma_i;

		// Check the number of deleted 0's in post_comma
		while(sizeOfToken1 > 1){
			post_comma_d *= 0.1;
			sizeOfToken1--;
		}

		ret = pre_comma_d + post_comma_d;
	}
	return ret;
}

int strtoken(char *str, char *separator, char *tokens[]){
	int i = 0;
	tokens[0] = strtok(str, separator);
	while ( tokens[i] ) {
		i++;
		tokens[i] = strtok(NULL, separator);
	}
	return i;
}

int get_length(char * string){
	int k = 0;
	while(string[k] != 0) k++;
	return k;
}

char ** get_filenames(const char * directory, int * num_of_files) {
	int i = 0;
	DIR *pDIR;
	struct dirent *pDirEnt;
	struct stat dirEntStat;
	char filename[256];

	/* Open the current directory */

	pDIR = opendir(directory);

	if ( pDIR == NULL ) {
		fprintf( stderr, "%s %d: opendir() failed (%s)\n",
				__FILE__, __LINE__, strerror( errno ));
		exit( -1 );
	}

	/* Count filenames */
	int filecounter = 0;
	pDirEnt = readdir( pDIR );
	while ( pDirEnt != NULL ) {
		/* Skip hidden entries starting with dot */
		if( pDirEnt->d_name[0] != '.' ) {
			/* Skip non-regular files (e.g. directories) */ 
			if ( (snprintf(filename, sizeof(filename), "%s/%s", directory, pDirEnt->d_name) < sizeof(filename))
					&& (stat(filename, &dirEntStat) == 0) && (S_ISREG(dirEntStat.st_mode)) ) {
				//printf( "FOUND: %s\n", pDirEnt->d_name );
				filecounter++;
			}
		}
		pDirEnt = readdir( pDIR );
	}

	*num_of_files = filecounter;

	//printf("\n%i relevant files found.\n", filecounter);
	/* Get each directory entry from pDIR and store its name
	 * in an array */
	char ** filenames;
	filenames = (char **)malloc(sizeof(char*)*filecounter);
	for (i = 0; i < filecounter; ++i)
		filenames[i] = (char*)malloc(sizeof(char)*MAX_SIZE_OF_FILENAME);

	// Rewind??
	rewinddir(pDIR);

	i = 0;
	pDirEnt = readdir( pDIR );
	while ( pDirEnt != NULL ) {
		//printf( "%s\n", pDirEnt->d_name );
		/* Skip hidden entries starting with dot */
		if( pDirEnt->d_name[0] != '.' ) {
			/* Skip non-regular files (e.g. directories) */ 
			if ( (snprintf(filename, sizeof(filename), "%s/%s", directory, pDirEnt->d_name) < sizeof(filename))
					&& (stat(filename, &dirEntStat) == 0) && (S_ISREG(dirEntStat.st_mode)) ) {
				snprintf(filenames[i], MAX_SIZE_OF_FILENAME-1, "%s", pDirEnt->d_name);
				//printf( "ADDED: %s\n", filenames[i] );
				i++;
			}
		}
		pDirEnt = readdir( pDIR );
	}

	/* Release the open directory */
	closedir( pDIR );

	return filenames;
}



