#include <unistd.h>
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/ConsoleLogger.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/DiskUtils.hpp"

#include "valdi/standalone_runtime/Arguments.hpp"
#include "snap_drawing/cpp/Utils/Image.hpp"
#include "snap_drawing/cpp/Layers/LayerRoot.hpp"
#include "snap_drawing/cpp/Layers/Layer.hpp"
#include "snap_drawing/cpp/Layers/ImageLayer.hpp"
#include "snap_drawing/cpp/Layers/TextLayer.hpp"
#include "snap_drawing/cpp/Text/FontManager.hpp"
#include "snap_drawing/cpp/Drawing/GraphicsContext/BitmapGraphicsContext.hpp"
#include "snap_drawing/cpp/Animations/Animation.hpp"

#import "SnapDrawingVideoEncoder.h"

using namespace Valdi;

int exitWithError(const Error &error) {
    VALDI_ERROR(ConsoleLogger::getLogger(), "{}", error.getMessage());
    return EXIT_FAILURE;
}

int printUsage(const char **argv) {
    VALDI_ERROR(ConsoleLogger::getLogger(), "Usage: {} image|video -i input -o output", argv[0]);
    return EXIT_FAILURE;
}

namespace snap::drawing {

static Valdi::Ref<Resources> initializeSnapDrawingResources() {
    auto &logger = ConsoleLogger::getLogger();
    auto fontManager = makeShared<FontManager>(logger);

    fontManager->load();

    return makeShared<Resources>(fontManager, 1.0f, logger);
}

struct Scene: public snap::drawing::LayerRoot {
    static constexpr Scalar VIGNETTE_OFFSET = 40;
//    static constexpr ColorType COLOR_TYPE =  ColorTypeRGBA8888;
    static constexpr ColorType COLOR_TYPE =  ColorTypeBGRA8888;

    Size vignetteSize;

    Ref<Layer> contentLayer;
    Ref<Layer> vignetteLayer;
    Ref<Layer> overlayLayer;
    Ref<ImageLayer> imageLayer;
    Ref<TextLayer> textLayer;

    Scene(const Valdi::Ref<Resources> &resources, Size vignetteSize): LayerRoot(resources), vignetteSize(vignetteSize) {}

    void onInitialize() override {
        // Create the layers from our scene
        contentLayer = makeLayer<Layer>(getResources());
        vignetteLayer = makeLayer<Layer>(getResources());
        overlayLayer = makeLayer<Layer>(getResources());
        imageLayer = makeLayer<ImageLayer>(getResources());
        textLayer = makeLayer<TextLayer>(getResources());

        contentLayer->addChild(vignetteLayer);
        vignetteLayer->addChild(imageLayer);
        vignetteLayer->addChild(overlayLayer);
        overlayLayer->addChild(textLayer);

        // Setup main content
        contentLayer->setBackgroundColor(Color::white());

        // Setup inside vignette
        vignetteLayer->setBorderColor(Color::black());
        vignetteLayer->setBorderWidth(4.0);
        vignetteLayer->setBorderRadius(BorderRadius::makeOval(VIGNETTE_OFFSET / 2, false));
        vignetteLayer->setClipsToBounds(true);
        vignetteLayer->setBoxShadow(2.0, 2.0, 5.0, Color::black().withAlphaRatio(0.5f));

        // Setup the text layer
        textLayer->setTextAlign(TextAlignCenter);
        textLayer->setText(STRING_LITERAL("SnapDrawing is pretty cool"));
        textLayer->setTextColor(Color::white());

        auto font = getResources()->getFontManager()->getFontForName(STRING_LITERAL("system-bold 24"), 1.0);
        if (font) {
            textLayer->setTextFont(font.value());
        } else {
            VALDI_ERROR(getResources()->getLogger(), "Failed to load Font: {}", font.error().getMessage());
        }

        // Setup the image layer
        imageLayer->setFittingSizeMode(FittingSizeModeFill);

        // Setup the overlay
        overlayLayer->setBackgroundLinearGradient({0.0, 1.0}, {Color::black().withAlphaRatio(0.0f), Color::black()}, LinearGradientOrientationTopBottom);

        // Layout the scene
        auto sceneSize = Size::make(vignetteSize.width + VIGNETTE_OFFSET * 2, vignetteSize.height + VIGNETTE_OFFSET * 2);

        contentLayer->setFrame(Rect::makeXYWH(0, 0, sceneSize.width, sceneSize.height));
        vignetteLayer->setFrame(contentLayer->getFrame().withInsets(VIGNETTE_OFFSET, VIGNETTE_OFFSET));
        imageLayer->setFrame(Rect::makeXYWH(0, 0, vignetteLayer->getFrame().width(), vignetteLayer->getFrame().height()));
        overlayLayer->setFrame(Rect::makeLTRB(0, vignetteLayer->getFrame().height() / 2.0, vignetteLayer->getFrame().width(), vignetteLayer->getFrame().height()));
        textLayer->setFrame(Rect::makeXYWH(0, 0, overlayLayer->getFrame().width(), overlayLayer->getFrame().height()));

        // Create the root, which host the whole scene
        setContentLayer(contentLayer, ContentLayerSizingModeMatchSize);
        setSize(sceneSize, 1.0);
    }

};

static Valdi::BitmapInfo bitmapInfoFromBufferData(BufferData &bufferData) {
    return  Valdi::BitmapInfo((int)bufferData.width, (int)bufferData.height, Scene::COLOR_TYPE, AlphaTypeOpaque, bufferData.bytesPerRow);
}

Valdi::Ref<DisplayList> updateSceneWithBufferData(const Ref<Scene> &scene, BufferData *bufferData, double timestamp) {
    VALDI_INFO(scene->getResources()->getLogger(), "Processing scene at {}s", timestamp);
    Valdi::Result<Ref<Image>> image;

    if (bufferData != nullptr) {
        auto bitmapInfo = bitmapInfoFromBufferData(*bufferData);
        Valdi::BytesView bytesView(nullptr, reinterpret_cast<const Valdi::Byte *>(bufferData->bytes), bufferData->bytesPerRow * bufferData->height);

        image = Image::makeFromPixelsData(bitmapInfo, bytesView, false);
    } else {
        image = Error("Failed to read input buffer data");
    }

    if (image) {
        scene->imageLayer->setImage(image.value());
    } else {
        scene->imageLayer->setImage(nullptr);
        VALDI_ERROR(scene->getResources()->getLogger(), "Failed to read image: {}", image.error().getMessage());
    }

    scene->processFrame(TimePoint(timestamp));
    return scene->getLastDrawnFrame();
}

Valdi::Result<Valdi::Void> rasterFrameIntoBuffer(const Valdi::Ref<DisplayList> &frame, BufferData &bufferData) {
    auto bitmapInfo = bitmapInfoFromBufferData(bufferData);
    BitmapGraphicsContext bitmapContext;
    auto drawableSurface = bitmapContext.createBitmapSurface(bitmapInfo, bufferData.bytes);

    auto canvas = drawableSurface->prepareCanvas();
    if (!canvas) {
        return canvas.moveError();
    }

    frame->draw(canvas.value(), 0, /* shouldClearCanvas */ true);

    drawableSurface->flush();

    return Valdi::Void();
}

SnapDrawingSampleBuffer *processVideoFrame(const Ref<Scene> &scene, SnapDrawingSampleBuffer *inputVideoBuffer, SnapDrawingSampleBuffer *outputVideoBuffer) {
    BufferData inputBufferData;
    BOOL inputLocked = [inputVideoBuffer lockBytes:&inputBufferData];

    auto frame = updateSceneWithBufferData(scene, inputLocked ? &inputBufferData : nullptr, [inputVideoBuffer timestamp]);

    BufferData outputBufferData;
    BOOL outputLocked = [outputVideoBuffer lockBytes:&outputBufferData];

    if (outputLocked) {
        auto rasterResult = rasterFrameIntoBuffer(frame, outputBufferData);
        if (!rasterResult) {
            VALDI_ERROR(scene->getResources()->getLogger(), "Failed to raster scene: {}", rasterResult.error().getMessage());
        }
    } else {
        VALDI_ERROR(scene->getResources()->getLogger(), "Failed to lock bytes on output buffer");
    }

    if (inputLocked) {
        [inputVideoBuffer unlockBytes:&inputBufferData];
    }

    if (outputLocked) {
        [outputVideoBuffer unlockBytes:&outputBufferData];
    }

    return inputVideoBuffer;
}

static void openOutput(const StringBox &output) {
    NSString *objcOutput = [NSString stringWithUTF8String:output.getCStr()];
    @try {
        [NSTask launchedTaskWithLaunchPath:@"/usr/bin/open" arguments:@[objcOutput]];
    } @catch (NSException *exception) {
        NSLog(@"Failed to open output file '%@': %@", objcOutput, exception.reason);
    }
}

int processVideo(StringBox input, StringBox output) {
    SnapDrawingVideoDecoder *decoder = [[SnapDrawingVideoDecoder alloc] initWithPath:[NSString stringWithUTF8String:input.getCStr()]];

    NSError *error = nil;

    if (![decoder prepareWithError:&error]) {
        return exitWithError(Valdi::Error(error.localizedDescription.UTF8String));
    }

    auto resources = initializeSnapDrawingResources();

    auto scene = makeLayer<Scene>(resources, Size::make(decoder.videoWidth, decoder.videoHeight));

    double fadeOutDuration = std::min(decoder.durationSeconds / 2.0, 5.0);

    scene->overlayLayer->addAnimation(STRING_LITERAL("opacity"), makeShared<Animation>(Duration(fadeOutDuration),
                                                                                        InterpolationFunctions::easeOut(),
                                                                                        [](Layer &layer, double ratio) {
        layer.setOpacity(1.0 - static_cast<Scalar>(ratio));
    }));

    SnapDrawingVideoEncoder *encoder = [[SnapDrawingVideoEncoder alloc] initWithPath:[NSString stringWithUTF8String:output.getCStr()]
                                                        videoWidth:scene->getContentLayer()->getFrame().width()
                                                       videoHeight:scene->getContentLayer()->getFrame().height()];

    if (![encoder prepareWithError:&error]) {
        return exitWithError(Valdi::Error(error.localizedDescription.UTF8String));
    }

    error = SnapDrawingTranscodeVideo(encoder, decoder, ^void (SnapDrawingSampleBuffer * _Nonnull inputBuffer, SnapDrawingSampleBuffer * _Nonnull outputBuffer) {
        processVideoFrame(scene, inputBuffer, outputBuffer);
    });

    if (error) {
        return exitWithError(Valdi::Error(error.localizedDescription.UTF8String));
    }
    openOutput(output);

    return EXIT_SUCCESS;
}

int processImage(StringBox input, StringBox output) {
    auto imageBytes = DiskUtils::load(Valdi::Path(input.toStringView()));
    if (!imageBytes) {
        return exitWithError(imageBytes.error().prepending("Failed to open image input: "));
    }

    auto imageResult = Image::make(imageBytes.value());
    if (!imageResult) {
        return exitWithError(imageResult.error().prepending("Failed to load image: "));
    }

    auto image = imageResult.moveValue();

    auto resources = initializeSnapDrawingResources();

    auto scene = makeLayer<Scene>(resources, Size::make(image->width(), image->height()));
    // Set the image to display on the scene
    scene->imageLayer->setImage(image);

    scene->processFrame(TimePoint(0.0));

    auto frame = scene->getLastDrawnFrame();

    BitmapGraphicsContext bitmapContext;
    auto drawableSurface = bitmapContext.createBitmapSurface(Valdi::BitmapInfo(static_cast<int>(frame->getSize().width), static_cast<int>(frame->getSize().height), Scene::COLOR_TYPE, AlphaTypePremul, 0));

    auto canvas = drawableSurface->prepareCanvas();
    if (!canvas) {
        return exitWithError(canvas.error());
    }

    frame->draw(canvas.value(), 0, /* shouldClearCanvas */ true);

    auto canvasSnapshot = canvas.value().snapshot();
    if (!canvasSnapshot) {
        return exitWithError(canvasSnapshot.error());
    }

    auto pngBytes = canvasSnapshot.value()->toPNG();
    if (!pngBytes) {
        return exitWithError(pngBytes.error());
    }

    auto writeResult = DiskUtils::store(Valdi::Path(output.toStringView()), pngBytes.value());
    if (!writeResult) {
        return exitWithError(writeResult.error().prepending("Failed to write PNG output: "));
    }

    openOutput(output);

    return EXIT_SUCCESS;
}

}

int main(int argc, const char **argv) {
    @autoreleasepool {
        Arguments arguments(argc, argv);

        arguments.next();

        if (!arguments.hasNext()) {
            return printUsage(argv);
        }

        auto command = arguments.next();
        StringBox input;
        StringBox output;

        while (arguments.hasNext()) {
            auto arg = arguments.next();

            if (arg == "-i") {
                if (!arguments.hasNext()) {
                    return printUsage(argv);
                }
                input = arguments.next();
            } else if (arg == "-o") {
                if (!arguments.hasNext()) {
                    return printUsage(argv);
                }
                output = arguments.next();
            } else {
                return printUsage(argv);
            }
        }

        if (command == "video") {
            return snap::drawing::processVideo(input, output);
        } else if (command == "image") {
            return snap::drawing::processImage(input, output);
        } else {
            return printUsage(argv);
        }
    }
}
