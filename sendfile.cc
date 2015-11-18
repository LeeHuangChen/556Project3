#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#include <map>
#include <string>
#include <iterator>
#include <iostream>
#include <thread>         // std::thread
#include <memory>

/*syntax:  ./sendfile -r 127.0.0.1:18000 -f testfile2.txt*/

////////////////////////																							//////////////////
//  GLOBAL VARIABLES  //																							//////////////////
////////////////////////																							//////////////////
struct packet{
	char* buffer;
	unsigned int size;
};
typedef struct packet PACKET;
enum ePacketType{
	ACK,
	DATA,
	FILENAME,
	FILE_END
};
//Send and Receive
std::map<unsigned int, std::shared_ptr<PACKET>> dataMap;
std::map<unsigned int, std::shared_ptr<PACKET>> ackMap;

//File i/o
char* filename;
FILE *inputFile; /* File to send*/

//Network Variables
char* ip;
unsigned short portnum = -1; 
struct sockaddr_in servaddr; /* server address */
int sock;
struct sockaddr_in myaddr;


/////////////////////////////																						//////////////////
//  FUNCTION DECLARATIONS  //																						//////////////////
/////////////////////////////																						//////////////////

//Sending Data/Filename functions
void threadSend();

//Receiving ack functions
void threadRecv();

//Network Connection and Initialization
void initConnection(int argc, char** argv);
void handleOptions(int argc, char** argv);
int setupSocket();
int setupServerAddress();

//File i/o
void sendMessage(const char *my_message, unsigned int messageLength);
void readfile(char *sendBuffer, unsigned int readSize);
void recvMessage(char *my_message, unsigned int messageLength);

//Helper
void printInputContents();
 
/////////////////////																								//////////////////
//  MAIN FUNCTION  //																								//////////////////
/////////////////////																								//////////////////
//ZMAIN
int main(int argc, char** argv){
	//initialize the network connection
	initConnection(argc, argv);
	//send the file to the receiver
	//threadSend();
	std::thread first (threadSend);
	std::thread second (threadRecv);
	first.join();
	second.join();

	//closes the file after the file is sent
	fclose(inputFile);
	//terminate the program
	return 0;
}
 
///////////////////////////////////////////																			//////////////////
//              SEND THREAD              //																			//////////////////
///////////////////////////////////////////																			//////////////////
//ZSEND

//Global Variables for this section:
int packetSize = 49990;
//int packetSize = 500;
int readingPositionInTheInputFile=0;
int bytesLeftToRead;
bool finishedReading=false;
bool finishedReceiving=false;
unsigned int filesize;



//Helper functions for this section:
int getPacketSize();
int readNextPacket(char *buffer);
void addInfoToDataMap(unsigned int seqNum, char *buffer, unsigned int size);
void addInfoToAckMap(unsigned int seqNum, char *buffer, unsigned int size);
void displayMap(std::map<unsigned int, std::shared_ptr<PACKET>> map, const char* name);
void makeBuffer(ePacketType type, unsigned int seqNum, char *payload, char *result);

//Code for this section:
void threadSend(){
	//open the input file
	inputFile = fopen(filename,"r");
	if(inputFile==NULL){
		fputs ("File error",stderr);exit(1);
	}

	//obtain file size:
	fseek(inputFile,0,SEEK_END);
	filesize = ftell(inputFile);
	rewind(inputFile);

	unsigned int seqNum=1;
	int readSize=0;
	while(!finishedReading){
		//make a buffer of .05 MB to send
		char *sendBuffer = (char*) malloc(50000); 
		// Read the Next packet and put it in sendbuffer
		readSize=readNextPacket(sendBuffer);
		//addInfoToMap(sendBuffer, readSize, seqNum, dataMap);
		char *formattedBuffer=new char[readSize+7];
		memset(formattedBuffer,0,sizeof(char)*(readSize+7));
		ePacketType type=DATA;
		memcpy(&formattedBuffer[0],&type,1);
		memcpy(&formattedBuffer[1],&seqNum,4);

		unsigned short checksum=0;
		memcpy(formattedBuffer+5,&checksum,2);
		printf("&&test3\n");
		memcpy(formattedBuffer+7,(const char*)sendBuffer,readSize);
		addInfoToDataMap(seqNum,formattedBuffer,readSize+7);
		//addInfoToDataMap(seqNum,sendBuffer,readSize);
		//printf("SeqNum:%d, size:%d\n", seqNum, dataMap[seqNum]->size);
		displayMap(dataMap,"DataMapContents");
		//Send the contents of send buffer to the reciever
		sendMessage((const char*) dataMap[seqNum]->buffer,dataMap[seqNum]->size);
		//sendMessage(formattedBuffer,readSize+7);
		//sendMessage(sendBuffer,readSize);
		seqNum++;
		//print the contents for testing
		//printf("sendBuffer[0]:%c\n",(char)sendBuffer[0]);

		
		//free the memory
		//free(sendBuffer);
	}
	
}

//Helper functions
int readNextPacket(char *buffer){
	int readSize=getPacketSize();
	//Read the input file for 'readSize' amount of data and store it in buffer
	readfile(buffer,readSize);
	return readSize;
}
int getPacketSize(){
	int readSize=0;
	bytesLeftToRead= filesize - readingPositionInTheInputFile;
	if(bytesLeftToRead<packetSize){
		readSize=bytesLeftToRead;
		finishedReading=true;
	}
	else{
		readSize=packetSize;
		readingPositionInTheInputFile+=packetSize;
	}
	return readSize;
}
void addInfoToDataMap(unsigned int seqNum, char *buffer, unsigned int size){
	std::shared_ptr<PACKET> packet (new PACKET());
	packet->buffer = buffer;
	packet->size = size;
	dataMap.insert(std::make_pair(seqNum, packet));
}

void addInfoToAckMap(unsigned int seqNum, char *buffer, unsigned int size){
	std::shared_ptr<PACKET> packet (new PACKET());
	packet->buffer = buffer;
	packet->size = size;
	ackMap.insert(std::make_pair(seqNum, packet));
}
void makeBuffer(ePacketType type, unsigned int seqNum, char *payload, char *result){
	// char *result= (char*)malloc(sizeof(&payload)+7);
	memcpy(&result[0],&type,1);
	memcpy(&result[1],&seqNum,4);
	memcpy(&result[7],&payload,sizeof(&payload));
	//printf("payload:%s\n", payload);
	
}

void displayMap(std::map<unsigned int, std::shared_ptr<PACKET>> map, const char* name){
	typedef std::map<unsigned int, std::shared_ptr<PACKET>>::iterator it_type;
	printf("\n");
	printf("MapName:%s\n", name);
	for(it_type iterator = map.begin(); iterator != map.end(); iterator++) {
	    // iterator->first = key
	    // iterator->second = value
		unsigned int SeqNum=iterator->first;
		char *buffer = iterator->second->buffer;
		unsigned int size = iterator->second->size;
		printf("  Entry(SeqNum):%d\n", SeqNum);
		printf("  Size:%d\n", size);
		printf("  SeqNumFromPayload:%d\n",(unsigned int)(buffer[1]));
		printf("  CheckSumFromPayload:%d\n", (unsigned int)(buffer[5]));
		printf("  BufferFromPayload:%.20s\n", buffer+7);
		/*printf("  Buffer:%c%c%c%c%c%c%c%c%c%c...\n", 
				buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]
				, buffer[5], buffer[6], buffer[7], buffer[8], buffer[9]);*/
		//printf("  Buffer:%s\n", buffer);

		
		
	}
}


void threadRecv(){
	int seqNum=1;
	while(!finishedReceiving){
		printf("### Receiving Message\n");
		char *recvBuffer = (char*) malloc(50000);
		recvMessage(recvBuffer,3000);
		addInfoToAckMap(seqNum,recvBuffer,sizeof(&recvBuffer));
		//displayMap(ackMap,"AckMap");
		if(seqNum==3){
			finishedReceiving=true;
		}
		seqNum++;
	}
}
 
///////////////////////////////////////////////////////																//////////////////
//  NETWORK CONNECTION AND INITIALIZATION FUNCTIONS  //																//////////////////
///////////////////////////////////////////////////////																//////////////////
//ZNETINIT
void initConnection(int argc, char** argv){
	handleOptions(argc, argv);
	//print read contents
	printInputContents();
	//setup the socket
	if(setupSocket()==0){printf("ERROR SETTING UP SOCKET\n");exit (1);}
	//setup the server address
	if(setupServerAddress()==0){printf("ERROR SETTING UP ADDRESS\n");exit (1);}
}
void handleOptions(int argc, char** argv){
	char* parseStr;
	char* IPAndPortnum;
	int option = 0;
	while ((option = getopt(argc,argv,"r:f:")) != -1){
		switch (option){
			case 'r': IPAndPortnum = optarg; 
				break;
			case 'f': filename = optarg;
				break;
			default: 
				exit(EXIT_FAILURE);
		}
	}
	parseStr=strtok(IPAndPortnum,":");
	ip=parseStr;
	parseStr=strtok(NULL,":");
	portnum=atoi(parseStr);
	
}

int setupSocket(){
	//setting up the socket
	printf("setting up the socket\n");
	if( (sock=socket(AF_INET, SOCK_DGRAM,0)) < 0 ){
		perror("cannot create socket");
		printf("cannot create socket\n" );
		return 0;
	}
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family=AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port=htons(0);
	if (bind(sock, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) { 
		perror("bind failed"); 
		printf("%s\n", "bind failed");
		return 0; 

	}
	return 1;
}

int setupServerAddress(){ 
	printf("setting up the server address\n");
	/* fill in the server's address and data */ 
	memset((char*)&servaddr, 0, sizeof(servaddr)); 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(portnum); 

	/* put the host's address into the server address structure */ 
	//memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);
	if (inet_aton(ip, &servaddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
	return 1;
}

//////////////////////////																							//////////////////
//  FILE I/O FUNCTIONS  //																							//////////////////
//////////////////////////																							//////////////////
//ZFILEIO
void readfile(char *sendBuffer, unsigned int readSize){
	int readResult;
	unsigned int accu=0;
	while(accu<readSize){
		readResult = fread(sendBuffer+accu,1,readSize,inputFile);
		accu+=readResult;
		printf("readResult:%d, accu:%d, readSize:%d\n",readResult, accu,readSize);
	}
	if(readResult!=readSize){
		printf("ERROR READING FILE\n");
		exit(1);
	}
}

void sendMessage(const char *my_message, unsigned int messageLength){
	printf("sending message to %s\n", ip);
	if (sendto(sock, my_message, messageLength/*strlen(my_message)*/, 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) { 
		perror("sendto failed"); 
		printf("sendto failed\n");
		exit(1);
	}
}

void recvMessage(char *my_message, unsigned int messageLength){
	printf("receiving message from %s\n", ip);
	int slen = sizeof(servaddr);
	int recvlen = recvfrom(sock, my_message, messageLength, 0, (struct sockaddr *)&servaddr, &slen);
	if (recvlen< 0) { 
		perror("recvfrom failed"); 
		printf("recvfrom failed\n");
		exit(1);
	}
	if (recvlen >= 0) {
        my_message[recvlen] = 0;	/* expect a printable string - terminate it */
        printf("received message: \"%s\"\n", my_message);
    }
}

////////////////////////																							//////////////////
//  HELPER FUNCTIONS  //																							//////////////////
////////////////////////																							//////////////////
//ZHELP
void printInputContents(){
	printf("\nprogram running, the following flags are read:\n");
	printf("filename:%s\n",filename);
	printf("portnum:%d\n",portnum);
	printf("ip:%s\n\n",ip);
}

