$fn = 128;

// variables
pcbZ1 = 0;
pcbZ2 = 1.6;

baseZ0 = 0.3 + 0.1; // height of bottom plane (+0.1 for adhesive label)
baseZ2 = baseZ0 + 1.7; // base plate
baseZ1 = baseZ2 - 0.8; // bearing surface

actuatorZ = baseZ1 + 9.8;

switchX = 6.5;
switchY = 10;
switchWidth = 5.8;
switchHeight = 12.8;
switchZ2 = actuatorZ - 1;
switchZ1 = switchZ2 - 5;
switchZ0 = switchZ1 - 1.5;

axisX2 = 37.2 / 2;
axisX1 = axisX2 - 15.85;
axisZ = baseZ2 + 8.2;
axisD = 2.5;

bodyWidth = 40;
bodyHeight = 33.4;
bodyZ1 = axisZ - 2.5;
bodyZ2 = axisZ + axisD / 2;

leverWidth = 16.2;
leverHeight = (40-34.4)/2;
leverY = (34.4+40)/2/2;
leverZ = baseZ2 + 6.9;
leverAngle = 5.4; // maximum actuation angle


// box with center/size in x/y plane and z ranging from z1 to z2
module box(x, y, w, h, z1, z2) {
	translate([x-w/2, y-h/2, z1])
		cube([w, h, z2-z1]);
}

module cuboid(x1, y1, x2, y2, z1, z2) {
	translate([x1, y1, z1])
		cube([x2-x1, y2-y1, z2-z1]);
}

module barrel(x, y, r, z1, z2) {
	translate([x, y, z1])
		cylinder(r=r, h=z2-z1);
}

module axis(x, y, d, l, z) {
	translate([x-l/2, y, z])
		rotate([0, 90, 0])
			cylinder(r=d/2, h=l);

}

module cone(x, y, r1, r2, z1, z2) {
	translate([x, y, z1])
		cylinder(r1=r1, r2=r2, h=z2-z1);
}

module longHoleX(x, y, r, l, z1, z2) {
	barrel(x=x-l/2, y=y, r=r, z1=z1, z2=z2);
	barrel(x=x+l/2, y=y, r=r, z1=z1, z2=z2);
	box(x=x, y=y, w=l, h=r*2, z1=z1, z2=z2);
}

module longHoleY(x, y, r, l, z1, z2) {
	barrel(x=x, y=y-l/2, r=r, z1=z1, z2=z2);
	barrel(x=x, y=y+l/2, r=r, z1=z1, z2=z2);
	box(x=x, y=y, w=r*2, h=l, z1=z1, z2=z2);
}

module frustum(x, y, w1, h1, w2, h2, z1, z2) {
	// https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Primitive_Solids#polyhedron	
	points = [
		// lower square
		[x-w1/2,  y-h1/2, z1],  // 0
		[x+w1/2,  y-h1/2, z1],  // 1
		[x+w1/2,  y+h1/2, z1],  // 2
		[x-w1/2,  y+h1/2, z1],  // 3
		// upper square
		[x-w2/2,  y-h2/2, z2],  // 4
		[x+w2/2,  y-h2/2, z2],  // 5
		[x+w2/2,  y+h2/2, z2],  // 6
		[x-w2/2,  y+h2/2, z2]]; // 7
	faces = [
		[0,1,2,3],  // bottom
		[4,5,1,0],  // front
		[7,6,5,4],  // top
		[5,6,2,1],  // right
		[6,7,3,2],  // back
		[7,4,0,3]]; // left  
	polyhedron(points, faces);
}

/*
module axis() {
	translate([-axisLength/2, 0, axisZ])
		rotate([0, 90, 0])
			cylinder(r=axisD/2, h=axisLength);

}
*/
module switchSpikes(x, y, angle) {
	translate([x, y, 0]) {
		rotate([0, 0, angle]) {
					
			// holes
			axis(0, 6.5/2, 1.9, 6, switchZ1);
			axis(0, -6.5/2, 1.9, 6, switchZ1);
		}
	}
}

module body() {
	y1 = switchY-switchWidth/2;
	height = y1*2;
	z1 = bodyZ1-2;
	z2 = bodyZ2-tan(5.4)*bodyWidth/2;
	z3 = bodyZ2-tan(5.4)*y1;
	
	// axis
	axis(0, 0, axisD, bodyWidth, axisZ);

	// carrier top
	frustum(0, 0,
		bodyWidth, height,
		bodyWidth, height-4,
		z1, bodyZ1);

	for (side = [1, -1]) {

		// carrier sides
		box(0, (y1-1)*side, bodyWidth, 2, pcbZ2, z1);
		box(0, y1*side, bodyWidth, 2, switchZ0-1.5, switchZ0-0.1);

		// catwalks
		box((axisX1+1)*side, 0, 2, height, z1, z3);
		frustum((axisX1+1)*side, 0, 2, height, 2, 0.1, z3, bodyZ2);

		difference() {
			union() {
				box((bodyWidth/2-1.2)*side, 0,
					2.4, bodyWidth,
					z1-1, z2);
				frustum((bodyWidth/2-1.2)*side, 0,
					2.4, bodyWidth,
					2.4, 0.1,
					z2, bodyZ2);
			}
			box((axisX2+1)*side, 0, 2, 7, bodyZ1, bodyZ2);
		}
		
		box((bodyWidth/2-3.5)*side, leverY,
			7, leverHeight,
			pcbZ2, leverZ-2-1.8);
		box((bodyWidth/2-3.5)*side, -leverY,
			7, leverHeight,
			pcbZ2, leverZ-2-1.8);
	}
	
	// spies for switch holes
	switchSpikes(switchX, -switchY+2, -90);
	switchSpikes(switchX, switchY-2, -90);
	switchSpikes(-switchX, -switchY+2, 90);
	switchSpikes(-switchX, switchY-2, 90);
}


module lever(side, angle) {
	x1 = (leverWidth/2+1);
	x2 = 15.2;
	w = 4;
	translate([0, 0, axisZ]) {
		rotate([angle, 0, 0]) {
			translate([0, 0, -axisZ]) {
				box(x1*side, leverY,
					leverWidth, leverHeight,
					leverZ-2, leverZ);
				box(x1*side, -leverY,
					leverWidth, leverHeight,
					leverZ-2, leverZ);
							
				difference() {
					union() {
						box(x2*side, 0,
							w, leverY * 2,
							leverZ-2, leverZ);
						frustum(x2*side, 0,
							w, leverY * 2,
							w, 0.1,
							leverZ-0.2, bodyZ2);
					}
					frustum(x2*side, 0,
						w+0.2, leverY * 2,
						w+0.2, 0.1,
						leverZ-2.1, leverZ+0.0);	
				}
			}
		}
	}
	box(x2*side, 0,
		w, axisD,
		bodyZ1-1, bodyZ2-1);
}

module switch(x, y, angle) {
	translate([x, y, 0]) {
		rotate([0, 0, angle]) {
			// body
			color([0, 0, 0]) {
				difference() {
					box(0, 0, switchWidth, switchHeight, switchZ0, switchZ2);
					
					// holes
					axis(0, 6.5/2, 2, 6, switchZ1);
					axis(0, -6.5/2, 2, 6, switchZ1);
				}
			}
			
			// actuator
			color([1, 0, 0]) {
				box(0, 5.2-6.5/2, 2.9, 1.2, switchZ2, switchZ2 + 1);
			}
			
			//barrel(0, 0, 0.9/2, switchZ0-3.5, switchZ0);
			barrel(0, 5.08, 0.9/2, switchZ0-3.5, switchZ0);
			//barrel(0, -5.08, 0.9/2, switchZ0-3.5, switchZ0);
		}
	}
}

module actuators(angle) {
	color ([1, 1, 1]) {
		translate([0, 0, axisZ]) {
			rotate([angle, 0, 0]) {
				translate([0, 0, -axisZ]) {
					box(17/2, 20/2, 3.2, 1, actuatorZ, actuatorZ+1);
					box(-17/2, 20/2, 3.2, 1, actuatorZ, actuatorZ+1);
					box(17/2, -20/2, 3.2, 1, actuatorZ, actuatorZ+1);
					box(-17/2, -20/2, 3.2, 1, actuatorZ, actuatorZ+1);
					
					box(0, leverY, 34, leverHeight, leverZ, leverZ+1);
					box(0, -leverY, 34, leverHeight, leverZ, leverZ+1);
			
				}
			}
		}	
	}
}

body();
lever(1, 0);
lever(-1, 0);

switch(switchX, -switchY, -90);
switch(switchX, switchY, -90);
//switch(-switchX, -switchY, 90);
//switch(-switchX, switchY, 90);

//actuators(0);
//actuators(-leverAngle);
