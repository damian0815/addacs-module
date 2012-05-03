//
//  IPCTestStruct.h
//  ipc-test-daemon
//
//  Created by Damian Stewart on 03.05.12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef ipc_test_daemon_IPCTestStruct_h
#define ipc_test_daemon_IPCTestStruct_h


static const char* SHM_NAME = "/ipc_test";


typedef struct _IPCTestStruct
{
	
	int bShouldRead; // BOOL
	int bShouldWrite; // BOOL
	int inputs[4];
	int outputs[4];
	int readSequenceId;
	
} IPCTestStruct;





#endif
