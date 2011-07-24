#import <Cocoa/Cocoa.h>
BOOL showCwDw = NO;
BOOL showRequests = NO;
#ifdef DEBUG
BOOL showEmmDebug = YES;
#else
BOOL showEmmDebug = NO;
#endif

BOOL doEcmEmmRecording = NO;
BOOL isEmmEnable = NO;


int main(int argc, char *argv[])
{
    return NSApplicationMain(argc,  (const char **) argv);
}
