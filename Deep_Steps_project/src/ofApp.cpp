/* Deep Steps by Alex Wastnidge
 
 Deep Steps is dependant on the ofxPd and ofxMidi add-ons from danomatika.  Their usage here is based on the examples provided by the author.
 
 The aubio library is used for onset detection.  The usage is based on the onset example; https://aubio.org/doc/latest/onset_2test-onset_8c-example.html
 
 Python embedding is based on the "Very high level" embedding methodology found here: https://docs.python.org/3/extending/embedding.html
 
 */

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
/*
 * Copyright (c) 2018 Dan Wilcox <danomatika@gmail.com>
 *
 * BSD Simplified License.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 *
 * See https://github.com/danomatika/ofxMidi for documentation
 *
 */
#include "ofApp.h"

#include <Python/Python.h>

#include <aubio/aubio.h>


//--------------------------------------------------------------
void ofApp::setup() {
    ofDisableDataPath();
    //OSX
    char *home = getenv("HOME");
    string subdir = "/data/";
    
    string dataPath = home + subdir;
    std::filesystem::path pathToData = dataPath;
    ofDirectory dataFolder("data");
    string dataFolderPath = dataFolder.getAbsolutePath();
    resourcesPath = dataFolderPath;


    
    ofSetFrameRate(60);
    ofSetVerticalSync(true);
    
    font.load(dataFolderPath + "/full_Pack_2025.ttf", 20);
    
    ofSetWindowShape(600, 600);

    Py_Initialize();

    
    //Py Initialise script
    data = dataFolderPath + "/AE_init.py";
    printer = resourcesPath;
    const char* scriptPath = data.c_str();

//
//    /// Run the Python script using PyRun_SimpleFile()
    FILE* file = fopen(scriptPath, "r");
    if (file == NULL) {
        fprintf(stderr, "Failed to open the script file\n");
        return 1;
    }
    int result = PyRun_SimpleFile(file, scriptPath);

    // Close the file
    fclose(file);

    // Check the result of running the script
    if (result != 0) {
        fprintf(stderr, "Failed to run the script\n");
        return 1;
    }
    



    // print the available output ports to the console
    //midiOut.listOutPorts();
    // connect
    midiOut.openPort(0); // by number
    //midiOut.openPort("IAC Driver Pure Data In"); // by name
    //midiOut.openVirtualPort("ofxMidiOut"); // open a virtual port
    
    channel = 0;
    currentPgm = 0;
    note = 0;
    velocity = 0;
    pan = 0;
    bend = 0;
    touch = 0;
    polytouch = 0;
    
    // open port by number (you may need to change this)
    midiIn.openPort(0);
    midiIn.ignoreTypes(false, // sysex  <-- don't ignore timecode messages!
                       false, // timing <-- don't ignore clock messages!
                       true); // sensing

    // add ofApp as a listener
    midiIn.addListener(this);
    
    
    // double check where we are ...
    //std::cout << ofFilePath::getCurrentWorkingDirectory() << std::endl;
    
    // the number of libpd ticks per buffer,
    // used to compute the audio buffer len: tpb * blocksize (always 64)
#ifdef TARGET_LINUX_ARM
    // longer latency for Raspberry PI
    int ticksPerBuffer = 32; // 32 * 64 = buffer len of 2048
    int numInputs = 0; // no built in mic
#else
    int ticksPerBuffer = 8; // 8 * 64 = buffer len of 512
    int numInputs = 1;
#endif
    
    // setup OF sound stream
    ofSoundStreamSettings settings;
    settings.numInputChannels = 0;
    settings.numOutputChannels = 2;
    settings.sampleRate = 44100;
    settings.bufferSize = ofxPd::blockSize() * ticksPerBuffer;
    settings.setInListener(this);
    settings.setOutListener(this);
    ofSoundStreamSetup(settings);
    
    // setup Pd
    //
    // set 4th arg to true for queued message passing using an internal ringbuffer,
    // this is useful if you need to control where and when the message callbacks
    // happen (ie. within a GUI thread)
    //
    // note: you won't see any message prints until update() is called since
    // the queued messages are processed there, this is normal
    //
    if(!pd.init(2, numInputs, 44100, ticksPerBuffer, false)) {
        OF_EXIT_APP(1);
    }
    
    midiChan = 1; // midi channels are 1-16
    
    // subscribe to receive source names
    pd.subscribe("toOF");
    pd.subscribe("loss");
    
    // add message receiver, required if you want to recieve messages
    pd.addReceiver(*this); // automatically receives from all subscribed sources
    //pd.ignoreSource(*this, "env");        // don't receive from "env"
    //pd.ignoreSource(*this);             // ignore all sources
    //pd.receiveSource(*this, "toOF");	  // receive only from "toOF"
    
    // add midi receiver, required if you want to recieve midi messages
    pd.addMidiReceiver(*this); // automatically receives from all channels
    //pd.ignoreMidiChannel(*this, 1);     // ignore midi channel 1
    //pd.ignoreMidiChannel(*this);        // ignore all channels
    //pd.receiveMidiChannel(*this, 1);    // receive only from channel 1
    
    // audio processing on
    pd.start();
    
    // -----------------------------------------------------
    std::cout << std::endl << "BEGIN Patch" << std::endl;
    
    // open patch
    Patch patch = pd.openPatch(dataFolderPath + "/pd/main.pd");
    std::cout << patch << std::endl;
    

    
    std::cout << "Patch Open" << std::endl;
    
    
    //GUI setup
    
    int x = -10, width = 20, step = 75;
    x += step;
    tempoSlider.setup(396, 20, 100, 20, 80, 200, 120, false, true, false);
    tempoSlider.setLabelString("Tempo");
    note1.setup(60, 90, width, 100, 24, 127, 50, true, false, true);
    note2.setup(90, 90, width, 100, 24, 127, 50, true, false, true);
    note3.setup(120, 90, width, 100, 24, 127, 50, true, false, true);
    note4.setup(150, 90, width, 100, 24, 127, 50, true, false, true);
    note5.setup(180, 90, width, 100, 24, 127, 50, true, false, true);
    note6.setup(210, 90, width, 100, 24, 127, 50, true, false, true);
    note7.setup(240, 90, width, 100, 24, 127, 50, true, false, true);
    note8.setup(270, 90, width, 100, 24, 127, 50, true, false, true);
    note9.setup(300, 90, width, 100, 24, 127, 50, true, false, true);
    note10.setup(330, 90, width, 100, 24, 127, 50, true, false, true);
    note11.setup(360, 90, width, 100, 24, 127, 50, true, false, true);
    note12.setup(390, 90, width, 100, 24, 127, 50, true, false, true);
    note13.setup(420, 90, width, 100, 24, 127, 50, true, false, true);
    note14.setup(450, 90, width, 100, 24, 127, 50, true, false, true);
    note15.setup(480, 90, width, 100, 24, 127, 50, true, false, true);
    note16.setup(510, 90, width, 100, 24, 127, 50, true, false, true);
    latentA.setup(160, 250, 150, 20, -10, 10, 0, false, true, true);
    latentA.setLabelString("A");
    latentB.setup(160, 280, 150, 20, -10, 10, 0, false, true, true);
    latentB.setLabelString("B");
    latentC.setup(160, 310, 150, 20, -10, 10, 0, false, true, true);
    latentC.setLabelString("C");
    latentD.setup(160, 340, 150, 20, -10, 10, 0, false, true, true);
    latentD.setLabelString("D");
    gateLength.setup(160, 370, 150, 20, 1, 1000, 100, false, true, true);
    gateLength.setLabelString("Gate Length");
    substepsScale.setup(160, 400, 150, 20, 0, 6, 0, false, true, true);
    substepsScale.setLabelString("Sub-Steps Scaling");
    seqLength.setup(160, 430, 150, 20, 1, 16, 16, false, true, true);
    seqLength.setLabelString("Seq Length");
    
    //buttons
    playButton.set(396, 50, 25, 20);
    openCorpus.set(20, 450, 100, 25);
    startButton.set(20,500,100,25);
    trainButton.set(20, 550, 100, 25);
    epochsButton.set(150, 550, 100, 25);
    keyButton.set(500, 300, 20, 20);
    scaleButton.set(500, 350, 100, 25);
    generateButton.set(20, 300, 100, 50);
    pitchReset.set(20, 370, 100, 20);
    
    
    midiInUp.set(500, 500, 20, 20);
    midiInCh.set(525,500,20,20);
    midiOutUp.set(500, 550, 20, 20);
    midiOutCh.set(525,550, 20, 20);
    
    CCa.set(130, 250, 30, 20);
    CCb.set(130, 280, 30, 20);
    CCc.set(130, 310, 30, 20);
    CCd.set(130, 340, 30, 20);

    loadFromFile("midiCCmap.txt");
    std::cout << aCC << std::endl;
    
    
    }

//--------------------------------------------------------------
void ofApp::update() {
	ofBackground(ofColor::lightSlateGrey);
	
	// since this is a test and we don't know if init() was called with
	// queued = true or not, we check it here
	if(pd.isQueued()) {
		// process any received messages, if you're using the queue and *do not*
		// call these, you won't receieve any messages or midi!
		pd.receiveMessages();
		pd.receiveMidi();
	}
    
    
    //pd.sendFloat("tempo", tempoSlider.getValue());
    pd.sendFloat("note1", note1.getValue());
    pd.sendFloat("note2", note2.getValue());
    pd.sendFloat("note3", note3.getValue());
    pd.sendFloat("note4", note4.getValue());
    pd.sendFloat("note5", note5.getValue());
    pd.sendFloat("note6", note6.getValue());
    pd.sendFloat("note7", note7.getValue());
    pd.sendFloat("note8", note8.getValue());
    pd.sendFloat("note9", note9.getValue());
    pd.sendFloat("note10", note10.getValue());
    pd.sendFloat("note11", note11.getValue());
    pd.sendFloat("note12", note12.getValue());
    pd.sendFloat("note13", note13.getValue());
    pd.sendFloat("note14", note14.getValue());
    pd.sendFloat("note15", note15.getValue());
    pd.sendFloat("note16", note16.getValue());
    
    pd.sendFloat("key", key);
    pd.sendFloat("scalenotes",scale);
    

    
    pd.sendFloat("gate", gateLength.getValue());
    pd.sendFloat("scale", substepsScale.getValue());
    pd.sendFloat("len", seqLength.getValue());
    
    // C++ variables
    float a_val = latentA.getValue();
    float b_val = latentB.getValue();
    float c_val = latentC.getValue();
    float d_val = latentD.getValue();

    // Construct Python code with formatted string
    std::stringstream pythonCode;
    pythonCode << "a_value = " << a_val << "\n";
    pythonCode << "b_value = " << b_val << "\n";
    pythonCode << "c_value = " << c_val << "\n";
    pythonCode << "d_value = " << d_val << "\n";
    pythonCode << "receive_latent(a_value,b_value,c_value,d_value) \n";


    // Execute Python code
    PyRun_SimpleString(pythonCode.str().c_str());    // C++ variables
    
    
    
};

//--------------------------------------------------------------
void ofApp::draw() {
    
    ofSetColor(ofColor::white);
    font.drawString("DEEP STEPS", 20, 30);
    
    stringstream text2;
    text2 << "MIDI In: " << midiIn.getName() << "\"" << endl
    << "receiving channel " << channel << endl << endl;
    ofDrawBitmapString(text2.str(), 290, 500);
    
    
    stringstream text;
    text << "MIDI Out: " << midiOut.getName() << "\"" << endl
    << "sending to channel " << channel << endl << endl;
    ofDrawBitmapString(text.str(), 290, 550);
    
    //ofDrawBitmapString(printer, 50, 50);
    ofSetColor(ofColor::slateGray);
    ofDrawRectangle(playButton);
    
    if(playState==1){
    ofSetColor(ofColor::lightGreen);// draw a yellow rectangle when clicked
    }else{
    ofSetColor(ofColor::white);
    }
    ofDrawTriangle(400, 50, 400, 70, 420, 60);

    ofSetColor(ofColor::steelBlue);
    ofDrawRectangle(trainButton);
    
    ofSetColor(ofColor::seaGreen);
    ofDrawRectangle(startButton);
    
    ofSetColor(ofColor::darkorange);
    ofDrawRectangle(openCorpus);
    
    ofSetColor(ofColor::darkGrey);
    ofDrawRectangle(midiInUp);
    ofDrawRectangle(midiOutUp);
    ofDrawRectangle(midiInCh);
    ofDrawRectangle(midiOutCh);
    
    //status
    ofSetColor(ofColor::white);
    std::string CCValue = "MIDI CC in:" + std::to_string(CCin);
    ofDrawBitmapString(CCValue, 20, 65);
    
    std::string midiTempo = "MIDI Tempo:" + std::to_string(int(clock.getBpm()));
    ofDrawBitmapString(midiTempo, 150, 65);

    
    ofSetColor(ofColor::green);
    string progressText = "Status: " + aeStatus;
    ofDrawBitmapString(progressText, 20, 50);
    
    //button text
    ofSetColor(ofColor::white);
    ofDrawBitmapString("Make Dataset", 20, 515);
    ofDrawBitmapString("Train", 20, 565);
    ofDrawBitmapString("Open Corpus", 20, 465);
    //button triangles
    ofDrawTriangle(500, 520, 510, 500, 520, 520);
    ofDrawTriangle(500, 570, 510, 550, 520, 570);
    ofDrawRectangle(epochsButton);
    
//    if (mode == true) {
//        ofDrawBitmapString("M1", 420, 55);
//    }else{
//        ofDrawBitmapString("M0", 420, 55);
//    }
    
    std::string lossString = std::to_string(loss);
    ofDrawBitmapString("Loss: " + lossString, 150, 600);
    
    //drag buttons
    ofDrawRectangle(CCa);
    ofDrawRectangle(CCb);
    ofDrawRectangle(CCc);
    ofDrawRectangle(CCd);
    ofDrawRectangle(keyButton);
    ofDrawRectangle(scaleButton);
    
    //MIDI CC text
    ofSetColor(ofColor::black);
    std::string aCCValue = std::to_string(aCC);
    ofDrawBitmapString(aCCValue, 135, 265);
    
    std::string bCCValue = std::to_string(bCC);
    ofDrawBitmapString(bCCValue, 135, 295);
    
    std::string cCCValue = std::to_string(cCC);
    ofDrawBitmapString(cCCValue, 135, 325);
    
    std::string dCCValue = std::to_string(dCC);
    ofDrawBitmapString(dCCValue, 135, 355);
    
    std::string epochString = std::to_string(epochs);
    ofDrawBitmapString("Epochs: " + epochString, 150, 560);
    
    
    
    // Step Sequencer GUI
    
    if(currentStep==0){
    ofSetColor(ofColor::red);
    }else{
    ofSetColor(ofColor::black);
    }
    ofDrawRectangle(60, 200, 20, 20);
    
    if(currentStep==1){
    ofSetColor(ofColor::red);
    }else{
    ofSetColor(ofColor::black);
    }
    ofDrawRectangle(90, 200, 20, 20);
    

    
    if(currentStep==2){
    ofSetColor(ofColor::red);
    }else{
    ofSetColor(ofColor::black);
    }
    ofDrawRectangle(120, 200, 20, 20);
    
    
    if(currentStep==3){
    ofSetColor(ofColor::red);
    }else{
    ofSetColor(ofColor::black);
    }
    ofDrawRectangle(150, 200, 20, 20);
    
    if(currentStep==4){
    ofSetColor(ofColor::red);
    }else{
    ofSetColor(ofColor::black);
    }
    ofDrawRectangle(180, 200, 20, 20);
    
    if(currentStep==5){
    ofSetColor(ofColor::red);
    }else{
    ofSetColor(ofColor::black);
    }
    ofDrawRectangle(210, 200, 20, 20);
    
    if(currentStep==6){
    ofSetColor(ofColor::red);
    }else{
    ofSetColor(ofColor::black);
    }
    ofDrawRectangle(240, 200, 20, 20);
    
    if(currentStep==7){
    ofSetColor(ofColor::red);
    }else{
    ofSetColor(ofColor::black);
    }
    ofDrawRectangle(270, 200, 20, 20);
    
    if(currentStep==8){
    ofSetColor(ofColor::red);
    }else{
    ofSetColor(ofColor::black);
    }
    ofDrawRectangle(300, 200, 20, 20);
    
    if(currentStep==9){
    ofSetColor(ofColor::red);
    }else{
    ofSetColor(ofColor::black);
    }
    ofDrawRectangle(330, 200, 20, 20);
    
    if(currentStep==10){
    ofSetColor(ofColor::red);
    }else{
    ofSetColor(ofColor::black);
    }
    ofDrawRectangle(360, 200, 20, 20);
    
    if(currentStep==11){
    ofSetColor(ofColor::red);
    }else{
    ofSetColor(ofColor::black);
    }
    ofDrawRectangle(390, 200, 20, 20);
    
    if(currentStep==12){
    ofSetColor(ofColor::red);
    }else{
    ofSetColor(ofColor::black);
    }
    ofDrawRectangle(420, 200, 20, 20);
    
    if(currentStep==13){
    ofSetColor(ofColor::red);
    }else{
    ofSetColor(ofColor::black);
    }
    ofDrawRectangle(450, 200, 20, 20);
    
    if(currentStep==14){
    ofSetColor(ofColor::red);
    }else{
    ofSetColor(ofColor::black);
    }
    ofDrawRectangle(480, 200, 20, 20);
    
    if(currentStep==15){
    ofSetColor(ofColor::red);
    }else{
    ofSetColor(ofColor::black);
    }
    ofDrawRectangle(510, 200, 20, 20);
    
    if(step1==true){
        ofSetColor(ofColor::lightGoldenRodYellow);
    }else{ofSetColor(ofColor::grey);
        
    }
    ofDrawCircle(70, 210, 5);
    
    if(step2==true){
        ofSetColor(ofColor::lightGoldenRodYellow);
    }else{ofSetColor(ofColor::grey);
        
    }
    ofDrawCircle(100, 210, 5);
    
    if(step3==true){
        ofSetColor(ofColor::lightGoldenRodYellow);
    }else{ofSetColor(ofColor::grey);
        
    }
    ofDrawCircle(130, 210, 5);
    
    if(step4==true){
        ofSetColor(ofColor::lightGoldenRodYellow);
    }else{ofSetColor(ofColor::grey);
        
    }
    ofDrawCircle(160, 210, 5);
    
    if(step5==true){
        ofSetColor(ofColor::lightGoldenRodYellow);
    }else{ofSetColor(ofColor::grey);
        
    }
    ofDrawCircle(190, 210, 5);
    
    if(step6==true){
        ofSetColor(ofColor::lightGoldenRodYellow);
    }else{ofSetColor(ofColor::grey);
        
    }
    ofDrawCircle(220, 210, 5);
    
    if(step7==true){
        ofSetColor(ofColor::lightGoldenRodYellow);
    }else{ofSetColor(ofColor::grey);
        
    }
    ofDrawCircle(250, 210, 5);
    
    if(step8==true){
        ofSetColor(ofColor::lightGoldenRodYellow);
    }else{ofSetColor(ofColor::grey);
        
    }
    ofDrawCircle(280, 210, 5);
    
    if(step9==true){
        ofSetColor(ofColor::lightGoldenRodYellow);
    }else{ofSetColor(ofColor::grey);
        
    }
    ofDrawCircle(310, 210, 5);
    
    if(step10==true){
        ofSetColor(ofColor::lightGoldenRodYellow);
    }else{ofSetColor(ofColor::grey);
        
    }
    ofDrawCircle(340, 210, 5);
    
    if(step11==true){
        ofSetColor(ofColor::lightGoldenRodYellow);
    }else{ofSetColor(ofColor::grey);
        
    }
    ofDrawCircle(370, 210, 5);
    
    if(step12==true){
        ofSetColor(ofColor::lightGoldenRodYellow);
    }else{ofSetColor(ofColor::grey);
        
    }
    ofDrawCircle(400, 210, 5);
    
    if(step13==true){
        ofSetColor(ofColor::lightGoldenRodYellow);
    }else{ofSetColor(ofColor::grey);
        
    }
    ofDrawCircle(430, 210, 5);
    
    if(step14==true){
        ofSetColor(ofColor::lightGoldenRodYellow);
    }else{ofSetColor(ofColor::grey);
        
    }
    ofDrawCircle(460, 210, 5);
    
    if(step15==true){
        ofSetColor(ofColor::lightGoldenRodYellow);
    }else{ofSetColor(ofColor::grey);
        
    }
    ofDrawCircle(490, 210, 5);
    
    if(step16==true){
        ofSetColor(ofColor::lightGoldenRodYellow);
    }else{ofSetColor(ofColor::grey);
        
    }
    ofDrawCircle(520, 210, 5);
    
    
    // Key and scale
    
    ofSetColor(ofColor::black);
    
    if (key == 0) {
        
        ofDrawBitmapString("C", 500, 310);
    }
    if (key == 1) {
        
        ofDrawBitmapString("C#", 500, 310);
    }
    if (key == 2) {
        
        ofDrawBitmapString("D", 500, 310);
    }
    if (key == 3) {
        
        ofDrawBitmapString("D#", 500, 310);
    }
    if (key == 4) {
        
        ofDrawBitmapString("E", 500, 310);
    }
    if (key == 5) {
        
        ofDrawBitmapString("F", 500, 310);
    }
    if (key == 6) {
        
        ofDrawBitmapString("F#", 500, 310);
    }
    if (key == 7) {
        
        ofDrawBitmapString("G", 500, 310);
    }
    if (key == 8) {
        
        ofDrawBitmapString("G#", 500, 310);
    }
    if (key == 9) {
        
        ofDrawBitmapString("A", 500, 310);
    }
    if (key == 10) {
        
        ofDrawBitmapString("A#", 500, 310);
    }
    if (key == 11) {
        
        ofDrawBitmapString("B", 500, 310);
    }
    
    
    if(scale == 0) {
        
        ofDrawBitmapString("Chromatic", 500, 360);
    }
    
    if(scale == 1) {
        
        ofDrawBitmapString("Minor", 500, 360);
    }
    
    if(scale == 2) {
        
        ofDrawBitmapString("Major", 500, 360);
    }
    
    //Generate Button
    ofSetColor(ofColor::black);
    ofDrawRectangle(generateButton);
    ofSetColor(ofColor::white);
    ofDrawBitmapString("GENERATE", 20, 330);
    
    //Pitch Reset button
    ofSetColor(ofColor::black);
    ofDrawRectangle(pitchReset);
    ofSetColor(ofColor::white);
    ofDrawBitmapString("Note reset", 20, 380);
    

    
}

//--------------------------------------------------------------
void ofApp::exit() {

	// cleanup
	ofSoundStreamStop();
    
    PyRun_SimpleString("server.server_close()");
    
    // Finalize the Python interpreter
    Py_Finalize();
    
    saveToFile("midiCCmap.txt", aCC, bCC, cCC, dCC);
    
}

//--------------------------------------------------------------

void ofApp::mousePressed(int x, int y, int button){

    
    
//    if (playButton.inside(x,y)) {
//
//        if (playState == false){
//            pd.sendSymbol("playstate", "play");
//            playState = true;
//        }
//        else if (playState == true) {
//            pd.sendSymbol("playstate", "stop");
//            playState = false;
//
//        };
//
//    }
    
    if (openCorpus.inside(x, y)) {

        
        // Get the path to the 'corpus' folder inside the 'data' folder
        string corpusFolderPath = resourcesPath + "/corpus";

        string command = "open \"" + corpusFolderPath + "\"";
        system(command.c_str());
        
        
    }
    
    
    if (startButton.inside(x, y)) {

        aeStatus = "Making dataset...";
        // Get the path to the 'corpus' folder inside the 'data' folder
        string corpusFolderPath = resourcesPath + "/corpus";

        // List the files in the 'corpus' folder
        ofDirectory corpusDir(corpusFolderPath);
        corpusDir.allowExt("wav");
        corpusDir.listDir();


        PyRun_SimpleString("dataset = np.empty(32)");

        // Iterate through the files in the 'corpus' folder
        for (int i = 0; i < corpusDir.size(); i++) {
            ofLogNotice(corpusDir.getName(i));

            PyRun_SimpleString("onsets = []");
            PyRun_SimpleString("data = []");
            string filePath = corpusDir.getPath(i);
            const char* inputPath = filePath.c_str();
            uint_t samplerate = 0;
            uint_t win_s = 512; // window size
            uint_t hop_size = win_s / 2;
            uint_t n_frames = 0, read = 0;

            aubio_source_t * source = new_aubio_source(inputPath, samplerate, hop_size);

            if (samplerate == 0 ) samplerate = aubio_source_get_samplerate(source);


            fvec_t* in = new_fvec(hop_size);
            fvec_t * out = new_fvec (2); // output position

            // create onset object
            aubio_onset_t * o = new_aubio_onset("default", win_s, hop_size, samplerate);
            aubio_onset_set_threshold(o, 0.8);

            do {
                // put some fresh data in input vector
                aubio_source_do(source, in, &read);
                // execute onset
                aubio_onset_do(o,in,out);

                // do something with the onsets
                if (out->data[0] != 0) {
                    float onset = aubio_onset_get_last(o);
                    std::stringstream pyScript;
                    pyScript << "onset = " << onset << "\n";
                    pyScript << "dur = " << aubio_source_get_duration(source) << "\n";
                    PyRun_SimpleString(pyScript.str().c_str());
                    PyRun_SimpleString("onsets.append(onset)");


                }
                n_frames += read;
            } while ( read == hop_size );

            // clean up memory
            del_aubio_source(source);
            del_aubio_onset(o);
            del_fvec(in);
            del_fvec(out);
            beach:
            aubio_cleanup();

            data = resourcesPath + "/Process_corpus.py";

            const char* scriptPath = data.c_str();


            /// Run the Python script using PyRun_SimpleFile()
            FILE* file = fopen(scriptPath, "r");
            if (file == NULL) {
                fprintf(stderr, "Failed to open the script file\n");
                return 1;
            }
            int result = PyRun_SimpleFile(file, scriptPath);

            // Close the file
            fclose(file);

            // Check the result of running the script
            if (result != 0) {
                fprintf(stderr, "Failed to run the script\n");
                return 1;

            }


        }
        PyRun_SimpleString("dataset=np.delete(dataset,0, axis=0)");
        PyRun_SimpleString("print(dataset.shape)");
        PyRun_SimpleString("np.savetxt('dataset.csv', dataset, delimiter=',')");
        aeStatus = "Dataset Ready";
}
    
    if (trainButton.inside(x, y)) {
        
        //pd.sendBang("train");
        std::stringstream pyScript;
        pyScript << "epochs = " << epochs << "\n";
        PyRun_SimpleString(pyScript.str().c_str());
        
        
        data = resourcesPath + "/AE_train.py";
        
        const char* scriptPath = data.c_str();
        
        
        /// Run the Python script using PyRun_SimpleFile()
        FILE* file = fopen(scriptPath, "r");
        if (file == NULL) {
            fprintf(stderr, "Failed to open the script file\n");
            return 1;
        }
        int result = PyRun_SimpleFile(file, scriptPath);
        
        // Close the file
        fclose(file);
        
        // Check the result of running the script
        if (result != 0) {
            fprintf(stderr, "Failed to run the script\n");
            return 1;
        }
        
        aeStatus = "Model trained - Ready";
        
    }
    
    if (midiInUp.inside(x, y)) {
        float increase = (midiIn.getPort() + 1) % midiIn.getNumInPorts();
        
        midiIn.openPort(increase);
    }
    
    if(midiOutCh.inside(x,y)) {
        channel = (channel + 1) % 17;
    }
    
    if (midiOutUp.inside(x, y)) {
        float increase = (midiOut.getPort() + 1) % midiOut.getNumOutPorts();
        midiOut.openPort(increase);
    }
    
    if (scaleButton.inside(x, y)) {
        
        scale = (scale + 1) % 3;
        
    }
    
    
    aIsPressed = CCa.inside(x, y);
    bIsPressed = CCb.inside(x, y);
    cIsPressed = CCc.inside(x, y);
    dIsPressed = CCd.inside(x, y);
    
    epochsIsPressed = epochsButton.inside(x,y);
    keyIsPressed = keyButton.inside(x, y);
    
    if (generateButton.inside(x, y)){
        latentA.setPercent(ofRandom(1));
        latentB.setPercent(ofRandom(1));
        latentC.setPercent(ofRandom(1));
        latentD.setPercent(ofRandom(1));
    }
    
    if (pitchReset.inside(x, y)){
        note1.setPercent(0.5);
        note2.setPercent(0.5);
        note3.setPercent(0.5);
        note4.setPercent(0.5);
        note5.setPercent(0.5);
        note6.setPercent(0.5);
        note7.setPercent(0.5);
        note8.setPercent(0.5);
        note9.setPercent(0.5);
        note10.setPercent(0.5);
        note11.setPercent(0.5);
        note12.setPercent(0.5);
        note13.setPercent(0.5);
        note14.setPercent(0.5);
        note15.setPercent(0.5);
        note16.setPercent(0.5);

        
        
        
    }
    
    
}
        

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
    if (aIsPressed) {
        
        aCC = ofMap(y, 0, ofGetHeight(), 127, 0);
    }
    
    if (bIsPressed) {
        
        bCC = ofMap(y, 0, ofGetHeight(), 127, 0);
    }
    
    if (cIsPressed) {
        
        cCC = ofMap(y, 0, ofGetHeight(), 127, 0);
    }
    
    if (dIsPressed) {
        
        dCC = ofMap(y, 0, ofGetHeight(), 127, 0);
    }
    
    if (epochsIsPressed) {
        
        epochs = y % 2000;
        
    }
    
    if (keyIsPressed) {
        
        key = y % 12;
        
    }
    
}



//--------------------------------------------------------------
void ofApp::keyPressed (int key) {
    
//    if (key == ' ') {
//
//        if (playState == false){
//            pd.sendSymbol("playstate", "play");
//            playState = true;
//        }
//        else if (playState == true) {
//            pd.sendSymbol("playstate", "stop");
//            playState = false;
//
//        }
//    }
//
//    if (key == 'm'){
//        if(mode == false){
//            mode = true;
//        }else{
//            mode = false;
//        }
//
//    }
//
    
}

//--------------------------------------------------------------
void ofApp::audioReceived(float * input, int bufferSize, int nChannels) {
	pd.audioIn(input, bufferSize, nChannels);
}

//--------------------------------------------------------------
void ofApp::audioRequested(float * output, int bufferSize, int nChannels) {
	pd.audioOut(output, bufferSize, nChannels);
}

//--------------------------------------------------------------
void ofApp::print(const std::string &message) {
	std::cout << message << std::endl;
}

//--------------------------------------------------------------
void ofApp::receiveBang(const std::string &dest) {
	std::cout << "OF: bang " << dest << std::endl;
}

void ofApp::receiveFloat(const std::string &dest, float value) {
    
    if (dest == "toOF"){
        
        currentStep = value;
    }
    
    if (dest == "loss"){
        
        loss = value;
    }
    
	//std::cout << "OF: float " << dest << ": " << value << std::endl;
}

void ofApp::receiveSymbol(const std::string &dest, const std::string &symbol) {
	std::cout << "OF: symbol " << dest << ": " << symbol << std::endl;
}

void ofApp::receiveList(const std::string &dest, const pd::List &list) {
    if(list.getFloat(0) == 1){
        step1 = true;
    }else{
        step1=false;
    }
    if(list.getFloat(1) == 1){
        step2 = true;
    }else{
        step2=false;
    }
    if(list.getFloat(2) == 1){
        step3 = true;
    }else{
        step3=false;
    }
    if(list.getFloat(3) == 1){
        step4 = true;
    }else{
        step4=false;
    }
    if(list.getFloat(4) == 1){
        step5 = true;
    }else{
        step5=false;
    }
    if(list.getFloat(5) == 1){
        step6 = true;
    }else{
        step6=false;
    }
    if(list.getFloat(6) == 1){
        step7 = true;
    }else{
        step7=false;
    }
    if(list.getFloat(7) == 1){
        step8 = true;
    }else{
        step8=false;
    }
    if(list.getFloat(8) == 1){
        step9 = true;
    }else{
        step9=false;
    }
    if(list.getFloat(9) == 1){
        step10 = true;
    }else{
        step10=false;
    }
    if(list.getFloat(10) == 1){
        step11 = true;
    }else{
        step11=false;
    }
    if(list.getFloat(11) == 1){
        step12 = true;
    }else{
        step12=false;
    }
    if(list.getFloat(12) == 1){
        step13 = true;
    }else{
        step13=false;
    }
    if(list.getFloat(13) == 1){
        step14 = true;
    }else{
        step14=false;
    }
    if(list.getFloat(14) == 1){
        step15 = true;
    }else{
        step15=false;
    }
    if(list.getFloat(15) == 1){
        step16 = true;
    }else{
        step16=false;
    }
    
}

void ofApp::receiveMessage(const std::string&dest, const std::string &msg, const pd::List &list) {
    
	std::cout << "OF: message " << dest << ": " << msg << " " << list.toString() << list.types() << std::endl;
}

//--------------------------------------------------------------
void ofApp::receiveNoteOn(const int channel, const int pitch, const int velocity) {
    
    midiOut.sendNoteOn(channel, pitch,  velocity);
	//std::cout << "OF MIDI: note on: " << channel << " " << pitch << " " << velocity << std::endl;
}

void ofApp::receiveControlChange(const int channel, const int controller, const int value) {
	std::cout << "OF MIDI: control change: " << channel << " " << controller << " " << value << std::endl;
}

// note: pgm nums are 1-128 to match pd
void ofApp::receiveProgramChange(const int channel, const int value) {
	std::cout << "OF MIDI: program change: " << channel << " " << value << std::endl;
}

void ofApp::receivePitchBend(const int channel, const int value) {
	std::cout << "OF MIDI: pitch bend: " << channel << " " << value << std::endl;
}

void ofApp::receiveAftertouch(const int channel, const int value) {
	std::cout << "OF MIDI: aftertouch: " << channel << " " << value << std::endl;
}

void ofApp::receivePolyAftertouch(const int channel, const int pitch, const int value) {
	std::cout << "OF MIDI: poly aftertouch: " << channel << " " << pitch << " " << value << std::endl;
}


//--------------------------------------------------------------
void ofApp::newMidiMessage(ofxMidiMessage& message) {
    
    if(message.status == MIDI_CONTROL_CHANGE) {
        
        CCin = message.control;
        //std::cout << message.control << message.value << std::endl;
        
        if (message.control == aCC) {
            
            float aSend = message.value;
            
            latentA.setPercent(aSend / 127);
            
        }
        
        if (message.control == bCC) {
            
            float bSend = message.value;
            
            latentB.setPercent(bSend / 127);
            
        }
        
        
        if (message.control == cCC) {
            
            float cSend = message.value;
            
            latentC.setPercent(cSend / 127);
            
        }
        
        if (message.control == dCC) {
            
            float dSend = message.value;
            
            latentD.setPercent(dSend / 127);
            
        }
        
        
    }
    
    // MIDI CLOCK
    
    pd.sendFloat("tempo", clock.getBpm());

    // update the clock
    if(clock.update(message.bytes)) {

        // we got a new song pos
        pd.sendMidiByte(0, 248);

    }

    // compute the seconds and bpm
    switch(message.status) {

        // compute seconds and bpm live, you may or may not always need this
        // which is why it is not integrated into the ofxMidiClock parser class
        case MIDI_TIME_CLOCK:
            seconds = clock.getSeconds();
            bpm += (clock.getBpm() - bpm) / 5; // average the last 5 bpm values
            // no break here so the next case statement is checked,
            // this way we can set clockRunning if we've missed a MIDI_START
            // ie. master was running before we started this example

        // transport control
        case MIDI_START: case MIDI_CONTINUE:
            if(!clockRunning) {
                clockRunning = true;
                pd.sendFloat("tempo", clock.getBpm());
                pd.sendSymbol("playstate", "play");
                playState = true;
                
            }
            break;
        case MIDI_STOP:
            if(clockRunning) {
                clockRunning = false;
                pd.sendSymbol("playstate", "stop");
                playState = false;
            }
            break;

        default:
            break;
    }
}
void ofApp::receiveMidiByte(const int port, const int byte) {
    midiOut.sendMidiByte(byte);
}


// Function to save variables to a text file
void ofApp::saveToFile(const std::string& filename, int variable1, int variable2, int variable3, int variable4) {
    std::ofstream outputFile(filename);

    if (outputFile.is_open()) {
        outputFile << variable1 << "\n";
        outputFile << variable2 << "\n";
        outputFile << variable3 << "\n";
        outputFile << variable4 << "\n";

        outputFile.close();
        std::cout << "Variables saved to file: " << filename << std::endl;
    } else {
        std::cerr << "Error opening file for writing: " << filename << std::endl;
    }
}

// Function to load variables from a text file
bool ofApp::loadFromFile(const std::string& filename) {
    std::ifstream inputFile(filename);

    if (inputFile.is_open()) {
        inputFile >> aCC;
        inputFile >> bCC;
        inputFile >> cCC;
        inputFile >> dCC;

        inputFile.close();
        std::cout << "Variables loaded from file: " << filename << std::endl;
        return true;
    } else {
        std::cerr << "Error opening file for reading: " << filename << std::endl;
        return false;
    }
}
