//
//  SCValdiAssetWithVideo.m
//  valdi_core
//
//  Created by Carson Holgate on 3/20/23.
//

#import "valdi_core/SCValdiAssetWithVideo.h"
#import "valdi_core/SCValdiAssetWithVideo+CPP.h"
#import "valdi_core/SCNValdiCoreAsset+Private.h"
#import "valdi_core/cpp/Resources/Asset.hpp"
#import "valdi_core/AssetOutputType.hpp"
#include "valdi_core/AssetLoadObserver.hpp"

namespace ValdiIOS {

IOSLoadedVideoAsset::IOSLoadedVideoAsset(id<SCValdiVideoPlayer> videoPlayer): ValdiIOS::ObjCObject(videoPlayer) {}

IOSLoadedVideoAsset::~IOSLoadedVideoAsset() = default;

id<SCValdiVideoPlayer> IOSLoadedVideoAsset::getVideoPlayer() const {
    return getValue();
}

VALDI_CLASS_IMPL(IOSLoadedVideoAsset)

class VideoAsset: public Valdi::Asset {
public:
    VideoAsset(id<SCValdiVideoPlayer> player): _player(player) {}

    ~VideoAsset() override {
        _player = nil;
    }

    bool canBeMeasured() const override {
        return true;
    }

    Valdi::StringBox getIdentifier() override {
        return STRING_LITERAL("VideoAsset");
    }

    double getWidth() const final {
        return _player.size.width;
    }

    double getHeight() const final {
        return _player.size.height;
    }

    void addLoadObserver(const std::shared_ptr<snap::valdi_core::AssetLoadObserver>& observer,
                         snap::valdi_core::AssetOutputType outputType,
                         int32_t /*preferredWidth*/,
                         int32_t /*preferredHeight*/,
                         const Valdi::Value& /*filter*/) override {
        if (outputType == snap::valdi_core::AssetOutputType::VideoIOS) {
            auto loadedAsset = Valdi::makeShared<IOSLoadedVideoAsset>(_player);
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
    id<SCValdiVideoPlayer> _player;
};

}

FOUNDATION_EXPORT SCNValdiCoreAsset *SCValdiAssetFromVideoPlayer(id<SCValdiVideoPlayer> player)
{
    auto asset = Valdi::makeShared<ValdiIOS::VideoAsset>(player).toShared();
    return djinni_generated_client::valdi_core::Asset::fromCpp(asset);
}
