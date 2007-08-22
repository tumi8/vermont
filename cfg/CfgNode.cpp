#include "CfgNode.h"

CfgNode::CfgNode(Graph* g, unsigned int id)
	: Node(g, id), cfg(NULL)
{
}

CfgNode::~CfgNode()
{
}

void CfgNode::addCfg(Cfg* config)
{
	cfg = config;
}
