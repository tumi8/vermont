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


#include "Matcher.h"


struct Matcher * new_matcher(const char* signature_directory){
	int j,i;
	/* Read in the signatures of the signature directory */
	int numOf_SignatureClasses = 0; // Gerhard: unclear why this is called number of signatures classes, it is the number of signatures
	struct BayesSignature ** signatures = read_signature_files(signature_directory, &numOf_SignatureClasses);
	/* Initialize the matcher with those read-in signatures */
	struct Matcher * matcher;
	matcher = malloc(sizeof(struct Matcher));

	/* Add to matcher->signatures */
	matcher->signatures = signatures;

	/*  Add to matcher->ktree */
	matcher->ktree = new_KTree();

	for (j = 0; j < numOf_SignatureClasses; ++j){
		for (i = 0; i < matcher->signatures[j]->numOfTokens; ++i){
			//printf("j = %i, Token = \"%s\"\n", j, matcher->signatures[j]->tokens[i]->tokenstring);
			add_keyword(matcher->ktree, matcher->signatures[j]->tokens[i]->tokenstring, matcher->signatures[j]->tokens[i]->tokenlength);
		}
	}

	/* Finally set the integer values */
	matcher->numOfClasses = numOf_SignatureClasses;

	/* Now initialize the matcher->mapper with the subsumed informations from above */
	matcher->mapper = new_mapper(matcher);

	return matcher;
}

void destruct_matcher(struct Matcher * matcher){
	int i;
	for (i = 0; i < matcher->numOfClasses; ++i)
		destruct_signature(matcher->signatures[i]);

	free(matcher->signatures);
	destruct_KTree(matcher->ktree);
	destruct_mapper(matcher->mapper);
	free(matcher);
}

struct Mapper * new_mapper(struct Matcher * matcher){
	int i, complete_numOfTokens = 0;
	for (i = 0; i < matcher->numOfClasses; ++i)
		complete_numOfTokens += matcher->signatures[i]->numOfTokens;

	struct Mapper * mapper;
	mapper = malloc(sizeof(struct Mapper));

	mapper->complete_numOfTokens = complete_numOfTokens;
	mapper->classwise_numOfTokens = malloc(sizeof(int)*matcher->numOfClasses);
	mapper->supports = malloc(sizeof(double)*complete_numOfTokens);
	mapper->classwise_thresholds = malloc(sizeof(double)*matcher->numOfClasses);

	int j, index_token = 0;
	for (i = 0; i < matcher->numOfClasses; ++i) {
		for (j = 0; j < matcher->signatures[i]->numOfTokens; ++j) {
			mapper->supports[index_token] = matcher->signatures[i]->tokens[j]->support;
			index_token++;
		}
		mapper->classwise_numOfTokens[i] = matcher->signatures[i]->numOfTokens;
		mapper->classwise_thresholds[i] = matcher->signatures[i]->threshold;
	}

	return mapper;
}

void destruct_mapper(struct Mapper * mapper){
	free(mapper->classwise_numOfTokens);
	free(mapper->supports);
	free(mapper->classwise_thresholds);
}

/* Gets a directory of signature files and
 * returns a BayesSignature ** and the number of its elements
 * as in_out parameter num_of_signatures */
struct BayesSignature ** read_signature_files(const char * signature_dir, int * num_of_signatures){
	int i;
	/* Read the filenames in that directory, and their number */
		char ** filenames_sig;
		filenames_sig = get_filenames(signature_dir, num_of_signatures);

	/* malloc a BayesSignature ** with the size of num_of_signatures */
		struct BayesSignature ** signatures;
		signatures = (struct BayesSignature **)malloc((*num_of_signatures)*sizeof(struct BayesSignature *));

	/* Fill it with signature data */
		for (i = 0; i < *num_of_signatures; ++i)
			signatures[i] = new_Signature_s(signature_dir, filenames_sig[i]);

	/* Free the filenames */
		for (i = 0; i < *num_of_signatures; ++i)
			free(filenames_sig[i]);
		free(filenames_sig);

	/* Return the final signatures */
		return signatures;
}

/* Returns an int[numOfClasses], which holds a 1 for a matching class-representative signature,
 * and a 0 for a non-matching class-representative signature.
 *
 * The returned value needs to be freed. */
int * match_single_flow(struct Matcher * matcher, const char *flow, int length, const char* flow2, int length2){
	int i, j;
	int * matches_tokenwise;
	matches_tokenwise = match(matcher->ktree, flow, length);
	int* matches_tokenwise2 = match(matcher->ktree, flow2, length2);

	double binary_matches_tokenwise[matcher->mapper->complete_numOfTokens];
	for (i = 0; i < matcher->mapper->complete_numOfTokens; ++i) {
		if(matches_tokenwise[i] != 0 || matches_tokenwise2[i] != 0) {
			binary_matches_tokenwise[i] = 1.0;
		} else
			binary_matches_tokenwise[i] = 0.0;
	}

	/* Converts tokenwise to classwise matches, since only representative signatures
	 * of each class are matched. Does this by using the mapper */
	double classwise_matching_sums[matcher->numOfClasses];
	for (i = 0; i < matcher->numOfClasses; ++i){
		classwise_matching_sums[i] = 0.0;
	}

	int index_tokens = 0;
	for (i = 0; i < matcher->numOfClasses; ++i) {
		for (j = 0; j < matcher->mapper->classwise_numOfTokens[i]; ++j) {
			classwise_matching_sums[i] += (binary_matches_tokenwise[index_tokens]*matcher->mapper->supports[index_tokens]);
			index_tokens++;
		}
	}

	int * matches_classwise;
	matches_classwise = malloc(sizeof(int)*matcher->numOfClasses);
	for (i = 0; i < matcher->numOfClasses; ++i) matches_classwise[i] = 0;

	int k;
	for (i = 0; i < matcher->numOfClasses; ++i) {
		if(classwise_matching_sums[i] >= matcher->mapper->classwise_thresholds[i]) {
			index_tokens = 0;
			for (k = 0; k < matcher->numOfClasses; ++k) {
				for (j = 0; j < matcher->mapper->classwise_numOfTokens[k]; ++j) {
					index_tokens++;
				}
			}

			matches_classwise[i] = 1;
		}
	}

	free(matches_tokenwise);
	free(matches_tokenwise2);

	return matches_classwise;
}

