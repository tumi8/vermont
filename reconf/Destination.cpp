#include "Destination.h"
#include <stdexcept>

Destination::~Destination()
{
}

void Destination::receive(Emitable *) {
	throw std::runtime_error("Destination was called with an Emitable\n; you have to overload the fkt\n");
}
