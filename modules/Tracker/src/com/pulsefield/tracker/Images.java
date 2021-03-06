package com.pulsefield.tracker;
import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.logging.Logger;

import processing.core.PImage;
import gifAnimation.*;

public class Images {
	List <PImage> imgs;
	Random rgen= new Random();
    private final static Logger logger = Logger.getLogger(Images.class.getName());

	public Images(String folder) {
		// Retrieve a random image from the given folder within data
		File d=new File("data/"+folder);
		if (!d.isDirectory()) 
			d=new File(folder);
		if (!d.isDirectory()) {
			logger.warning(d.getAbsolutePath()+" does not exist or is not a directory.");
			assert(false);
		}
		File allfiles[]=d.listFiles();
		if (allfiles==null || allfiles.length<1) {
			logger.warning("No files found in "+d.getAbsolutePath());
			assert(false);
		}
		imgs=new ArrayList<PImage>();
		for (int i=0;i<allfiles.length;i++) {
			if (allfiles[i].getName().equals(".DS_Store"))
				// Ignore these
				continue;
			logger.info("Loading image from "+allfiles[i].getAbsolutePath()+" ("+allfiles[i].getName()+")");
			PImage img;
			if (allfiles[i].getName().endsWith(".gif")) {
				 Gif gimg=new Gif(Tracker.theTracker,allfiles[i].getAbsolutePath());
				 gimg.play();
				 img=gimg;
			} else
				 img=Tracker.theTracker.loadImage(allfiles[i].getAbsolutePath());
			if (img==null) {
				logger.warning("Failed load of "+allfiles[i].getAbsolutePath());
			} else {
				imgs.add(img);
			}
		}
		assert(imgs.size()>0);
	}
	
	public PImage getRandom() {
		int choose=rgen.nextInt(imgs.size());
		//logger.fine("getRandom -> "+choose+"/"+imgs.size());
		return imgs.get(choose);
	}
	
	public PImage get(int id) {
		return imgs.get(id%imgs.size());
	}
}
