#import <Cocoa/Cocoa.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Launch a Cocoa application backed by Valdi.
 * It will the given root component path as the root.
 */
void SCValdiAppMain(int argc,
                    const char** argv,
                    NSString* componentPath,
                    NSString* title,
                    int windowWidth,
                    int windowHeight,
                    bool windowResizable);
#ifdef __cplusplus
}
#endif
