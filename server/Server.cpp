/*
	Standalone server application for UDP networking with Unity for social VR.

	Does not verify any game state. Simply stores and repeats information across peers essentially blindly.
	Serialization/deserialization are left to the clients.

	Authentication is on the roadmap, but not implemented yet.

	The point of this server is to facilitate high throughput for data-intensive VR multiplayer, and very little else.

	Author: Ezra Stallings
*/

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <pthread.h>

#include "Net.h"
#include "Model.h"

using namespace std;
using namespace net;

// Haven't calibrated this yet - timeout can't be properly set until tick rate of server is well-understood.
#define TIMEOUT_TICKS 10000000

// Lock to keep the address list from getting changed at the wrong moment.
std::mutex addresses_mutex;

// The current client list.
vector<Address> addresses;

//The current world state of the game.
Model model = Model();

int packetsSent = 0;
int packetsRecv = 0;

void doReceiving(Socket socket) {
	assert(socket.isOpen());

	while (1) {
		Address sender;
		unsigned char buffer[256];
		int bytes_read = socket.receive(sender, buffer, sizeof(buffer));
		bool newSender = false;
		if (!bytes_read) 
			continue;

		//Just in case the size changes on us...
		addresses_mutex.lock();
		int numAddresses = (int)addresses.size();

		//Don't need mutex (I think) because this is read-only
		if(std::find(addresses.begin(), addresses.end(), sender) != addresses.end()) {
			for(int j = 0; j < numAddresses; ++j) {
				if(addresses[j] == sender) {
					sender = addresses[j];
					break;
				}
			}
		}
		else {
			newSender = true;
		}
		addresses_mutex.unlock();
		
		int newId, theirId;
		unsigned char sendBack[255];
		addresses_mutex.lock();
		sender.resetTimeout();
		addresses_mutex.unlock();

		// Process incoming messages based on first byte
		switch(buffer[0]) {
			case 'a': // a - handshake signal from client
				// printf("Recieved Handshake Signal From Client\n");
				if(newSender) {
					printf("Discovered Peer! \n");
					addresses_mutex.lock();
					addresses.push_back(sender);
					addresses_mutex.unlock();
					sendBack[0] = 'b';
					socket.send(sender, sendBack, sizeof(sendBack));
				}
				break;
			case 'b': // b - handshake signal from server
				// printf("Incorrectly Recieved Server-Side Handshake Signal\n");
				break;
			case 'c': // c - ping signal, no data
				// printf("Pinged by peer!\n");
				break;
			case 'd': // d - disconnected by server due to timeout
				// printf("Incorrectly Recieved Server-Side Disconnect Signal\n");
				break;
			case 'e': // e - disconnected from server
				// printf("Client Disconnected\n");
				addresses_mutex.lock();
				sender.maxTimeout();
				addresses_mutex.unlock();
				break;
			case 'i': // i - initialize global object
							// Object exists for all people. This will only happen during boot-up sequence.
							// Ignored by server if it already has this information
				printf("Initialize Global Object\n");
				model.initializeGlobal(buffer, sender.getAddress());
				break;
			case 'j': // j - initialize non-global object
							// Object created on one client, needs to be replicated across all clients
							// Sender client needs to be informed what the new (server-side) object ID is.
				printf("Initialize Local Object\n");
				newId = model.initializeLocal(buffer, sender.getAddress());
				theirId = (int)buffer[4];

				//Build response packet
				sendBack[0] = (unsigned char)'k';
				sendBack[1] = (unsigned char)newId;
				sendBack[4] = (unsigned char)theirId;

				//Send back newid/oldid pair
				socket.send(sender, sendBack, sizeof(sendBack));
				break;
			case 'k': // k - new ID to old ID callback for non-local initiation
				// printf("Incorrectly Recieved ID Callback\n");
				break;
			case 'm': // m - object update Client-To-Server
				// printf("Object Update\n");
				packetsRecv++; 
				model.sendUpdate(buffer, sender.getAddress());
				break;
			case 'n': // n - object update Server-To-Client
				// printf("Incorrectly Recieved Server-Side Object Update\n");
				break;
			default:
				// printf("Unknown Command For Server: %c\n", buffer[0]);
				break;
		}

	}
}

void doSending(Socket socket) {
	assert(socket.isOpen());

	while (1) {
		map<int, StateObject> stateObjects = model.getStateObjects();
		vector<int> idsToUpdate = model.getUpdatedIds();
		vector<Address> nextAddresses;

		//Just in case the size changes on us...
		addresses_mutex.lock();
		int numAddresses = (int)addresses.size();

		for (int i = 0; i < numAddresses; ++i) {
			
			if(addresses[i].getTimeout() > TIMEOUT_TICKS) {
				printf("Disconnecting Peer %d\n", i);
				unsigned char data[] = {'d'};
				socket.send(addresses[i], data, 1);
				continue;
			}

			int ip = addresses[i].getAddress();
			for(int j = 0; j < (int) idsToUpdate.size(); ++j) {
				int id = idsToUpdate[j];

				if(stateObjects.find(id) != stateObjects.end()) {
					if(stateObjects[id].lastUpdatedIP == ip) continue;
					packetsSent++;
					stateObjects[id].data[0] = (unsigned char)'n';
					socket.send(addresses[i], stateObjects[id].data, OBJ_PACK_LENGTH);
				}
			}
			
			addresses[i].incTimeout();
			nextAddresses.push_back(addresses[i]);
		}
		addresses = nextAddresses;
		addresses_mutex.unlock();

		model.resetIdsToUpdate();
		
		// Lock at ~60fps
		usleep(16666);
	}
}

// Launch with first arg as port number, otherwise it uses 30000
int main(int argc, char * argv[]) {
	
	// create socket
	int port = 30000;

	if (argc == 2)
		port = atoi(argv[1]);

	printf("Initializing on port: %d\n", port);

	Socket socket;
	if (!socket.open(port)) {
		printf("Failed to open socket\n");
		return 1;
	}
	
	// send and receive packets until the user ctrl-breaks...
	std::thread sendThread (doSending, socket);
	std::thread recvThread (doReceiving, socket);
	sendThread.join();
	recvThread.join();

	return 0;
}
