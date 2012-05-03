#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
	
	ofSetVerticalSync(false);
	sharedData = NULL;

	// open shared memory with address '/ipc_test', readwrite, with a+rw permissions
	int mm_fd = shm_open( SHM_NAME, O_RDWR, NULL );
	if ( mm_fd < 0 )
	{
		ofLogError("Client","shm_open failed: err "+ofToString(errno)+" "+strerror(errno));
	}
	else
	{
		// attempt to memory map mm_fd 
		sharedData = (IPCTestStruct*)mmap(NULL, sizeof(IPCTestStruct), PROT_READ|PROT_WRITE, MAP_SHARED, mm_fd, 0);
		if ( sharedData == NULL )
		{
			ofLogError("Client","mmap failed: err "+ofToString(errno)+" "+strerror(errno));
			close( mm_fd );
			mm_fd =0;
		}
	}
	
	lastReadSequenceId = sharedData->readSequenceId;

}

//--------------------------------------------------------------
void testApp::update(){

	
	
}

//--------------------------------------------------------------
void testApp::draw(){

	if ( sharedData )
	{
		ofSetColor( ofColor::white );
		for ( int i=0; i<4; i++ )
		{	
			ofDrawBitmapString( ofToString(i) + ": " + ofToString( sharedData->inputs[i], 15, ' ' ), ofPoint( 10, 10+14*i ) );
		}
		
	}
	
	int nextReadSequenceId = sharedData->readSequenceId;
	if ( nextReadSequenceId > lastReadSequenceId+1 )
	{
		ofLogWarning( "Client", "dropped "+ofToString( nextReadSequenceId-(lastReadSequenceId+1) )+" reads" );
	}
	lastReadSequenceId = nextReadSequenceId;
	
}

//--------------------------------------------------------------
void testApp::exit()
{
	// cleanup
	if ( sharedData )
		munmap( sharedData, sizeof(IPCTestStruct ));
	sharedData = NULL;
	if ( mm_fd )
		close(mm_fd);

}

//--------------------------------------------------------------
void testApp::keyPressed(int key){

	if ( key == 'r' )
	{
		if ( sharedData )
			sharedData->bShouldRead = true;
	}
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}