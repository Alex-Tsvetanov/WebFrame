//
//  ImageLoaderTask.hpp
//  valdi-desktop-apple
//
//  Created by Simon Corsin on 7/19/22.
//

#pragma once

#include "valdi/runtime/Interfaces/IRemoteDownloader.hpp"
#include "valdi/runtime/Resources/AssetLoaderCompletion.hpp"
#include "valdi_core/Cancelable.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"
#include "valdi_core/cpp/Utils/Value.hpp"

namespace snap::drawing {

class ImageLoader;

class ImageLoaderTask : public Valdi::SharedPtrRefCountable {
public:
    ImageLoaderTask(Valdi::Weak<ImageLoader> imageLoader,
                    const Valdi::StringBox& url,
                    int32_t preferredWidth,
                    int32_t preferredHeight,
                    const Valdi::Value& filter,
                    const Valdi::Ref<Valdi::IRemoteDownloader>& remoteDownloader,
                    const Valdi::Weak<Valdi::AssetLoaderCompletion>& completion);

    ~ImageLoaderTask() override;

    const Valdi::StringBox& getUrl() const;
    int32_t getPreferredWidth() const;
    int32_t getPreferredHeight() const;

    const Valdi::Value& getFilter() const;
    const Valdi::Ref<Valdi::IRemoteDownloader>& getRemoteDownloader() const;

    const Valdi::Weak<ImageLoader>& getImageLoader() const;

    void setCurrentCancelable(const Valdi::Shared<snap::valdi_core::Cancelable>& currentCancelable);

    void notifyCompletion(const Valdi::Result<Valdi::Ref<Valdi::LoadedAsset>>& result);

    void cancel();

    bool wasCanceled() const;

private:
    Valdi::Weak<ImageLoader> _imageLoader;
    Valdi::StringBox _url;
    int32_t _preferredWidth;
    int32_t _preferredHeight;
    Valdi::Value _filter;
    Valdi::Ref<Valdi::IRemoteDownloader> _remoteDownloader;
    Valdi::Weak<Valdi::AssetLoaderCompletion> _completion;
    Valdi::Shared<snap::valdi_core::Cancelable> _currentCancelable;
    mutable Valdi::Mutex _mutex;
    std::atomic_bool _wasCanceled = false;
};

} // namespace snap::drawing
