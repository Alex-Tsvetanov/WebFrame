//
//  SCValdiAssetWithImage.m
//  valdi_core-ios
//
//  Created by Simon Corsin on 12/13/22.
//

#import "valdi_core/SCValdiAssetWithImage.h"
#import "valdi_core/SCValdiAssetWithImage+CPP.h"
#import "valdi_core/SCNValdiCoreAsset+Private.h"
#import "valdi_core/cpp/Resources/Asset.hpp"
#import "valdi_core/AssetOutputType.hpp"
#include "valdi_core/AssetLoadObserver.hpp"

namespace ValdiIOS {

IOSLoadedAsset::IOSLoadedAsset(SCValdiImage *image): ValdiIOS::ObjCObject(image) {}

IOSLoadedAsset::~IOSLoadedAsset() = default;

SCValdiImage *IOSLoadedAsset::getImage() const {
    return getValue();
}

VALDI_CLASS_IMPL(IOSLoadedAsset)

class ImageAsset: public Valdi::Asset {
public:
    ImageAsset(SCValdiImage *image): _image(image) {}

    ~ImageAsset() override {
        _image = nil;
    }

    bool canBeMeasured() const override {
        return true;
    }

    Valdi::StringBox getIdentifier() override {
        return STRING_LITERAL("ImageAsset");
    }

    double getWidth() const final {
        return _image.size.width;
    }

    double getHeight() const final {
        return _image.size.height;
    }

    void addLoadObserver(const std::shared_ptr<snap::valdi_core::AssetLoadObserver>& observer,
                         snap::valdi_core::AssetOutputType outputType,
                         int32_t /*preferredWidth*/,
                         int32_t /*preferredHeight*/,
                         const Valdi::Value& /*filter*/) override {
        if (outputType == snap::valdi_core::AssetOutputType::ImageIOS) {
            auto loadedAsset = Valdi::makeShared<IOSLoadedAsset>(_image);
            observer->onLoad(Valdi::strongRef(this), Valdi::Value(loadedAsset), std::nullopt);
        } else {
            observer->onLoad(Valdi::strongRef(this), Valdi::Value(), {STRING_LITERAL("Unsupported output type")});
        }
    }

    void removeLoadObserver(const std::shared_ptr<snap::valdi_core::AssetLoadObserver>& /*observer*/) override {}

    void updateLoadObserverPreferredSize(const std::shared_ptr<snap::valdi_core::AssetLoadObserver>& observer,
                                         int32_t preferredWidth,
                                         int32_t preferredHeight) override {}

    std::optional<Valdi::AssetLocation> getResolvedLocation() const override {
        return Valdi::AssetLocation(Valdi::StringBox(), true);
    }

private:
    SCValdiImage *_image;
};

}

FOUNDATION_EXPORT SCNValdiCoreAsset *SCValdiAssetFromImage(SCValdiImage *image)
{
    auto asset = Valdi::makeShared<ValdiIOS::ImageAsset>(image).toShared();
    return djinni_generated_client::valdi_core::Asset::fromCpp(asset);
}
