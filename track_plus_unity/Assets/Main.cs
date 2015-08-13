/*
 * Touch+ Software
 * Copyright (C) 2015
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Aladdin Free Public License as
 * published by the Aladdin Enterprises, either version 9 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Aladdin Free Public License for more details.
 *
 * You should have received a copy of the Aladdin Free Public License
 * along with this program.  If not, see <http://ghostscript.com/doc/8.54/Public.htm>.
 */

using UnityEngine;
using System.Collections;
using Assets;

public class Main : MonoBehaviour
{
    IPC ipc;

    // GameObject cube0, cube1, cube2, cube3, cube4;

    // float x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3, xCenter, yCenter, zCenter;

	void Start ()
	{
  	 	ipc = new IPC("unity_plus");

		// cube0 = GameObject.Find("Cube 0");
        // cube1 = GameObject.Find("Cube 1");
        // cube2 = GameObject.Find("Cube 2");
        // cube3 = GameObject.Find("Cube 3");
        // cube4 = GameObject.Find("Cube 4");

		// ipc.SetUDPCallback(delegate(string message)
  //       {
  //       	print(message);

            // string[] data = message.Split('!');

            // float.TryParse(data[0], out x0);
            // float.TryParse(data[1], out y0);
            // float.TryParse(data[2], out z0);

            // float.TryParse(data[3], out x1);
            // float.TryParse(data[4], out y1);
            // float.TryParse(data[5], out z1);

            // float.TryParse(data[6], out x2);
            // float.TryParse(data[7], out y2);
            // float.TryParse(data[8], out z2);

            // float.TryParse(data[9], out x3);
            // float.TryParse(data[10], out y3);
            // float.TryParse(data[11], out z3);

            // float.TryParse(data[12], out xCenter);
            // float.TryParse(data[13], out yCenter);
            // float.TryParse(data[14], out zCenter);

        	// return 1;
        // });
	}
	
	void Update ()
	{
		// ipc.Update();

        // cube0.transform.position = new Vector3(x0 / 100, -y0 / 100, -z0 / 100 + 6);
        // cube1.transform.position = new Vector3(x1 / 100, -y1 / 100, -z1 / 100 + 6);
        // cube2.transform.position = new Vector3(x2 / 100, -y2 / 100, -z2 / 100 + 6);
        // cube3.transform.position = new Vector3(x3 / 100, -y3 / 100, -z3 / 100 + 6);
        // cube4.transform.position = new Vector3(xCenter / 100, -yCenter / 100, -zCenter / 100 + 6);
	}
}
