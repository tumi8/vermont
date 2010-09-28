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


#ifndef SIGNATURE_H
#define SIGNATURE_H
#define MAX_TOKENSIZE 800
#define ID_STRING_SIZE 50 // Size of Signature ID

#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <string.h>
#include "Utils.h"

/* A Token is the most elementary part of a Bayes Signature,
 * consisting of an ASCII-char* and a corresponding support,
 * i.e. a value 0 < support <=1, that is defined in the token
 * extraction process. */
struct Token {
	char * tokenstring;
	double support;
	int    tokenlength;
};

/* Each Bayes Signature consists of Tokens and a threshold.
 * Furthermore it has an individual ID (the ID of the originating .pcap),
 * and it does belong to a class of signatures, calculated by the refinement
 * process after the actual signature creation. */
struct BayesSignature {
	struct Token ** tokens;
	double threshold;
	char * id;
	/* Each signature belongs to a class, defined by its contained members. */
	struct SignatureClass * classinformation;
	/* Counter of the actual number of tokens */
	int numOfTokens;
	/* The final complete number of contained tokens. Needs to be known for
	 * initialization */
	int maxNumOfTokens;
};

/* Stores the char* - IDs of the members of the same class.
 * The most representative signature of those members can be
 * addressed via the representative, which behaves as an index
 * to the member_ids */
// Gerhard: If the idea is that multiple signatures should belong to one class, this is not how it is implemented!
struct SignatureClass {
	int representative;
	char ** member_IDs;
	/* Number of actually stored members */
	int numOfMembers;
};

struct BayesSignature * new_Signature_i(int , char * filename);
struct BayesSignature * new_Signature_s(const char*, char *);
void destruct_signature(struct BayesSignature *);

struct Token * new_Token(char *, double, int);
void destruct_token(struct Token *);

struct SignatureClass * new_signatureclass();
void destruct_signatureclass(struct SignatureClass *);
void add_classid_to_signatureclass(struct SignatureClass *, char *);

void add_Token(struct BayesSignature *, struct Token *);
void add_threshold(struct BayesSignature *, double);

int token_matches(struct Token *, char *);
char * token_to_string(struct Token *);
char * deescape_string(char*, int *);

void print_signature(struct BayesSignature *);
void print_classmembers(struct BayesSignature *);

#endif

