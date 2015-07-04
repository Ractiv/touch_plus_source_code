using UnityEngine;
using System.Collections;
using System.Threading;
using System.Collections.Generic;

public class Main : MonoBehaviour
{
    Assets.IPC ipc = new Assets.IPC("unity_demo");
    GameObject mainCamera;
    GameObject sphere;

    float x = 0;
    float y = 0;
    float z = 0;

    string status = "";

	void Start ()
    {
        mainCamera = GameObject.Find("Main Camera");
        sphere = GameObject.Find("Sphere");

        ipc.SetUDPCallback(delegate(string message)
        {
            print(message);

            if (message != "no_left" && message != "left" && message != "no_right" && message != "right")
            {
                string[] xyzStr = message.Split('!');
                string xStr = xyzStr[0];
                string yStr = xyzStr[1];
                string zStr = xyzStr[2];

                print(xStr + " " + yStr + " " + zStr);

                float.TryParse(xStr, out x);
                float.TryParse(yStr, out y);
                float.TryParse(zStr, out z);
            }
            else
                status = message;

            return 1;
        });
	}
	
	void Update ()
    {
        ipc.Update();

        if (status == "left")
        {
            // sphere.transform.position = new Vector3(z / 5, y / 5, x / 5);
            mainCamera.transform.position = new Vector3(z / 10, y / 5, x / 5);
            mainCamera.transform.LookAt(new Vector3(0, 0, 0));
        }
        else if (status == "right")
        {

        }
	}
}
