#pragma strict
import System.Collections.Generic;

var thePath : List.<Vector3> = new List.<Vector3>();

var sphere : GameObject;

var ray : Ray;
var hit : RaycastHit;
var dtxt : String;
var mouseIsDown : boolean = false;

var lineRender: LineRenderer;
var numberOfPoints : int = 0;

function Start () {
	sphere = GameObject.Find("Sphere");
}

function Update () {

	dtxt = "";


	//method 1 (3D)


	// ray = GetComponent.<Camera>().ScreenPointToRay (Input.mousePosition);
	
	// if (Physics.Raycast (ray, hit, 1000)){
		// if (mouseIsDown) {
			thePath.Add(sphere.transform.position);
			lineRender.SetVertexCount( thePath.Count );
			lineRender.SetPosition(thePath.Count - 1, sphere.transform.position);
		// }
	// }
	
	// if (Input.GetMouseButtonDown(0)){
 		// mouseIsDown = true; 
 	// }
 	// if (Input.GetMouseButtonUp(0)){
 		// mouseIsDown = false;
 	// }
 	
 	// transform.parent.gameObject.transform.eulerAngles.y += 0.2;
	
	//method 2 (2D)
			
	/*
	if( Input.GetKey( KeyCode.Mouse0 ) ) {
		numberOfPoints++;
		lineRender.SetVertexCount( numberOfPoints );
		var mousePos : Vector3 = new Vector3(0,0,0);
		mousePos = Input.mousePosition;
		mousePos.z = 1.0f;
		var worldPos : Vector3 = camera.ScreenToWorldPoint(mousePos);
		lineRender.SetPosition(numberOfPoints - 1, worldPos);
	} else {
		numberOfPoints = 0;
		lineRender.SetVertexCount(0);
	}
	*/
	
}

function OnGUI(){
	for (var z=0;z<thePath.Count;z++){
		//dtxt += thePath[z].x+" "+thePath[z].y+" "+thePath[z].z+"\n";
	}
	GUI.Box(Rect( 20, 20, 200, 600 ), ""+dtxt);
}