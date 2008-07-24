#include "configReader.h"

///Format is String 'spaces/tabs' data where data is anything you want that doesn't contain a space character, and the data part doesn't have a pound (comments are delimited with pounds)
int ReadConfig(const char * fName, list<vector<string> > &configList)
{
	FILE* fp = fopen(fName, "r");
	if (fp == NULL)
	{
		cerr << "CONFIG - Couldn't open file [" << fName << "]!" << endl;
		return 1;
	}
	cout << "CONFIG -  Opened file [" << fName << "]!\n";
	char c;
	while (!feof(fp))
	{
		c = fgetc(fp);
		if (c == '#' || c == ';' || isspace(c))
		{
			while(!feof(fp) && (c != '\n'))
				c = fgetc(fp);
			continue;
		}

		ungetc(c, fp);

		char tempName[100];
		char tempValue[100];
		fscanf(fp, " %s %s ", tempName, tempValue);
		//The following clears extraneous pounds but stopping at either a pound or a null, we will put a null at the last place in any case
		unsigned int k;
		for (k = 0; tempValue[k] != '#' && tempValue[k] != '\0';k++)
			;//Note this semicolon is here
		tempValue[k] = '\0';

		//This handles the @, if one is found as the first letter of a name, we will use its value as a new config name and append its items to this list
		//Care should be taken to avoid looping!
		//We take the given path, and remove the end letters until we clear the whole string, or we reach a /, then append this value
		if (tempName[0] == '@')
		{
			cout << "CONFIG - Appending contents of [" << tempValue << "]!" << endl;
			char tempFileName[50];
			strcpy(tempFileName, fName);

			int tempLen = strlen(tempFileName) - 1;
			for (int x = tempLen; x >= 0; x--)
			{
				if (tempFileName[x] == '/')
					break;
				else
					tempFileName[x] = '\0';
			}

			strcat(tempFileName, tempValue);
			ReadConfig(tempFileName, configList);
		}
		else
		{
			cout << "Name: " << tempName << " Value: " << tempValue << '\n';
			vector<string> temp_element(2);
			temp_element[0] = tempName;
			temp_element[1] = tempValue;
			configList.push_back(temp_element);
		}
	}

	fclose(fp);
	return 0;
}

//TODO Make safe by checking length!
void StringToX(const char *sValue, char *value)
{
	strcpy(value, sValue);
	cout << " string [" << sValue << "] " << value << endl;
}

void StringToX(const char *sValue,  unsigned int &value)
{
	double tempValue;
	sscanf(sValue,"%lf", &tempValue);
	value = (unsigned int)tempValue;
	cout << " uint [" << sValue << "] " << value << endl;
}

void StringToX(const char *sValue, int &value)
{
	double tempValue;
	sscanf(sValue,"%lf", &tempValue);
	value = (int)tempValue;
	cout << " int [" << sValue << "] " << value << endl;
}

void StringToX(const char *sValue,  double &value)
{
	sscanf(sValue,"%lf", &value);
	cout << " double [" << sValue << "] " << value << endl;
}

void StringToX(const char *sValue,  float &value)
{
	sscanf(sValue,"%f", &value);
	cout << " float [" << sValue << "] " << value << endl;
}
