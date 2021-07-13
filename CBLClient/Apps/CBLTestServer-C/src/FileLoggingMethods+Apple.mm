//
//  FileLoggingMethods+Apple.mm
//  TestServer
//
//  Created by Jim Borden on 7/2/21.
//

#import "FilePathResolver.h"
#import <Foundation/Foundation.h>
using namespace std;

string LogTempDirectory() {
#if !TARGET_OS_OSX
    NSError* err;
    NSURL* cacheDir = [[NSFileManager defaultManager] URLForDirectory:NSCachesDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:NO error:&err];
    if(!cacheDir) {
        throw domain_error("Failed to get directory from NSFileManager");
    }

    return string([cacheDir.path cStringUsingEncoding:NSUTF8StringEncoding]);
#else
    return "/tmp";
#endif
}
