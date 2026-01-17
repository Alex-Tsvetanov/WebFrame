//
//  SCValdiDefaultImageLoader.m
//  ios
//
//  Created by Simon Corsin on 5/7/20.
//

#import "SCValdiDefaultImageLoader.h"
#import "valdi_core/SCMacros.h"

@interface SCValdiDefaultImageLoadTask: NSObject<SCValdiCancelable>

@property (readonly, strong, nonatomic) NSURL *imageURL;
@property (readonly, nonatomic) SCValdiImageLoaderCompletion completion;
@property (readonly, nonatomic) SCValdiImageLoaderBytesCompletion bytesCompletion;

@end

@implementation SCValdiDefaultImageLoadTask {
    NSURLSessionTask *_sessionTask;
}

- (instancetype)initWithURL:(NSURL *)imageURL
                 completion:(SCValdiImageLoaderCompletion)completion
{
    self = [super init];

    if (self) {
        _imageURL = imageURL;
        _completion = completion;
    }

    return self;
}

- (instancetype)initWithURL:(NSURL *)imageURL
            bytesCompletion:(SCValdiImageLoaderBytesCompletion)completion
{
    self = [super init];

    if (self) {
        _imageURL = imageURL;
        _bytesCompletion = completion;
    }

    return self;
}

- (void)cancel
{
    @synchronized (self) {
        _completion = nil;

        if (_sessionTask) {
            [_sessionTask cancel];
            _sessionTask = nil;
        }
    }
}

- (void)notifyCompletionWithImage:(SCValdiImage *)image error:(NSError *)error
{
    SCValdiImageLoaderCompletion completion;
    SCValdiImageLoaderBytesCompletion bytesCompletion;

    @synchronized (self) {
        completion = _completion;
        _completion = nil;
        bytesCompletion = _bytesCompletion;
        _bytesCompletion = nil;
    }
    
    if (completion) {
        completion(image, error);
    }
    if (bytesCompletion) {
        bytesCompletion([image toPNG], error);
    }
}

- (void)startSessionTask:(NSURLSessionTask *)sessionTask
{
    @synchronized (self) {
        if (!_completion) {
            return;
        }

        _sessionTask = sessionTask;
        [sessionTask resume];
    }
}

@end

@interface SCValdiDefaultImageLoader()  <NSCacheDelegate>
@end

@implementation SCValdiDefaultImageLoader {
    dispatch_queue_t _queue;
    NSCache *_cache;
}

- (instancetype)init
{
    self = [super init];

    if (self) {
        _queue = dispatch_queue_create("com.snap.valdi.DefaultImageLoader", dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INITIATED, 0));
        _cache = [NSCache new];
        _cache.delegate = self;
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_clearCache) name:UIApplicationDidReceiveMemoryWarningNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_clearCache) name:UIApplicationWillResignActiveNotification object:nil];
    }

    return self;
}

- (void)_clearCache
{
    dispatch_async(_queue, ^{
        [self->_cache removeAllObjects];
    });
}

- (void)_completeTask:(SCValdiDefaultImageLoadTask *)task withImage:(SCValdiImage *)image error:(NSError *)error
{
    if (!image && !error) {
        error = [NSError errorWithDomain:@"" code:0 userInfo:@{NSLocalizedFailureReasonErrorKey: @"Cannot load image"}];
    }

    if (image) {
        [_cache setObject:image forKey:task.imageURL];
    }

    [task notifyCompletionWithImage:image error:error];
}

- (SCValdiImage *)_cachedImageForURL:(NSURL *)url
{
    return [_cache objectForKey:url];
}

- (void)_doLoadWithTask:(SCValdiDefaultImageLoadTask *)task
{
    SCValdiImage *image = [self _cachedImageForURL:task.imageURL];
    if (image) {
        [task notifyCompletionWithImage:image error:nil];
        return;
    }

    NSURL *imageURL = task.imageURL;
    if ([imageURL.scheme isEqualToString:@"file"]) {
        NSError *error = nil;
        SCValdiImage *image = [SCValdiImage imageWithFilePath:imageURL.path error:&error];
        [self _completeTask:task withImage:image error:error];

        return;
    }

    if ([imageURL.scheme isEqualToString:@"valdi-res"]) {
        NSString *moduleName = imageURL.host;
        NSString *path = imageURL.path;
        if ([path hasPrefix:@"/"]) {
            path = [path substringFromIndex:1];
        }

        SCValdiImage *image = [SCValdiImage imageWithModuleName:moduleName resourcePath:path];

        [self _completeTask:task withImage:image error:nil];

        return;
    }

    NSMutableURLRequest *urlRequest = [[NSMutableURLRequest alloc] initWithURL:task.imageURL];
    urlRequest.HTTPMethod = @"GET";

    NSURLSessionTask *urlTask = [NSURLSession.sharedSession dataTaskWithRequest:urlRequest
                                                              completionHandler:^(NSData * _Nullable data, NSURLResponse * _Nullable response, NSError * _Nullable error) {
        dispatch_async(self->_queue, ^{
            if (error) {
                [self _completeTask:task withImage:nil error:error];
            } else {
                NSError *imageDataError = nil;
                SCValdiImage *image = [SCValdiImage imageWithData:data error:&imageDataError];

                [self _completeTask:task withImage:image error:imageDataError];
            }
        });
    }];

    [task startSessionTask:urlTask];
}

- (NSArray<NSString *> *)supportedURLSchemes
{
    return @[@"file", @"http", @"https", @"valdi-res"];
}

- (id)requestPayloadWithURL:(NSURL *)imageURL
                      error:(NSError *__autoreleasing  _Nullable *)error
{
    return imageURL;
}

- (id<SCValdiCancelable>)loadImageWithRequestPayload:(id)requestPayload
                                             parameters:(SCValdiAssetRequestParameters)parameters
                                             completion:(SCValdiImageLoaderCompletion)completion
{
    NSURL *imageURL = (NSURL *)requestPayload;
    SCValdiImage *image = [self _cachedImageForURL:imageURL];

    if (image) {
        completion(image, nil);
        return nil;
    }

    SCValdiDefaultImageLoadTask *task = [[SCValdiDefaultImageLoadTask alloc] initWithURL:imageURL completion:completion];

    dispatch_async(_queue, ^{
        [self _doLoadWithTask:task];
    });

    return task;
}

- (id<SCValdiCancelable>)loadBytesWithRequestPayload:(id)requestPayload
                                             completion:(SCValdiImageLoaderBytesCompletion)completion
{
    NSURL *imageURL = (NSURL *)requestPayload;
    SCValdiImage *image = [self _cachedImageForURL:imageURL];

    if (image) {
        completion([image toPNG], nil);
        return nil;
    }

    SCValdiDefaultImageLoadTask *task = [[SCValdiDefaultImageLoadTask alloc] initWithURL:imageURL bytesCompletion:completion];

    dispatch_async(_queue, ^{
        [self _doLoadWithTask:task];
    });

    return task;
}

@end
