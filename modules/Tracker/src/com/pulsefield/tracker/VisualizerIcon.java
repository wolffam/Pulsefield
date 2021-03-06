package com.pulsefield.tracker;
import processing.core.PApplet;
import processing.core.PGraphics;
import processing.core.PImage;
import processing.core.PShape;

// Visualizer that just displays a dot for each person

public class VisualizerIcon extends Visualizer {
	String icons[];
	PShape iconShapes[];
	Images images=null;
	final boolean useImages=true;
	
	VisualizerIcon(PApplet parent) {
		super();
	}
	
	public void setIcons(PApplet parent, String ic[]) {
		this.icons=ic;
		iconShapes=new PShape[icons.length];
		for (int i=0;i<iconShapes.length;i++) {
			iconShapes[i]=parent.loadShape(Tracker.SVGDIRECTORY+icons[i]);
			assert(iconShapes[i]!=null);
			logger.config("Loaded "+icons[i]+" with "+iconShapes[i].getChildCount()+" children, size "+iconShapes[i].width+"x"+iconShapes[i].height);
		}
	}

	public void setImages(PApplet parent, String imageDir) {
		images=new Images(imageDir);
	}
	
	public void start() {
		super.start();
		Laser.getInstance().setFlag("body",0.0f);
		Laser.getInstance().setFlag("legs",0.0f);
	}
	
	public void stop() {
		super.stop();
	}

	public void update(PApplet parent, People p) {
		;
	}

	public void draw(Tracker t, PGraphics g, People p) {
		super.draw(t, g, p);
		if (p.pmap.isEmpty())
			return;
		g.background(127);
		g.shapeMode(PApplet.CENTER);
		g.imageMode(PApplet.CENTER);
		final float sz=0.5f;  // Size to make the image's largest dimension, in meters

		for (Person ps: p.pmap.values()) {  
			int c=ps.getcolor();
			g.fill(c,255);
			g.stroke(c,255);
			if (useImages) {
				PImage img=images.get(ps.id);
				float scale=Math.min(sz/img.width,sz/img.height);
				Visualizer.drawImage(g,img,ps.getOriginInMeters().x, ps.getOriginInMeters().y,img.width*scale,img.height*scale);
			} else {
				PShape icon=iconShapes[ps.id%iconShapes.length];
				//icon.translate(-icon.width/2, -icon.height/2);
				//			logger.fine("Display shape "+icon+" with native size "+icon.width+","+icon.height);
				float scale=Math.min(sz/icon.width,sz/icon.height);
				Visualizer.drawShape(g, icon,ps.getOriginInMeters().x, ps.getOriginInMeters().y,icon.width*scale,icon.height*scale);
				//icon.resetMatrix();
			}
		}
	}
	
	public void drawLaser(PApplet parent, People p) {
		Laser laser=Laser.getInstance();
		for (Person ps: p.pmap.values()) {  
			String icon=icons[ps.id%icons.length];
			laser.cellBegin(ps.id);
			laser.svgfile(icon,0.0f,0.0f,0.7f,0.0f);
			laser.cellEnd(ps.id);
		}
	}
}

