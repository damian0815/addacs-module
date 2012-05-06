//
//  IPCTestStruct.h
//  ipc-test-daemon
//
//  Created by Damian Stewart on 03.05.12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef ipc_test_daemon_IPCTestStruct_h
#define ipc_test_daemon_IPCTestStruct_h

#include <stdint.h>

static const char* SHM_NAME = "/ipc_test";


typedef struct _IPCTestStruct
{
	
	unsigned char bShouldRead; // BOOL
	unsigned char bShouldWrite; // BOOL
	uint16_t inputs[4];
	uint16_t outputs[4];
	uint32_t readSequenceId;
	
} IPCTestStruct;





#endif
