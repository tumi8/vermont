//
// C++ Interface: node
//
// Description: 
//
//
// Author: Peter Baumann <siprbaum@users.berlios.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef NODE_H
#define NODE_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

class Graph;

class Node {
	friend class Graph;
public:
	Node(Graph *g, unsigned int id);
	~Node();

	unsigned int getID();
	
	
protected:
	void setID(unsigned int);
	
private:
	Graph *graph;
	unsigned int id;
};

#endif
