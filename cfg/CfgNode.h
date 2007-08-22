#ifndef CFGNODE_H_
#define CFGNODE_H_

#include "Cfg.h"
#include "Node.h"

class CfgNode 
	: public Node
{
public:
	CfgNode(Graph* g, unsigned int id);
	virtual ~CfgNode();

	void addCfg(Cfg*);
	Cfg* getCfg() { return cfg; }
private:
	Cfg* cfg;
};

#endif /*CFGNODE_H_*/
