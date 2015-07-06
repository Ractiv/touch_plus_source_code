using UnityEngine;
using System.Collections;
using Assets;

public class Main : MonoBehaviour
{
    IPC ipc;

	void Start ()
	{
		ipc = new IPC("unity_demo");
		ipc.SetUDPCallback(delegate(string message)
        {
        	print(message);
        	return 1;
        });
	}
	
	void Update ()
	{
		ipc.Update();
	}
}
