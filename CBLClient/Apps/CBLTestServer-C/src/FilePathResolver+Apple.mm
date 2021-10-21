//
//  FilePathResolver+Apple.m
//  TestServer
//
//  Created by Jim Borden on 7/2/21.
//

#import "FilePathResolver.h"
#import <Foundation/Foundation.h>
using namespace std;

namespace file_resolution {
    string resolve_path(const string& relativePath, bool unzip) {
        NSString* nsRelative = [NSString stringWithUTF8String:relativePath.c_str()];
        NSString* extension = [nsRelative pathExtension];
        nsRelative = [nsRelative stringByDeletingPathExtension];
        NSString* finalPath = [[NSBundle mainBundle] pathForResource:nsRelative ofType:extension];
        if(!finalPath) {
            throw domain_error("Unable to find file in main bundle!");
        }
        
        return [finalPath cStringUsingEncoding:NSUTF8StringEncoding];
    }
}
