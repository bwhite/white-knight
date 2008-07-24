/**
 * This class allows for a UDPStream of images to be provided to the caller
 *
 * \author Brandyn White
 */
#include <cstdio>
#include <vector>
#include <iostream>
#include <sys/time.h>
#include <time.h>
#include "UDPClient.h"

void diep(char *s)
{
    perror(s);
    exit(1);
}

//Begin packet buffer

/* This class is used to store packets from a UDP data source, as UDP can send files out of order, it is important that we can easily...
 * 1. Ensure packets aren't overwritten
 * 2. Know how many unique packets we have (constant time)
 * 3. Know what frame number this is representing
 */
class PacketFile
{
public:
    PacketFile(unsigned int frameNumber, unsigned int totalPackets, unsigned int dataSize) : frameNumber(frameNumber), dataSize(dataSize), uniquePackets(0), dataBuffer(0), packetSize(totalPackets, (unsigned int) 0),packet(totalPackets, (unsigned char *) 0)
    {}

	void reset()
	{
		for (unsigned int packetIter = 0; packetIter < packet.size(); packetIter++)
        	if (packet[packetIter])
			{
                delete [] packet[packetIter];
				packet[packetIter] = 0;
			}
		dataBuffer = 0;//NOTE If we created a dataBuffer, the caller is responsible for cleaning it up
		uniquePackets = 0;
	}
	

    ~PacketFile()
    {
        for (unsigned int packetIter = 0; packetIter < packet.size(); packetIter++)
            if (packet[packetIter])
			{
                delete [] packet[packetIter];
				packet[packetIter] = 0;
			}
        //if (dataBuffer)
        //    delete [] dataBuffer;
    }

    //Returns 0 if the packet hasn't been finished, a nonzero pointer if it has (with the address implicitly as the pointer)
	//NOTE ---->The data the pointer points to is considered property of the caller now, so they are in charge of freeing it!
    unsigned char* addPacket(unsigned char* data, unsigned int packetNumber, unsigned int curPacketSize)
    {
        //See if we already have this packet, if not add it and increment unique packet counter
        if (packet[packetNumber] == 0)
        {
			//cout << "Pack:" << packetNumber << '\n';
			if (uniquePackets == 0)
			{
				//cout << "First Packet for file " << frameNumber << '\n';
				gettimeofday(&startTime,0);
			}
            packet[packetNumber] = new unsigned char[curPacketSize];
            memcpy(packet[packetNumber],data,curPacketSize);
            packetSize[packetNumber] = curPacketSize;
            uniquePackets++;
			assert(uniquePackets < packet.size() + 1);
            //If our data is complete, build it and return the pointer to it
            if (uniquePackets == packet.size())
            {
				if (!dataBuffer)
                	dataBuffer = new unsigned char[dataSize];

                //Here have have integrated a simple sanity check to ensure we have exactly the right number of bytes
#ifndef NDEBUG
                unsigned int numBytes = 0;
#endif
                unsigned char *dataPosition = dataBuffer;
                for (unsigned int packetIter = 0; packetIter < packet.size(); packetIter++)
                {
                    assert(packet[packetIter]);
#ifndef NDEBUG
                    numBytes += packetSize[packetIter];
					cout << "Cur: " << packetSize[packetIter] << "Tot: " << numBytes << endl;
#endif
                    memcpy(dataPosition,packet[packetIter],packetSize[packetIter]);
                    dataPosition += packetSize[packetIter];
                }
                assert(numBytes == dataSize);
				struct timeval endTime;
				gettimeofday(&endTime,0);
				cout << "Time to send packet is " << (endTime.tv_sec - startTime.tv_sec) + .000001 * (endTime.tv_usec - startTime.tv_usec) << " sec." << endl;
                return dataBuffer;//CALLER MUST FREE THIS BUFFER!!
            }
        }
        return 0;
    }

    unsigned int getFrameNumber()
    {
        return frameNumber;
    }
private:
    const unsigned int frameNumber;
    unsigned int dataSize;
    unsigned int uniquePackets;
	struct timeval startTime;

    unsigned char *dataBuffer;
    vector<unsigned int> packetSize;//Refers to how much data each of the 'packets' pointers is pointing to (used when packet is complete)
    vector<unsigned char*> packet;//Holds packet data
};
//End packet buffer

//Begin internal UDP thread functions
void* UDPBuffer(void* arg)
{
	udpBufferData_t *data = (udpBufferData_t*) arg;

	socklen_t socklen = sizeof(struct sockaddr_in);
    if (bind(data->udpSocket, (struct sockaddr *) &(data->sock_server), socklen) == -1)
    {
        diep("bind()");
    }

	struct timeval waitTime;//For select statement
	waitTime.tv_usec = 0;
	waitTime.tv_sec = 5;
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(data->udpSocket, &fdset);
	while(data->keepRunning)
	{
		fd_set fdsetCPY = fdset;
		if (select((data->udpSocket)+1,&fdsetCPY,NULL,NULL,&waitTime) > 0)
		{
			unsigned char *transientBuffer = new unsigned char[data->dataSize];
			struct sockaddr_in sock_client;
			int recLen = recvfrom(data->udpSocket, transientBuffer, data->dataSize, 0,(struct sockaddr *) &sock_client, &socklen);
			if (recLen < 0)
	            diep("recvfrom()");
	        assert(recLen > 0);
			udpBufferElement_t tempElement;
			tempElement.data = transientBuffer;
			tempElement.bytes = recLen;
			//cout << "Thread: Recvd " << tempElement.bytes << " bytes" << endl;
			pthread_mutex_lock(&(data->packetBufferMutex));
			data->packetBuffer.push_back(tempElement);
			if (data->packetBuffer.size() > data->maxPacketBufferSize)
			{
				//cerr << "Packet buffer in UDPSocket is full (you must not be processing them fast enough)!" << endl;
				delete [] data->packetBuffer.front().data;
				data->packetBuffer.front().data = 0;
				data->packetBuffer.pop_front();
			}
			pthread_mutex_unlock(&(data->packetBufferMutex));
		}
	}
	return NULL;
}

udpBufferElement_t getPacket(udpBufferData_t *data)
{
	//If the data isn't available, we sleep for a very small amount of time so that we can reduce the effect of the tightloop
	struct timespec sleepTime;//Time to sleep for
	sleepTime.tv_sec = 0;
	sleepTime.tv_nsec = 1;
	while (data->keepRunning)
	{
		if (!data->packetBuffer.empty())
		{
			pthread_mutex_lock(&(data->packetBufferMutex));
			if (data->packetBuffer.empty())//For the possible race if it was not empty above, but when we lock it is now empty
			{
				pthread_mutex_unlock(&(data->packetBufferMutex));
				continue;
			}
			udpBufferElement_t tempPointer = data->packetBuffer.front();
			data->packetBuffer.pop_front();
			pthread_mutex_unlock(&(data->packetBufferMutex));
			return tempPointer;
		}
		nanosleep(&sleepTime,NULL);//TODO Change this to a spinlock that is unlocked by the saving function (so it is locked when the last packet is read, and unlocked when a packet is added)Sleep for a very small amount of time (effectivly the response time of the OS)
	}
	udpBufferElement_t temp;
	temp.data = 0;
	temp.bytes = 0;
	return temp;
}

//This function runs in its own thread to collect packets and place them in a buffer for retrieval by the client
void* getMessages(void* arg)
{
	udpBufferData_t *threadData = (udpBufferData_t*) arg;
	
	//Initialize packet receiving thread
	pthread_t recPackThread;
	int iret1 = pthread_create( &recPackThread, NULL, UDPBuffer, threadData);
	assert(!iret1);
	PacketFile **bufferFiles = new PacketFile*[threadData->maxFileIDValues];//Since the number unique files to be held is what can be held in an unsigned char
	memset(bufferFiles,0,sizeof(PacketFile*) * threadData->maxFileIDValues);
	
	//Create buffer for each file (using the PacketFile class)
	for (unsigned int bufferInit = 0; bufferInit < threadData->maxFileIDValues; bufferInit++)
		bufferFiles[bufferInit] = new PacketFile(bufferInit, threadData->numberOfPackets, threadData->dataSize);
	
	unsigned int bytesLeftToRecv = threadData->dataSize;

	unsigned int currentFile = 0;//TODO This method should be replaced with a deque (see comments elsewhere)

    while (threadData->keepRunning)//We will go through the buffer made in the receiver thread, and process all of the packets
    {
		udpBufferElement_t tempPacketBuffer = getPacket(threadData);
   		if (!tempPacketBuffer.data)
			break;
        bytesLeftToRecv -= (tempPacketBuffer.bytes - threadData->headerSize);
        unsigned char *fileNum = (unsigned char*) tempPacketBuffer.data;
		unsigned int *packetNum = (unsigned int*) &(tempPacketBuffer.data[threadData->fileIDSize]);
        unsigned char *packetData = (unsigned char*) &(tempPacketBuffer.data[threadData->headerSize]);

		if (*fileNum != currentFile)
		{
			bufferFiles[currentFile]->reset();
			currentFile = *fileNum;
		}

		#ifndef NDEBUG
		cout << "FileNum:" << (unsigned int)(*fileNum) << "\nPacketNum" << *packetNum << "\nRecvd: " << tempPacketBuffer.bytes << "\nbytesLeftToRecv: " << bytesLeftToRecv << endl;
		#endif
		
		unsigned char* finalFile = bufferFiles[(unsigned int)(*fileNum)]->addPacket(packetData,*packetNum, tempPacketBuffer.bytes - threadData->headerSize);
		if (finalFile)
		{
			/*
			TODO Use a deque to keep track of image chronology
			when one is completed, all others under it are deleted
			
			For now I am making the reasonable assumption that completed frames will only be completed in sequence
			A problem is that we don't ever know if we get a "new" file because it will wrap around. To get around
			this, we will also assume the files will be transmitted in order starting at 0, so when we get to 0 we
			delete/clear all of the packetBuffers
			
			A forced convention due to the above is that the server only increment its file ID when it actually sends.
			
			For now I am also making the assumption not so reasonable (thus temporary) assumption that if we start a
			packet for another file than one we were working on, we will clear the previous
			*/
			
			//Add to retrieval buffer
			pthread_mutex_lock(&(threadData->imageBufferMutex));
			threadData->imageBuffer.push_back(finalFile);
			
			if (threadData->imageBuffer.size() > threadData->maxImageBufferSize)
			{
				//cerr << "Image buffer in UDPSocket is full (you must not be processing them fast enough)! (Images: " << threadData->imageBuffer.size() << endl;
				delete [] threadData->imageBuffer.front();
				threadData->imageBuffer.front() = 0;
				threadData->imageBuffer.pop_front();
			}
			pthread_mutex_unlock(&(threadData->imageBufferMutex));
			bufferFiles[(unsigned int)(*fileNum)]->reset();
		}
		delete [] tempPacketBuffer.data;//TODO instead of deleting this save it in a list, we only need a small number of them
		tempPacketBuffer.data = 0;
    }

	threadData->keepRunning = false;
	cout << "Joining worker threads and closing..." << endl;
	pthread_join(recPackThread, NULL);
	
	for (unsigned int freeBufIter = 0; freeBufIter < threadData->maxFileIDValues; freeBufIter++)
	{
		if (bufferFiles[freeBufIter])
		{
			delete bufferFiles[freeBufIter];
			bufferFiles[freeBufIter] = 0;
		}
	}

	delete [] bufferFiles;
	bufferFiles = 0;
	return 0;
}

//End Internal UDP Thread Functions


UDPClient::UDPClient(list<vector<string> > configList) : fileCounter(0)
{
	//Load config settings
	int notLoaded = 0;
	
	//set height width channels from config for now
	notLoaded += FindValue(configList, "UDPHeight", height);
	notLoaded += FindValue(configList, "UDPWidth", width);
	notLoaded += FindValue(configList, "UDPChannels", channels);
	
	//TODO Get necessary data
	//notLoaded += FindValue(configList, "UDPServer", server);
	unsigned int port;//Used only during connection for now!
	unsigned int maxPacketBufferSize;
	unsigned int maxImageBufferSize;
	unsigned int packetSize;
	notLoaded += FindValue(configList, "UDPPort", port);
	notLoaded += FindValue(configList, "UDPMaxPacketBufferSize", maxPacketBufferSize);
	notLoaded += FindValue(configList, "UDPMaxImageBufferSize", maxImageBufferSize);
	notLoaded += FindValue(configList, "UDPPacketSize", packetSize);
	//notLoaded += FindValue(configList, "UDP", );
	assert(!notLoaded);

	//Bind port here
	int udpSocket;
    if ((udpSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        diep("socket()");
    }
	struct sockaddr_in sock_server;
    sock_server.sin_family = AF_INET;
    sock_server.sin_port = htons(port);
    sock_server.sin_addr.s_addr = htonl(INADDR_ANY);

	//Creates threadData used by the threads during message collection

	threadData.keepRunning = true;
	threadData.maxPacketBufferSize = maxPacketBufferSize;
	threadData.maxImageBufferSize = maxImageBufferSize;
	threadData.udpSocket = udpSocket;
	threadData.sock_server = sock_server;
	
	threadData.dataSize = height*width*channels;
	threadData.packetSize = packetSize;
	threadData.fileIDSize = sizeof(unsigned char);
	threadData.packetIDSize = sizeof(unsigned int);
	threadData.headerSize = threadData.fileIDSize + threadData.packetIDSize;
	threadData.maxDataPerPacket = threadData.packetSize - threadData.headerSize;
	threadData.numberOfPackets = (unsigned int)ceil((double)threadData.dataSize/(double)threadData.maxDataPerPacket);
	threadData.maxFileIDValues = (unsigned int)ceil(pow(2.0,(sizeof(unsigned char) * 8.0)));

	cout << "Using... DataSize: " << threadData.dataSize << " PacketSize: " << threadData.packetSize << " Port: " << port << endl;
	
	pthread_mutex_init(&(threadData.packetBufferMutex),0);
	pthread_mutex_init(&(threadData.imageBufferMutex),0);
	int iret = pthread_create( &messageThread, NULL, getMessages, &threadData);
	assert(!iret);
}

UDPClient::~UDPClient()
{
	//TODO go through both buffers and clear any remaining data
	threadData.keepRunning = false;
	cout << "Joining message thread and closing..." << endl;
	pthread_join(messageThread, NULL);
	
	close(threadData.udpSocket);
}

void UDPClient::getImageInfo(unsigned int &height, unsigned int &width, unsigned int &channels)
{
	height = this->height;
	width = this->width;
	channels = this->channels;
}

void UDPClient::reset()
{
	//<3 abstractions
}
void UDPClient::getImage (Image8 &image) throw (NoMoreImages)
{
	if (image)
		getImage(*image);
}

void UDPClient::getImage (Image8 &image) throw (NoMoreImages)
{
	assert(image.getHeight() == height);
	assert(image.getWidth() == width);
	assert(image.getChannel() == channels);
	//If the data isn't available, we sleep for a very small amount of time so that we can reduce the effect of the tightloop
	struct timespec sleepTime;//Time to sleep for
	sleepTime.tv_sec = 0;
	sleepTime.tv_nsec = 1;

	//Spin waiting for image TODO Change that to a mutex
	while (threadData.keepRunning)
	{
		if (!threadData.imageBuffer.empty())
		{	
			pthread_mutex_lock(&(threadData.imageBufferMutex));
			//NOTE If there is a way for any other operation to retrieve images, then a check will have to be made here (another if like above)
			unsigned char * tempData = threadData.imageBuffer.front();
			threadData.imageBuffer.pop_front();
			pthread_mutex_unlock(&(threadData.imageBufferMutex));
			memcpy(image.getDataPointer(),tempData,image.getByteSize());
			
			/*
			char tempName[100];
			struct timeval curTime;
			gettimeofday(&curTime,0);
			sprintf(tempName, "Output-%u-%.4u.ppm",curTime.tv_sec,curTime.tv_usec,fileCounter++);

			// Start Brandyn's 4 Line PPM
			FILE *imageFile = fopen(tempName,"w");
			fprintf(imageFile,"P6\n%d %d\n255\n",image.getWidth(),image.getHeight());
			fwrite(tempData,sizeof(unsigned char),3*image.getWidth()*image.getHeight(),imageFile);
			fclose(imageFile);
			// End Brandyn's 4 Line PPM
			*/
			delete [] tempData;
			tempData = 0;
			return;
		}
		nanosleep(&sleepTime,NULL);//TODO Change this to a spinlock that is unlocked by the saving function (so it is locked when the last packet is read, and unlocked when a packet is added)Sleep for a very small amount of time (effectivly the response time of the OS)
	}
	cerr << "Requested image when we are not receiving data" << endl;
	assert(0);
}
