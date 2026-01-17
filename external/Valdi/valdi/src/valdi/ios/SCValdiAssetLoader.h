//
//  SCValdiAssetLoader.h
//  ios
//
//  Created by Simon Corsin on 5/7/20.
//

#ifdef __cplusplus

#import "valdi/runtime/Resources/AssetLoader.hpp"
#import "valdi/runtime/Resources/AssetLoaderFactory.hpp"
#import "valdi_core/SCValdiObjCConversionUtils.h"
#import <Foundation/Foundation.h>

namespace Valdi {
class DispatchQueue;
}

@protocol SCValdiBaseAssetLoader;
@protocol SCValdiImageLoader;
@protocol SCValdiVideoLoader;

namespace ValdiIOS {

class AssetLoaderIOS : public Valdi::AssetLoader {
public:
    AssetLoaderIOS(id<SCValdiBaseAssetLoader> imageLoader);
    ~AssetLoaderIOS() override;

    id<SCValdiBaseAssetLoader> getAssetLoader() const;

    Valdi::Result<Valdi::Value> requestPayloadFromURL(const Valdi::StringBox& url) override;

private:
    ObjCObjectDirectRef _assetLoader;
};

class BaseImageLoaderIOS : public AssetLoaderIOS {
public:
    BaseImageLoaderIOS(id<SCValdiImageLoader> imageLoader);
    ~BaseImageLoaderIOS() override;

    id<SCValdiImageLoader> getImageLoader() const;
};

class ImageAssetLoaderIOS : public BaseImageLoaderIOS {
public:
    ImageAssetLoaderIOS(id<SCValdiImageLoader> imageLoader, const Valdi::Ref<Valdi::DispatchQueue>& workerQueue);
    ~ImageAssetLoaderIOS() override;

    snap::valdi_core::AssetOutputType getOutputType() const override;

    Valdi::Shared<snap::valdi_core::Cancelable> loadAsset(
        const Valdi::Value& requestPayload,
        int32_t preferredWidth,
        int32_t preferredHeight,
        const Valdi::Value& attachedData,
        const Valdi::Ref<Valdi::AssetLoaderCompletion>& completion) override;

private:
    Valdi::Ref<Valdi::DispatchQueue> _workerQueue;
};

/**
 * AssetLoaderFactory implementation for iOS which takes a remote downloader that
 * can load asset as bytes, and load it as a SCValdiImage, potentially applying
 * postprocessing effects on it.
 */
class ImageAssetLoaderFactoryIOS : public Valdi::AssetLoaderFactory {
public:
    ImageAssetLoaderFactoryIOS(const Valdi::Ref<Valdi::DispatchQueue>& workerQueue);
    ~ImageAssetLoaderFactoryIOS() override;

    snap::valdi_core::AssetOutputType getOutputType() const final;

    Valdi::Ref<Valdi::AssetLoader> createAssetLoader(const std::vector<Valdi::StringBox>& urlSchemes,
                                                     const Valdi::Ref<Valdi::IRemoteDownloader>& downloader) final;

private:
    Valdi::Ref<Valdi::DispatchQueue> _workerQueue;
};

class VideoAssetLoaderIOS : public AssetLoaderIOS {
public:
    VideoAssetLoaderIOS(id<SCValdiVideoLoader> videoLoader, const Valdi::Ref<Valdi::DispatchQueue>& workerQueue);
    ~VideoAssetLoaderIOS() override;

    snap::valdi_core::AssetOutputType getOutputType() const override;

    bool canReuseLoadedAssets() const override;

    id<SCValdiVideoLoader> getVideoLoader() const;

    Valdi::Shared<snap::valdi_core::Cancelable> loadAsset(
        const Valdi::Value& requestPayload,
        int32_t preferredWidth,
        int32_t preferredHeight,
        const Valdi::Value& attachedData,
        const Valdi::Ref<Valdi::AssetLoaderCompletion>& completion) override;

private:
    Valdi::Ref<Valdi::DispatchQueue> _workerQueue;
    ObjCObjectDirectRef _videoLoader;
};

class BytesAssetLoaderIOS : public BaseImageLoaderIOS {
public:
    BytesAssetLoaderIOS(id<SCValdiImageLoader> imageLoader);
    ~BytesAssetLoaderIOS() override;

    snap::valdi_core::AssetOutputType getOutputType() const override;

    Valdi::Shared<snap::valdi_core::Cancelable> loadAsset(
        const Valdi::Value& requestPayload,
        int32_t preferredWidth,
        int32_t preferredHeight,
        const Valdi::Value& attachedData,
        const Valdi::Ref<Valdi::AssetLoaderCompletion>& completion) override;
};

} // namespace ValdiIOS

#endif