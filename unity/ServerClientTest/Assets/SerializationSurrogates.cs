using System.Runtime.Serialization;
using UnityEngine;

sealed class Vector3SerializationSurrogate : ISerializationSurrogate {
	 
	// Method called to serialize a Vector3 object
	public void GetObjectData(System.Object obj,
							   SerializationInfo info, StreamingContext context) {
		 
		Vector3 v3 = (Vector3) obj;
		info.AddValue("x", v3.x);
		info.AddValue("y", v3.y);
		info.AddValue("z", v3.z);
		// Debug.Log(v3);
	}
	 
	// Method called to deserialize a Vector3 object
	public System.Object SetObjectData(System.Object obj,
										SerializationInfo info, StreamingContext context,
										ISurrogateSelector selector) {
		 
		Vector3 v3 = (Vector3) obj;
		v3.x = (float)info.GetValue("x", typeof(float));
		v3.y = (float)info.GetValue("y", typeof(float));
		v3.z = (float)info.GetValue("z", typeof(float));
		obj = v3;
		return obj;   // Formatters ignore this return value //Seems to have been fixed!
	}
}

sealed class QuaternionSerializationSurrogate : ISerializationSurrogate {
	 
	// Method called to serialize a Quaternion object
	public void GetObjectData(System.Object obj,
							   SerializationInfo info, StreamingContext context) {
		 
		Quaternion q = (Quaternion) obj;
		info.AddValue("x", q.x);
		info.AddValue("y", q.y);
		info.AddValue("z", q.z);
		info.AddValue("w", q.w);
		// Debug.Log(q + " : " + q.x + " , " + q.y + " , " + q.z + " , " + q.w);
	}
	 
	// Method called to deserialize a Quaternion object
	public System.Object SetObjectData(System.Object obj,
										SerializationInfo info, StreamingContext context,
										ISurrogateSelector selector) {
		 
		Quaternion q = (Quaternion) obj;
		q.x = (float)info.GetValue("x", typeof(float));
		q.y = (float)info.GetValue("y", typeof(float));
		q.z = (float)info.GetValue("z", typeof(float));
		q.w = (float)info.GetValue("w", typeof(float));
		obj = q;
		return obj;   // Formatters ignore this return value //Seems to have been fixed!
	}
}