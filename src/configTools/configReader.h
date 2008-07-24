#ifndef CONFIG_READER_H
#define CONFIG_READER_H
#include <iostream>
#include <list>
#include <vector>
#include <cstdio>

using namespace std;

int ReadConfig(const char * fName, list<vector<string> > &configList);

//TODO Clean up string usage in this (we convert from char* to stl strings)
///This will find the FIRST match of the value and return zero if its found
template <class T>
int FindValue(list<vector<string> > &configList, const char * valueName, T &value)
{
	for (list<vector<string> >::iterator iter = configList.begin(); iter != configList.end(); iter++)
	{
		if ((*iter)[0] == valueName)
		{
			cout << "CONFIG - " << valueName << " ";
			char tempVal[100];
			strcpy(tempVal, (*iter)[1].c_str());
			StringToX(tempVal, value);
			return 0;
		}
	}
	cerr << "CONFIG - Keyword [" << valueName << "] not found!" << endl;
	return 1;
}

void StringToX(const char *sValue, char *value);
void StringToX(const char *sValue,  unsigned int &value);
void StringToX(const char *sValue, int &value);
void StringToX(const char *sValue,  double &value);
void StringToX(const char *sValue,  float &value);
#endif
