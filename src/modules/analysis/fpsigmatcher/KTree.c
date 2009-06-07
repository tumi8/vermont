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


#include "KTree.h"
struct KTree * new_KTree(){
	struct KTree *ktree;
	ktree = malloc(sizeof(struct KTree));

	/* Initialize root node */
	struct KTreeNode *root;
	root = new_KTreeNode(-1, NULL);

	ktree->root = root;
	ktree->keywords = NULL;
	ktree->numofkeywords_actual = 0;
	ktree->numofkeywords_max = 0;
	ktree->signatureLength = 0;

	return ktree;
}

/** Destructor for the whole KTree */
void destruct_KTree(struct KTree * ktree){
	/* Retrieve all KTreeNodes of the ktree */
	struct KTreeNode * nodes[ktree->signatureLength];

	int i, count_nodes = 0;
	for (i = 0; i < ktree->numofkeywords_actual; ++i) {
		struct Keyword_in_linkedlist * keyword;
		keyword = get_keyword(ktree, i);

		int keyword_length = 0;
		struct KTreeNode * ptr_actualNode;
		ptr_actualNode = keyword->term_node;

		while(ptr_actualNode->ptr_parentnode != NULL){
			keyword_length++;

			/* Add if not yet added, since backtracing retrieves redundant pointers */
			int j, insert = 1;
			for (j = 0; j < count_nodes; ++j) {
				if(ptr_actualNode == nodes[j]) insert = 0;
			}
			if(insert == 1){
				nodes[count_nodes] = ptr_actualNode;
				count_nodes++;
			}
			ptr_actualNode = ptr_actualNode->ptr_parentnode;
		}
	}

	/* Free keywords in linked list and their specific keywords */
	struct Keyword_in_linkedlist * keywords[ktree->numofkeywords_actual];
	for (i = 0; i < ktree->numofkeywords_actual; ++i)
		keywords[i] = get_keyword(ktree, i);

	for (i = 0; i < ktree->numofkeywords_actual; ++i) {
		free(keywords[i]->keyword);
		free((keywords[i]));
	}

	/* Free all ptr_hashtables
	 * Free all ptr_nodes */
	for (i = 0; i < count_nodes; ++i) {
		if(nodes[i]->num_of_term_keywords != 0)
			free(nodes[i]->keywordnumbers);
		free(nodes[i]->childnodes_hashtable);
		free(nodes[i]);
	}

	/* Finally free the whole tree */
	free(ktree);

}

struct KTreeNode * new_KTreeNode(  int singlechar,
									struct KTreeNode *ptr_parentnode){
	struct KTreeNode * node;
	node = malloc(sizeof(struct KTreeNode));

	node->singlechar = singlechar;
	node->num_of_term_keywords = 0;
	node->keywordnumbers = NULL;
	node->ptr_parentnode = ptr_parentnode;
	node->childnodes_hashtable = NULL;

	return node;
}

/* Adds the keyword to the KTree, i.e. the linked list of KTreeNodes.
 * The Keywords it gets are escaped. Thus every char needs to be escaped before it is added.
 * Finally returns a pointer to the terminal node, i.e. the last node. */
struct KTreeNode * add_keyword(struct KTree *ktree, char *escaped_keyword, int escapedkeyword_length){
	int i = 0;
	struct KTreeNode * ptr_actualnode, * ptr_childnode;
	ptr_actualnode = ktree->root;
	int deescapedkeyword_length = 0;

	char * deescaped_keyword;
	deescaped_keyword = deescape_string(escaped_keyword, &deescapedkeyword_length);
	//printf("LENGTH: %i\n", deescapedkeyword_length);
	/** Add keyword to the KTreeNodes
	 * Each keyword is read until given length is reached,
	 * since newlines might be contained */
	while(i < deescapedkeyword_length) {
		int actualchar = (int)deescaped_keyword[i];
		//printf("%i\n", actualchar);
		if(ptr_actualnode->childnodes_hashtable == NULL)
			ptr_childnode = NULL;
		else
			ptr_childnode = find_childnode(ptr_actualnode, actualchar);

		/** Add complete rest of keyword, charwise */
		if(ptr_childnode == NULL){
			ptr_childnode = new_KTreeNode(actualchar, ptr_actualnode);

			/** Create hashvalue for the new childnode and point to the real childnode */
			struct Childnode_hashed *ptr_childnode_hashed;
			ptr_childnode_hashed = malloc(sizeof(struct Childnode_hashed));
			ptr_childnode_hashed->singlechar = actualchar;

			ptr_childnode_hashed->ptr_childnode = ptr_childnode;
			HASH_ADD_INT( 	ptr_actualnode->childnodes_hashtable,
							singlechar, ptr_childnode_hashed );
		}

		ptr_actualnode = ptr_childnode;
		i++;
	}
	free(deescaped_keyword);

	/* Reached the last char */
	ptr_actualnode->num_of_term_keywords++;
	/* Malloc/Realloc memory for the keywords */
	if(ptr_actualnode->keywordnumbers == NULL)
		ptr_actualnode->keywordnumbers = malloc(sizeof(int));
	else
		ptr_actualnode->keywordnumbers = realloc(ptr_actualnode->keywordnumbers, sizeof(int)*ptr_actualnode->num_of_term_keywords);

	/* Add the actual keywordnumber to the list of keywordnumbers */
	ptr_actualnode->keywordnumbers[ptr_actualnode->num_of_term_keywords-1] = ktree->numofkeywords_actual;

	/* Add the full keyword to the KTree */
	struct Keyword_in_linkedlist * ptr_next_keyword,
							  * ptr_actual_keyword;
	/* If no keywords yet, add the first one */
	if(ktree->keywords == NULL){
		ptr_actual_keyword = malloc(sizeof(struct Keyword_in_linkedlist));
		ktree->keywords = ptr_actual_keyword;
		ktree->keywords->keyword = malloc(sizeof(char)*(escapedkeyword_length + 1));
		strcpy(ktree->keywords->keyword, escaped_keyword);
		ktree->keywords->keywordlength = escapedkeyword_length;
		ktree->keywords->term_node = ptr_actualnode;
	/* else add the next one */
	}else{
		ptr_actual_keyword = get_keyword(ktree, (ktree->numofkeywords_actual)-1);
		ptr_next_keyword = malloc(sizeof(struct Keyword_in_linkedlist));
		ptr_actual_keyword->ptr_next_keyword = ptr_next_keyword;
		ptr_next_keyword->keyword = malloc(sizeof(char)*(escapedkeyword_length + 1));
		ptr_next_keyword->term_node = ptr_actualnode;
		strcpy(ptr_next_keyword->keyword, escaped_keyword);
		ptr_next_keyword->keywordlength = escapedkeyword_length;
	}
	ktree->numofkeywords_actual++;
	ktree->signatureLength += escapedkeyword_length;

	return ptr_actualnode;
}

int trace_back_keyword(struct KTree * ktree, int numOfKeyword){
	int ret = 0;
	if(numOfKeyword < ktree->numofkeywords_actual){
		struct Keyword_in_linkedlist * keyword;
		keyword = get_keyword(ktree, numOfKeyword);
		trace_back_keyword_node(keyword->term_node);
		ret = 1;
	}
	return ret;
}

/* Makes a keyword of the Keyword_linkedlist of the KTree available.
 * I.e. it is not read from the KTreeNodes. */
struct Keyword_in_linkedlist * get_keyword(struct KTree *ktree, int index){
	int var = 0;
	struct Keyword_in_linkedlist * ptr_keyword;
	ptr_keyword = ktree->keywords;
	if(index < ktree->numofkeywords_actual)
		if(index > 0)
			while(var < index){
				ptr_keyword = ptr_keyword->ptr_next_keyword;
				var++;
			}
	return ptr_keyword;
}

/* Prints the backtrace path of the keyword, backtracking from the provided terminalNode */
void trace_back_keyword_node(struct KTreeNode * ptr_terminalNode){
	int keyword_length = 0;
	char actChar;
	struct KTreeNode * ptr_actualNode;
	ptr_actualNode = ptr_terminalNode;

	while(ptr_actualNode->ptr_parentnode != NULL){
		keyword_length++;
		actChar = (char)(ptr_actualNode->singlechar);
		printf("%c",actChar);
		ptr_actualNode = ptr_actualNode->ptr_parentnode;
	}
	printf("\n");
}

/* Simple function to check, if the provided string is stored in the KTreeNodes.
 * Returns 1 if exactly the provided string was found, else 0. */
int find_string(struct KTree *ktree, char * string){
	int i = 0, actualchar, out = 1;
	struct KTreeNode 	* ptr_actualnode,
						* ptr_childnode = NULL;
	ptr_actualnode = ktree->root;

	while((int)string[i] != 0 && out == 1){
		actualchar = (int)string[i];

		/* check the KTreeNodes for the given string */
		ptr_childnode = find_childnode(ptr_actualnode, actualchar);
		if(ptr_childnode != NULL){
			ptr_actualnode = ptr_childnode;
			i++;
		}
		else out = 0;
	}
	if(out == 1)
		if(ptr_childnode->num_of_term_keywords == 0) out = 0;

	return out;
}

/* Exact Set Matching Algorithm based on Aho-Corasick, so far without failure links.
 * Counts, how often each keyword occurs. Returns an array of the counted occurrences
 * of each keyword in the string. */
int * match(struct KTree *ktree, const char * string, int length){
	/* The provided string is matched charwise against the sequential KTreeNodes */
	int i = 0, out , actualchar;
	int * matches = malloc(sizeof(int)*ktree->numofkeywords_actual);
	for (i = 0; i < ktree->numofkeywords_actual; ++i) matches[i] = 0;

	int i_tree=0, i_string = 0;
	struct KTreeNode 	* ptr_actualnode,
						* ptr_childnode;

	for (i = 0; i < length; ++i) {
		ptr_actualnode = ktree->root;
		i_tree = i_string;
		out = 1;
		while(i_tree < length && out == 1){
			actualchar = (int)string[i_tree];
			ptr_childnode = find_childnode(ptr_actualnode, actualchar);
			if(ptr_childnode != NULL){
				ptr_actualnode = ptr_childnode;
				if(ptr_actualnode->num_of_term_keywords != 0){
					int j = 0;
					for (j = 0; j < ptr_actualnode->num_of_term_keywords; ++j)
						matches[ptr_actualnode->keywordnumbers[j]]++;
				}

				if(is_terminalnode(ptr_actualnode) == 0)
					i_tree++;
				else out = 0;
			} else out = 0;
		}
		i_string++;
	}

	return matches;
}

struct KTreeNode *find_childnode(struct KTreeNode *ptr_parentnode,
								 int singlechar) {
    struct Childnode_hashed *ptr_childnode_hashed;

    HASH_FIND_INT(	ptr_parentnode->childnodes_hashtable,
					&singlechar,
					ptr_childnode_hashed );  /* ptr_childnode = output pointer */

    struct KTreeNode *ptr_childnode;
    if(ptr_childnode_hashed != NULL)
    	ptr_childnode = ptr_childnode_hashed->ptr_childnode;
    else
    	ptr_childnode = NULL;

    return ptr_childnode;
}

/* Returns 1 if this node is terminal, 0 if it has children */
int is_terminalnode(struct KTreeNode * ptr_actualnode){
	int ret = 0;
	if(ptr_actualnode->childnodes_hashtable == NULL) ret = 1;
	return ret;
}



