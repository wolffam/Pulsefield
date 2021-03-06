package com.pulsefield.tracker;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import processing.core.PApplet;
import processing.core.PConstants;
import processing.core.PGraphics;
import processing.core.PImage;
import processing.core.PVector;

class Marble {
	private static final float DENSITY=1f;   // mass=DENSITY*r^2
	private static final float DAMPING=0.2f; //0.2f;  // accel=-DAMPING * v
	private static final float MASSEXCHANGERATE=0.05f;  // mass loss for in-contact marbles in kg/sec
	protected static final float MAXMASS=0.5f;
	private static final float G=0.02f;   // Gravitational constant
	PVector location;
	PVector velocity;
	float mass;
	PImage img;
	boolean isAlive;
	private static Images imgs=null;
	private static HashSet<Marble> allMarbles = new HashSet<Marble>();
	
	Marble(float mass, PVector pos, PVector vel, PImage img) {
		if (imgs==null)
			imgs=new Images("osmos/bubbles");
		velocity=new PVector();
		velocity.x=vel.x; velocity.y=vel.y;
		location=new PVector();
		location.x=pos.x; location.y=pos.y;
		this.mass=mass;
		assert(img!=null);
		this.img=img;
		allMarbles.add(this);
		//logger.fine("Created marble at "+location+" with velocity "+vel+" and mass "+mass);
		isAlive=true;
	}
	public void destroy() {
		//logger.fine("Destroy marble at "+location);
		allMarbles.remove(this);
		isAlive=false;
	}
	public float getRadius() {
		return (float) Math.pow(mass/DENSITY,1/2f);
	}
	public PVector getMomentum() {
		return PVector.mult(velocity, mass);
	}
	public void update() {
		// Damping
		velocity.add(PVector.mult(velocity, -DAMPING/Tracker.theTracker.frameRate));
		location.add(PVector.mult(velocity,1/Tracker.theTracker.frameRate));
		float radius=getRadius();
		// Out of bounds, bounce back
		if (location.x+radius > Tracker.maxx && velocity.x>0)
			velocity.x=-velocity.x;
		if (location.x-radius < Tracker.minx && velocity.x<0)
			velocity.x=-velocity.x;
		if (location.y+radius > Tracker.maxy && velocity.y>0)
			velocity.y=-velocity.y;
		if (location.y-radius < Tracker.miny && velocity.y<0)
			velocity.y=-velocity.y;
	}
	public void draw(PGraphics g) {
		float r=getRadius();
		//g.ellipse(location.x, location.y, r*2, r*2);
		g.imageMode(PConstants.CENTER);
		g.image(img,location.x,location.y,r*2,r*2);
	}
	static Marble create(float mass, PVector pos, PVector vel)  {
		return new Marble(mass,pos,vel,imgs.getRandom());
	}
	static void destroyAll() {
		Object all[]=allMarbles.toArray();
		for (Object o: all) {
			Marble b=(Marble)o;
			b.destroy();
		}
	}
	
	static void updateAll(Effects effects) {
		Object all[]=allMarbles.toArray();
		for (Object o: all) {
			Marble b=(Marble)o;
			b.update();
//			if (! (b  instanceof PlayerMarble)) {
//				// Player marbles destroyed by visualizer when person is lost
//				if (!Tracker.theTracker.inBounds(b.location))
//					// Out of bounds
//					b.destroy();
//			}
		}
		// Find all contacts
		for (Object o1: all) {
			Marble b1=(Marble)o1;
			if (!b1.isAlive)
				continue;
			for (Object o2: all) {
				Marble b2=(Marble)o2;
				if (!b2.isAlive)
					continue;
				if (b1==b2)
					continue;
				PVector sep=PVector.sub(b2.location, b1.location);
				float minsep=b1.getRadius()+b2.getRadius();
				if (sep.mag() <= minsep && b1.mass<b2.mass && b1.isAlive) {
					// Contact
					//logger.fine("Contact at "+b1.location+" - "+b2.location);
					if (b1 instanceof PlayerMarble && b2 instanceof PlayerMarble && b2.mass>MAXMASS*0.95f & b1.mass<PlayerMarble.MINMASS*1.1) {
						// Small - big interaction -- reverse it!
						b1.mass=(b1.mass+b2.mass)/2;
						b2.mass=b1.mass;
						effects.play("SPLIT",127,1000);
					} else {
						float xfr=Math.min(b1.mass, MASSEXCHANGERATE/Tracker.theTracker.frameRate);
						if (b2.mass<MAXMASS && !(b1 instanceof PlayerMarble && b1.mass-xfr<PlayerMarble.MINMASS)) {
							b1.mass-=xfr;
							b2.mass+=xfr;
							b2.velocity=PVector.add(PVector.mult(b2.velocity,(b2.mass-xfr)/b2.mass), PVector.mult(b1.velocity, xfr/b2.mass));
							//logger.fine("Xfr "+xfr+", new masses: "+b1.mass+", "+b2.mass);
							if (b1.mass==0)
								b1.destroy();
							else if (b1 instanceof PlayerMarble && b2 instanceof PlayerMarble) {
								effects.play("COLLIDE",127,1000);
							}
						}
					}
					minsep=b1.getRadius()+b2.getRadius(); // Update sep
					float overlap=0.02f;   // Keep them overlapped up to this much
					if (sep.mag() < minsep-overlap) {
						// Keep them in exact contact (actually a little closer, so the stay in contact)
						float movedist=minsep-overlap-sep.mag();
						assert(movedist>0);
						sep.normalize();
						PVector m1=PVector.mult(sep,-movedist*b2.mass/(b1.mass+b2.mass));
						PVector m2=PVector.mult(sep,movedist*b1.mass/(b1.mass+b2.mass));
						//logger.fine("Separate: sep="+sep+", minsep="+minsep+", movedist="+movedist,", m1="+m1+", m2="+m2);
						b2.location.add(m2);
						b1.location.add(m1);
					}
					// Fuse their momentum
//					PVector momentum=PVector.add(b1.getMomentum(), b2.getMomentum());
//					b1.velocity=PVector.div(momentum, b1.mass+b2.mass);
//					b2.velocity=b1.velocity;
				}
				float attraction = G*b1.mass*b2.mass/sep.magSq();
				PVector acc=PVector.mult(sep,attraction/b1.mass);
				PVector deltaVelocity=PVector.mult(acc, 1/Tracker.theTracker.frameRate);
				b1.velocity.add(deltaVelocity);
			}
		}
	}
	static void drawAll(PGraphics g) {
		for (Marble b: allMarbles) {
			b.draw(g);
		}
	}
}

class PlayerMarble extends Marble {
	private static final float SPRINGCONSTANT=0.1f;   // force=SPRINGCONSTANT*dx  (Newtons/m)
	private static final float EJECTSPEED=1f;	// Speed of ejected particles (m/s)
	private static final float INITIALMASS=0.15f;  // In kg
	public static final float MINMASS=0.02f;    // In kg
	private static final float EJECTFUDGE=400;   // Make ejections give this much more momentum than they should
	PVector pilot;
	Person person;
	
	PlayerMarble(Person person, PVector pos, PVector vel, PImage img) {
		super(INITIALMASS,pos,vel,img);
		pilot=new PVector();
		pilot.x=pos.x;
		pilot.y=pos.y;
		this.person=person;
	}
	
	public void updatePosition(PVector newpilot) {
		pilot.x=newpilot.x; 
		pilot.y=newpilot.y;
	}
	
	@Override
	public void update() {
		PVector delta=PVector.sub(pilot,location);
		if (delta.mag() > getRadius()) {
			PVector force=PVector.mult(delta,SPRINGCONSTANT*mass);  // Not really a spring, same accel indep of mass
			PVector acc=PVector.mult(force,1/mass);
			PVector deltaVelocity=PVector.mult(acc, 1/Tracker.theTracker.frameRate);
			velocity.add(deltaVelocity);
			PVector ejectVelocity=PVector.mult(deltaVelocity.normalize(), -EJECTSPEED);
			float ejectMass=deltaVelocity.mag()*mass/(EJECTSPEED+deltaVelocity.mag())/EJECTFUDGE;
			PVector ejectLocation=PVector.add(location, PVector.mult(ejectVelocity,1.05f*getRadius()/ejectVelocity.mag()));
			Marble.create(ejectMass,ejectLocation,PVector.add(ejectVelocity,velocity));
			if (mass>(MINMASS+MAXMASS)/2)
				mass=Math.max(MINMASS,mass-ejectMass);
		}
		super.update();
	}
	
	@Override
	public void draw(PGraphics g) {
		g.line(location.x, location.y, pilot.x, pilot.y);
		super.draw(g);
	}
}

// Osmos visualizer
public class VisualizerOsmos extends Visualizer {
	long startTime;
	Images marbleImages;
	Effects effects;

	HashMap<Integer, PlayerMarble> marbles;

	VisualizerOsmos(PApplet parent, Synth synth) {
		super();
		marbles = new HashMap<Integer, PlayerMarble>();
		marbleImages=new Images("osmos/marbles");
		effects=new Effects(synth,124);
		effects.add("COLLIDE",52,55);
		effects.add("SPLIT",40,42);
	}
	
	public void update(PApplet parent, People allpos) {		
		// Update internal state of the Marbles
		for (int id: allpos.pmap.keySet()) {
			if (!marbles.containsKey(id))
				marbles.put(id,new PlayerMarble(allpos.get(id),allpos.get(id).getOriginInMeters(),allpos.get(id).getVelocityInMeters(),marbleImages.getRandom()));
			PVector currentpos=allpos.get(id).getOriginInMeters();
			marbles.get(id).updatePosition(currentpos);
			//logger.fine("Marble "+id+" moved to "+currentpos.toString());
		}
		// Remove Marbles for which we no longer have a position (exitted)
		for (Iterator<Integer> iter = marbles.keySet().iterator();iter.hasNext();) {
			int id=iter.next().intValue();
			if (!allpos.pmap.containsKey(id)) {
				logger.fine("Removing ID "+id);
				marbles.get(id).destroy();
				iter.remove();
			}
		}
		Marble.updateAll(effects);
	}


	public void start() {
		super.start();
		marbles = new HashMap<Integer, PlayerMarble>();
		Ableton.getInstance().setTrackSet("Osmos");
	}

	public void stop() {
		Marble.destroyAll();
		super.stop();
		logger.fine("Stopping Osmos at "+System.currentTimeMillis());
	}

	@Override
	public void draw(Tracker t, PGraphics g, People p) {
		super.draw(t, g,p);
		Marble.drawAll(g);
	}

}

