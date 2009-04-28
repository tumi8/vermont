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


#include "Signature.h"
#include <errno.h>

/** Empty constructor for synthetic construction of a new signature */
struct BayesSignature * new_Signature_i(int maxNumOfTokens, char * filename){

	struct BayesSignature * signature;
	signature = (struct BayesSignature *)malloc(sizeof(struct BayesSignature));
	// Allocate memory for the pointers to the actual Tokens
	signature->tokens = (struct Token **)malloc(maxNumOfTokens*sizeof(struct Token *));

	signature->id = malloc(sizeof(char)*(ID_STRING_SIZE));
	filename[ID_STRING_SIZE] = 0;
	strcpy(signature->id, filename);
	signature->classinformation = new_signatureclass();
	signature->numOfTokens = 0;
	signature->maxNumOfTokens = maxNumOfTokens;
	signature->threshold = -1.0;

	return signature;
}

/* Constructor for a signature, based on a the full filename (i.e. signature.sig) */
struct BayesSignature * new_Signature_s(const char * absolute_path, char * filename_sig){
	FILE * fp;
	char line[MAX_TOKENSIZE] = "";
	int numOfLines = 0;

	/** Open file */
	char 	absolute_filename[300] = "";
	snprintf(absolute_filename,299, "%s/%s", absolute_path, filename_sig);

	if( (fp=fopen(absolute_filename, "r")) == NULL) {
	  printf("\nCannot open file \"%s\"\n", absolute_filename);
	  perror(NULL);
	  exit(1);
	}

	/** Check filesize for content allocation
	 *  and count the number of tokens */
	int numOfTokens = 0;
	while(!feof(fp)) {
		if(fgets(line, MAX_TOKENSIZE-1, fp))
			numOfLines++;

		if(strncmp(line, "TOKEN", 5) == 0)
			numOfTokens++;
	}

	/** Allocate Memory for the **file and for the tokens and supports */
	int i = 0;
	char ** lines = (char **)malloc(numOfLines * sizeof(char *));

	/** Read file content into char** */
	i = 0;
	rewind(fp);

	while(!feof(fp))
		  if(fgets(line, MAX_TOKENSIZE-1, fp)){
			  lines[i] = (char *)malloc(MAX_TOKENSIZE*sizeof(char));
			  strcpy(lines[i], line);
			  i++;
		  }

    fclose(fp);

    /** Read the specific supports, token and threshold out of the file */
	double 	threshold = 0.0,
			support = 0.0;
	char 	temp_token[MAX_TOKENSIZE] = "";
	char 	temp_id[ID_STRING_SIZE] = "";
	int 	get_signatureid = 0, get_classmembers = 0,
			get_token = 0, get_support = 0, get_threshold = 0,
			token_length = 0;
	struct Token * token;
	struct BayesSignature * signature;
	signature = new_Signature_i(numOfTokens, filename_sig);
	for (i = 0; i < numOfLines; i++) {
		if(get_signatureid == 1){
			strcpy(temp_id, lines[i]);
			temp_id[ID_STRING_SIZE] = 0;
			strcpy(signature->id, temp_id);
			get_signatureid = 0;
		}

		if(get_classmembers == 1 && !strncmp(lines[i], "TOKEN", 5) == 0){
			/* Read the next classmemberID */
			add_classid_to_signatureclass(signature->classinformation, lines[i]);
		}

		if(get_token == 1 && !strncmp(lines[i], "SUPPORT", 7) == 0){
			int j = 0;
			while(lines[i][j] != 0){
				token_length++;
				j++;
			}
			strcat(temp_token,lines[i]);
		}

		if(get_support == 1){
			support = get_double(lines[i]);

			/* Remove last \n of temp_token */
			temp_token[token_length-1] = 0;

			token = new_Token(temp_token, support, token_length-1);
			add_Token(signature, token);
			temp_token[0] = 0;
			token_length = 0;
			get_support = 0;
		}

		if(get_threshold == 1)
			threshold = get_double(lines[i]);

		if(strncmp(lines[i], "SIGNATUREID", 11) == 0)
			get_signatureid = 1;

		if(strncmp(lines[i], "CLASSMEMBERS", 12) == 0)
			get_classmembers = 1;

		if(strncmp(lines[i], "TOKEN", 5) == 0){
			get_token = 1;
			get_classmembers = 0;
		}

		if(strncmp(lines[i], "SUPPORT", 7) == 0){
			get_support = 1;
			get_token = 0;
		}

		if(strncmp(lines[i], "THRESHOLD", 9) == 0)
			get_threshold = 1;
	}

	signature->threshold = threshold;

	for(i = 0; i< numOfLines; i++)
		free(lines[i]);
	free(lines);

	return signature;
}

struct SignatureClass * new_signatureclass(){
	struct SignatureClass * class;
	class = malloc(sizeof(struct SignatureClass));
	class->member_IDs = NULL; //(char**) malloc(sizeof(char *));
	class->numOfMembers = 0;
	// The first element of the member_IDs is always the representative
	class->representative = 0;
	return class;
}

void destruct_signatureclass(struct SignatureClass * class){
	int i;
	for (i = 0; i < class->numOfMembers; ++i)
		free(class->member_IDs[i]);
	free(class);
}

void add_classid_to_signatureclass(struct SignatureClass * class, char * new_memberID){
	class->member_IDs = realloc(class->member_IDs, sizeof(char *) * (class->numOfMembers + 1));
	class->member_IDs[class->numOfMembers] = malloc(sizeof(char) * ID_STRING_SIZE);
	new_memberID[ID_STRING_SIZE] = 0;
	int id_length = 0;
	while(new_memberID[id_length] != 0) id_length++;

	new_memberID[id_length-1] = 0;
	strcpy(class->member_IDs[class->numOfMembers], new_memberID);
	class->numOfMembers++;
}

void destruct_signature(struct BayesSignature * signature){
	int i;
	for (i = 0; i < signature->numOfTokens; ++i) {
		destruct_token(signature->tokens[i]);
	}
	free(signature->tokens);
	free(signature->id);
	destruct_signatureclass(signature->classinformation);
	free(signature);
}

struct Token * new_Token(char * tokenstring, double support, int tokenlength){
	// Allocate memory of the actual Token
	struct Token * token;
	token = (struct Token *)malloc(sizeof(struct Token));

	token->tokenstring = (char *)malloc(MAX_TOKENSIZE * sizeof(char));
	strcpy(token->tokenstring,tokenstring);
	token->support = support;
	token->tokenlength = tokenlength;
	return token;
}

void destruct_token(struct Token * token){
	free(token->tokenstring);
	free(token);
}

void add_Token(struct BayesSignature * signature, struct Token * token){
	signature->tokens[signature->numOfTokens] = token;
	signature->numOfTokens++;
}

void add_threshold(struct BayesSignature * signature, double threshold){
	signature->threshold = threshold;
}

int token_matches(struct Token *token, char * flow){
	return strcmp((*token).tokenstring, flow);;
}

char* token_to_string(struct Token *token){
	return (*token).tokenstring;;
}

/* Returned char* needs to be freed.
 * length is an in/out-Parameter */
char * deescape_string(char *s, int *length){
 char *t = malloc(sizeof(char)*MAX_TOKENSIZE);
 char buf[5] = "0x00";
 unsigned int j, i, k;

 for (k = 0, i = 0; i < strlen(s); i++) {
     if (s[i] != '%') {
         t[k++] = s[i];
         continue;
     }

     /* Skip truncated sequence */
     if (strlen(s) - i < 2)
         break;

     buf[2] = s[++i];
     buf[3] = s[++i];
     sscanf(buf, "%x", (unsigned int *) &j);
     t[k++] = (char) j;
 }
 *length = k;
 return t;
}
void print_signature(struct BayesSignature * signature){
	int i = 0;
	printf("BAYES SIGNATURE\n");
	printf("ID\n%s\n", signature->id);
	for (i = 0; i < signature->maxNumOfTokens; ++i) {
		printf("TOKEN\n");
		printf("%s\n",signature->tokens[i]->tokenstring);
		printf("SUPPORT\n%1.5f\n", signature->tokens[i]->support);
	}
	printf("THRESHOLD\n%f\n", signature->threshold);
}

void print_classmembers(struct BayesSignature * signature){
	int i;
	for (i = 0; i < signature->classinformation->numOfMembers; ++i)
		printf("%s\n", signature->classinformation->member_IDs[i]);
}
