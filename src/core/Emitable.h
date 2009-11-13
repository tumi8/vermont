#ifndef EMITABLE_H
#define EMITABLE_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/
class Emitable {
public:
	virtual ~Emitable() { };
};

/* This class is here to mark Modules to be no Source/Destination
 * A module has to be from type Source and Destination, but e.g. Observer is _only_
 * a Source and no Destination
 * So we derive Observer from Destination<NullEmitable*>, and all our other wrapper classes
 * work as expected.
 */
class NullEmitable: public Emitable
{
public:
	NullEmitable(): Emitable() { };
	
	inline void addReference(int count = 1) { };
	inline void removeReference() { };
};

#endif

