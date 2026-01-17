//
//  SCValdiAssetWithImage+CPP.h
//  valdi_core
//
//  Created by Simon Corsin on 12/13/22.
//

#pragma once

#import "valdi_core/SCValdiObjCConversionUtils.h"
#import "valdi_core/cpp/Resources/LoadedAsset.hpp"

@class SCValdiImage;

namespace ValdiIOS {

class IOSLoadedAsset : public Valdi::LoadedAsset, public ValdiIOS::ObjCObject {
public:
    IOSLoadedAsset(SCValdiImage* image);

    ~IOSLoadedAsset() override;

    SCValdiImage* getImage() const;

    VALDI_CLASS_HEADER(IOSLoadedAsset)
};

} // namespace ValdiIOS
