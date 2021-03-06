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


//////////////////////////////
//		SENDFILE CODE		//
//////////////////////////////
void threadSend(){
	int delay=100000000;
	int flags=5;
	
	for(int j=0;j<flags;j++)
	{
		for(int i=0; i<delay;i++){
		//wait
		}
		//std::cout << "send,flag"+j+"\n";
		printf("ThreadSend: flag %d\n",j);
	}

}
void threadRecv(){
	int delay=140000000;
	int flags=5;
	
	for(int j=0;j<flags;j++)
	{
		for(int i=0; i<delay;i++){
		//wait
		}
		//std::cout << "send,flag"+j+"\n";
		printf("ThreadRecv: flag %d\n",j);
	}	
}

int main(){
	std::thread first (threadSend);
	std::thread second (threadRecv);
	first.join();
	second.join();
	return 0;
}

