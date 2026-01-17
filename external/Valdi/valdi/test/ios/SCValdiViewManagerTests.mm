//
//  SCValdiViewManagerTests.m
//  ios_tests
//
//  Created by Edward Lee on 09/05/24.
//

#import <Foundation/Foundation.h>
#import <XCTest/XCTest.h>
#import "valdi/ios/CPPBindings/SCValdiViewManager.h"
#import "valdi/ios/Text/SCValdiFontManager.h"

@interface MockExceptionReporter : NSObject <SCValdiExceptionReporter>
@property (nonatomic, strong) NSString *reportedMessage;
@property (nonatomic, strong) NSString *reportedModule;
@property (nonatomic, strong) NSString *reportedStackTrace;
@property (nonatomic, assign) NSInteger reportedErrorCode;
@property (nonatomic, assign) BOOL reportedIsANR;
@end

@implementation MockExceptionReporter

- (void)reportNonFatalWithErrorCode:(NSInteger)errorCode message:(NSString *)message module:(NSString *)module stackTrace:(NSString *)stackTrace {
    self.reportedErrorCode = errorCode;
    self.reportedMessage = message;
    self.reportedModule = module;
    self.reportedStackTrace = stackTrace;
}

- (void)reportCrashWithMessage:(NSString *)message module:(NSString *)module stackTrace:(NSString *)stackTrace isANR:(BOOL)isANR
{
    self.reportedMessage = message;
    self.reportedModule = module;
    self.reportedStackTrace = stackTrace;
    self.reportedIsANR = isANR;
}

@end

@interface SCValdiViewManagerTests: XCTestCase

@property (nonatomic, assign) ValdiIOS::ViewManager *viewManager;
@property (nonatomic, strong) MockExceptionReporter *mockExceptionReporter;

@end

@implementation SCValdiViewManagerTests

- (void)setUp {
    [super setUp];
    self.viewManager = new ValdiIOS::ViewManager([SCValdiFontManager new]); // Initialize C++ object
    self.mockExceptionReporter = [[MockExceptionReporter alloc] init];
}

- (void)tearDown {
    delete self.viewManager; // Clean up C++ object
    self.viewManager = nullptr;
    self.mockExceptionReporter = nil;
    [super tearDown];
}

- (void)testOnUncaughtJsError {
    self.viewManager->setExceptionReporter(self.mockExceptionReporter);
    
    int32_t errorCode = 404;
    Valdi::StringBox moduleName = STRING_LITERAL("TestModule");
    std::string errorMessage = "Test error message";
    std::string stackTrace = "Test stack trace";
    
    self.viewManager->onUncaughtJsError(errorCode, moduleName, errorMessage, stackTrace);
    
    XCTAssertEqual(self.mockExceptionReporter.reportedErrorCode, errorCode);
    XCTAssertEqualObjects(self.mockExceptionReporter.reportedMessage, @"Test error message");
    XCTAssertEqualObjects(self.mockExceptionReporter.reportedModule, @"TestModule");
    XCTAssertEqualObjects(self.mockExceptionReporter.reportedStackTrace, @"Test stack trace");
}
-(void)testOnJsCrash {
    self.viewManager->setExceptionReporter(self.mockExceptionReporter);
    
    Valdi::StringBox moduleName = STRING_LITERAL("TestModule");
    std::string errorMessage = "Test error message";
    std::string stackTrace = "Test stack trace";
    
    self.viewManager->onJsCrash(moduleName, errorMessage, stackTrace, true);

    XCTAssertEqualObjects(self.mockExceptionReporter.reportedMessage, @"Test error message");
    XCTAssertEqualObjects(self.mockExceptionReporter.reportedModule, @"TestModule");
    XCTAssertEqualObjects(self.mockExceptionReporter.reportedStackTrace, @"Test stack trace");
    XCTAssertTrue(self.mockExceptionReporter.reportedIsANR);
}

@end
