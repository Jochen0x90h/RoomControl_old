//$fn = 128;

// variables

// pcb
pcbWidth = 75;
pcbHeight = 75;
pcbX = 0; // center of pcb
pcbY = 0;
pcbZ1 = 3;
pcbZ2 = pcbZ1+1.6; // mounting surface
pcbX1 = pcbX-pcbWidth/2;
pcbX2 = pcbX+pcbWidth/2;
pcbY1 = pcbY-pcbHeight/2;
pcbY2 = pcbY+pcbHeight/2;

// poti (Bourns PEC12R-4215F-S0024)
potiX = 22;
potiY = -22;
potiH1 = 7; // height of body above pcb (variant with switch)
potiH2 = 11.2; // height of shaft bearing above pcb
potiL = 17.5; // height of shaft end above pcb
potiF = 5; // shaft cutaway
potiZ1 = pcbZ2 + potiH1;
potiZ2 = pcbZ2 + potiH2;
potiZ4 = pcbZ2 + potiL;
potiZ3 = potiZ4 - potiF;
potiR1 = 3.5; // shaft bearing radius
potiR2 = 3; // shaft radius

// base
baseZ1 = 0;
baseZ2 = pcbZ1;

// cover
coverZ1 = potiZ4 - 1;
coverZ2 = potiZ4 + 1; // use height of poti mounted on pcb plus cap
coverFit = 0.4;
coverOverlap = 2.5;

// wheel
wheelR1 = 5; // radios of shaft
wheelR2 = 19.5; // radius of wheel
wheelGap = 0.5; // visible air gab between box and wheel
wheelZ1 = potiZ3;
wheelZ2 = coverZ2 - 3; // thickness of wheel
wheelZ3 = coverZ2;

// micro usb
usbX = 22;
usbWidth = 8;
usbZ1 = pcbZ2;
usbZ2 = usbZ1 + 3; // thickness of usb connector
usbPlugWidth = 12;
usbPlugThickness = 8; // thickness of plug of usb cable

// display (2.42 inch)
displayTolerance = 0.4;
displayCableWidth1 = 13;
displayCableWidth2 = 16; // where the flat cable is connected to display
displayCableLength = 35;

// display screen (active area of display)
screenX = 0;
screenY = 20.5;
screenWidth = 57.01;
screenHeight = 29.49;
screenOffset = 1.08 + displayTolerance; // distance between upper panel border and upper screen border (tolerance at upper border)
screenX1 = screenX - screenWidth/2;
screenX2 = screenX + screenWidth/2;
screenY1 = screenY - screenHeight/2;
screenY2 = screenY + screenHeight/2;

// display panel
panelWidth = 60.5 + displayTolerance;
panelHeight = 37 + displayTolerance;
panelThickness1 = 1.2; // at side where cable is connected
panelThickness2 = 2.3;
panelX = screenX;
panelX1 = panelX - panelWidth / 2; // left border of panel
panelX2 = panelX + panelWidth / 2; // right border of panel
panelY2 = screenY + screenHeight/2+screenOffset; // upper border of panel
panelY1 = panelY2-panelHeight; // lower border of panel
panelY = (panelY2+panelY1)/2;
panelZ1 = coverZ2 - 1 - panelThickness2;
panelZ2 = coverZ2 - 1 - panelThickness1;
panelZ3 = coverZ2 - 1;

// power 
powerX1 = -37.5 - 0.5;
powerY1 = -11.5 - 0.5;
powerX2 = -17.5 + 0.5;
powerY2 = 22.5 + 0.5;
powerZ1 = pcbZ2 + 15 + 0.2;
assert(powerZ1 <= panelZ1, "power supply intersects display");

// clamps
clampsX1 = -17.2 - 0.5;
clampsY1 = -14.1 - 0.5;
clampsX2 = 35.1 + 0.5;
clampsY2 = 22.7 + 0.5;
clampsZ1 = pcbZ2 + 8 + 1;
clampsZ2 = pcbZ2 + 13 + 1;

// cable hole
cableX1 = -17.0;
cableY1 = -1.6;
cableX2 = 34;
cableY2 = 10.2;
cableY = (cableY1 + cableY2) / 2;
fixationHeight = cableY2 - cableY1 - 2 * 2;


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

module wheel(x) {
	y = potiY;

	// wheel
	difference() {
		// wheel
		barrel(x=x, y=y, r=wheelR2, z1=wheelZ2, z2=wheelZ3);

		// cutout for poti shaft (with tolerance)
		box(x=x, y=y, w=10, h=4.5, z1=1, z2=potiZ4+0.1);
		box(x=x, y=y, w=4.5, h=10, z1=1, z2=potiZ4+0.1);
	}
	
	// shaft holder for poti shaft
	difference() {
		intersection() {
			// box that has 1mm wall on sides
			box(x=x, y=y, w=8, h=8, z1=wheelZ1, z2=wheelZ2+1);

			// make round corners
			barrel(x=x, y=y, r=wheelR1, z1=wheelZ1, z2=wheelZ2+1);
		}

		// subtract rounded hole
		intersection() {
			box(x=x, y=y, w=potiR2*2, h=potiR2*2, z1=wheelZ1-1, z2=wheelZ3-1);
			barrel(x=x, y=y, r=3.8, z1=wheelZ1-1, z2=wheelZ3-1);
		}
	}
	
	// poti shaft cutaway
	difference() {
		// fill shaft cutaway which is 1.5mm
		union() {
			box(x=x, y=y+2.5, w=1, h=2, z1=potiZ3, z2=wheelZ3-2);
			box(x=x, y=y+2.0, w=2, h=1, z1=potiZ3, z2=wheelZ3-2);
		}

		// subtract slanted corners to ease insertion of poti shaft
		translate([x, y+1.6, potiZ3-0.5])
			rotate([30, 0, 0])
				box(x=0, y=0, w=4, h=1, z1=0, z2=2);
	}
}

// for adding wheel base
module wheelBase(x, y) {
	// base in z-direction: 1mm air, 2mm wall
	barrel(x=x, y=y, r=wheelR2+wheelGap+2, z1=wheelZ2-1-2, z2=wheelZ3);
	
	// shaft radial: 5mm wheel shaft, air gap, 1mm wall
	// zhaft z-direction: 2mm wall, 1mm air, wheel
	barrel(x=x, y=y, r=wheelR1+wheelGap+1, z1=wheelZ1-2-1, z2=wheelZ3);

	// shaft radial: poti shaft, no air gap, 1.5mm wall
	barrel(x=x, y=y, r=potiR1+1.5, z1=potiZ1, z2=wheelZ3);
}

// for subtracting from wheel base
module potiCutout(x, y) {
	// cutout for wheel: 3mm wheel, 1mm air
	barrel(x=x, y=y, r=wheelR2+wheelGap, z1=wheelZ2-1, z2=coverZ2+1);

	// hole for poti shaft bearing
	barrel(x=x, y=y, r=potiR1+0.1, z1=pcbZ2, z2=wheelZ2);

	// hole for poti shaft
	barrel(x=x, y=y, r=potiR2+0.1, z1=pcbZ2, z2=wheelZ2);

	// hole for wheel shaft
	barrel(x=x, y=y, r=wheelR1+wheelGap, z1=wheelZ1-1, z2=wheelZ2);
}

module detectorCutout(x, y) {
	barrel(x=x, y=y, r=6, z1=pcbZ2, z2=coverZ2+1);
}

module speakerCutout() {

	barrel(x=0, y=-25, r=14, z1=wheelZ2-3, z2=/*wheelZ2-1*/coverZ2+1);

}

// snap lock between cover and base
module snap(y, l) {
	translate([0, y, coverZ1+1])
		rotate([45, 0, 0])
			cuboid(x1=-l/2, x2=l/2, y1=-0.65, y2=0.65, z1=-0.65, z2=0.65);
}

module usb() {
	color([0.5, 0.5, 0.5])
	box(x=usbX, y=pcbY1+1, w=usbWidth, h=10, z1=usbZ1, z2=usbZ2);		
}

module usbPlug() {
	usbPlugZ1 = (usbZ1+usbZ2)/2-usbPlugThickness/2;
	color([0, 0, 0])
	box(x=usbX, y=pcbY1-10, w=usbPlugWidth, h=14,
		z1=usbPlugZ1, z2=usbPlugZ1+usbPlugThickness);		
}

module base() {
color([0.3, 0.3, 1]) {
	difference() {
		union() {
			difference() {
				// base with slanted walls
				union() {
					box(x=0, y=0, w=76-coverFit, h=76-coverFit,
						z1=0, z2=baseZ2-0.5);
					frustum(x=0, y=0,
						w1=76-coverFit, h1=76-coverFit,
						w2=76-coverFit-0.2, h2=76-coverFit-0.2,
						z1=baseZ2-0.5, z2=baseZ2);
				}
				
				// subtract inner volume
				box(x=0, y=0, w=72-coverFit, h=72-coverFit, z1=2, z2=baseZ2+1);
			
				// subtract snap lock
				snap(-76/2, 44);
				snap(76/2, 44);			
			}
		}
		
		// generated from .drl files using drill2cad.py
		barrel(30.8, -14.68, 0.9, pcbZ2-4.2, pcbZ2);
		barrel(30.8, -22.3, 0.9, pcbZ2-4.2, pcbZ2);
		barrel(33.34, -22.3, 0.9, pcbZ2-4.2, pcbZ2);
		barrel(35.88, -14.68, 0.9, pcbZ2-4.2, pcbZ2);
		barrel(35.88, -22.3, 0.9, pcbZ2-4.2, pcbZ2);
		barrel(-23.796, -20.204, 0.9, pcbZ2-4.2, pcbZ2);
		barrel(-23.796, -23.796, 0.9, pcbZ2-4.2, pcbZ2);
		barrel(-20.204, -20.204, 0.9, pcbZ2-4.2, pcbZ2);
		barrel(-34.1, -28.6, 1, pcbZ2-4.2, pcbZ2);
		barrel(-34.1, -31.14, 1, pcbZ2-4.2, pcbZ2);
		barrel(31.2, 36.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(31.2, 31.42, 1, pcbZ2-4.2, pcbZ2);
		barrel(31.2, 28.88, 1, pcbZ2-4.2, pcbZ2);
		barrel(31.2, 26.34, 1, pcbZ2-4.2, pcbZ2);
		barrel(36.28, 36.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(36.28, 31.42, 1, pcbZ2-4.2, pcbZ2);
		barrel(36.28, 28.88, 1, pcbZ2-4.2, pcbZ2);
		barrel(36.28, 26.34, 1, pcbZ2-4.2, pcbZ2);
		barrel(8.4, 36.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(8.4, 31.42, 1, pcbZ2-4.2, pcbZ2);
		barrel(8.4, 28.88, 1, pcbZ2-4.2, pcbZ2);
		barrel(8.4, 26.34, 1, pcbZ2-4.2, pcbZ2);
		barrel(13.48, 36.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(13.48, 31.42, 1, pcbZ2-4.2, pcbZ2);
		barrel(13.48, 28.88, 1, pcbZ2-4.2, pcbZ2);
		barrel(13.48, 26.34, 1, pcbZ2-4.2, pcbZ2);
		barrel(-13.7, 17.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(-13.7, 12.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(-8.7, 17.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(-8.7, 12.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(-3.7, 17.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(-3.7, 12.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(1.3, 17.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(1.3, 12.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(6.3, 17.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(6.3, 12.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(11.3, 17.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(11.3, 12.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(16.3, 17.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(16.3, 12.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(21.3, 17.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(21.3, 12.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(26.3, 17.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(26.3, 12.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(31.3, 17.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(31.3, 12.7, 1, pcbZ2-4.2, pcbZ2);
		barrel(-14.4, 36.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(-14.4, 31.42, 1, pcbZ2-4.2, pcbZ2);
		barrel(-14.4, 28.88, 1, pcbZ2-4.2, pcbZ2);
		barrel(-14.4, 26.34, 1, pcbZ2-4.2, pcbZ2);
		barrel(-9.32, 36.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(-9.32, 31.42, 1, pcbZ2-4.2, pcbZ2);
		barrel(-9.32, 28.88, 1, pcbZ2-4.2, pcbZ2);
		barrel(-9.32, 26.34, 1, pcbZ2-4.2, pcbZ2);
		barrel(16, 36.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(16, 31.42, 1, pcbZ2-4.2, pcbZ2);
		barrel(16, 28.88, 1, pcbZ2-4.2, pcbZ2);
		barrel(16, 26.34, 1, pcbZ2-4.2, pcbZ2);
		barrel(21.08, 36.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(21.08, 31.42, 1, pcbZ2-4.2, pcbZ2);
		barrel(21.08, 28.88, 1, pcbZ2-4.2, pcbZ2);
		barrel(21.08, 26.34, 1, pcbZ2-4.2, pcbZ2);
		barrel(8.26, -23.4, 1, pcbZ2-4.2, pcbZ2);
		barrel(10.8, -23.4, 1, pcbZ2-4.2, pcbZ2);
		barrel(-13.4, -4.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(-13.4, -9.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(-8.4, -4.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(-8.4, -9.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(-3.4, -4.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(-3.4, -9.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(1.6, -4.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(1.6, -9.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(6.6, -4.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(6.6, -9.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(11.6, -4.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(11.6, -9.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(16.6, -4.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(16.6, -9.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(21.6, -4.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(21.6, -9.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(26.6, -4.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(26.6, -9.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(31.6, -4.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(31.6, -9.1, 1, pcbZ2-4.2, pcbZ2);
		barrel(-6.8, 36.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(-6.8, 31.42, 1, pcbZ2-4.2, pcbZ2);
		barrel(-6.8, 28.88, 1, pcbZ2-4.2, pcbZ2);
		barrel(-6.8, 26.34, 1, pcbZ2-4.2, pcbZ2);
		barrel(-1.72, 36.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(-1.72, 31.42, 1, pcbZ2-4.2, pcbZ2);
		barrel(-1.72, 28.88, 1, pcbZ2-4.2, pcbZ2);
		barrel(-1.72, 26.34, 1, pcbZ2-4.2, pcbZ2);
		barrel(23.6, 36.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(23.6, 31.42, 1, pcbZ2-4.2, pcbZ2);
		barrel(23.6, 28.88, 1, pcbZ2-4.2, pcbZ2);
		barrel(23.6, 26.34, 1, pcbZ2-4.2, pcbZ2);
		barrel(28.68, 36.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(28.68, 31.42, 1, pcbZ2-4.2, pcbZ2);
		barrel(28.68, 28.88, 1, pcbZ2-4.2, pcbZ2);
		barrel(28.68, 26.34, 1, pcbZ2-4.2, pcbZ2);
		barrel(0.8, 36.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(0.8, 31.42, 1, pcbZ2-4.2, pcbZ2);
		barrel(0.8, 28.88, 1, pcbZ2-4.2, pcbZ2);
		barrel(0.8, 26.34, 1, pcbZ2-4.2, pcbZ2);
		barrel(5.88, 36.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(5.88, 31.42, 1, pcbZ2-4.2, pcbZ2);
		barrel(5.88, 28.88, 1, pcbZ2-4.2, pcbZ2);
		barrel(5.88, 26.34, 1, pcbZ2-4.2, pcbZ2);
		barrel(-22, 36.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(-22, 31.42, 1, pcbZ2-4.2, pcbZ2);
		barrel(-22, 28.88, 1, pcbZ2-4.2, pcbZ2);
		barrel(-22, 26.34, 1, pcbZ2-4.2, pcbZ2);
		barrel(-16.92, 36.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(-16.92, 31.42, 1, pcbZ2-4.2, pcbZ2);
		barrel(-16.92, 28.88, 1, pcbZ2-4.2, pcbZ2);
		barrel(-16.92, 26.34, 1, pcbZ2-4.2, pcbZ2);
		barrel(-35.2, 20.2, 1, pcbZ2-4.2, pcbZ2);
		barrel(-30, -9.2, 1, pcbZ2-4.2, pcbZ2);
		barrel(-25, -9.2, 1, pcbZ2-4.2, pcbZ2);
		barrel(-19.8, 20.2, 1, pcbZ2-4.2, pcbZ2);
		barrel(19.5, -15, 1, pcbZ2-4.2, pcbZ2);
		barrel(19.5, -29.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(22, -29.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(24.5, -15, 1, pcbZ2-4.2, pcbZ2);
		barrel(24.5, -29.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(-34.2, -35.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(-31.66, -35.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(-29.12, -35.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(-26.58, -35.5, 1, pcbZ2-4.2, pcbZ2);
		barrel(-33.1, -22.75, 1, pcbZ2-4.2, pcbZ2);
		barrel(-30.9, -15.25, 1, pcbZ2-4.2, pcbZ2);
		barrel(19.1, -36.5, 1.05, pcbZ2-4.2, pcbZ2);
		barrel(24.9, -36.5, 1.05, pcbZ2-4.2, pcbZ2);
		barrel(15.9, -22, 1.6, pcbZ2-4.2, pcbZ2);
		barrel(28.1, -22, 1.6, pcbZ2-4.2, pcbZ2);
		barrel(33.34, -14.68, 0.9, pcbZ2-4.2, pcbZ2);
		
		// subtract mounting screw holes
		for (i = [0 : 3]) {
			cone(cos(i) * 60 - 30, sin(i) * 60, 1.4, 3, -0.1, 2.1);
			cone(30 - cos(i) * 60, sin(i) * 60, 1.4, 3, -0.1, 2.1);
		}
		//for (y = [0 : 3]) {
		//	cone(-30, y, 1.4, 3, -0.1, 2.1);
		//	cone(30, y, 1.4, 3, -0.1, 2.1);
		//}

		// subtract cable hole
		cuboid(cableX1+1, cableY1-1, 25.5, cableY2+1, -1, 3);
		box(cableX1, cableY+fixationHeight/2+0.5, 10, 1, -1, 3);
		box(cableX1, cableY-fixationHeight/2-0.5, 10, 1, -1, 3);
	}

	// poti support
	barrel(potiX, potiY, 2, 0, pcbZ1);

	// left pcb fixation
	box(cableX1+0.6, cableY, 1.2, fixationHeight, 0, pcbZ2+2);
	box(cableX1-2, cableY, 2, fixationHeight, 0, pcbZ1);
	frustum(cableX1, cableY, 1, fixationHeight, 0.1, fixationHeight,
		pcbZ2+0.2, pcbZ2+2);

	// right pcb fixation
	box(cableX2-0.6, cableY, 1.2, fixationHeight, 0, pcbZ2+2);
	box(cableX2+2, cableY, 2, fixationHeight, 0, pcbZ1);
	frustum(cableX2, cableY, 1, fixationHeight, 0.1, fixationHeight,
		pcbZ2+0.2, pcbZ2+2);
} // color
}

module cover() {
color([1, 0, 0]) {
	difference() {
		union() {
			// cover with 2mm wall thickness
			difference() {
				box(x=0, y=0, w=80, h=80, z1=coverZ1, z2=coverZ2);
				box(x=0, y=0, w=76, h=76, z1=coverZ1-1, z2=coverZ2-2);
			}
			
			intersection() {
				// poti base
				union() {
					wheelBase(x=potiX, y=potiY);
					
					// distance holer for display cable
					box(x=0, y=-11, w=displayCableWidth2, h=2.5,
						z1=coverZ2-6, z2=coverZ2);
				}
			
				// cut away poti base outside of cover and at display
				cuboid(x1=-40, y1=-40, x2=40, y2=panelY1, z1=pcbZ2, z2=coverZ2-2);
			}
			
			// lower display holder
			box(x=panelX, y=panelY1-0.5, w=panelWidth+2, h=3,
				z1=panelZ1, z2=coverZ2);
			box(x=panelX, y=panelY1, w=panelWidth+2, h=2,
				z1=panelZ2-2, z2=panelZ2);
		
			// upper display holder
			box(x=panelX, y=panelY2+0.75, w=80, h=1.5,
				z1=panelZ1-5, z2=panelZ1);
			box(x=panelX, y=panelY2, w=10, h=1,
				z1=panelZ1-1, z2=coverZ2 - 2.2);
			frustum(panelX, panelY2, 10, 0.1, 10, 3,
				z1=panelZ1-4, z2=panelZ1);
			
			
			
			// left display holder
			//box(x=panelX1, y=panelY, w=2, h=panelHeight+2,
			//	z1=panelZ1-1, z2=coverZ2);

			// right display holder with snap
			//box(x=panelX2+0.5, y=panelY, w=1, h=panelHeight+4, 
			//	z1=baseZ2, z2=panelZ1); 
			//box(x=panelX2+1, y=panelY1-2, w=2, h=2, 
			//	z1=baseZ2, z2=coverZ2); 
			//translate([panelX2, panelY, panelZ1-4])
			//	rotate([0, -7, 0])
			//		box(x=0, y=0, w=1, h=6, z1=-4, z2=4); 
		}
		
		// subtract poti cutout for wheel and axis
		potiCutout(x=potiX, y=potiY);		

		detectorCutout(x=-potiX, y=potiY);

		speakerCutout();

		// subtract display screen window
		frustum(x=screenX, y=screenY,
			w1=screenWidth-4, h1=screenHeight-4,
			w2=screenWidth+4, h2=screenHeight+4,
			z1=coverZ2-3, z2=coverZ2+1);

		// subtract display panel (glass carrier)
		box(x=panelX, y=panelY, w=panelWidth, h=panelHeight,
			z1=panelZ2, z2=panelZ3);

		// subtract display cable cutout
		box(x=panelX, y=panelY1, w=displayCableWidth2, h=6,
			z1=panelZ3-10, z2=panelZ3+0.2);
		cuboid(x1=panelX1, y1=panelY1, x2=panelX2, y2=panelY1+5,
			z1=panelZ2, z2=panelZ3+0.2);
		frustum(x=screenX, y=panelY1, 
			w1=displayCableWidth1, h1=45,
			w2=displayCableWidth1, h2=6,
			z1=panelZ3-10, z2=panelZ3-0.5);
	
		// subtract usb plug
		//usbPlug();
		
		// subtract power and clamps
		power();
		clamps();
	}

	// snap lock between cover and base
	snap(-(76)/2, 40);
	snap((76)/2, 40);
} // color
}

module pcb() {
	color([0, 0.6, 0, 0.3])
	box(x=pcbX, y=pcbY, w=pcbWidth, h=pcbHeight,
		z1=pcbZ1, z2=pcbZ2);
}

module poti(x) {
	y = potiY;
	color([0.5, 0.5, 0.5]) {
		//box(x=x, y=y, w=13.4, h=12.4, z1=pcbZ2, z2=potiZ1);
		//box(x=x, y=y, w=15, h=6, z1=pcbZ2, z2=potiZ1);
		barrel(x=x, y=y, r=potiR1, z1=pcbZ2, z2=potiZ2);
		difference() {
			barrel(x=x, y=y, r=potiR2, z1=pcbZ2, z2=potiZ4);
			box(x=x, y=y+potiR2, w=10, h=3, z1=potiZ3, z2=potiZ4+0.5);
		}
	}
}

module relays() {
	color([1, 1, 1]) {
		cuboid(-23.2, 22.8, 37.5, 37.5, pcbZ2, pcbZ2+9.2);
	}
}

module power() {
	color([0, 0, 0]) {
		cuboid(powerX1, powerY1, powerX2, powerY2, pcbZ2, powerZ1);
	}	
}

module clamps() {
	color([0.8, 0.8, 0.8]) {
		x = (clampsX1 + clampsX2) / 2;
		y = (clampsY1 + clampsY2) / 2;
		w = clampsX2 - clampsX1;
		h = clampsY2 - clampsY1;
		box(x, y, w, h, pcbZ2, pcbZ2+8);
		frustum(x, y, w, h, w, h-10, clampsZ1, clampsZ2);
	}		
}

// casing parts that need to be printed
base();
//cover();
//wheel(potiX);


// reference parts
//pcb();
//poti(potiX);
//usb(); usbPlug();
//relays();
//power();
//clamps();
