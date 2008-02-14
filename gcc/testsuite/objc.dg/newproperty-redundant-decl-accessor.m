/* APPLE LOCAL file radar 4897159 */
/* Test that a redundant user declaration of a setter which is later to be synthesize 
   does not ICE. */
/* { dg-options "-fobjc-new-property -mmacosx-version-min=10.5 -framework Foundation -framework CoreFoundation" } */
/* { dg-do run } */
#include <Foundation/Foundation.h>

@interface MyClass : NSObject {
    NSString	*_myName;
}

@property(copy) NSString *myName;

- (NSString *)myName;
- (void)setMyName:(NSString *)name;

@end

@implementation MyClass

@synthesize myName = _myName;

@end

int main(int argc, const char *argv[], const char *envp[]) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool allocWithZone:NULL] init];
    MyClass *myObject = [[MyClass allocWithZone:NULL] init];

    [myObject setMyName:@"Foo"];

    [pool release];

    exit(EXIT_SUCCESS);
    return 0;
}
