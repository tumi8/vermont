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

#ifndef KTREE_H_
#define KTREE_H_

#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <string.h>
#include "uthash.h"
#include "Signature.h"

/* Hashable pointer to a childnode */
struct Childnode_hashed {
    int singlechar;					/* hash-key; int of ASCII */
    struct KTreeNode *ptr_childnode;/* pointer to the real child node */
    UT_hash_handle hh;      		/* makes this structure hashable */
};

/* Contains pointers to the KTreeNodes, that correspond to the specific
 * ASCII-char. Since its static, most of them are NULL */
//struct Childnodes_hashArray {
//	struct KTreeNode * character[256];
//};

/* KTreeNode */
struct KTreeNode {
	int singlechar;						/* stored ASCII-value of char */
	/* If this is a terminal node, the number of terminal keywords
	 * stored in keywordnumbers is kept here */
	int num_of_term_keywords;
	/* If this is a terminal node, the numbers of the keywords are stored here.
	 * There may occurr the same token several times, since one ktree may hold
	 * several signatures. */
	int * keywordnumbers;
	struct KTreeNode *ptr_parentnode;	/* pointer to parent node */
	struct Childnode_hashed *childnodes_hashtable; /* hashed pointers to child nodes */
};

/* A Keywordtree KTree consists of
 * - a KTreeNode root, with a NULL-Pointer as *parentNode, and an empty singlechar
 * - direct pointers to the terminal nodes of the tree, for fast extraction of the contended keywords
 * - a counter for the terminal nodes, since they are stored dynamically
 * - a counter for the char-wise nodes, since they are stored dynamically
 */
struct KTree {
	struct KTreeNode *root;
	struct Keyword_in_linkedlist *keywords;
	int numofkeywords_actual;
	int numofkeywords_max;
	/* The sum of the lengths of all keywords */
	int signatureLength;

};

struct Keyword_in_linkedlist {
	char * keyword;
	int keywordlength;
	struct KTreeNode * term_node;
	struct Keyword_in_linkedlist * ptr_next_keyword;
};

struct KTree * 		new_KTree();
struct KTreeNode * 	new_KTreeNode(int, struct KTreeNode *);

void 				destruct_KTree(struct KTree *);

// @PROGRAMMER Free the returned value if no longer needed
int * 				match(struct KTree *, const char *, int);

struct KTreeNode * 	add_keyword(struct KTree *, char *, int);
void 				add_childnode(struct KTreeNode *, int);

struct Keyword_in_linkedlist *get_keyword(struct KTree*, int);

int 				find_string(struct KTree *, char*);
struct KTreeNode *	find_childnode(struct KTreeNode *, int);
int 				trace_back_keyword(struct KTree *, int);
void 				trace_back_keyword_node(struct KTreeNode *);
int 				is_terminalnode(struct KTreeNode *);

#endif /* KTREE_H_ */
