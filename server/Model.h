#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <map>
#include <mutex>

using namespace std;

#define OBJ_PACK_LENGTH 255

std::mutex model_mutex;


// Responsible for keeping the object state for a single object in our 'world' model
int SOID = 0; // Bookkeeping variable;
class StateObject {
public:
	int lastUpdatedTimestamp;
	int lastUpdatedIP;
	int objectID;
	unsigned char data[OBJ_PACK_LENGTH];
	int timesUpdated;

	StateObject() {
		objectID = SOID++;
		lastUpdatedIP = 0;
		timesUpdated = 0;
		lastUpdatedTimestamp = 0;
	}

	// Updates a StateObject - someone's client has updated this data and we need to reflect that here
	// timestamp - most recent update-tick
	// ip - the server-side ID for the user that sent the update
	// newdata - our new data! yay!
	void update(int timestamp, int ip, unsigned char* newData) {
		int i;
		for(i = 0; i < OBJ_PACK_LENGTH; i++) {
			data[i] = newData[i];
		}
		lastUpdatedTimestamp = timestamp;
		lastUpdatedIP = ip;
		timesUpdated++;
	}

	// Needed to store objects in STL maps correctly.
	bool operator < ( const StateObject & other ) const {
		return objectID < other.objectID;
	}
};

// Maintains all the StateObjects that our 'world' model must contain
int modelid = 0; // Bookkeeping variable
class Model {
public:
	Model() {
		id = modelid++;
	}
	vector<int> getUpdatedIds() {
		return updatedIds;
	}

	map<int, StateObject> getStateObjects() {
		return stateObjects;
	}

	// params: newdata is the packet data, ip is the id of the peer that sent the packet
	void sendUpdate(unsigned char* newData, int ip) {
		int timestamp = (int)newData[1];
		int id = (int)newData[5];
		
		if(stateObjects.find(id) == stateObjects.end()) {
			printf("Object not found in model");
			return;
		}

		//If the update is more recent than the last update...		
		if(stateObjects[id].lastUpdatedTimestamp > 250) printf("Old TS: %d, new TS: %d\n", stateObjects[id].lastUpdatedTimestamp, timestamp);
		if((stateObjects[id].lastUpdatedTimestamp <= timestamp)|| (stateObjects[id].lastUpdatedTimestamp > 250 && timestamp < 5)){
			// printf("Updating Object: id: %d, t: %d, ip:%d\n", id, timestamp, ip);
			stateObjects[id].update(timestamp, ip, newData);
			if(std::find(updatedIds.begin(), updatedIds.end(), stateObjects[id].objectID) == updatedIds.end()){
				model_mutex.lock();
				updatedIds.push_back(stateObjects[id].objectID);
				model_mutex.unlock();
			}
		}
	}

	// Initialize a global object - something that comes with the scene.
	void initializeGlobal(unsigned char* newData, int ip) {
		int timestamp = 0;
		int id = (int)newData[5];
		//Check that the object hasn't already been initialized
		if(stateObjects.find(id) != stateObjects.end()){
			return; //the object has already been initialized.
		}

		//Create object and add it to map.
		stateObjects[id] = StateObject();
		stateObjects[id].objectID = id;

		//Need to give the actual data to the object.
		stateObjects[id].update(timestamp, ip, newData);

		// Since all global objects (we trust) have same starting conditions, we can just skip updating other users
	}

	// Initialize a local object - something that one of the clients made that needs to be initialized on other clients
	// returns new, global ID to use for object.
	bool initializeLocal(unsigned char* newData, int ip) {
		int timestamp = 0;

		//Create object and add it to map.
		StateObject stateObject = StateObject();
		int id = stateObject.objectID;
		stateObjects[id] = stateObject;

		//Give data to object.
		if(std::find(updatedIds.begin(), updatedIds.end(), stateObjects[id].objectID) == updatedIds.end())
		stateObject.update(timestamp, ip, newData);

		//We do need to update because other users don't have this object
		if(std::find(updatedIds.begin(), updatedIds.end(), stateObjects[id].objectID) == updatedIds.end()){
			model_mutex.lock();
			updatedIds.push_back(id);
			model_mutex.unlock();
		}

		return id;
	}

	// Clear out the updatedIDs field.
	void resetIdsToUpdate() {
		vector<int> cleared;
		model_mutex.lock();
		updatedIds = cleared;
		model_mutex.unlock();
	}

private:
	map<int, StateObject> stateObjects;
	vector<int> updatedIds;
	int id;
};

#endif