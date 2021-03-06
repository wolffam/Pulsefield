#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    // Syphon setup
    
    syphon[0].setName("Main");
    syphon[1].setName("Screen");
    syphon[2].setName("Velocity");
    syphon[3].setName("Temperature");
    syphon[4].setName("Pressure");
    
    // OSC Setup
    int PORT=7713;
    std::cout << "listening for osc messages on port " << PORT << "\n";
    receiver.setup( PORT );
    
    ofSetFrameRate(30); // if vertical sync is off, we can go a bit fast... this caps the framerate
    ofEnableAlphaBlending();
    ofSetCircleResolution(100);
    frozen = false;
    
    setSize(200, 200, 1.0, true);
    
    fluid.dissipation = 1.0f; //0.99;
    fluid.velocityDissipation = 1.0f;//0.99;
    
    //fluid.setGravity(ofVec2f(0.0,0.0));
    fluid.setGravity(ofVec2f(0.0,0.0098));
    
    // Adding constant forces
    //
    flameEnable=true;
    flamePosition=ofPoint(0.0,0.7);
    flameVelocity=ofPoint(0.0,-2.0);
    flameColor= ofFloatColor(1,0.2,0.0,1.0);
    flameRadius=10.0f;
    flameTemperature=10.0f;
    flameDensity=1.0f;
    updateFlame();
    
    borderColor=ofColor(255,0,0);
    borderEnable=true;
    
}

// Set size of fluid with scale times extra internal points per output pixel
void ofApp::setSize(int _width, int _height, float scale, Boolean hd) {
    width=_width;
    height=_height;
    fluid.allocate(width*scale,height*scale,scale,hd);
    ofSetWindowShape(width, height);
}

void ofApp::updateFlame() {
    if (flameEnable) {
        pendingForces.push_back(punctualForce(ofPoint(width*(flamePosition.x+1)/2,height*(flamePosition.y+1)/2), flameVelocity, flameColor, flameRadius, flameTemperature, flameDensity));
    }
}

//--------------------------------------------------------------
void ofApp::update(){
    // Adding temporal Force
    //
    ofPoint m = ofPoint(mouseX,mouseY);
    ofPoint d = (m - oldM)*10.0;
    oldM = m;
    ofPoint c = ofPoint(640*0.5, 480*0.5) - m;
    c.normalize();
    ofFloatColor col(c.x*sin(ofGetElapsedTimef()),c.y*sin(ofGetElapsedTimef()),0.5f,1.0f);
   // cout << "mouse d=" << d << endl;
    
    //  Set obstacle
    //
    fluid.begin();
    if (borderEnable) {
        ofSetColor(255);
        //ofDrawCircle(width*0.5, height*0.35, 40);
        ofDrawLine(1, 1, 1, height);
        ofDrawLine(width,1,width,height);
        ofDrawLine(1, height, width, height);
        ofDrawLine(1, 1, width, 1);
        //ofDrawLine(0,0,width-1,height-1);  // Test with a diagonal
    } else {
        ofClear(0);
    }
    fluid.end();
    // The main texture of fluid is copied to the obstacles FBO at each fluid.update()
    fluid.setUseObstacles(true);
    
    if (!frozen) {
        fluid.addTemporalForce(m, d, col,10.0f);
        
        //  Update
        //
        
        fluid.update();
    }
    
    ofSetWindowTitle(ofToString(ofGetFrameRate()));
    
    // Check for OSC messages
    // check for waiting messages
    while( receiver.hasWaitingMessages() ) {
        // get the next message
        ofxOscMessage m;
        receiver.getNextMessage( m );
        if (  m.getAddress()=="/navier/force" ) {
            // both the arguments are int32's
            int cellX = m.getArgAsInt32( 0 );
            int cellY = m.getArgAsInt32( 1 );
            float dX = m.getArgAsFloat(2);  // Velocity in pixels/sec
            float dY = m.getArgAsFloat(3);
            float red = m.getArgAsFloat(4);
            float green = m.getArgAsFloat(5);
            float blue = m.getArgAsFloat(6);
            float alpha = m.getArgAsFloat(7);
            float radius = m.getArgAsFloat(8);
            float temp = m.getArgAsFloat(9);
            float den=m.getArgAsFloat(10);
//            cout << "(" << cellX << "," << cellY << ") v=(" << dX << "," << dY << ")" << ", col=(" << red << "," << green << "," << blue << "," << alpha << ")" << " radius=" << radius << ", temp=" << temp << ", den=" << den << endl;
            ofPoint m = ofPoint(cellX, cellY);
            ofPoint d = ofPoint(dX, dY)/ofGetFrameRate();  // Velocity in pixels/frame
            pendingForces.push_back(punctualForce(m, d, ofFloatColor(red,green,blue,alpha),radius,temp,den));
        } else if (m.getAddress()=="/navier/updateForces") {
            fluid.setConstantForces(pendingForces);  // Fluid will use these until they are updated again
            pendingForces.clear();
            updateFlame();
        } else if (m.getAddress()=="/navier/smoke") {
            if (fluid.smokeEnabled!=m.getArgAsFloat(0)>0.5 || fluid.smokeBuoyancy!=m.getArgAsFloat(1) || fluid.smokeWeight!=m.getArgAsFloat(2)) {
                fluid.smokeEnabled=m.getArgAsFloat(0)>0.5;
                fluid.smokeBuoyancy=m.getArgAsFloat(1);
                fluid.smokeWeight=m.getArgAsFloat(2);
                cout << "smoke=" << (fluid.smokeEnabled?"Enabled":"Disabled") << ", " << fluid.smokeBuoyancy << ", " << fluid.smokeWeight << endl;
            }
        } else if (m.getAddress()=="/navier/border") {
            ofColor newColor=ofColor(m.getArgAsFloat(1),m.getArgAsFloat(2),m.getArgAsFloat(3));
            if (borderEnable!=m.getArgAsFloat(0)>0.5 || borderColor!=newColor) {
                borderEnable=m.getArgAsFloat(0)>0.5;
                borderColor=newColor;
                cout << "border=" << (borderEnable?"Enabled":"Disabled") << ": " << borderColor << endl;
            }
        } else if (m.getAddress()=="/navier/viscosity") {
            if (fluid.viscosity!=m.getArgAsFloat(0)) {
                fluid.viscosity=m.getArgAsFloat(0);
                cout << "viscosity=" << fluid.viscosity << endl;
            }
        } else if (m.getAddress()=="/navier/diffusion") {
            if (fluid.diffusion!=m.getArgAsFloat(0)) {
                fluid.diffusion=m.getArgAsFloat(0);
                cout << "diffusion=" << fluid.diffusion << endl;
            }
        } else if (m.getAddress()=="/navier/iterations") {
            if (fluid.numJacobiIterations != m.getArgAsInt32(0)) {
                fluid.numJacobiIterations=m.getArgAsInt32(0);
                cout << "iterations=" << fluid.numJacobiIterations << endl;
            }
        } else if (m.getAddress()=="/navier/dissipation") {
            if (fluid.dissipation != m.getArgAsFloat(0)) {
                fluid.dissipation=m.getArgAsFloat(0);
                cout << "dissipation=" << fluid.dissipation << endl;
            }
        } else if (m.getAddress()=="/navier/velocityDissipation") {
            if (fluid.velocityDissipation!=m.getArgAsFloat(0)) {
                fluid.velocityDissipation=m.getArgAsFloat(0);
                cout << "velocity dissipation=" << fluid.velocityDissipation << endl;
            }
        } else if (m.getAddress()=="/navier/ambient") {
            if (fluid.ambientTemperature!=m.getArgAsFloat(0)) {
                fluid.ambientTemperature=m.getArgAsFloat(0);
                cout << "ambient temp=" << fluid.ambientTemperature << endl;
            }
        } else if (m.getAddress()=="/navier/temperatureDissipation") {
            if (fluid.temperatureDissipation!=m.getArgAsFloat(0)) {
                fluid.temperatureDissipation=m.getArgAsFloat(0);
                cout << "temperature dissipation=" << fluid.temperatureDissipation << endl;
            }
        } else if (m.getAddress()=="/navier/pressureDissipation") {
            if (fluid.pressureDissipation!=m.getArgAsFloat(0)) {
                fluid.pressureDissipation=m.getArgAsFloat(0);
                cout << "pressure dissipation=" << fluid.pressureDissipation << endl;
            }
        } else if (m.getAddress()=="/navier/frozen") {
            if (frozen!=m.getArgAsFloat(0)>0.5f) {
                frozen=m.getArgAsFloat(0)>0.5f;
                cout << "frozen=" << (frozen?"true":"false") << endl;
            }
        } else if (m.getAddress()=="/navier/gravity") {
            ofVec2f newGrav(m.getArgAsFloat(0),m.getArgAsFloat(1));
            if (fluid.getGravity()!=newGrav) {
                fluid.setGravity(newGrav);
                cout << "gravity=" << fluid.getGravity() << endl;
            }
        } else if (m.getAddress()=="/navier/capture") {
            string filename=m.getArgAsString(0);
            saveTexture(filename+"-den.tif",fluid.getTexture());
            saveTexture(filename+"-vel.tif",fluid.getVelocityTexture());
            saveTexture(filename+"-temp.tif",fluid.getTemperatureTexture());
            saveTexture(filename+"-press.tif",fluid.getPressureTexture());
            saveTexture(filename+"-div.tif",fluid.getDivergenceTexture());
            fluid.update();
            saveTexture(filename+"-den2.tif",fluid.getTexture());
            cout << "Saved textures to " << filename << endl;
        } else if (m.getAddress()=="/navier/setsize") {
            int width=m.getArgAsInt32(0);
            int height=m.getArgAsInt32(1);
            float scale=m.getArgAsFloat(2);
            if (width*scale!=fluid.getWidth() || height*scale!=fluid.getHeight() || scale!=fluid.scale) {
                cout << "Resetting size to " << width << "x" << height << " with scale=" << scale << endl;
                setSize(width*scale,height*scale,scale,true);
            }
        } else if (m.getAddress()=="/navier/clear") {
            fluid.clear();
        } else if (m.getAddress()=="/navier/quit") {
            ofExit();
        } else if (m.getAddress()=="/navier/flame") {
            if ((flameEnable!=m.getArgAsFloat(0)>0.5) ||
                (flamePosition!=ofPoint(m.getArgAsFloat(1),m.getArgAsFloat(2))) ||
                (flameVelocity!=ofPoint(m.getArgAsFloat(3),m.getArgAsFloat(4))) ||
                (flameDensity!=m.getArgAsFloat(5)) ||
                (flameTemperature!=m.getArgAsFloat(6)) ||
                (flameRadius!=m.getArgAsFloat(7))) {
                flameEnable=m.getArgAsFloat(0)>0.5;
                flamePosition=ofPoint(m.getArgAsFloat(1),m.getArgAsFloat(2));
                flameVelocity=ofPoint(m.getArgAsFloat(3),m.getArgAsFloat(4));
                flameDensity=m.getArgAsFloat(5);
                flameTemperature=m.getArgAsFloat(6);
                flameRadius=m.getArgAsFloat(7);
                if (flameEnable) {
                    cout << "Flame enabled, pos=[" << flamePosition << "], vel=[" << flameVelocity << "], color=[" << flameColor << "], radius=" << flameRadius << ", temp=" << flameTemperature << ", den=" << flameDensity << endl;
                } else
                    cout << "Flame disabled" << endl;
            }
        } else {
            cout << "Unexpected OSC message: " << m.getAddress() << endl;
        }
        
    }
    
    fluid.clearAlpha();
    syphon[0].publishTexture(&fluid.getTexture());
    syphon[2].publishTexture(&fluid.getVelocityTexture());
    syphon[3].publishTexture(&fluid.getTemperatureTexture());
    syphon[4].publishTexture(&fluid.getPressureTexture());
    
    
    if (false) {
        // Display the pixel values from top left corner for debuggin
        ofTexture &t=fluid.getVelocityTexture();
        ofTextureData td=t.getTextureData();
        cout << "Texture type=" << hex << td.glInternalFormat << ", min=" << td.minFilter << ", mag=" << td.magFilter << ", compression=" << td.compressionType << dec << endl;
        ofFloatPixels pixels;  // Converts to 8-bit from 32F texture format
        t.readToPixels(pixels);
        cout << "pixels: type=" << pixels.getImageType() << ", width=" << pixels.getWidth() << ", height=" << pixels.getHeight() << ", bits/pix=" << pixels.getBitsPerPixel() << endl;
        for (int x=0;x<3;x++) {
            for (int y=0;y<3;y++) {
                auto c=pixels.getColor(x,y);
                cout << "(" << x << "," << y << ")="  << c << " " ;
            }
            cout << endl;
        }
    }
}

void ofApp::saveTexture(string filename, const ofTexture &tex) {
    const ofTextureData &td=tex.getTextureData();
    if (td.glInternalFormat==GL_RGBA || td.glInternalFormat==GL_RGB ) {
        ofPixels pixels;  // Converts to 8-bit from 32F texture format
        tex.readToPixels(pixels);
        ofSaveImage(pixels,filename);
    } else if (td.glInternalFormat==GL_RGBA32F || td.glInternalFormat==GL_RGB32F || td.glInternalFormat==GL_RGB16F ) {
        ofFloatPixels pixels;  // Converts to 8-bit from 32F texture format
        tex.readToPixels(pixels);
        ofSaveImage(pixels,filename);
    } else {
        cout << "ofApp::saveTexture - " << filename << ": unrecognized internal format 0x" << hex << td.glInternalFormat << dec << endl;
        return;
    }
    cout << "Saved texture to " << filename << endl;
}
//--------------------------------------------------------------
void ofApp::draw(){
    // ofBackgroundGradient(ofColor::gray, ofColor::black, OF_GRADIENT_LINEAR);
    
    fluid.draw();
    
    syphon[1].publishScreen();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 
    
}
