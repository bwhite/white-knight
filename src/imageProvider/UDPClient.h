/**
 * This class allows for a stream of UDP images (from a separate server) to be provided to the caller, the height, width, and number of channels must all be the same between all of the files in the list; however, the actual file type doesn't matter as long as there is an image format that handles it
 *
 * \author Brandyn White
 */
#ifndef UDPCLIENT_H
#define UDPCLIENT_H
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <list>
#include <cmath>
#include "../ImageProvider.h"
#include "../imageClass/Image8.h"
#include "../ImageFormat.h"
#include "../configTools/configReader.h"

using namespace std;

//Holds the actual received data (unsigned char* for data and unsigned int for bytes received) only contains valid data (so errors are handled before)
typedef struct
{
	unsigned char* data;
	unsigned int bytes;
} udpBufferElement_t;

//Data structure that is used to pass parameters to UDPBuffer
typedef struct
{
	bool keepRunning;
	list<udpBufferElement_t> packetBuffer;
	list<unsigned char *> imageBuffer;//This is where the completed images are held.  Access to it should be locked
	pthread_mutex_t packetBufferMutex;//Controls access to packetBuffer
	pthread_mutex_t imageBufferMutex;//Controls access to imageBuffer
	unsigned int maxPacketBufferSize;//Used so that if we really aren't keeping up with our buffer, we don't run out of memory
	unsigned int maxImageBufferSize;//Used so that if we really aren't keeping up with our buffer, we don't run out of memory
	unsigned int udpSocket;
	struct sockaddr_in sock_server;
	
	//These are sizes that define the structure of the packet so that we can pack information in it
	unsigned int dataSize;
	unsigned int packetSize;
	unsigned int fileIDSize;
	unsigned int packetIDSize;
	unsigned int headerSize;
	unsigned int maxDataPerPacket;
	unsigned int numberOfPackets;
	unsigned int maxFileIDValues;
} udpBufferData_t;

class UDPClient : public ImageProvider
{
public:
	UDPClient(list<vector<string> > configList);
	virtual ~UDPClient();

	///Returns our internal height/width/channel
	void getImageInfo(unsigned int &height, unsigned int &width, unsigned int &channels);

	///Given an image that matches our internal height/width/channels, we will take the next file in our list, find its extension handler, and load the file
	void getImage (Image8 &image) throw (NoMoreImages);

	///Erases all current state information, which can have different effects depending on the underlying implementation
	void reset ();
	
	void stop()
	{
		threadData.keepRunning = false;
		cout << "Joining message thread and closing..." << endl;
		pthread_join(messageThread, NULL);
	}
	
private:
	unsigned int height;
	unsigned int width;
	unsigned int channels;
	pthread_t messageThread;
	udpBufferData_t threadData;
	unsigned int fileCounter;
};
#endif
