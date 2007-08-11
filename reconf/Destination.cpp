#include "Destination.h"
#include <stdexcept>

BaseDestination::~BaseDestination()
{
}

void BaseDestination::receive(Emitable *) {
	throw std::runtime_error("Destination was called with an Emitable\n; you have to overload the fkt\n");
}
