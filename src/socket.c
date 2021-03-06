// Copyright (c) 2016 David Morton
// Use of this source code is governed by a license that can be
// found in the LICENSE file.

#include "include/socket.h"
#include <string.h> // memcpy()
#include <fcntl.h>

int socket_EXPORT Socket(int family, int type, int protocol)
{
	int sock = socket(family, type, protocol);
	if (sock < 0)
	{
		perror("Error in Socket()");
		exit(1); // Exit failure
	}

	int enable = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&enable, sizeof(enable)) < 0)
	{
		perror("setsockopt(SO_REUSEADDR) failed");
	}

#ifdef SO_REUSEPORT
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (void*)&enable, sizeof(enable)) < 0)
	{
		perror("setsockopt(SO_REUSEPORT) failed");
	}
#endif
	return sock;
}

int socket_EXPORT Connection(const char *hostname, const char *service /* Port number */, int type /* Client or Server */, int protocol /* UDP or TCP */)
{
	int errorReturnValue;

#ifdef _WIN32
	WSADATA wsaData;
	// Initialize Winsock
	errorReturnValue = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (errorReturnValue != 0) {
		printf("WSAStartup failed with error: %d\n", errorReturnValue);
		exit(1);
		return 1;
	}
#endif
	int sockFileDescriptor;
	struct addrinfo hints;
	struct addrinfo *result;
	struct addrinfo *tempAddrInfo;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // use either IPv4 or IPv6
	hints.ai_socktype = protocol; // UDP / TCP

	if(type == TYPE_CLIENT)
	{
		// For clients use the hostname passed in as a parameter
		if ((errorReturnValue = getaddrinfo(hostname, service, &hints, &result)) != 0) {
			fprintf(stderr, "Connection() : getaddrinfo(): %s\n", gai_strerror(errorReturnValue));
			exit(1);
		}
	}
	else if(type == TYPE_SERVER)
	{
		// For servers set flags to passive to signify using this hosts name or to listen for
		// all incoming connections from clients.
		hints.ai_flags = AI_PASSIVE; // use my IP address

		// Use NULL with getaddrinfo() to listen for all incoming connections
		if ((errorReturnValue = getaddrinfo(NULL, service, &hints, &result)) != 0) {
		    fprintf(stderr, "Connection() : getaddrinfo(): %s\n", gai_strerror(errorReturnValue));
		    exit(1); // Exit failure
		}
	}
	else
	{
		// Application type unspecified TYPE_CLIENT / TYPE_SERVER
		printf("Error in Connection() application type unspecified\n");
		return -1;
	}

	// Loop through each result in the addrinfo struct and connect to the first one available
	for(tempAddrInfo = result; tempAddrInfo != NULL; tempAddrInfo = tempAddrInfo->ai_next) {
	    if ((sockFileDescriptor = socket(tempAddrInfo->ai_family, tempAddrInfo->ai_socktype, tempAddrInfo->ai_protocol)) == -1) {
	        perror("Connection() : socket()");
	        continue;
	    }

	    // For each possible connection perform the appropriate connection
	    // process based on application type
	    if(type == TYPE_CLIENT)
	    {
		    // For clients use connect()
			if (connect(sockFileDescriptor, tempAddrInfo->ai_addr, tempAddrInfo->ai_addrlen) == -1) {
				Close(sockFileDescriptor);
				//perror("Connection() : connect()");
				continue;
			}
	    }
	    else if(type == TYPE_SERVER)
	    {
	    	// For servers use bind()
	    	if (bind(sockFileDescriptor, tempAddrInfo->ai_addr, tempAddrInfo->ai_addrlen) == -1) {
				Close(sockFileDescriptor);
				perror("Connection() : bind()");
				continue;
			}
	    }
	    else
	    {
	    	// Application type unspecified TYPE_CLIENT / TYPE_SERVER
			printf("Error in Connection() application type unspecified\n");
	    	return -1;
	    }

	    // Connection successful :)
	    break;
	}

	if (tempAddrInfo == NULL) {
	    // No connections found for the peer
	    //fprintf(stderr, "No connections found. Failed to Connect\n");
	    freeaddrinfo(result);
	    return -1;
	    //exit(2);
	}

	// Free the addrinfo struct after using it to store peer information
	freeaddrinfo(result);
	return sockFileDescriptor;
}

// Accept all incoming TCP connections and return a file descriptor
// used to communicate with the client.
int socket_EXPORT Accept(int iListenSocketFileDescriptor, struct Address *address)
{
	int connfd;
	socklen_t client_len = sizeof(address->m_sAddress);

	// Accept connections from clients
	connfd = accept(iListenSocketFileDescriptor, (struct sockaddr *) &address->m_sAddress, &client_len);

	if (connfd < 0)
	{
		// There was an error (interrupt)
		if( errno == EINTR )
		{
			// Try another Accept() in the event of a system interrupt
			//continue;
			perror("AcceptConnections() system interrupt");
			exit(1); // Exit failaure
		}
		else
		{
			// There was an error other than an interrupt so close the Parent process
			perror("Accept error");
			exit(3);
		}
	}
	return connfd;
}

void socket_EXPORT Connect(int socketFileDescriptor, const struct sockaddr* socketAddress, socklen_t socketSize)
{
	if (connect(socketFileDescriptor, socketAddress, socketSize) < 0)
	{
		perror("Error in Connect()");
		exit(1); // Exit failure
	}
}

int socket_EXPORT Select(int maxFileDescriptorsPlus1, fd_set *readFileDescriptorSet, fd_set *writeFileDescriptorSet, fd_set *exceptFileDescriptorSet, struct timeval *timeout)
{
	int n;
	if ( (n = select(maxFileDescriptorsPlus1, readFileDescriptorSet, writeFileDescriptorSet, exceptFileDescriptorSet, timeout)) < 0)
	{
		perror("Error in Select()");
		exit(1); // Exit failure
	}
	return(n);		/* can return 0 on timeout */
}

int socket_EXPORT Read(int fileDescriptor, void *buffer, size_t numberOfBytes)
{
	int n;
	if ((n = read(fileDescriptor, buffer, numberOfBytes)) == -1)
	{
		perror("Error in Read()");
		exit(1); // Exit failure
	}
	return(n);
}

void socket_EXPORT Write(int fileDescriptor, void *buffer, size_t numberOfBytes)
{
	if (write(fileDescriptor, buffer, numberOfBytes) != numberOfBytes)
	{
		perror("Error in Write()");
		exit(1); // Exit failure
	}
}

void Shutdown(int fileDescriptor, int shutdownOption)
{
	if (shutdown(fileDescriptor, shutdownOption) < 0)
	{
		perror("Error in Shutdown()");
		exit(1); // Exit failure
	}
}

int socket_EXPORT Max(int x, int y)
{
	return ( x < y ) ? y : x;
}

void socket_EXPORT Bind(int socketFileDescriptor, const struct sockaddr* socketAddress, socklen_t socketSize)
{
	if (bind(socketFileDescriptor, socketAddress, socketSize) < 0) {
		perror("Error in Bind()");
		exit(1); // Exit failure
	}
}

void socket_EXPORT Listen(int socketFileDescriptor, int maxListenQSize)
{
	if(listen(socketFileDescriptor, maxListenQSize) < 0)
	{
		perror("Error in Listen()");
		exit(1); // Exit failure
	}
}

void socket_EXPORT MultiplexIO(FILE* fp, int socketFileDescriptor)
{
	int maxFileDescriptorsPlus1;
	int stdinEOF = 0;
	fd_set readFileDescriptorSet;
	char buffer[MAX_BUF_SIZE];
	int numberOfBytesReceived;

	// clear the read set bits
	FD_ZERO(&readFileDescriptorSet);

	for( ; ; )
	{
		if( stdinEOF == 0)
		{
			// get the integer value for the stdin file descriptor and set this is the read set
			FD_SET(_fileno(fp), &readFileDescriptorSet);
		}

		// set the socket file descriptor in the read set
		FD_SET(socketFileDescriptor, &readFileDescriptorSet);

		// find the highest index for the readset
		maxFileDescriptorsPlus1 = Max(_fileno(fp), socketFileDescriptor) + 1;

		// call the select function to check each file descriptor for activity
		Select(maxFileDescriptorsPlus1, &readFileDescriptorSet, NULL, NULL, NULL);

		// socket file descriptor is active
		if( FD_ISSET(socketFileDescriptor, &readFileDescriptorSet) )
		{
			numberOfBytesReceived = Read(socketFileDescriptor, buffer, MAX_BUF_SIZE);
			if( numberOfBytesReceived == 0 )
			{
				if( stdinEOF == 1 )
				{
					// Client has shutdown the connection
					return;
				}
				else
				{
					// Server has terminated the connection
					perror("MultiplexIO() Server terminated");
					exit(0);
				}
			}

			// write the results of reading the socket
			Write(_fileno(stdout), buffer, numberOfBytesReceived);
		}

		// input file descriptor is active
		if( FD_ISSET(_fileno(fp), &readFileDescriptorSet) )
		{
			numberOfBytesReceived = Read(_fileno(fp), buffer, MAX_BUF_SIZE);

			// if the client is terminated the socket is shutdown
			if( numberOfBytesReceived == 0 )
			{
				//printf("Client has terminated the connection");
				stdinEOF = 1;
				Shutdown(socketFileDescriptor, 1);
				FD_CLR(_fileno(fp), &readFileDescriptorSet);
				continue;
			}

			Write(socketFileDescriptor, buffer, numberOfBytesReceived);
		}
	}
}

int socket_EXPORT Send(int socketFileDescriptor, char *message, size_t size, int flags)
{
	int numberOfBytesSent = send(socketFileDescriptor, message, size, flags);
	if(numberOfBytesSent < 0)
	{
		perror("Error in Send()");
		//exit(1); // Exit failure
	}
	return numberOfBytesSent;
}

int socket_EXPORT SendTo(int socketFileDescriptor, char *message, size_t size, int flags, struct sockaddr *sender, socklen_t sendsize)
{
	int numberOfBytesSent = sendto(socketFileDescriptor, message, size, flags, sender, sendsize);
	if(numberOfBytesSent < 0)
	{
		perror("Error in SendTo()");
		exit(1); // Exit failure
	}
	return numberOfBytesSent;
}

int socket_EXPORT Recv(int socketFileDescriptor, char *message, size_t size, int flags)
{
	int numberOfBytesReceived = recv(socketFileDescriptor, message, size, flags);
	if(numberOfBytesReceived < 0)
	{
		//perror("Error in Recv()");
		//exit(1); // Exit failure
	}
	return numberOfBytesReceived;
}

int socket_EXPORT ReceiveFrom(int socketFileDescriptor, char *message, int bufferSize, int flags , struct sockaddr *sender, socklen_t *sendsize)
{
	int numberOfBytesReceived = recvfrom(socketFileDescriptor, message, bufferSize, flags, sender, sendsize);
	if(numberOfBytesReceived < 0)
	{
		perror("Error in ReceiveFrom()");
		exit(1); // Exit failure
	}
	return numberOfBytesReceived;
}

int socket_EXPORT SetNonBlocking(int socketFileDescriptor)
{
#ifdef _WIN32
	unsigned long on = 1;
	if (0 != ioctlsocket(socketFileDescriptor, FIONBIO, &on))
	{
		perror("Error in SetNonBlocking()");
		exit(1); // Exit failure
	}
#else
	// where socketfd is the socket you want to make non-blocking
	int success = fcntl(socketFileDescriptor, F_SETFL, O_NONBLOCK);

	if (success == -1){
	  perror("Error in SetNonBlocking()");
	  //exit(1); // Exit failure
	}
	return success;
#endif
}

void socket_EXPORT Close(int socketFileDescriptor)
{
#ifdef _WIN32
	closesocket(socketFileDescriptor);
	WSACleanup();
#else
	close(socketFileDescriptor);
#endif
}


