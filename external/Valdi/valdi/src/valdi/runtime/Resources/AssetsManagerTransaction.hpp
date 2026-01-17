//
//  AssetsManagerTransaction.hpp
//  valdi
//
//  Created by Simon Corsin on 7/7/21.
//

#pragma once

#include "valdi/runtime/Resources/AssetKey.hpp"
#include "valdi_core/cpp/Utils/SmallVector.hpp"

#include <mutex>
#include <optional>

namespace Valdi {

class AssetsManagerTransaction {
public:
    explicit AssetsManagerTransaction(std::unique_lock<std::recursive_mutex>&& lock);
    ~AssetsManagerTransaction();

    void releaseLock();
    void acquireLock();

    void enqueueUpdate(const AssetKey& assetKey);

    std::optional<AssetKey> dequeueUpdate();

    static AssetsManagerTransaction* current();
    static void setCurrent(AssetsManagerTransaction* current);

private:
    std::unique_lock<std::recursive_mutex> _lock;
    SmallVector<AssetKey, 8> _updates;
};

} // namespace Valdi
