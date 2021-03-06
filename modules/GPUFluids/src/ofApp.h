#pragma once

#include "ofMain.h"
#include "ofxSyphon.h"
#include "ofxFluid.h"
#include "ofxOsc.h"

class ofApp : public ofBaseApp{
    
public:
    void setup();
    void update();
    void draw();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    void setSize(int _width, int _height, float scale, Boolean hd);
private:
    vector<punctualForce> pendingForces;
    void updateFlame();
    ofxSyphonServer syphon[5];
    void saveTexture(string filename, const ofTexture &tex);
    
    // ofxFluid stuff
    ofxFluid fluid;
    
    ofVec2f oldM;   // Prior mouse position
    int     width,height;
    
    // ofxOsc stuff
    ofxOscReceiver receiver;
    
    // Flame parameters
    Boolean flameEnable;
    ofPoint flamePosition,flameVelocity;
    ofFloatColor flameColor;
    float flameRadius,flameTemperature,flameDensity;
    
    // Border parameters
    Boolean borderEnable;
    ofFloatColor borderColor;
    
    // Freeze update
    Boolean frozen;
};
