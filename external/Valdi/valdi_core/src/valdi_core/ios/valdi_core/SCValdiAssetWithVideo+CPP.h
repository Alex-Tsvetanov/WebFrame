//
//  SCValdiAssetWithVideo+CPP.h
//  valdi_core
//
//  Created by Carson Holgate on 3/20/23.
//
#pragma once

#import "valdi_core/SCValdiObjCConversionUtils.h"
#import "valdi_core/cpp/Resources/LoadedAsset.hpp"

@protocol SCValdiVideoPlayer;

namespace ValdiIOS {

class IOSLoadedVideoAsset : public Valdi::LoadedAsset, public ValdiIOS::ObjCObject {
public:
    IOSLoadedVideoAsset(id<SCValdiVideoPlayer> player);

    ~IOSLoadedVideoAsset() override;

    id<SCValdiVideoPlayer> getVideoPlayer() const;

    VALDI_CLASS_HEADER(IOSLoadedVideoAsset)
};

} // namespace ValdiIOS
