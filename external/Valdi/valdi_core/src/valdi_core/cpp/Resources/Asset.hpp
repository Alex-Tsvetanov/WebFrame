//
//  Asset.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 9/25/19.
//

#pragma once

#include "valdi_core/Asset.hpp"
#include "valdi_core/cpp/Context/PlatformType.hpp"
#include "valdi_core/cpp/Resources/AssetLocation.hpp"
#include "valdi_core/cpp/Utils/ValdiObject.hpp"

namespace Valdi {

class Asset : public ValdiObject, public snap::valdi_core::Asset {
public:
    Asset();
    ~Asset() override;

    /**
     Return the resolved location for where this Asset is stored.
     */
    virtual std::optional<AssetLocation> getResolvedLocation() const = 0;

    virtual bool canBeMeasured() const = 0;

    /**
     Return an LTR or RTL representation of the asset.
     If the asset is not directional, this will return "this".
     */
    virtual Ref<Asset> withDirection(bool rightToLeft);

    /**
     Return a platform specific asset depending on
     the platform it will be rendered upon.
     If the asset is not platform specific, this will return "this".
     */
    virtual Ref<Asset> withPlatform(PlatformType platformType);

    virtual double getWidth() const = 0;
    virtual double getHeight() const = 0;

    std::pair<double, double> measure(double maxWidth, double maxHeight) const;

    double measureWidth(double maxWidth, double maxHeight) final;

    double measureHeight(double maxWidth, double maxHeight) final;

    bool isURL() const;

    bool isLocalResource() const;

    bool needResolve() const;

    StringBox getResolvedURL() const;

    VALDI_CLASS_HEADER(Asset)
};

} // namespace Valdi
