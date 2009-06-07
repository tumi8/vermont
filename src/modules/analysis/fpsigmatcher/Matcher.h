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


#ifndef MATCHER_H_
#define MATCHER_H_

#include "Signature.h"
#include "KTree.h"
#include <dirent.h>

/* Matcher unites the functionality of the Signature and the KTree classes.
 * Therefore it stores a bunch of signatures in several ways:
 * - as individually addressable BayesSignatures
 * - all together in one big KTree
 * - their tokenwise and classwise mappings */
struct Matcher {
	/* Number of classes stored in this matcher. */
	int numOfClasses;
	/* Simple array of all stored signatures */
	struct BayesSignature ** signatures;
	/* Keyword-tree of the signature tokens. */
	struct KTree * ktree;
	/* Stores the informations needed for mapping
	 * between tokens, signatures and classes. */
	struct Mapper * mapper;
};

/* The mapper contains all information needed to map
 * - tokens to their corresponding signature
 * - a signature to their corresponding class */
struct Mapper {
	/* Array for mapping the tokens of the ktree to their
	 * corresponding class. */
	int * classwise_numOfTokens;
	int complete_numOfTokens;
	/* Explicitly stored supports for a fast mapping
	 * of the matching results. */
	double * supports;
	/* Stores the threshold, the classwise_matching_sums
	 * have to reach for each class. */
	double * classwise_thresholds;
};

struct Matcher * new_matcher(const char *);
void destruct_matcher(struct Matcher *);

struct Mapper * new_mapper(struct Matcher *);
void destruct_mapper(struct Mapper *);

struct BayesSignature ** read_signature_files(const char *, int *);

/* @PROGRAMMER: Free the returned value if no longer needed. */
int * match_single_flow(struct Matcher *, const char *, int, const char*, int);

#endif /* MATCHER_H_ */
