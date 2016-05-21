import processing.core.PApplet;
import processing.core.PConstants;
import processing.core.PGraphics;
import processing.core.PVector;

public class VisualizerTestPattern extends VisualizerDot {

	public VisualizerTestPattern(PApplet parent) {
		super(parent);
	}

	@Override
	public void start() {
		super.start();
		// Other initialization when this app becomes active
	}
	
	@Override
	public void stop() {
		super.stop();
		// When this app is deactivated
	}

	@Override
	public void update(PApplet parent, People p) {
		// Update internal state
	}

	@Override
	public void draw(Tracker t, PGraphics g, People p) {
		super.draw(t, g, p);

		// Draw grid
		float minxpos=(float) (Math.ceil(Tracker.rawminx*10)/10f);
		float maxxpos=(float) (Math.floor(Tracker.rawmaxx*10)/10f);
		float minypos=(float) (Math.ceil(Tracker.rawminy*10)/10f);
		float maxypos=(float) (Math.floor(Tracker.rawmaxy*10)/10f);
		float x=minxpos;
		while (x<=maxxpos) {
			if (Math.abs(x)<0.01f)
				g.stroke(0,0,255);
			else if (Math.abs(x-Math.round(x)) < 0.01f)
				g.stroke(255);
			else
				g.stroke(20);
			g.line(x,minypos,x,maxypos);
			x+=0.1f;
		}
		float y=minxpos;
		while (y<=maxypos) {
			if (Math.abs(y-Math.round(y)) < 0.01f)
				g.stroke(255);
			else
				g.stroke(20);
			g.line(minxpos,y,maxxpos,y);
			y+=0.1f;
		}
		// Draw radial circles
		g.stroke(255,0,0);
		g.fill(0,0);
		g.ellipseMode(PConstants.CENTER);
		float maxr=Math.max(Math.max(-Tracker.rawminx,Tracker.rawmaxx),Tracker.rawmaxy);
		for (float r=0;r<=maxr;r++) {
			g.ellipse(0f, 0f, 2*r, 2*r);
		}
		// Draw axes
		g.strokeWeight(0.1f);
		g.stroke(255,0,0);
		g.line(0, 0.05f, 2, 0.05f);
		g.stroke(0,0,255);
		g.line(0,0, 0, 2);
		
	}
	
	@Override
	public void drawLaser(PApplet parent, People p) {
//		Laser laser=Laser.getInstance();
//		laser.bgBegin();   // Start a background drawing
//		for (Position ps: p.positions.values()) {  
//			laser.cellBegin(ps.id); // Start a cell-specific drawing
//			Laser drawing code
//			laser.cellEnd(ps.id);
//		}
//		laser.bgEnd();
	}
}