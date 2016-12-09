#include "ofApp.h"

const int N = 256;
float spectrum[ N ];	//Smoothed spectrum values
float Rad = 500;
float Vel = 0.1;
int bandRad = 2;
int bandVel = 100;

const int n = 300;

//Offsets for Perlin noise calculation for points
float tx[n], ty[n];

ofPoint p[n];			//Cloud's points positions

float time0 = 0;		//Time value, used for dt computing

//--------------------------------------------------------------
void ofApp::setup(){
    sound.load( "DreamSpark.mp3" );
    sound.setLoop( true );
    sound.play();
    
    synth.load("synth.wav");
    synth.setVolume(1.85f);
    synth.setMultiPlay(false);
    
    //Set spectrum values to 0
    for (int i=0; i<N; i++) {
        spectrum[i] = 0.0f;
    }
    for ( int j=0; j<n; j++ ) {
        tx[j] = ofRandom( 0, 1000 );
        ty[j] = ofRandom( 0, 1000 );
    }
    
    ofSetVerticalSync(false);
    ofSetLogLevel(OF_LOG_NOTICE);
    
    
    drawWidth = 1024;
    drawHeight = 768;
    
    // process all but the density on 16th resolution
    flowWidth = drawWidth / 4;
    flowHeight = drawHeight / 4;
    
    // FLOW & MASK
    opticalFlow.setup(flowWidth, flowHeight);
    velocityMask.setup(drawWidth, drawHeight);
    
    // FLUID & PARTICLES
    fluidSimulation.setup(flowWidth, flowHeight, drawWidth, drawHeight);
    particleFlow.setup(flowWidth, flowHeight, drawWidth, drawHeight);
    
    velocityDots.setup(flowWidth / 4, flowHeight / 4);
    
    // VISUALIZATION
    displayScalar.setup(flowWidth, flowHeight);
    velocityField.setup(flowWidth / 4, flowHeight / 4);
    temperatureField.setup(flowWidth / 4, flowHeight / 4);
    pressureField.setup(flowWidth / 4, flowHeight / 4);
    velocityTemperatureField.setup(flowWidth / 4, flowHeight / 4);
    
    // MOUSE DRAW
    mouseForces.setup(flowWidth, flowHeight, drawWidth, drawHeight);
    
    // CAMERA
    kinect.init(true);
    kinect.open();
    kinectFbo.allocate(kinect.getWidth(), kinect.getHeight(), GL_RGBA32F);
    kinectFbo.getTexture().setRGToRGBASwizzles(true);
    //std::cout<<"djsk:"<<&kinect.getDistancePixels();
    
    kinectFbo.begin();
    ofClear(255, 255, 255, 0);
    kinectFbo.end();
    ofLogError("kinect inited");
    
    
    // GUI
    setupGui();
    lastTime = ofGetElapsedTimef();
    
}

//--------------------------------------------------------------
void ofApp::setupGui() {
    
    gui.setup("settings");
    gui.setDefaultBackgroundColor(ofColor( 107, 193, 219));
    gui.setDefaultFillColor(ofColor(160, 160, 160, 160));
    gui.add(doFullScreen.set("fullscreen (F)", false));
    doFullScreen.addListener(this, &ofApp::setFullScreen);
    doFlipCamera = false;
    
    gui.add(drawName.set("MODE", "draw"));
    
    
    int guiColorSwitch = 0;
    ofColor guiHeaderColor[2];
    
    guiHeaderColor[0].set(102,153,204);
    guiHeaderColor[1].set(51,102,153);
    
    ofColor guiFillColor[2];
    
    guiFillColor[0].set(51,102,153);
    guiFillColor[1].set(102,153,204);
    
    visualizeParameters.setName("visualizers");
    visualizeParameters.add(showScalar.set("show scalar", true));
    visualizeParameters.add(showField.set("show field", true));
    visualizeParameters.add(displayScalarScale.set("scalar scale", 0.15, 0.05, 1.0));
    displayScalarScale.addListener(this, &ofApp::setDisplayScalarScale);
    visualizeParameters.add(velocityFieldScale.set("velocity scale", 0.1, 0.0, 0.5));
    velocityFieldScale.addListener(this, &ofApp::setVelocityFieldScale);
    visualizeParameters.add(temperatureFieldScale.set("temperature scale", 0.1, 0.0, 0.5));
    temperatureFieldScale.addListener(this, &ofApp::setTemperatureFieldScale);
    visualizeParameters.add(pressureFieldScale.set("pressure scale", 0.02, 0.0, 0.5));
    pressureFieldScale.addListener(this, &ofApp::setPressureFieldScale);
    visualizeParameters.add(velocityLineSmooth.set("line smooth", false));
    velocityLineSmooth.addListener(this, &ofApp::setVelocityLineSmooth);
    
    gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
    gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
    guiColorSwitch = 1 - guiColorSwitch;
    gui.add(particleFlow.parameters);
    
    gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
    gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
    guiColorSwitch = 1 - guiColorSwitch;
    gui.add(visualizeParameters);
    
    
    // if the settings file is not present the parameters will not be set during this setup
    if (!ofFile("settings.xml"))
        gui.saveToFile("settings.xml");
    gui.loadFromFile("settings.xml");
    gui.minimizeAll();
    toggleGuiDraw = true;
    
}

//--------------------------------------------------------------
void ofApp::update(){
    ofSoundUpdate();
    
    //Get current spectrum with N bands
    float *val = ofSoundGetSpectrum( N );
    
    //Update our smoothed spectrum,
    for ( int i=0; i<N; i++ ) {
        spectrum[i] *= 0.97;	//Slow decreasing
        spectrum[i] = max( spectrum[i], val[i] );
    }
    
    //Update particles using spectrum values
    float time = ofGetElapsedTimef();
    float dt = time - time0;
    dt = ofClamp( dt, 0.0, 0.1 );
    time0 = time; //Store the current time
    
    //Update Rad and Vel from spectrum
    Rad = ofMap( spectrum[ bandRad ], 1, 3, 400, 800, true );
    Vel = ofMap( spectrum[ bandVel ], 0, 0.1, 0.05, 0.5 );
    
    //Update particles positions
    for (int j=0; j<n; j++) {
        tx[j] += Vel * dt;	//move offset
        ty[j] += Vel * dt;	//move offset
        
        p[j].x = ofSignedNoise( tx[j] ) * Rad;
        p[j].y = ofSignedNoise( ty[j] ) * Rad;
    }
    
    deltaTime = ofGetElapsedTimef() - lastTime;
    lastTime = ofGetElapsedTimef();
    
    //Update the Kinect
    
    kinect.update();
    
    if (kinect.isFrameNew()) {
        ofPushStyle();
        ofEnableBlendMode(OF_BLENDMODE_DISABLED);
        if (doFlipCamera)
            kinect.drawDepth(kinectFbo.getWidth(), 0, -kinectFbo.getWidth(), kinectFbo.getHeight());
        else
            kinect.drawDepth(0, 0, kinectFbo.getWidth(), kinectFbo.getHeight());
        
        
        ofPopStyle();
        opticalFlow.setSource(kinect.getTexture());
        
        opticalFlow.update();
        
        velocityMask.setDensity(kinectFbo.getTexture());
        velocityMask.setVelocity(opticalFlow.getOpticalFlow());
        velocityMask.update();
    }
    
    
    fluidSimulation.addVelocity(opticalFlow.getOpticalFlowDecay());
    fluidSimulation.addDensity(velocityMask.getColorMask());
    fluidSimulation.addTemperature(velocityMask.getLuminanceMask());
    
    mouseForces.update(deltaTime);
    
    for (int i=0; i<mouseForces.getNumForces(); i++) {
        if (mouseForces.didChange(i)) {
            switch (mouseForces.getType(i)) {
                case FT_DENSITY:
                    fluidSimulation.addDensity(mouseForces.getTextureReference(i), mouseForces.getStrength(i));
                    break;
                case FT_VELOCITY:
                    fluidSimulation.addVelocity(mouseForces.getTextureReference(i), mouseForces.getStrength(i));
                    particleFlow.addFlowVelocity(mouseForces.getTextureReference(i), mouseForces.getStrength(i));
                    break;
                case FT_TEMPERATURE:
                    fluidSimulation.addTemperature(mouseForces.getTextureReference(i), mouseForces.getStrength(i));
                    break;
                case FT_PRESSURE:
                    fluidSimulation.addPressure(mouseForces.getTextureReference(i), mouseForces.getStrength(i));
                    break;
                default:
                    break;
            }
        }
    }
    
    fluidSimulation.update();
    
    if (particleFlow.isActive()) {
        particleFlow.setSpeed(fluidSimulation.getSpeed());
        particleFlow.setCellSize(fluidSimulation.getCellSize());
        particleFlow.addFlowVelocity(opticalFlow.getOpticalFlow());
        particleFlow.addFluidVelocity(fluidSimulation.getVelocity());
        particleFlow.setObstacle(fluidSimulation.getObstacle());
    }
    particleFlow.update();
    
}
//--------------------------------------------------------------

void ofApp::mousePressed(int x, int y, int button){
    float widthStep = ofGetWidth();
    if (x < widthStep){
        synth.play();
        synth.setSpeed( 0.1f + ((float)(ofGetHeight() - y) / (float)ofGetHeight())*5);
        synth.setPan(ofMap(x, 0, widthStep, -1, 1, true));
    }
}

//--------------------------------------------------------------

void ofApp::keyPressed(int key){
    switch (key) {
        case 'G':
        case 'g': toggleGuiDraw = !toggleGuiDraw; break;
        case 'f':
        case 'F': doFullScreen.set(!doFullScreen.get()); break;
            
        case '1': drawMode.set(DRAW_COMPOSITE); break;
        case '2': drawMode.set(DRAW_FLUID_VELOCITY); break;
        case '3': drawMode.set(DRAW_FLUID_PRESSURE); break;
        case '4': drawMode.set(DRAW_FLOW_MASK); break;
        case '5': drawMode.set(DRAW_SOURCE); break;
            
        case 'r':
        case 'R':
            fluidSimulation.reset();
            mouseForces.reset();
            break;
        default: break;
    }
    if(key == OF_KEY_UP){
        angle++;
        if(angle>30) angle=30;
        kinect.setCameraTiltAngle(angle);
    }
    else if(key == OF_KEY_DOWN){
        angle--;
        if(angle<-30) angle=-30;
        kinect.setCameraTiltAngle(angle);
    }
    
}

//--------------------------------------------------------------
void ofApp::drawModeSetName(int &_value) {
    switch(_value) {
        case DRAW_COMPOSITE:		drawName.set("DrawSomething     (1)"); break;
        case DRAW_PARTICLES:		drawName.set("Particles      "); break;
        case DRAW_FLUID_VELOCITY:	drawName.set("Velocity (2)"); break;
        case DRAW_FLUID_PRESSURE:	drawName.set("Pressure (3)"); break;
        case DRAW_FLUID_VORTICITY:	drawName.set("Fluid Vorticity"); break;
        case DRAW_FLOW_MASK:		drawName.set("Mask      (4)"); break;
        case DRAW_SOURCE:			drawName.set("Source     (5)"); break;
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofClear(0,0);
    
    if (!toggleGuiDraw) {
        ofHideCursor();
        switch(drawMode.get()) {
            case DRAW_COMPOSITE: drawComposite(); break;
            case DRAW_PARTICLES: drawParticles(); break;
            case DRAW_FLUID_FIELDS: drawFluidFields(); break;
            case DRAW_FLUID_DENSITY: drawFluidDensity(); break;
            case DRAW_FLUID_VELOCITY: drawFluidVelocity(); break;
            case DRAW_FLUID_PRESSURE: drawFluidPressure(); break;
            case DRAW_FLOW_MASK: drawMask(); break;
            case DRAW_SOURCE: drawSource(); break;
        }
    }
    else {
        ofShowCursor();
        switch(drawMode.get()) {
            case DRAW_COMPOSITE: drawComposite(); break;
            case DRAW_PARTICLES: drawParticles(); break;
            case DRAW_FLUID_FIELDS: drawFluidFields(); break;
            case DRAW_FLUID_DENSITY: drawFluidDensity(); break;
            case DRAW_FLUID_VELOCITY: drawFluidVelocity(); break;
            case DRAW_FLUID_PRESSURE: drawFluidPressure(); break;
            case DRAW_FLOW_MASK: drawMask(); break;
            case DRAW_SOURCE: drawSource(); break;
        }
        drawGui();
    }
}

//--------------------------------------------------------------
void ofApp::drawComposite(int _x, int _y, int _width, int _height) {
    ofPushStyle();
    
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    fluidSimulation.draw(_x, _y, _width, _height);
    
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    if (particleFlow.isActive())
        particleFlow.draw(_x, _y, _width, _height);
    
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawParticles(int _x, int _y, int _width, int _height) {
    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    if (particleFlow.isActive())
        particleFlow.draw(_x, _y, _width, _height);
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidFields(int _x, int _y, int _width, int _height) {
    ofPushStyle();
    
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    pressureField.setPressure(fluidSimulation.getPressure());
    pressureField.draw(_x, _y, _width, _height);
    velocityTemperatureField.setVelocity(fluidSimulation.getVelocity());
    velocityTemperatureField.setTemperature(fluidSimulation.getTemperature());
    velocityTemperatureField.draw(_x, _y, _width, _height);
    temperatureField.setTemperature(fluidSimulation.getTemperature());
    
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidDensity(int _x, int _y, int _width, int _height) {
    ofPushStyle();
    
    fluidSimulation.draw(_x, _y, _width, _height);
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidVelocity(int _x, int _y, int _width, int _height) {
    ofPushStyle();
    if (showScalar.get()) {
        ofClear(0,0);
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        displayScalar.setSource(fluidSimulation.getVelocity());
        displayScalar.draw(_x, _y, _width, _height);
    }
    if (showField.get()) {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        velocityField.setVelocity(fluidSimulation.getVelocity());
        velocityField.setColor(ofColor(ofRandom(225),ofRandom(255),ofRandom(60)*sin(ofGetElapsedTimef()*2)));
        velocityField.draw(_x, _y, _width, _height);
    }
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidPressure(int _x, int _y, int _width, int _height) {
    ofPushStyle();
    ofClear(128);
    if (showScalar.get()) {
        ofEnableBlendMode(OF_BLENDMODE_DISABLED);
        displayScalar.setSource(fluidSimulation.getPressure());
        displayScalar.draw(_x, _y, _width, _height);
    }
    if (showField.get()) {
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        pressureField.setPressure(fluidSimulation.getPressure());
        pressureField.draw(_x, _y, _width, _height);
    }
    ofPopStyle();
}


//--------------------------------------------------------------
void ofApp::drawFluidVorticity(int _x, int _y, int _width, int _height) {
    ofPushStyle();
    
    if (showScalar.get()) {
        ofEnableBlendMode(OF_BLENDMODE_DISABLED);
        displayScalar.setSource(fluidSimulation.getConfinement());
        displayScalar.draw(_x, _y, _width, _height);
    }
    if (showField.get()) {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        ofSetColor(255, 255);
        velocityField.setVelocity(fluidSimulation.getConfinement());
        velocityField.setColor(ofColor(ofRandom(255),ofRandom(255),ofRandom(40)*sin(ofGetElapsedTimef()*2)));
        velocityField.draw(_x, _y, _width, _height);
    }
    ofPopStyle();
}


//--------------------------------------------------------------
void ofApp::drawMask(int _x, int _y, int _width, int _height) {
    
    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_DISABLED);
    velocityMask.getTexture().draw(_x, _y, _width, _height);
    ofPopStyle();
}

void ofApp::drawSource(int _x, int _y, int _width, int _height) {
    ofPushStyle();
    ofClear(0,0);
    ofColor colorOne(255,228,225);
    ofColor colorTwo(255,192,203);
    ofBackgroundGradient(colorOne, colorTwo, OF_GRADIENT_LINEAR);
    //Draw cloud
    ofPushMatrix();
    ofTranslate( ofGetWidth() / 2, ofGetHeight() / 2 );
    float hue = fmodf(ofGetElapsedTimef()*10,255);
    
    ofColor color = ofColor::fromHsb(hue,
                                     ofMap( mouseX, 0,ofGetWidth(), 0,255 ),
                                     ofMap( mouseY, ofGetHeight(),0, 0,255 ) );
    ofSetColor( color );
    ofFill();
    for (int i=0; i<n; i++) {
        ofDrawCircle( p[i], 2 );
    }
    
    float dist = 80;	//distance
    for (int j=0; j<n; j++) {
        for (int k=j+1; k<n; k++) {
            if ( ofDist( p[j].x, p[j].y, p[k].x, p[k].y )
                < dist ) {
                ofDrawLine( p[j], p[k] );
            }
        }
    }
    //Restore coordinate system
    ofPopMatrix();
    ofSetColor( color );
    ofFill();
    ofDrawCircle( mouseX, mouseY, 10);
    
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawGui() {
    guiFPS = (int)(ofGetFrameRate() + 0.5);
    
    // calculate minimum fps
    deltaTimeDeque.push_back(deltaTime);
    
    while (deltaTimeDeque.size() > guiFPS.get())
        deltaTimeDeque.pop_front();
    
    float longestTime = 0;
    for (int i=0; i<deltaTimeDeque.size(); i++){
        if (deltaTimeDeque[i] > longestTime)
            longestTime = deltaTimeDeque[i];
    }
    
    guiMinFPS.set(1.0 / longestTime);
    
    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    gui.draw();
    ofPopStyle();
}
