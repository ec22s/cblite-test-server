//
//  CBLTestServerBridge.m
//  TestServer
//
//  Created by Jim Borden on 5/20/21.
//

#import "CBLTestServerBridge.h"

#include <civetweb.h>
#include "TestServer.h"
#include "Defines.h"
#include INCLUDE_CBL(CouchbaseLite.h)

@implementation CBLTestServerBridge

+(void)startTestServer {
    mg_init_library(0);

    TestServer server{};
    server.start();
#ifdef COUCHBASE_ENTERPRISE
    NSLog(@"Using CBL C version %s-%d (Enterprise)", CBLITE_VERSION, CBLITE_BUILD_NUMBER);
#else
    NSLog(@"Using CBL C version %s-%d", CBLITE_VERSION, CBLITE_BUILD_NUMBER);
#endif
    
    NSLog(@"%@", [NSString stringWithFormat:@"Listening on port %u...", TestServer::PORT]);
}

@end
