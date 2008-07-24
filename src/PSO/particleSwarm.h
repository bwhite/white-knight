#ifndef PSO_H
#define PSO_H

#include <iostream>
#include <list>
#include <cstdlib>
#include <cassert>
#include "../configTools/configReader.h"

using namespace std;

typedef struct
{
	char name[100];
	double position;
	double bestLocalPosition;
	double velocity;
	double minValue;
	double maxValue;
}
swarmDimension_t;

class ParticleSwarmAgent
{
private:
	static double bestGlobalScore;
	static double inertialComponent;
	static double cognitiveComponent;
	static double socialComponent;
	static double constrictionFactor;
	static unsigned int updateMode;
	static bool initialized;
	double bestLocalScore;
	bool firstRun;//NOTE Used so that we don't update the position/velocity before we have a chance to run
	static std::list<swarmDimension_t> globalBest;
	double bestGlobalPosition(char * valueName);
	void setGlobalPosition(char *name, double position);
	int nodeOwner;//Used to hold information related to the thread/node running the process, only needed if run concurrently (-1 if unowned)
public:
	ParticleSwarmAgent(std::list<swarmDimension_t> &initialSwarmDim, std::list<vector<string> > &configList);
	std::list<swarmDimension_t> dimensions;//Public so that higher level processes can use its parameters TODO Remove saving to config file + list (put in library)
	void saveToConfig(std::list<vector<string> > &configList);
	void saveToFile(FILE *file);
	void saveHeaderToFile(FILE *file);
	void update(double currentScore);
	void updateVelocityPosition();
	void updateMaxima(double currentScore);
	void unsetOwner()
	{
		nodeOwner = -1;
	}
	void setOwner(int owner)//These 2 are used to keep track of who is running a task if there are multiple threads of execution
	{
		nodeOwner = owner;
	}
	int getOwner()
	{
		return nodeOwner;
	}

};

void XToString (double value, char * string);

template <class T>
int ForceValue(std::list<vector<string> > &configList, const char * valueName, T value)
{
	for (list<vector<string> >::iterator iter = configList.begin(); iter != configList.end(); iter++)
	{
		if ((*iter)[0] == valueName)
		{
			cout << "VALFORCE: " << valueName << " " << value << endl;
			char tempVal[100];//TODO Change this to an STL string compatible version also
			XToString(value, tempVal);
			(*iter)[1] = tempVal;
			return 0;
		}
	}
	cout << "VALFORCE: Keyword not found " << valueName << endl;
	return 1;
}
#endif
