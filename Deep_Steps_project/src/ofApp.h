/*
 * Copyright (c) 2011 Dan Wilcox <danomatika@gmail.com>
 *
 * BSD Simplified License.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 *
 * See https://github.com/danomatika/ofxPd for documentation
 *
 */
#pragma once

#include "ofMain.h"
#include "ofxSimpleSlider.h"
#include "ofxPd.h"
#include "ofxMidi.h"
#include "ofxMidiClock.h"


// a namespace for the Pd types
using namespace pd;

// inherit pd receivers to receive message and midi events
class ofApp : public ofBaseApp, public PdReceiver, public PdMidiReceiver, public ofxMidiListener {
    
public:
    
    // main
    void setup();
    void update();
    void draw();
    void exit();
    
    string resourcesPath;
    string printer;
    std::vector<std::string> CCVector;

    int CCin = 0;
    
    int aCC;
    bool aIsPressed = false;
    int bCC;
    bool bIsPressed = false;
    int cCC;
    bool cIsPressed = false;
    int dCC;
    bool dIsPressed = false;
    
    int epochs = 50;
    bool epochsIsPressed = false;
    
    int key = 0;
    bool keyIsPressed = false;
    
    int scale = 0;
    
    float loss;
    
    void saveToFile(const std::string& filename, int variable1, int variable2, int variable3, int variable4);
    bool loadFromFile(const std::string& filename);
    
    // input callbacks
    void keyPressed(int key);
    void mousePressed(int x, int y, int button);
    void mouseDragged(int x, int y, int button);
    
    // audio callbacks
    void audioReceived(float * input, int bufferSize, int nChannels);
    void audioRequested(float * output, int bufferSize, int nChannels);
    
    string data;
    // pd message receiver callbacks
    void print(const std::string &message);
    
    void receiveBang(const std::string &dest);
    void receiveFloat(const std::string &dest, float value);
    void receiveSymbol(const std::string &dest, const std::string &symbol);
    void receiveList(const std::string &dest, const pd::List &list);
    void receiveMessage(const std::string &dest, const std::string &msg, const pd::List &list);
    
    // pd midi receiver callbacks
    void receiveNoteOn(const int channel, const int pitch, const int velocity);
    void receiveControlChange(const int channel, const int controller, const int value);
    void receiveProgramChange(const int channel, const int value);
    void receivePitchBend(const int channel, const int value);
    void receiveAftertouch(const int channel, const int value);
    void receivePolyAftertouch(const int channel, const int pitch, const int value);
    void receiveMidiByte(const int port, const int byte);
    
    ofxPd pd;
    std::vector<float> scopeArray;
    std::vector<Patch> instances;
    
    int midiChan;
    bool playState = false;
    float currentStep;
    bool step1 = false;
    bool step2 = false;
    bool step3 = false;
    bool step4 = false;
    bool step5 = false;
    bool step6 = false;
    bool step7 = false;
    bool step8 = false;
    bool step9 = false;
    bool step10 = false;
    bool step11 = false;
    bool step12 = false;
    bool step13 = false;
    bool step14 = false;
    bool step15 = false;
    bool step16 = false;
    
    string aeStatus = "Initialised - Ready to Train";
    
    
    ofxMidiOut midiOut;
    int channel;
    
    /// ofxMidiListener callback
    void newMidiMessage(ofxMidiMessage& eventArgs);
    
    ofxMidiIn midiIn;
    //std::vector<ofxMidiMessage> midiMessages;
    bool verbose = false;
    
    // MIDI CLOCK
    
    ofxMidiClock clock; //< clock message parser
    bool clockRunning = false; //< is the clock sync running?
    unsigned int beats = 0; //< song pos in beats
    double seconds = 0; //< song pos in seconds, computed from beats
    double bpm = 120; //< song tempo in bpm, computed from clock length
    
    unsigned int currentPgm;
    int note, velocity;
    int pan, bend, touch, polytouch;
    
    //gui elements
    ofxSimpleSlider tempoSlider, note1, note2, note3, note4, note5, note6, note7, note8, note9, note10, note11, note12, note13, note14, note15, note16, latentA, latentB, latentC, latentD, gateLength, substepsScale, seqLength;
    
    ofRectangle playButton, trainButton, startButton, openCorpus, midiInUp, midiOutUp, midiInCh, midiOutCh, CCa, CCb, CCc, CCd, epochsButton, keyButton, scaleButton, generateButton, pitchReset;
    
    ofTrueTypeFont font;
    
    bool mode = false;
    
    

    
};
    

