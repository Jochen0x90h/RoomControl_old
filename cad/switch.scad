$fn = 128;

// variables
minWall = 1;
minGap = 0.8;

pcbZ1 = 0;
pcbZ2 = 1.6;

baseZ0 = 0.3 + 0.1; // height of bottom plane (+0.1 for adhesive label)
baseZ2 = baseZ0 + 1.7; // base plate
baseZ1 = baseZ2 - 0.8; // bearing surface

axisD = 2.5;
axisX2 = 37.2 / 2; // outer rocker axis
axisX1 = axisX2 - 15.85; // inner rocker axis
axisZ = baseZ2 + 8.2;
axisCoutoutWidth = 7;
axisCutoutZ = axisZ - 2.5;

bodyWidth = 40;
bodyHeight = 33.4;
bodyZ1 = pcbZ1 + 4.0; // length of through-hole wires 
bodyZ2 = axisZ + axisD / 2;

// switch actuator
actuatorWidth = 3.2;
actuatorX = 17 / 2;
actuatorY = 20 / 2;
actuatorZ = baseZ1 + 9.8;

// micro switch Omron D2F-01
switchWidth = 5.8 + 0.2; // tolerance
switchHeight = 12.8;
switchX = switchHeight / 2;
switchY = 10;
switchZ2 = actuatorZ - 1 - 1;
switchZ1 = switchZ2 - 5;
switchZ0 = switchZ1 - 1.5;
switchesX2 = switchX + switchHeight / 2;
switchesWidth = switchesX2 * 2;
switchesY2 = switchY + switchWidth / 2;

// middle carrier
carrierWidth = 48;
carrierHeight = (switchY - switchWidth / 2) * 2;
carrierZ1 = pcbZ2;
carrierZ2 = bodyZ1 + 1;

// top above switches
topWidth = switchesWidth;
topHeight = switchWidth + 2;
topY = switchY;
topY1 = switchY - topHeight/2;
topY2 = switchY + topHeight/2;

screwX = 20.5;
screwY = 6.6;
screwD = 2.5;
screwMountSize = 5.8; // overall clearance is 6x6mm

// actuation angle
angle = 5.4; // maximum actuation angle
tanAngle = tan(angle);

// actuator bar (front)
barWidth = axisX2 - minWall - minGap;
barX = barWidth / 2;
barY1 = 34.4 / 2;
barY2 = 40 / 2;
barY = (barY1 + barY2) / 2;
barZ2 = baseZ2 + 6.9;
barZ1 = barZ2 - 2; // thickness of bar
barHeight = barY2 - barY1;
barTravel = barY * tanAngle;

// lever (left / right)
leverX1 = switchesX2 + 1; // 1mm left/right tolerance
leverX2 = axisX2 - minWall - minGap;
leverX = (leverX1 + leverX2) / 2;
leverY1 = minGap / 2;
leverY2 = topY2 + minGap;// 33.3 / 2;
leverY = (leverY1 + leverY2) / 2;
leverZ1 = pcbZ1 + 4 + 1;
leverZ2 = bodyZ2;
leverZ = (leverZ1 + leverZ2) / 2;
leverWidth = leverX2 - leverX1;
leverHeight = leverY2 - leverY1;

// lever front
front1Y1 = topY2 + minGap;
front1Y2 = barY1 - minGap;
front1Y = (front1Y1 + front1Y2) / 2;
front1Height = front1Y2 - front1Y1;
front2Y1 = topY2 + minGap;
front2Y2 = 33.3 / 2;
front2Y = (front2Y1 + front2Y2) / 2;
front2Height = front2Y2 - front2Y1;


// mount
mount1X1 = 22.5;
mount1X2 = 24;
mount1X = (mount1X1 + mount1X2) / 2;
mount1Y1 = 0;
mount1Y2 = 18.2;
mount1Y = (mount1Y1 + mount1Y2) / 2;
mount1Width = (mount1X2 - mount1X1);
mount1Height = (mount1Y2 - mount1Y1);
mount2X1 = 13.2;
mount2X2 = 18.2;
mount2X = (mount2X1 + mount2X2) / 2;
mount2Y1 = 22;
mount2Y2 = 24;
mount2Y = (mount2Y1 + mount2Y2) / 2;
mount2Width = (mount2X2 - mount2X1);
mount2Height = (mount2Y2 - mount2Y1);
mountZ1 = pcbZ2 + 2; // thickness of frame
mountZ2 = bodyZ2 - mount2Y2 * tanAngle;
mountZ3 = bodyZ2 - mount1Y2 * tanAngle;
mountDiag = 58.1;



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

module slopeX(x, y, w, h, z1, z2, z3, z4, rl=1) {
	// https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Primitive_Solids#polyhedron

	if (rl < 0) {
		slopeX(x, y, w, h, z2, z1, z4, z3);
	} else {
		x1 = x - w / 2;
		x2 = x + w / 2;
		y1 = y - h / 2;
		y2 = y + h / 2;
		
		points = [
			// lower square
			[x1,  y1, z1],  // 0
			[x2,  y1, z2],  // 1
			[x2,  y2, z2],  // 2
			[x1,  y2, z1],  // 3
			// upper square
			[x1,  y1, z3],  // 4
			[x2,  y1, z4],  // 5
			[x2,  y2, z4],  // 6
			[x1,  y2, z3]]; // 7
		faces = [
			[0,1,2,3],  // bottom
			[4,5,1,0],  // front
			[7,6,5,4],  // top
			[5,6,2,1],  // right
			[6,7,3,2],  // back
			[7,4,0,3]]; // left  
		polyhedron(points, faces);
	}
}

module slopeY(x, y, w, h, z1, z2, z3, z4, tb=1) {
	// https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Primitive_Solids#polyhedron

	if (tb < 0) {
		slopeY(x, y, w, h, z2, z1, z4, z3);
	} else {
		x1 = x - w / 2;
		x2 = x + w / 2;
		y1 = y - h / 2;
		y2 = y + h / 2;	
		
		points = [
			// lower square
			[x1,  y1, z1],  // 0
			[x2,  y1, z1],  // 1
			[x2,  y2, z2],  // 2
			[x1,  y2, z2],  // 3
			// upper square
			[x1,  y1, z3],  // 4
			[x2,  y1, z3],  // 5
			[x2,  y2, z4],  // 6
			[x1,  y2, z4]]; // 7
		faces = [
			[0,1,2,3],  // bottom
			[4,5,1,0],  // front
			[7,6,5,4],  // top
			[5,6,2,1],  // right
			[6,7,3,2],  // back
			[7,4,0,3]]; // left  
		polyhedron(points, faces);
	}
}

module drill(x, y, d) {
	h1 = 2;
	w1 = h1 + 2;
	h2 = d + 0.1;
	w2 = h2 + 2;
	frustum(x, y, w1, h1, w2, h2, pcbZ2-0.1, pcbZ2+1.5);
	box(x, y, w2, h2, pcbZ2, bodyZ1);
}

module drills() {
	drill(1.6, 2.54, 1);
	drill(1.6, -2.54, 1);
	drill(6.68, 2.54, 1);
	drill(6.68, -2.54, 1);
	drill(9.22, 2.54, 1);
	drill(9.22, -2.54, 1);
	drill(11.76, 2.54, 1);
	drill(11.76, -2.54, 1);
	drill(-11.76, 2.54, 1);
	drill(-11.76, -2.54, 1);
	drill(-9.22, 2.54, 1);
	drill(-9.22, -2.54, 1);
	drill(-6.68, 2.54, 1);
	drill(-6.68, -2.54, 1);
	drill(-1.6, 2.54, 1);
	drill(-1.6, -2.54, 1);
	drill(17.46, 0, 1);
	drill(17.46, -2.54, 1);
	drill(17.46, -5.08, 1);
	drill(17.46, -10.16, 1);
	drill(22.54, 0, 1);
	drill(22.54, -2.54, 1);
	drill(22.54, -5.08, 1);
	drill(22.54, -10.16, 1);
	drill(-22.54, 0, 1);
	drill(-22.54, -2.54, 1);
	drill(-22.54, -5.08, 1);
	drill(-22.54, -10.16, 1);
	drill(-17.46, 0, 1);
	drill(-17.46, -2.54, 1);
	drill(-17.46, -5.08, 1);
	drill(-17.46, -10.16, 1);
	drill(9.24, -25.5, 1);
	drill(11.78, -25.5, 1);
	drill(14.32, -25.5, 1);
	drill(16.86, -25.5, 1);
	drill(19.4, -25.5, 1);
	drill(1.32, 10, 1.1);
	drill(6.4, 10, 1.1);
	drill(11.48, 10, 1.1);
	drill(-5, -16, 1.1);
	drill(-5, -21, 1.1);
	drill(0, -16, 1.1);
	drill(0, -21, 1.1);
	drill(5, -16, 1.1);
	drill(5, -21, 1.1);
	drill(-14.85, 21, 1.1);
	drill(-14.85, 16, 1.1);
	drill(-9.85, 21, 1.1);
	drill(-9.85, 16, 1.1);
	drill(-4.85, 21, 1.1);
	drill(-4.85, 16, 1.1);
	drill(0.15, 21, 1.1);
	drill(0.15, 16, 1.1);
	drill(5.15, 21, 1.1);
	drill(5.15, 16, 1.1);
	drill(10.15, 21, 1.1);
	drill(10.15, 16, 1.1);
	drill(15.15, 21, 1.1);
	drill(15.15, 16, 1.1);
	drill(1.32, -10, 1.1);
	drill(6.4, -10, 1.1);
	drill(11.48, -10, 1.1);
	drill(-11.48, -10, 1.1);
	drill(-6.4, -10, 1.1);
	drill(-1.32, -10, 1.1);
	drill(-11.48, 10, 1.1);
	drill(-6.4, 10, 1.1);
	drill(-1.32, 10, 1.1);
}

module switchSpikes(x, y, angle) {
	translate([x, y, 0]) {
		rotate([0, 0, angle]) {
					
			// holes
			axis(0, 6.5/2, 1.9, 6, switchZ1);
			axis(0, -6.5/2, 1.9, 6, switchZ1);
		}
	}
}

module catwalk(x, w) {
	for (tb = [1, -1]) {
		slopeY(x, carrierHeight/4*tb,
			w, carrierHeight/2,
			bodyZ1, bodyZ1,
			bodyZ2, bodyZ2-carrierHeight/2*tanAngle,
			tb);
	}
}

module body() {
	
	// width of both switches
	switchesWidth = switchX * 2 + switchHeight + 0.1;
	
	// catwalks
	leftCatwalkWidth = 2;
	leftCatwalkX = axisX1 + leftCatwalkWidth / 2;
	middleCatwalkWidth = 2;
	middleCatwalkX = leverX1 - minGap - middleCatwalkWidth / 2;
	rightCatwalkWidth = bodyWidth / 2 - axisX2 + minWall;
	rightCatwalkX = bodyWidth / 2  - rightCatwalkWidth / 2;

	// actuator
	for (rl = [1, -1]) {
		box(actuatorX*rl, 0,
			actuatorWidth, (actuatorY+1)*2,
			actuatorZ-1, actuatorZ);
	}
	
	// switch top
	difference() {
		union() {
			for (tb = [1, -1]) {
				// switch top
				slopeY(0, topY*tb,
					topWidth, topHeight,
					switchZ2, switchZ2,
					bodyZ2-tanAngle*topY1, bodyZ2-tanAngle*topY2,
					tb);

				// switch holder
				box(0, (topY2-0.5)*tb,
					topWidth, 1,
					switchZ1, bodyZ2-tanAngle*topY2);
			}
		}
		
		// right / left
		for (rl = [1, -1]) {
			// cutaway space for actuator
			box(actuatorX*rl, 0,
				actuatorWidth+minGap, switchesY2*2,//(actuatorY+1+minGap)*2,
				switchZ2-0.1, bodyZ2);
		}
	}
	
	// carrier, axis and screw mount blocks
	difference() {
		// additive components
		union() {
			// axis
			axis(0, 0, axisD, carrierWidth, axisZ);
			
			// carrier
			box(0, 0, carrierWidth, carrierHeight, carrierZ1, carrierZ2);
			
			for (rl = [1, -1]) {			
				// left catwalk
				catwalk(leftCatwalkX*rl, leftCatwalkWidth);
				
				// middle catwalk
				catwalk(middleCatwalkX*rl, middleCatwalkWidth);		
				
				// right catwalk with axis cutout
				difference() {
					catwalk(rightCatwalkX*rl, rightCatwalkWidth);
					box((axisX2+1)*rl, 0, 2, axisCoutoutWidth, axisCutoutZ, bodyZ2);
				}

				// screw mount block
				slopeY(screwX*rl, screwY,
					screwMountSize, screwMountSize,
					pcbZ2, pcbZ2,
					bodyZ2-(screwY-3)*tanAngle, bodyZ2-(screwY+3)*tanAngle);
			}
		}
		
		// right / left
		for (rl = [1, -1]) {
					
			// cut away space for lever
			box(leverX*rl, 0,
				leverWidth+2*minGap, leverHeight,
				leverZ1-max(minGap, carrierHeight/2*tanAngle+0.1), leverZ2+1);
		
			// cut away mount for lever
			box(leverX*rl, 0,
				leverWidth+2*minGap, minGap+1+minGap+1+minGap,
				pcbZ2+1.5, leverZ2);			
			
			// hole for screw
			barrel(screwX*rl, screwY, screwD/2, pcbZ1, bodyZ2);
		}
	}

	
	
	// spies for switch holes
	//switchSpikes(switchX, -switchY+2, -90);
	//switchSpikes(switchX, switchY-2, -90);
	//switchSpikes(-switchX, -switchY+2, 90);
	//switchSpikes(-switchX, switchY-2, 90);
}


module lever(rl, angle) {

	// top/bottom
	for (tb = [1, -1]) {
		// connection to carrier
		box(leverX*rl, (minGap/2+0.5)*tb,
			leverWidth, 1, pcbZ2, leverZ2);
		
		// rotatable part
		translate([0, 0, axisZ]) {
			rotate([angle, 0, 0]) {
				translate([0, 0, -axisZ]) {
					// lever left/right
					difference() {
						box(leverX*rl, leverY*tb,
							leverWidth, leverHeight, leverZ1, leverZ2);
						
						// cut away gap so that lever can rotate
						axis(leverX*rl, (leverY1+1+0.5)*tb,
							1, leverWidth+1, leverZ2-2);
						box(leverX*rl, (leverY1+1+0.5)*tb,
							leverWidth+1, 1, 0, leverZ2-2);
					}

					// lever front
					slopeY(barX*rl, front1Y*tb,
						barWidth, front1Height,
						leverZ1, leverZ1+0.8, // avoid thru-hole wires
						barZ2, barZ2,
						tb);
					box(barX*rl, front2Y*tb,
						barWidth, front2Height,
						barZ2, leverZ2);

					// bar (increase height to connect to lever)
					box(barX*rl, (barY-0.5)*tb,
						barWidth, barHeight+1,
						barZ1, barZ2);

					// spring upper block
					box(barHeight/2*rl, barY*tb,
						barHeight, barHeight,
						(pcbZ2+barZ1+barTravel)/2, barZ2);
				}
			}
		}
		
		// spring lower block
		box(barHeight/2*rl, barY*tb,
			barHeight, barHeight,
			pcbZ2, (pcbZ2+barZ1-barTravel)/2);
		
		// spring
		slopeX(barX*rl, barY*tb,
			barWidth, barHeight,
			pcbZ2, pcbZ2+2,
			pcbZ2+1, pcbZ2+3,
			rl);
		slopeX(barX*rl, barY*tb,
			barWidth, barHeight,
			pcbZ2+4.5, pcbZ2+2.5,
			pcbZ2+5.5, pcbZ2+3.5,
			rl);
	}	
}

module mount() {

	// top/bottom
	for (tb = [1, -1]) {
		// right/left
		for (rl = [1, -1]) {
			slopeY(mount1X*rl, mount1Y*tb,
				mount1Width, mount1Height,
				pcbZ2, pcbZ2,
				bodyZ2, mountZ3, tb);
			
			box(mount2X*rl, mount2Y*tb,
				mount2Width, mount2Height,
				pcbZ2, mountZ2);			
		}
	}
	
	for (angle = [45, 135, 225, 315]) {
		rotate([0, 0, angle]) {
			box(mountDiag/2-0.5, 0,
				1, 10,
				pcbZ2, mountZ2);
			slopeX(mountDiag/2, 0,
				1.2, 8,
				mountZ1, mountZ1,
				mountZ2, mountZ1+0.1);
		}
	}
}


// Omron D2F-01
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
			
			// wires
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
					box(actuatorX, actuatorY,
						actuatorWidth, 1,
						actuatorZ, actuatorZ+1);
					box(-actuatorX, actuatorY,
						actuatorWidth, 1,
						actuatorZ, actuatorZ+1);
					box(actuatorX, -actuatorY,
						actuatorWidth, 1,
						actuatorZ, actuatorZ+1);
					box(-actuatorX, -actuatorY,
						actuatorWidth, 1,
						actuatorZ, actuatorZ+1);
					
					box(0, barY,
						barWidth*2, barHeight,
						barZ2, barZ2+1);
					box(0, -barY,
						barWidth*2, barHeight,
						barZ2, barZ2+1);
			
				}
			}
		}	
	}
}

difference() {
	union() {
		body();
		lever(1, 0);
		lever(-1, 0);
		mount();
	}
		
	// holes for pcb drills
	drills();
}


//switch(switchX, -switchY, -90);
//switch(switchX, switchY, -90);
//switch(-switchX, -switchY, 90);
//switch(-switchX, switchY, 90);

//actuators(0);
//actuators(-leverAngle);

// screws
//barrel(20.5, 8.5, 1.5, pcbZ1, pcbZ1+15);
//barrel(-20.5, 8.5, 1, pcbZ1, pcbZ1+10);
