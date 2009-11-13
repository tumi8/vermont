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


#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <string.h>

#include "Matcher.h"

void convert_char(char a){
	int i = (int)a;
	printf("%i\n",i);
}

void test1(){
	struct KTree *ktree;
	ktree = new_KTree();

	struct KTreeNode * ptr_addedKeyword;
	ptr_addedKeyword = add_keyword(ktree, "01", 2);
	ptr_addedKeyword = add_keyword(ktree, "012", 3);
	ptr_addedKeyword = add_keyword(ktree, "012", 3);
	char * string_broken = "012\n3";
	ptr_addedKeyword = add_keyword(ktree, string_broken, 5);

	trace_back_keyword(ktree, 3);

	destruct_KTree(ktree);
}

void test1_5(){
	struct KTree *ktree;
	ktree = new_KTree();

	struct KTreeNode * ptr_addedKeyword;
	ptr_addedKeyword = add_keyword(ktree, "bist", 4);
	ptr_addedKeyword = add_keyword(ktree, "ist", 3);
	ptr_addedKeyword = add_keyword(ktree, "ist", 3);

	ptr_addedKeyword = add_keyword(ktree, "abcdef", 6);

	printf("find_string: %i\n", find_string(ktree, "012"));

	int * matches;
	char *  flow = "istn\n \0\12\n30130123\n012313230103";
	matches = match(ktree, flow, strlen(flow));

	int i;
	for (i = 0; i < ktree->numofkeywords_actual; ++i)
		printf("%i : ", matches[i]);
	printf("\n");

	destruct_KTree(ktree);
}


void test2(){
	struct BayesSignature * signature;
	char * signature_directory = "/home/liquid/eclipseworkspace/SignatureMatcher";
	signature = new_Signature_s(signature_directory, "real_signature1");
	print_signature(signature);
	destruct_signature(signature);
}

void test3(){
	struct BayesSignature * signature;
	signature = new_Signature_i(2, "6f0c36d455f86922ae69808d00bcbadd");

	struct Token * t1, * t2;
	t1 = new_Token("test", 0.7, 4);
	t2 = new_Token("tada", 0.6, 4);
	add_Token(signature, t1);
	add_Token(signature, t2);
	add_threshold(signature, 2.04);

	print_signature(signature);
}

void test4(){
	int a;
		a = 6;
		int * ptr_a, * ptr_b;
		ptr_a = &a;
		ptr_b = &a;
		printf("%p - %p - %i\n",ptr_a, ptr_b, ptr_a == ptr_b);
}

void test6(){
	int a = 10;
	int *b, *c;
	b = &a;
	c = b;
	printf("%i\n",*c);
	int i;
	for (i = 0; i < *c; ++i) {
		printf(".");
	}
}

void test7(){
	char file[200] = "/home/liquid/eclipseprojects/botzilladata/signatures/test/1c1bc117650566267c6ef0a4138a57a5.sig";
	FILE * fp;
	if( (fp=fopen(file, "r")) == NULL) {
		  printf("Cannot open file.\n");
		  exit(1);
		}else printf("Opening file ok.\n");
}

void test8(){
	char wort[10] = "";
	strcpy(wort, "12345\n");
	printf("%i\n", get_length(wort));
	char wort2[6] = "";
	strncpy(wort2, wort, 5);
	printf("%i\n", get_length(wort2));
}

void test9(){
	double d;
	d = atof("0.0234");
	printf("D: %f\n", d);

}

/* Testing the final vermont workflow */
void test_final_vermont_workflow(){
	int i, j;
	// Enter the absolute or relative path to the signatures thou shalt match with.
	char * signature_directory = "./signatures";

	printf("\nInitializing signaturematcher ... ");
	struct Matcher * signaturematcher = new_matcher(signature_directory);
	printf("Done.\n");

	// Define the flow thou shalt match upon.
	char * flow = "dies ist ein kleiner und doch guter test .üovfisnr0q9832ur0ocdijwaoercú32qruq3222cxjfsopijreif7rq203	iß3+2ai p09u ppppppu4esrrrr4crss8cp0s9ewy";

	int runs = 10000;

	int * results = NULL;
	for (j = 0; j < runs; ++j) {
		results = match_single_flow(signaturematcher, flow, strlen(flow));

		for (i = 0; i < signaturematcher->numOfClasses; ++i) {
			if (results[i] == 1)
			{}//printf("match: %s\n", signaturematcher->signatures[i]->id);
		}
		free(results);
	}

	destruct_matcher(signaturematcher);
}

int main(void) {
	test_final_vermont_workflow();
	//test1_5();
	return EXIT_SUCCESS;
}
