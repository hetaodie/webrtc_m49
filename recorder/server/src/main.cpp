#include <sys/types.h>
#include <sys/socket.h>

#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "iostream"
#include "string"

#include "NetworkManager.h"
#include "NetworkListenerImpl.h"

#define MAXLINE 80
#define SERV_PORT 8888

using namespace std;

#if 0
using namespace network;

class callback : public WebSocket::Delegate{

        virtual void onOpen(WebSocket* ws){
		printf("onOpen");
	}

        virtual void onMessage(WebSocket* ws, const WebSocket::Data& data) {

		printf("onMessage");
	}

        virtual void onClose(WebSocket* ws){

		printf("onClose");
	}

        virtual void onError(WebSocket* ws, const WebSocket::ErrorCode& error){

		printf("onError");
	}
};

WebSocket *socket_;
callback *s_callback;
#endif

void do_echo(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen)
{
	int n;
	socklen_t len;
	char mesg[1600];

	for(;;)
	{
		len = clilen;
		/* waiting for receive data */
		n = recvfrom(sockfd, mesg, 1600, 0, pcliaddr, &len);
		printf("recv data %s size %d",mesg,n);
		NetworkManager::getInstance()->handleData(mesg,n);
		/* sent data back to client */
//		sendto(sockfd, mesg, n, 0, pcliaddr, len);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *args[])
{
	
	if(argc != 2)
            {
            printf("usage: server_recorder <port number>\n");
            exit(1);
            }

	int sockfd;
	struct sockaddr_in servaddr, cliaddr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0); /* create a socket */

	/* init servaddr */
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //#define INADDR_ANY   ((unsigned long int) 0x00000000)
	servaddr.sin_port = htons(atoi(args[1]));

	/* bind address and port to socket */
	if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
	{
		perror("bind error");
		exit(1);
	}

	NetworkManager::getInstance();
	NetworkListener * listener = new NetworkListenerImpl();
	NetworkManager::getInstance()->registerListener(listener);

	do_echo(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr));

	return 0;
}

#if 0
int main(int argc, char** argv) {

	printf("fewfe");
	socket_ = new WebSocket();
	s_callback = new callback();

	printf("fewfe");
	if (!socket_->init(*s_callback,"ws://192.168.0.117:5555" ))
    	{
		printf("connect error");
    	}		

	printf("fewfe");
	socket_->close();

        return 1;
}

#endif
