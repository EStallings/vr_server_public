
#ifndef NET_H
#define NET_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>

namespace net {

	// IP address
	class Address {
	public:
		
		Address() {
			address = 0;
			port = 0;
			timeout = 0;
		}
	
		Address(unsigned int address, unsigned short port) {
			this->address = address;
			this->port = port;
			this->timeout = 0;
		}
	
		unsigned int getAddress() const {
			return address;
		}
	
		unsigned short getPort() const { 
			return port;
		}

		unsigned int getTimeout() const {
			return timeout;
		}

		void incTimeout() {
			timeout++;
		}

		void maxTimeout() {
			timeout = 999999;
		}

		void resetTimeout() {
			timeout = 0;
		}
	
		// Need these to ensure we can store Addresses in STL containers
		bool operator == (const Address & other) const {
			return address == other.address && port == other.port;
		}
	
		bool operator != (const Address & other) const {
			return ! (*this == other);
		}
	
	private:
		unsigned int timeout;
		unsigned int address;
		unsigned short port;
	};

	// sockets
	class Socket {
	public:
	
		Socket() {
			socket = 0;
		}
	
		bool open(unsigned short port) {
			assert(!isOpen());
		
			// create socket
			socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

			if (socket <= 0) {
				printf("failed to create socket\n");
				socket = 0;
				return false;
			}

			// bind to port
			sockaddr_in address;
			address.sin_family = AF_INET;
			address.sin_addr.s_addr = INADDR_ANY;
			address.sin_port = htons((unsigned short) port);
		
			if (bind(socket, (const sockaddr*) &address, sizeof(sockaddr_in)) < 0) {
				printf("failed to bind socket\n");
				closeSocket();
				return false;
			}

			// set non-blocking io
			int nonBlocking = 1;
			if (fcntl(socket, F_SETFL, O_NONBLOCK, nonBlocking) == -1) {
				printf("failed to set non-blocking socket\n");
				closeSocket();
				return false;
			}

			return true;
		}
	
		void closeSocket() {
			if (socket != 0) {
				close(socket);
				socket = 0;
			}
		}
	
		bool isOpen() const {
			return socket != 0;
		}
	
		bool send(const Address & destination, const void * data, int size) {
			assert(data);
			assert(size > 0);
		
			if (socket == 0)
				return false;
		
			sockaddr_in address;
			address.sin_family = AF_INET;
			address.sin_addr.s_addr = htonl(destination.getAddress());
			address.sin_port = htons((unsigned short) destination.getPort());

			int sent_bytes = sendto(socket, (const char*)data, size, 0, (sockaddr*)&address, sizeof(sockaddr_in));

			return sent_bytes == size;
		}
	
		int receive(Address & sender, void * data, int size) {
			assert(data);
			assert(size > 0);
		
			if (socket == 0)
				return false;
			
			
			sockaddr_in from;
			socklen_t fromLength = sizeof(from);

			int received_bytes = recvfrom(socket, (char*)data, size, 0, (sockaddr*)&from, &fromLength);

			if (received_bytes <= 0)
				return 0;

			unsigned int address = ntohl(from.sin_addr.s_addr);
			unsigned int port = ntohs(from.sin_port);

			sender = Address(address, port);

			return received_bytes;
		}
		int id;
		
	private:
		int socket;
	};
}

#endif