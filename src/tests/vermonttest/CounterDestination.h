#ifndef COUNTERDESTINATION_H
#define COUNTERDESTINATION_H

/**
 * class which counts incoming elements
 * used for tests where a chain of module is checked
 */
template <class T>
class CounterDestination : public Destination<T>
{
public:
	CounterDestination() :
		count(0)
	{
	}

	virtual void receive(T t)
	{
		t->removeReference();
		count++;
	}

	unsigned int getCount()
	{
		return count;
	}

	void reset()
	{
		count = 0;
	}

private:
	unsigned int count;
};

#endif
