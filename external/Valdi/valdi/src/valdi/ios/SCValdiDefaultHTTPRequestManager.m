//
//  SCValdiDefaultHTTPRequestManager.m
//  valdi-ios
//
//  Created by Simon Corsin on 8/27/19.
//

#import "valdi/ios/SCValdiDefaultHTTPRequestManager.h"

#import "valdi_core/SCMacros.h"
#import "valdi_core/SCValdiFunction.h"
#import "valdi_core/SCNValdiCoreHTTPRequestManagerCompletion.h"
#import "valdi_core/SCNValdiCoreCancelable.h"

#import <Foundation/Foundation.h>

@interface SCValdiURLSessionTaskCancelable: NSObject<SCNValdiCoreCancelable>

@end

@implementation SCValdiURLSessionTaskCancelable {
    NSURLSessionTask *_task;
}

- (instancetype)initWithTask:(NSURLSessionTask *)task
{
    self = [super init];

    if (self) {
        _task = task;
    }

    return self;
}

- (void)cancel
{
    [_task cancel];
    _task = nil;
}

@end

@implementation SCValdiDefaultHTTPRequestManager

- (id<SCNValdiCoreCancelable>)performRequest:(SCNValdiCoreHTTPRequest *)request completion:(SCNValdiCoreHTTPRequestManagerCompletion *)completion
{
    NSURL *url = [NSURL URLWithString:request.url];

    NSMutableURLRequest *urlRequest = [[NSMutableURLRequest alloc] initWithURL:url];
    urlRequest.HTTPMethod = request.method;
    urlRequest.HTTPBody = request.body;

    NSDictionary *headers = ObjectAs(request.headers, NSDictionary);
    for (NSString *headerField in headers) {
        NSString *headerValue = ObjectAs(headers[headerField], NSString);
        if (headerValue) {
            [urlRequest setValue:headerValue forHTTPHeaderField:headerField];
        }
    }

    NSURLSessionTask *task = [NSURLSession.sharedSession dataTaskWithRequest:urlRequest
                                                           completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
                                                               if (error) {
                                                                   if (error.code == NSURLErrorCancelled) {
                                                                       return;
                                                                   }

                                                                   [completion onFail:error.localizedDescription];
                                                               } else {
                                                                   NSHTTPURLResponse *urlResponse = ObjectAs(response, NSHTTPURLResponse);

                                                                   [completion onComplete:[[SCNValdiCoreHTTPResponse alloc] initWithStatusCode:(int32_t)urlResponse.statusCode headers:urlResponse.allHeaderFields body:data]];
                                                               }
   }];

    [task resume];

    return [[SCValdiURLSessionTaskCancelable alloc] initWithTask:task];
}

@end
