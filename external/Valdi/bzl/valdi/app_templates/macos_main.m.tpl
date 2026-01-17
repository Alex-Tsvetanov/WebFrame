#import "valdi/macos/Bootstrap/SCValdiAppMain.h"

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        SCValdiAppMain(argc,
                       argv,
                       @"@VALDI_ROOT_COMPONENT_PATH@",
                       @"@VALDI_TITLE@",
                       @VALDI_WINDOW_WIDTH@,
                       @VALDI_WINDOW_HEIGHT@,
                       @VALDI_WINDOW_RESIZABLE@
                    );
    }
    return 0;
}