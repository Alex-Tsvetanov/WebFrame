//
//  SnapDrawingVideoEncoder.m
//  snap_drawing-demo
//
//  Created by Simon Corsin on 1/21/22.
//

#import "SnapDrawingVideoEncoder.h"
#import <AVFoundation/AVFoundation.h>

#define PIXEL_FORMAT kCVPixelFormatType_32BGRA

@implementation SnapDrawingSampleBuffer {
    CMTime _presentationTimestamp;
    CMSampleBufferRef _sampleBufferRef;
    CVPixelBufferRef _pixelBufferRef;
}

- (instancetype)initWithSampleBufferRef:(CMSampleBufferRef)sampleBufferRef
{
    self = [super init];

    if (self) {
        _sampleBufferRef = sampleBufferRef;
        CFRetain(_sampleBufferRef);
        _pixelBufferRef = CMSampleBufferGetImageBuffer(_sampleBufferRef);
        if (_pixelBufferRef) {
            CVPixelBufferRetain(_pixelBufferRef);
        }

        _presentationTimestamp = CMSampleBufferGetPresentationTimeStamp(_sampleBufferRef);
    }

    return self;
}

- (instancetype)initWithPixelBufferRef:(CVPixelBufferRef)pixelBufferRef
                 presentationTimestamp:(CMTime)presentationTimestamp
{
    self = [super init];

    if (self) {
        _pixelBufferRef = pixelBufferRef;
        CVPixelBufferRetain(_pixelBufferRef);
        _presentationTimestamp = presentationTimestamp;
    }

    return self;
}

- (void)dealloc
{
    if (_pixelBufferRef) {
        CVPixelBufferRelease(_pixelBufferRef);
    }

    if (_sampleBufferRef) {
        CFRelease(_sampleBufferRef);
    }
}

- (BOOL)lockBytes:(BufferData *)bufferData
{
    if (!_pixelBufferRef) {
        return NO;
    }

    CVPixelBufferLockBaseAddress(_pixelBufferRef, 0);

    bufferData->bytes = CVPixelBufferGetBaseAddress(_pixelBufferRef);
    bufferData->bytesPerRow = CVPixelBufferGetBytesPerRow(_pixelBufferRef);
    bufferData->width = CVPixelBufferGetWidth(_pixelBufferRef);
    bufferData->height = CVPixelBufferGetHeight(_pixelBufferRef);

    return YES;
}

- (void)unlockBytes:(BufferData *)bufferData
{
    bufferData->bytes = nil;
    bufferData->bytesPerRow = 0;
    bufferData->width = 0;
    bufferData->height = 0;

    CVPixelBufferUnlockBaseAddress(_pixelBufferRef, 0);
}

- (CMSampleBufferRef)sampleBufferRef
{
    return _sampleBufferRef;
}

- (CVPixelBufferRef)pixelBufferREf
{
    return _pixelBufferRef;
}

- (double)timestamp
{
    return CMTimeGetSeconds(_presentationTimestamp);
}

- (CMTime)presentationTimestamp
{
    return _presentationTimestamp;
}

@end

@implementation SnapDrawingVideoDecoder {
    AVAsset *_asset;
    AVAssetReader *_assetReader;
    AVAssetReaderTrackOutput *_videoOutput;
    AVAssetTrack *_videoTrack;
}

- (instancetype)initWithPath:(NSString *)path
{
    self = [super init];

    if (self) {
        NSURL *url = [NSURL fileURLWithPath:path];
        _asset = [AVAsset assetWithURL:url];
    }

    return self;
}

- (void)dealloc
{
    [_assetReader cancelReading];
}

- (double)durationSeconds
{
    return CMTimeGetSeconds(_asset.duration);
}

- (NSInteger)videoWidth
{
    CGSize size = [_videoTrack naturalSize];
    return (NSInteger)size.width;
}

- (NSInteger)videoHeight
{
    CGSize size = [_videoTrack naturalSize];
    return (NSInteger)size.height;
}

- (BOOL)prepareWithError:(NSError * _Nullable *)error
{
    NSArray<AVAssetTrack *> *videoTracks = [_asset tracksWithMediaType:AVMediaTypeVideo];
    if (!videoTracks.count) {
        *error = [NSError errorWithDomain:@"" code:0 userInfo:@{NSLocalizedDescriptionKey: @"No Video track in media"}];
        return NO;
    }

    _videoTrack = videoTracks.firstObject;

    AVAssetReader *reader = [AVAssetReader assetReaderWithAsset:_asset error:error];
    if (!reader) {
        return NO;
    }

    AVAssetReaderTrackOutput *output = [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:_videoTrack outputSettings:@{
        (id)kCVPixelBufferPixelFormatTypeKey     : @(PIXEL_FORMAT),
        (id)kCVPixelBufferIOSurfacePropertiesKey : @{}
    }];

    if (![reader canAddOutput:output]) {
        *error = [NSError errorWithDomain:@"" code:0 userInfo:@{NSLocalizedDescriptionKey: @"Cannot add output"}];
        return NO;
    }
    [reader addOutput:output];

    if (![reader startReading]) {
        *error = reader.error;
        return NO;
    }

    _assetReader = reader;
    _videoOutput = output;

    return YES;
}

- (SnapDrawingSampleBuffer *)copyNextSampleBufferWithError:(NSError * _Nullable *)error
{
    CMSampleBufferRef sampleBufferRef = nil;

    while (!sampleBufferRef) {
        switch (_assetReader.status) {
            case AVAssetReaderStatusUnknown:
            case AVAssetReaderStatusReading: {
                sampleBufferRef = [_videoOutput copyNextSampleBuffer];
                break;
            case AVAssetReaderStatusCompleted:
                return nil;
            case AVAssetReaderStatusFailed:
            case AVAssetReaderStatusCancelled:
                *error = _assetReader.error;
                return nil;
            }
        }
    }

    SnapDrawingSampleBuffer *buffer = [[SnapDrawingSampleBuffer alloc] initWithSampleBufferRef:sampleBufferRef];
    CFRelease(sampleBufferRef);

    return buffer;
}

@end

@implementation SnapDrawingVideoEncoder {
    NSURL *_url;
    AVAssetWriter *_assetWriter;
    AVAssetWriterInput *_videoInput;
    NSInteger _videoWidth;
    NSInteger _videoHeight;
    AVAssetWriterInputPixelBufferAdaptor *_pixelBufferAdaptor;
}

- (instancetype)initWithPath:(NSString *)path
                  videoWidth:(NSInteger)videoWidth
                 videoHeight:(NSInteger)videoHeight
{
    self = [super init];

    if (self) {
        _url = [NSURL fileURLWithPath:path];
        _videoWidth = videoWidth;
        _videoHeight = videoHeight;

        if ([[NSFileManager defaultManager] fileExistsAtPath:_url.path]) {
            [[NSFileManager defaultManager] removeItemAtPath:_url.path error:nil];
        }
    }

    return self;
}

- (void)dealloc
{
}

- (BOOL)prepareWithError:(NSError *__autoreleasing  _Nullable *)error
{
    AVAssetWriter *writer = [AVAssetWriter assetWriterWithURL:_url fileType:AVFileTypeMPEG4 error:error];
    if (!writer) {
        return NO;
    }

    AVAssetWriterInput *videoInput = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeVideo outputSettings:@{
        AVVideoCodecKey : AVVideoCodecTypeH264,
        AVVideoScalingModeKey : AVVideoScalingModeResizeAspectFill,
        AVVideoWidthKey : @(_videoWidth),
        AVVideoHeightKey : @(_videoHeight),
        AVVideoCompressionPropertiesKey : @{
            AVVideoAverageBitRateKey: @(6000000),
        }
    }];

    if (![writer canAddInput:videoInput]) {
        *error = [NSError errorWithDomain:@"" code:0 userInfo:@{NSLocalizedDescriptionKey: @"Cannot add video output"}];
        return NO;
    }
    [writer addInput:videoInput];

    _pixelBufferAdaptor = [AVAssetWriterInputPixelBufferAdaptor assetWriterInputPixelBufferAdaptorWithAssetWriterInput:videoInput sourcePixelBufferAttributes:@{
        (id)kCVPixelBufferPixelFormatTypeKey : @(PIXEL_FORMAT),
        (id)kCVPixelBufferWidthKey : @(_videoWidth),
        (id)kCVPixelBufferHeightKey : @(_videoHeight),
    }];

    if (![writer startWriting]) {
        *error = writer.error;
        return NO;
    }

    [writer startSessionAtSourceTime:kCMTimeZero];

    _assetWriter = writer;
    _videoInput = videoInput;

    return YES;
}

- (BOOL)writeSampleBuffer:(SnapDrawingSampleBuffer *)sampleBuffer error:(NSError *__autoreleasing  _Nullable *)error
{
    while (!_videoInput.isReadyForMoreMediaData && _assetWriter.status == AVAssetWriterStatusWriting) {
        // Dirty hack to simplify implementation
        [NSThread sleepForTimeInterval:0.01];
    }

    if ([sampleBuffer sampleBufferRef]) {
        if (![_videoInput appendSampleBuffer:[sampleBuffer sampleBufferRef]]) {
            *error = _assetWriter.error;
            return NO;
        }
    } else {
        if (![_pixelBufferAdaptor appendPixelBuffer:[sampleBuffer pixelBufferREf] withPresentationTime:[sampleBuffer presentationTimestamp]]) {
            *error = _assetWriter.error;
            return NO;
        }
    }

    return YES;
}

- (void)finishWriting
{
    __block BOOL done = NO;
    [_assetWriter finishWritingWithCompletionHandler:^{
        done = YES;
    }];

    while (!done) {
        [[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
    }
}

- (SnapDrawingSampleBuffer *)dequeueOutputSampleBufferForTimestamp:(CMTime)timestamp error:(NSError **)error
{
    CVPixelBufferRef outputPixelBuffer = nil;
    CVReturn ret = CVPixelBufferPoolCreatePixelBuffer(NULL, [_pixelBufferAdaptor pixelBufferPool], &outputPixelBuffer);

    if (ret != kCVReturnSuccess) {
        *error = [NSError errorWithDomain:@"" code:0 userInfo:@{NSLocalizedDescriptionKey: @"Cannot dequeue output sample buffer"}];
        return nil;
    }

    SnapDrawingSampleBuffer *sampleBuffer = [[SnapDrawingSampleBuffer alloc] initWithPixelBufferRef:outputPixelBuffer presentationTimestamp:timestamp];
    CVPixelBufferRelease(outputPixelBuffer);

    return sampleBuffer;
}

@end

NSError *SnapDrawingTranscodeVideo(SnapDrawingVideoEncoder *encoder,
                          SnapDrawingVideoDecoder *decoder,
                          SnapDrawingVideoTranscodeBlock transcodeBlock) {
    NSError *error = nil;
    BOOL reachedEnd = NO;
    double lastTimestamp = NAN;

    while (!error && !reachedEnd) {
        @autoreleasepool {
            SnapDrawingSampleBuffer *inputBuffer = [decoder copyNextSampleBufferWithError:&error];

            if (!inputBuffer) {
                reachedEnd = YES;
            }

            double timestamp = inputBuffer.timestamp;

            if (isnan(lastTimestamp) || timestamp > lastTimestamp) {
                lastTimestamp = timestamp;

                SnapDrawingSampleBuffer *outputBuffer = [encoder dequeueOutputSampleBufferForTimestamp:[inputBuffer presentationTimestamp] error:&error];
                if (!outputBuffer) {
                    break;
                }

                transcodeBlock(inputBuffer, outputBuffer);

                if (![encoder writeSampleBuffer:outputBuffer error:&error]) {
                    break;
                }
            }
        }
    }

    if (!error) {
        [encoder finishWriting];
    }

    return error;

}

