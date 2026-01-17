//
//  JSPropertyNameIndex.hpp
//  valdi
//
//  Created by Simon Corsin on 5/14/21.
//

#pragma once

#include "utils/base/NonCopyable.hpp"
#include "valdi/runtime/Interfaces/IJavaScriptContext.hpp"

#include <array>
#include <string_view>

namespace Valdi {

void loadPropertyNames(IJavaScriptContext* context,
                       size_t size,
                       std::string_view* cppPropertyNames,
                       JSPropertyName* outJsPropertyNames);
void unloadPropertyNames(IJavaScriptContext* context, size_t size, JSPropertyName* jsPropertyNames);

template<size_t size>
class JSPropertyNameIndex : public snap::NonCopyable {
public:
    JSPropertyNameIndex() = default;
    ~JSPropertyNameIndex() {
        setContext(nullptr);
    }

    bool hasContext() const {
        return _context != nullptr;
    }

    void setContext(IJavaScriptContext* context) {
        if (_context != context) {
            unload();
            _context = context;
            load();
        }
    }

    void set(size_t index, std::string_view propertyName) {
        _cppPropertyNames[index] = propertyName;
    }

    constexpr const std::string_view& getCppName(size_t index) {
        return _cppPropertyNames[index];
    }

    constexpr const JSPropertyName& getJsName(size_t index) const {
        return _jsPropertyNames[index];
    }

private:
    IJavaScriptContext* _context = nullptr;
    std::array<JSPropertyName, size> _jsPropertyNames;
    std::array<std::string_view, size> _cppPropertyNames;

    void unload() {
        unloadPropertyNames(_context, size, _jsPropertyNames.data());
    }

    void load() {
        loadPropertyNames(_context, size, _cppPropertyNames.data(), _jsPropertyNames.data());
    }
};

} // namespace Valdi
