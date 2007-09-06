#include <stdio.h>
#include <string>

#include "configfile.h"
#include "common/msg.h"

#include <sampler/Observer.h>
#include <sampler/PacketSink.h>

#include "cfg/XMLDocument.h"
#include "cfg/XMLElement.h"

#include "cfg/ObserverCfg.h"
#include "cfg/PacketFilterCfg.h"
#include "cfg/QueueCfg.h"

#include "reconf/ConnectionQueue.h"
#include "reconf/FilterModule.h"

#include <algorithm>

ConfigFile::ConfigFile()
{
}


ConfigFile::~ConfigFile()
{

}

void print_recursive(XMLNode* node, std::string s) {

	XMLNode::XMLNodeSet children = node->getChildren();
	XMLNode::XMLNodeSet::iterator iter = children.begin();
	for (; iter != children.end(); iter++) {
		XMLNode* n = *iter;

		XMLElement* e = dynamic_cast<XMLElement*>(n);
		if (e) {
			printf("%s%s:", s.c_str(), e->getName().c_str());

			XMLElement::XMLAttributeSet as = e->getAttributes();
			XMLElement::XMLAttributeSet::iterator ai = as.begin();
			for (; ai != as.end(); ai++) {
				printf(" %s='%s'", (*ai)->getName().c_str(),
						   (*ai)->getValue().c_str());
			}
			printf("\n");

			print_recursive(n, s+ "    ");
		}

		XMLTextNode* t = dynamic_cast<XMLTextNode*>(n);
		if (t && !t->isBlank()) {
			printf(" %s%s\n", s.c_str(), t->getContent().c_str());
		}
	}
}

typedef std::pair< Node*, Cfg*> NodeCfgPair;

/*
static void addNodeAndMapping(Graph* graph, Cfg* cfg, std::map<int, NodeCfgPair >& m)
{
	Node* n = graph->addNode();

	std::map<int, std::pair<Node*, Cfg*> >::iterator it = m.find(cfg->getID());
	if (it != m.end())
		THROWEXCEPTION("Duplicate node id\n");

	m[cfg->getID()] = NodeCfgPair(n, cfg);
}
*/

Graph* ConfigFile::parse(std::string fileName)
{
	map<int, NodeCfgPair > node_map;
	Graph* graph = new Graph();

	XMLDocument* document = XMLDocument::parse_file(fileName);

	XMLElement* root = document->getRootNode();
	if (!root)
		THROWEXCEPTION("%s is an empty XML-Document!", fileName.c_str());

//	msg(MSG_FATAL, "%s:%u: Reading %s", __FILE__, __LINE__, fileName.c_str());

	if (!root->matches("ipfixConfig")) {
		THROWEXCEPTION("Root element does not match \"ipfixConfig\"."
			       " This is not a valid configuration file!");
	}
/*
	XMLElement* obs = dynamic_cast<XMLElement*>(root->getFirstChild("observer"));
	Cfg* obsCfg = new ObserverCfg(obs);
	//Observer* observer   = obsCfg->getInstance();
	addNodeAndMapping(graph, obsCfg, node_map);


	XMLElement* fil = dynamic_cast<XMLElement*>(root->getFirstChild("filter"));
	PacketFilterCfg filterCfg(fil);
	FilterModule* filter = filterCfg.getInstance();
	addNodeAndMapping(graph, &filterCfg, node_map);

	XMLElement* q = dynamic_cast<XMLElement*>(root->getFirstChild("packetQueue"));
	PacketQueueCfg pqCfg(q);
//	ConnectionQueue<Packet>* queue = pqCfg.getInstance();
	addNodeAndMapping(graph, &pqCfg, node_map);


	for (map<int, NodeCfgPair>::iterator it = node_map.begin();
	     it != node_map.end();
	     it++) {
		NodeCfgPair p1 = it->second;
		Node* node_a = p1.first;


		fprintf(stderr, "proceccessing node with id = %u\n", it->first);
		// get the id of the next cfg
		std::vector<unsigned int> next = p1.second->getNext();
		for (std::vector<unsigned int>::const_iterator i = next.begin();
		     i != next.end();
		     i++) {
			NodeCfgPair p2 = node_map[*i];
			Node* node_b = p2.first;
			graph->addEdge(node_a, node_b);

			fprintf(stderr, "connecting nodes %u -> %u\n", it->first, *i);
			p1.second->connectInstances(p2.second);
		}
	}

//	observer->startCapture();


	ConnectionQueue<Packet>* queue = new ConnectionQueue<Packet>();

//	observer->connectTo(filter);


	PacketSink* dummySink = new PacketSink();
	filter->connectTo(queue);
	queue->connectTo(dummySink);




	sleep(10000);
*/
	return NULL;

/*
	XMLElement::XMLAttributeSet as = obs->getAttributes();
	XMLElement::XMLAttributeSet::iterator i = as.begin();

	for (; i != as.end(); i++) {
		XMLAttribute* n = *i;
		printf("obs attr:  %s = %s\n", n->getName().c_str(),
				               n->getValue().c_str());
	}

	XMLNode::XMLSet<XMLTextNode*> textNodes = obs->getTextChildren();
	XMLNode::XMLSet<XMLTextNode*>::iterator it = textNodes.begin();

	printf("TEXT = '%s'\n", obs->getFirstText().c_str());

	for (; it != textNodes.end(); it++) {
		XMLTextNode* t = dynamic_cast<XMLTextNode*>(*it);
		printf("obs  text = '%s'\n", t->getContent().c_str());
	}
	printf("end listing obs\n");

	const XMLNode::XMLNodeSet children = root->getChildren();

	printf("recursive printing\n");
	print_recursive(root, "");
*/
	delete document;

	return NULL;

/*
	for (XMLNode::XMLNodeSet::const_iterator it = children.begin();
	     it != children.end(); it++)
	{
		XMLNode* n = *it;
		printf("  root child = %s\n", n->getName().c_str());
		if (n->matches("observationPoint")) {
			printf("match observationPoint\n");
		} else if (n->matches("meteringProcess")) {
			printf("match meteringProcess\n");
		} else if (n->matches("exportingProcess")) {
			printf("match exportingProcess\n");
		} else if (n->matches("collectingProcess")) {
			printf("match collectingProcess'n");
		} else if (n->matches("vermont_main")) {
			printf("match vermont_main\n");
		} else {
			THROWEXCEPTION("Unkown config element %s", n->getName().c_str());
		}

//		Node* n = graph->addNode(); // Node* n = graph->addNode(cfg);
	}

*/
	return graph;
}
