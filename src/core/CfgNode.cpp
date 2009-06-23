#include "CfgNode.h"

CfgNode::CfgNode(Graph* g, unsigned int id)
	: Node(g, id), cfg(NULL)
{
}

CfgNode::~CfgNode()
{
	if (cfg)
		delete cfg;
}

void CfgNode::setCfg(Cfg* config)
{
	cfg = config;
}
