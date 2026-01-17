//
//  SnapDrawingVideoEncoder.h
//  snap_drawing-demo
//
//  Created by Simon Corsin on 1/21/22.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class SnapDrawingSampleBuffer;

struct BufferData {
    void* bytes;
    size_t bytesPerRow;
    size_t width;
    size_t height;
};

@interface SnapDrawingSampleBuffer : NSObject

@property (readonly, nonatomic) double timestamp;

- (BOOL)lockBytes:(BufferData*)bufferData;
- (void)unlockBytes:(BufferData*)bufferData;

@end

@interface SnapDrawingVideoDecoder : NSObject

@property (readonly, nonatomic) double durationSeconds;
@property (readonly, nonatomic) NSInteger videoWidth;
@property (readonly, nonatomic) NSInteger videoHeight;

- (instancetype)initWithPath:(NSString*)path;

- (BOOL)prepareWithError:(NSError**)error;

- (SnapDrawingSampleBuffer*)copyNextSampleBufferWithError:(NSError**)error;

@end

@interface SnapDrawingVideoEncoder : NSObject

- (instancetype)initWithPath:(NSString*)path videoWidth:(NSInteger)videoWidth videoHeight:(NSInteger)videoHeight;

- (BOOL)prepareWithError:(NSError**)error;

- (BOOL)writeSampleBuffer:(SnapDrawingSampleBuffer*)sampleBuffer error:(NSError**)error;

- (void)finishWriting;

@end

typedef void (^SnapDrawingVideoTranscodeBlock)(SnapDrawingSampleBuffer* inputBuffer,
                                               SnapDrawingSampleBuffer* outputBuffer);

NSError* SnapDrawingTranscodeVideo(SnapDrawingVideoEncoder* encoder,
                                   SnapDrawingVideoDecoder* decoder,
                                   SnapDrawingVideoTranscodeBlock transcodeBlock);

NS_ASSUME_NONNULL_END
