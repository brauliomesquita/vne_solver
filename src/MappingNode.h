#ifndef MAPNODE_H
#define MAPNODE_H

#include "Node.h"

class MappingNode {
	Node phys;
	Node virt;

	public:
		MappingNode(Node p, Node v);
		void setPhysNode(Node p);
		void setVirtNode(Node v);
		Node getPhysNode();
		Node getVirtNode();
};

#endif /* GC_H */

