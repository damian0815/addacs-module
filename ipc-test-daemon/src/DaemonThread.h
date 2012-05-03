//
//  DaemonThread.h
//  ipc-test-daemon
//
//  Created by Damian Stewart on 03.05.12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#pragma once
#include "ofMain.h"
#include "IPCTestStruct.h"

class DaemonThread : public ofThread
{
public:
	DaemonThread() : sharedData(NULL) {};
	
	void draw(); // must happen on main thread
	
private:
	void threadedFunction();

	IPCTestStruct* sharedData;
	
	int readCount;
};

