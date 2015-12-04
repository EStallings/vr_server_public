using UnityEngine; 
using System.IO;
using System.IO.Compression;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;
using System.Collections;
using System.Collections.Generic;

class ObjectWrapper {
	Transform myObj;
	Vector3 lastPos;
	Quaternion lastRot;

	public int timestamp = 1;

	private static int posLength = 92;
	private static int rotLength = 103;

	public ObjectWrapper(Transform t){
		lastPos = t.position;
		lastRot = t.rotation;
		myObj = t;
	}

	public bool Updated(){
		if(lastPos != myObj.position || lastRot != myObj.rotation) {
			lastPos = myObj.position;
			lastRot = myObj.rotation;
			return true;
		}
		return false;
	}
	
	public byte[] GetPacketData(BinaryFormatter bf){
		MemoryStream ms = new MemoryStream();
		bf.Serialize(ms, lastPos);
		bf.Serialize(ms, lastRot);

		return ms.ToArray();
	}
	
	private static System.Object DeserializeObj(byte[] data, BinaryFormatter bf) {
		using (var ms = new MemoryStream(data))
		{
			return bf.Deserialize(ms);
		}
	}

	public void LoadFromPacketData(byte[] data, int ts, BinaryFormatter bf) {
		// Debug.Log("LoadFromPacketData!");
		byte[] posPart = data.SubArray(0, posLength);
		byte[] rotPart = data.SubArray(posLength, rotLength);
		Vector3 posD = (Vector3)DeserializeObj(posPart, bf);
		Quaternion rotD = (Quaternion)DeserializeObj(rotPart, bf);
		myObj.position = posD;
		myObj.rotation = rotD;
		lastPos = posD;
		lastRot = rotD;
		timestamp = ts;
		// Debug.Log("Const");
		// Debug.Log(posD.x);
		// Debug.Log((posD == lastPos) + " and " + (rotD == lastRot));
	}
}

public class ObjectManager : MonoBehaviour {
	public Transform[] globalObjects;
	public client2 client;

	public int masterLocalID = 0;

	private SurrogateSelector ss;
	private MemoryStream ms;
	private BinaryFormatter bf;
	
	private int localId = 0;

	Dictionary<int, ObjectWrapper> objectWrappersWithLocalID; // unsynced with server, awaiting a foreign ID
	Dictionary<int, ObjectWrapper> objectWrappersWithGlobalID;

	// Use this for initialization
	void Start () {
		// Set up our serialization objects - a formatter and the SurrogateSelector
		bf = new BinaryFormatter();
		ss = new SurrogateSelector();
		Vector3SerializationSurrogate v3ss = new Vector3SerializationSurrogate();
		QuaternionSerializationSurrogate qss = new QuaternionSerializationSurrogate();
		ss.AddSurrogate(typeof(Vector3), 
						new StreamingContext(StreamingContextStates.All), 
						v3ss);
		ss.AddSurrogate(typeof(Quaternion), 
						new StreamingContext(StreamingContextStates.All), 
						qss);
		bf.SurrogateSelector = ss;

		// Set up maps for object wrappers
		objectWrappersWithLocalID = new Dictionary<int, ObjectWrapper>();
		objectWrappersWithGlobalID = new Dictionary<int, ObjectWrapper>();

		// Process the statically-assigned objects.
		for(int i = 0; i < globalObjects.Length; i++) {
			// print("Adding global object: " + i);
			ObjectWrapper ow = new ObjectWrapper(globalObjects[i]);
			objectWrappersWithGlobalID.Add(i, ow);
			client.AddSendPacket("i", 0, i, ow.GetPacketData(bf));
		}
	}
	
	// Update is called once per frame
	void Update () {
		foreach(KeyValuePair<int, ObjectWrapper> pair in objectWrappersWithLocalID) {
			ObjectWrapper ow = pair.Value;
			if(ow.Updated()){
				//Make packet, add to client to-send list
				ow.timestamp += 1;
				client.AddSendPacket("m", ow.timestamp, pair.Key, ow.GetPacketData(bf));
			}
		}

		foreach(KeyValuePair<int, ObjectWrapper> pair in objectWrappersWithGlobalID) {
			ObjectWrapper ow = pair.Value;
			if(ow.Updated()){
				//Make packet, add to client to-send list
				ow.timestamp += 1;
				client.AddSendPacket("m", ow.timestamp, pair.Key, ow.GetPacketData(bf));
			}
		}
	}

	public void RecieveUpdate(byte[] packet) {
		int timestamp = System.BitConverter.ToInt32(packet, 1);
		int id = System.BitConverter.ToInt32(packet, 5);
		byte[] subPacket = packet.SubArray(9, packet.Length-9);
		ObjectWrapper ow = objectWrappersWithGlobalID[id];
		if(ow != null) ow.LoadFromPacketData(subPacket, timestamp, bf);
		client.packetsProcessed++;
	}

	public void RecieveId(byte[] packet) {
		int newid = System.BitConverter.ToInt32(packet, 1);
		int oldid = System.BitConverter.ToInt32(packet, 5);
		
		ObjectWrapper ow = objectWrappersWithLocalID[oldid];
		if(ow != null){
			objectWrappersWithGlobalID.Add(newid, ow);
			objectWrappersWithLocalID.Remove(oldid);
		}
		
		client.packetsProcessed++;
	}

	public void AddNewItem(Transform t) {
		int id = localId++;
		ObjectWrapper ow = new ObjectWrapper(t);
		objectWrappersWithLocalID.Add(id, ow);
		client.AddSendPacket("j", 0, id, ow.GetPacketData(bf));
	}
}
