using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Text;
using UnityEngine;
using System.Collections;
using System.Collections.Generic;

// State object for receiving data from remote device.
public class StateObject {
	// Client socket.
	public Socket workSocket = null;
	// Size of receive buffer.
	public const int BufferSize = 256;
	// Receive buffer.
	public byte[] buffer = new byte[BufferSize];
	// Received data string.
	public StringBuilder sb = new StringBuilder();
}

public class client2 : MonoBehaviour {
	// Server IP
	public string hostIP = "127.0.0.1";

	// The port number for the remote device.
	public int targetPort = 30000;
	public int listenPort = 3450;

	public ObjectManager objectManager;

	// Modes
	public bool mode_strict_incoming_IP = true; //Ensure incoming packets ONLY come from server
	public bool mode_strict_incoming_PORT = true; //Ensure incoming packets ONLY come from server
	public bool mode_manual_ping = true;
	public int ping_after_ticks = 100;

	public bool handshakeComplete = false;

	// The response from the remote device.
	public string response = String.Empty;
	public byte[] rawResponse;

	public Queue<byte[]> outgoingPackets = new Queue<byte[]>();
	public int ticksSinceLastSend = 0;

	public int packetsRecieved = 0;
	public int packetsProcessed = 0;
	public int packetsPrepared = 0;
	public int packetsSent = 0;

	UdpClient listener = null;
	IPEndPoint groupEP = null;
	IPEndPoint remoteEP = null;
	void Connect() {
		print("Starting Client");
		// Connect to a remote device.
		try {

			listener = new UdpClient(listenPort);
			groupEP = new IPEndPoint(IPAddress.Any, listenPort);

			// Establish the remote endpoint for the socket.
			IPHostEntry ipHostInfo = Dns.GetHostEntry(hostIP);
			IPAddress ipAddress = ipHostInfo.AddressList[0];
			remoteEP = new IPEndPoint(ipAddress, targetPort);

		} catch (Exception e) {	print(e.ToString()); }
	}

	void Kill() {
		try {
			// client.Close();
			Send(Encoding.ASCII.GetBytes("e"));
			listener.Close();
		} catch (Exception e) {	print(e.ToString()); }
	}

	void Receive() {
		try {
			// Debug.Log(listener.Available + " Packets Waiting");
			while(listener.Available > 0){
				rawResponse = listener.Receive(ref groupEP);
				response = Encoding.ASCII.GetString(rawResponse, 0, rawResponse.Length);

				if(rawResponse.Length < 1){
					return;
				}
				char command = Convert.ToChar(rawResponse[0]);

				switch(command){	
					case 'a': // a - handshake signal from client
						// print("Incorrectly Recieved Handshake Signal From Client");
						break;
					case 'b': // b - handshake signal from server
						print("Recieved Server-Side Handshake Signal");
						handshakeComplete = true;
						break;
					case 'c': // c - ping signal, no data
						// print("Incorrectly Pinged");
						break;
					case 'd': // d - disconnected by server due to timeout
						print("Recieved Server-Side Disconnect Signal");
						handshakeComplete = false;
						Send(Encoding.ASCII.GetBytes("a"));
						break;
					case 'e': // e - disconnected from server
						// print("Incorrectly Recieved Client Disconnected");
						break;
					case 'i': // i - initialize global object
						// print("Incorrectly Recieved Initialize Global Object");
						break;
					case 'j': // j - initialize non-global object
						// print("Incorrectly Recieved Initialize Local Object");
						break;
					case 'k': // k - new ID to old ID callback for non-local initiation
						print("Recieved ID Callback");
						objectManager.RecieveId(rawResponse);
						break;
					case 'm': // m - object update Client-To-Server
						// print("Incorrectly Received Object Update");
						break;
					case 'n': // n - object update Server-To-Client
						// print("Recieved Server-Side Object Update");
						packetsRecieved++;
						objectManager.RecieveUpdate(rawResponse);
						break;
					default:
						// print("Unknown Command For Client: " + command);
						break;
				}
			}

		} catch (Exception e) {	print(e.ToString()); }	
	}

	bool Send(byte[] byteData) {
		try {
			// Begin sending the data to the remote device.
			listener.Send(byteData, byteData.Length, remoteEP);
			// print("Sent " + byteData.Length + " bytes to server.");
			ticksSinceLastSend = 0;
			packetsSent++;
			return true;
		} catch (Exception e) {	print(e.ToString()); }
		return false;
	}

	public void AddSendPacket(string prefix, int timestamp, int id, byte[] data){
		byte[] b_pr = Encoding.ASCII.GetBytes(prefix);
		byte[] b_ts = BitConverter.GetBytes(timestamp);
		byte[] b_id = BitConverter.GetBytes(id);

		List<byte> list = new List<byte>();
		list.AddRange(b_pr);
		list.AddRange(b_ts);
		list.AddRange(b_id);
		list.AddRange(data);
		byte[] packet = list.ToArray();
		outgoingPackets.Enqueue(packet);
		packetsPrepared++;
	}

	void Start() {
		Connect();
		Send(Encoding.ASCII.GetBytes("a"));
	}

	void Update() {
		// Debug.Log("Tick");
		ticksSinceLastSend++;
		
		Receive();

		while(outgoingPackets.Count > 0){
			Send(outgoingPackets.Dequeue());
		}
		
		if(ticksSinceLastSend > ping_after_ticks && mode_manual_ping){
			Send(Encoding.ASCII.GetBytes("c"));
		}
	}
}