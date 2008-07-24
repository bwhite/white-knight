/*
Particle Swarm Optimization - Brandyn White
This is used to find "good" settings for the background subtraction given a data set with manually made truth images of idea difference images that can be simply modified difference images from the background subtraction.  White is fg, black is bg, grey is ignore/shadow/etc.  PSO is a method of creating a population that goes through timesteps at which point their current positions fitness is found, the interesting thing about this method is that it takes into account the current optimal solution to the problem which others will generally converge to based on the PSO formula weights.  The main advantage and thus reasons for using this method over genetic algorithms is that each test of fitness is expensive, and we want to generally converge to a position in the hyperspace.  While GA's work well for bit strings, their usefulness is limited when only real valued numbers are used as each parameter as the meaning of "crossover" and "mutate" don't really hold.
 
The formula for updating each "position" at every timestep (the vector representing each of the settings) is...
X(t+1) = X(t) + V(t)
 
The formula for updating each "velocity" is...
V(t+1) = [W(1) * V(t)] + [W(2) * R(1) * (X(b) - X)] + [W(3) * R(2) * (X(g) - x)]
 
The position equation is fairly straightforward, you calculate your new position vector based on the current vector plus your per timestep velocity vector.
 
The velocity is more complicated, but very intuitive...
-The first set of braces gives you a factor of your current velocity by taking the current V(t) and muliplying it by a weight W(1) which should be (0,1], better values are slightly less than 1.
-The second set of braces modifies your velocity by taking the distance the particle is currently at and the best it has seen X(b).  Two weights are muliplied by this distance, one set constant W(2) [0,1] around 1 and the other random [0,1].
-The third set is similar to the second but it is the global maximum.
 
As far as convergence goes, with such a large search space, we can't hope to get the perfect answer, but we can get one of the many good answers that will work for us.  We can either let this thing run indefinately (until we need an answer when we will get its global best), wait till all of the particles converge to a certain local area (could possibly never happen), till a certain number o iterations has been run, when we haven't gained a certain amount in a certain amount of time in our global maxima, etc.
 
Initialization can be random, selected based on previously known settings that work on similar videos, or some mixture of the two.
 
Implementation Steps
Form a structure to represent each particle, each particle will have a list of the string name given to each parameter, a local high score with the position of it (parameters), the current position and velocity
One global maximum list will be kept to keep track of the best combination of values along with its score and increased score from the previous run.
A way of outputting the best global settings to a file with its score, and background differences
Come up with a way of combining the correct positive and false positive into a score
Output benchmark statistics such as Average time for fitness, and average time for generation.
*/

//TODO (Mainly) During second generation, objects tend to stagnate if they happen to have the highest fitness (as current, local, and global are the same position and velocity is 0) Possibly postpone updating velocity and position until we need the parameters, then we accumulate all of the global knowledge gained since then (as it is now, we are basically a generation behind), possibly allow for skipping particles that won't be updated

//Objects required
//Class representing each member of the population
#include "particleSwarm.h"
#include <math.h>

double ParticleSwarmAgent::bestGlobalScore = 0;
bool ParticleSwarmAgent::initialized = false;//Used to ensure that we use srand and read config once (Set during rand seeding)

//Parameters used
double ParticleSwarmAgent::inertialComponent;//Inertial
double ParticleSwarmAgent::cognitiveComponent;//Cognitive
double ParticleSwarmAgent::socialComponent;//Social
unsigned int ParticleSwarmAgent::updateMode;
double ParticleSwarmAgent::constrictionFactor;
std::list<swarmDimension_t> ParticleSwarmAgent::globalBest;

void XToString (double value, char * string)
{
	sprintf(string,"%.10e", value);
}

ParticleSwarmAgent::ParticleSwarmAgent(std::list<swarmDimension_t> &initialSwarmDim, std::list<vector<string> > &configList)
{
	dimensions.clear();
	//Set config entries from config file
	if (!initialized)//Only need to do once
	{
		int notLoaded = 0;
		notLoaded += FindValue(configList, "PSOSocial",socialComponent);
		notLoaded += FindValue(configList, "PSOCognitive",cognitiveComponent);
		notLoaded += FindValue(configList, "PSOInertial",inertialComponent);
		notLoaded += FindValue(configList, "PSOUpdateMode",updateMode);//Used to change between different PSO methods such as classical or canonical (per papers)
		if (notLoaded != 0)
			exit(1);

		if (updateMode == 1)
		{
			double tempRho = socialComponent + cognitiveComponent;
			if (tempRho <= 4)
			{
				cerr << "Your socialComponent and your cognitiveComponent must sum to larger than 4" << endl;//See 'Comparing Inertia Weights and Constriction Factors...' paper
				exit(1);
			}
			constrictionFactor = 2.0 / fabs(2 - tempRho - sqrt(tempRho * tempRho - 4 * tempRho));
		}
		else
			constrictionFactor = 0;
	}

	//Set initial values to the particle
	//We want to initialize randomly, but within the given ranges
	for (list<swarmDimension_t>::iterator iter = initialSwarmDim.begin(); iter != initialSwarmDim.end(); iter++)
	{
		swarmDimension_t temp;
		strcpy(temp.name,iter->name);
		temp.minValue = iter->minValue;
		temp.maxValue = iter->maxValue;

		//Randomly select a point in the given range
		if (!initialized)
		{
			srand((unsigned int)time(0));
			initialized = true;//NOTE This is where initialized is set
		}
		temp.bestLocalPosition = 0;
		double range = temp.maxValue - temp.minValue;
		assert(range > 0);//is your max possibly <= min?
		temp.position = range * rand()/RAND_MAX + temp.minValue;
		temp.velocity = 0;//TODO Experiment (check other implementations/papers) and see if this should be random too
		dimensions.push_back(temp);
	}
	unsetOwner();
	bestLocalScore = 0;
	firstRun = true;//NOTE Used so that we don't update the position/velocity before we have a chance to run
}

//Append the current position to a given config list
void ParticleSwarmAgent::saveToConfig(std::list<vector<string> > &configList)
{
	int returnVal = 0;
	for (list<swarmDimension_t>::iterator iter = dimensions.begin(); iter != dimensions.end(); iter++)
		returnVal += ForceValue(configList, iter->name, iter->position);
	if (returnVal != 0)
	{
		cout << "PSO: One of the dimensions names isn't located in the configList!  Quitting..." << endl;
		exit(1);
	}
}

//TODO Make this as a parsable vector format
void ParticleSwarmAgent::saveToFile(FILE *file)
{
	for (list<swarmDimension_t>::iterator iter = dimensions.begin(); iter != dimensions.end(); iter++)
	{
		fprintf(file, "%.10e " , iter->position);
	}
}

void ParticleSwarmAgent::saveHeaderToFile(FILE *file)
{
	for (list<swarmDimension_t>::iterator iter = dimensions.begin(); iter != dimensions.end(); iter++)
	{
		fprintf(file, "%s ", iter->name);
	}
}

void ParticleSwarmAgent::setGlobalPosition(char *name, double position)
{
	for (list<swarmDimension_t>::iterator iter = globalBest.begin(); iter != globalBest.end(); iter++)
		if (!strcmp(iter->name, name))
		{
			iter->position = position;
			return;
		}
	swarmDimension_t temp;
	strcpy(temp.name,name);
	temp.position = position;
	globalBest.push_back(temp);
}

double ParticleSwarmAgent::bestGlobalPosition(char *valueName)
{
	for (list<swarmDimension_t>::iterator iter = globalBest.begin(); iter != globalBest.end(); iter++)
	{
		if (!strcmp(iter->name, valueName))
		{
			return iter->position;
		}
	}
	cout << "PSO: Keyword not found " << valueName << endl;
	exit(1);
}

//simply a part of the total update statement, used in combination with updateVelocityPosition to control at what point these happen at
void ParticleSwarmAgent::updateMaxima(double currentScore)
{
	if (currentScore > bestLocalScore)
	{
		printf("PSO: New LocalBest - New: %f Old: %f\n", currentScore, bestLocalScore);
		bestLocalScore = currentScore;
		bool isGlobalBest = false;
		if (currentScore > bestGlobalScore)
		{
			printf("PSO: New GlobalBest - New: %f Old: %f\n", currentScore, bestGlobalScore);
			isGlobalBest = true;
			bestGlobalScore = currentScore;
		}
		for (list<swarmDimension_t>::iterator iter = dimensions.begin(); iter != dimensions.end(); iter++)
		{
			iter->bestLocalPosition = iter->position;
			if (isGlobalBest)
				setGlobalPosition(iter->name, iter->position);
		}
	}
	firstRun = false;
}
//Used in conjunction with updateMaxima to equate to the old update function, this should occur after it!!
void ParticleSwarmAgent::updateVelocityPosition()
{
	if (firstRun)
		return;

	for (list<swarmDimension_t>::iterator iter = dimensions.begin(); iter != dimensions.end(); iter++)
	{
		if (updateMode == 1)//'Canonical' Method, used in 'Off The Shelf PSO', doesn't use inertialComponent (implicit in constrictionFactor)
		{
			iter->velocity = constrictionFactor * (iter->velocity + cognitiveComponent * rand()/RAND_MAX * (iter->bestLocalPosition - iter->position) + socialComponent * rand()/RAND_MAX * (bestGlobalPosition(iter->name) - iter->position));
		}
		else
		{
			iter->velocity = inertialComponent * iter->velocity + cognitiveComponent * rand()/RAND_MAX * (iter->bestLocalPosition - iter->position) + socialComponent * rand()/RAND_MAX * (bestGlobalPosition(iter->name) - iter->position);
		}
		double tempPosition = iter->position + iter->velocity; //Used to calculate if we use this velocity, will we go out of bounds, if we will, set it to 0 so that we are attracted back towards higher scoring areas in bounds
		//TODO Make a useful output format for this!//printf("PSO: Velocity:%f  Name: %s Position:%f\n", iter->velocity, iter->name, iter->position);
		if (tempPosition > iter->maxValue || tempPosition < iter->minValue)
		{
			if (tempPosition > iter->maxValue)
			{
				cerr << "Position " << iter->name << " larger than max[" << iter->maxValue << "], setting velocity to 0" << endl;
			}
			else
			{
				cerr << "Position " << iter->name << " smaller than min[" << iter->minValue << "], setting velocity to 0" << endl;
			}
			iter->velocity = 0;
		}
	}
	for (list<swarmDimension_t>::iterator iter = dimensions.begin(); iter != dimensions.end(); iter++)
		iter->position += iter->velocity;
}
