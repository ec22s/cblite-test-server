//
//  CBLTestServerBridge.m
//  TestServer
//
//  Created by Jim Borden on 5/20/21.
//

#import "CBLTestServerBridge.h"

#include <civetweb.h>
#include "TestServer.h"

@implementation CBLTestServerBridge

+(void)startTestServer {
    mg_init_library(0);

    TestServer server{};
    server.start();
    NSLog(@"%@", [NSString stringWithFormat:@"Listening on port %u...", TestServer::PORT]);
}

@end
