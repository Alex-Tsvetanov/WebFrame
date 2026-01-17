#pragma once

// Synopsis:
// This header introduces snap::Function, a move-only equivalent to std::function
// and snap::CopyableFunction.
// These components should track very closely std::function, see the documentation
// for std::function, differences not listed below are probably a bug to be reported
// These are the differences:
// - the only valid operations on a "moved-from" state are destruction and assignment (quasi-destructive move)
// - snap::Function accepts move-only targets (for example lambdas that capture unique_ptr) and is itself move-only
// - RTTI will be removed in the future, do not rely on .type()
// - the internal trampolining does not convert the arguments into references

// clang-format off

// The zoo library does not detect whether or not exceptions are disabled.
// This file should be included before any zoo headers.
#if !defined(ZOO_DISABLE_EXCEPTIONS) && !defined(__cpp_exceptions)
  // This C++ feature test macro is supported by both clang and gcc,
  // but not MSVC.
  #define ZOO_DISABLE_EXCEPTIONS 1
#endif

#include "zoo/Any/VTablePolicy.h"
#include "zoo/AnyContainer.h"
#include "zoo/FunctionPolicy.h"

// clang-format on

#include <typeinfo>

namespace snap {

namespace callable::affordance {

// clang-format off

struct ICFSafeDestroy: zoo::Destroy {
    struct VTableEntry: zoo::Destroy::VTableEntry {
        bool default_; // NOLINT
    };

    template<typename Container>
    constexpr static inline VTableEntry
        Default = { zoo::Destroy::Default<Container>, true }; // NOLINT

    template<typename Container>
    constexpr static inline VTableEntry
        Operation = { zoo::Destroy::Operation<Container>, false }; // NOLINT

    template<typename UserContainer>
    struct UserAffordance: zoo::Destroy::UserAffordance<UserContainer> {
        bool isDefault() const noexcept {
            return
                static_cast<const UserContainer *>(this)->
                    container()->template vTable<ICFSafeDestroy>()->default_;
        }
    };
};

}  // namespace callable::affordance

/// \brief restricts the public API of zoo::AnyContainer
///
/// zoo::Function offers access to all of the public interface of its
/// given type erasure container, this might be confusing with regards to
/// AnyContainer::swap, reset and emplace that only make changes to
/// the type-erased value but not the internal trampoline in zoo::Function
// NOLINTNEXTLINE(readability-identifier-naming)
template<typename Policy_>
struct SaferTypeErasureContainer: protected zoo::AnyContainer<Policy_>
{
    using Base = zoo::AnyContainer<Policy_>;
    using Base::Base;
    using Policy = typename Base::Policy;
};

template<typename T>
using MoveOnlyTypeErasurePolicy =
    zoo::Policy<
        T, callable::affordance::ICFSafeDestroy, zoo::Move
    >;

template<typename T>
using CopyableTypeErasurePolicy =
    zoo::Policy<
        T, callable::affordance::ICFSafeDestroy, zoo::Move, zoo::Copy
    >;

template<typename OptimizeFor, typename Signature>
using OptimizedMoveOnlyFunction =
    zoo::Function<
        SaferTypeErasureContainer<MoveOnlyTypeErasurePolicy<OptimizeFor>>,
        Signature
    >;

template<typename OptimizeFor, typename Signature>
using OptimizedCopyableFunction =
    zoo::Function<
        SaferTypeErasureContainer<CopyableTypeErasurePolicy<OptimizeFor>>,
        Signature
    >;

// clang-format on

template<typename Signature>
using Function = OptimizedMoveOnlyFunction<void* [4], Signature>;

template<typename Signature>
using CopyableFunction = OptimizedCopyableFunction<void* [4], Signature>;

} // namespace snap
