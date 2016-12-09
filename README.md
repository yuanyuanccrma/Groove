# Groove

Music 256a/CS 476a Final Project 

The idea is to create an immersive environment for a real dancer which develops an interactive blend of body movements with graphics and sound. Groove combines the 2D fluid simulation, using GLSL shaders with live Kinect camera input (or with mouse motion).
Based on the ofxFlowTools, I added and a simple mask, using GL_RGBA32F for buffers.

Instructions
* This version was created using OSX 10.11.6, OF 0.9.6 and a kinect device xBox 360
* Open Frameworks addons: ofxKinect, ofxGui, <a href="https://github.com/moostrik/ofxFlowTools">ofxFlowTools</a>
* Key Commands:
1: Fluid and Particle System
2: Fluid Velocity Field
3: Fluid Pressure Field 
4: Kinect Source 
5: Music Visualization
Key Up/Down: Adjust Kinect Angle 
Mouse Pressed: a preset synth automatically trigered and the speed and panning of sound can be controled by mouse moving up/down and left/right.
---
Have fun!
Implemented by Yuan Li (yuanyuanATccrmaDOTstanfordDOTedu) for Music 256a / CS 476a (fall 2016).
