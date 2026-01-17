//
//  ViewTransactionScope.hpp
//  valdi
//
//  Created by Simon Corsin on 7/26/24.
//

#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

class IViewManager;
class IViewTransaction;
class View;
class MainThreadManager;

/**
The ViewTransactionScope encapsulates the creation and flush of IViewTransaction
instances during a ViewNodeTree update pass. It will take care of creating
the transaction if it was requested, and will flush it when the ViewTransactionScope
is destructed.
 */
class ViewTransactionScope : public SimpleRefCountable {
public:
    ViewTransactionScope(IViewManager* viewManager,
                         const Ref<MainThreadManager>& mainThreadManager,
                         bool useMainThreadTransaction);
    ~ViewTransactionScope() override;

    /**
      Set the root view on which the transaction should apply
     */
    void setRootView(Ref<View> rootView);

    /**
      Submit the current transaction and terminate it
     */
    void submit();

    /**
    Flush the current transaction immediately, without waiting until the ViewTransactionScope is out
    of scope. Will be a no-op if the view transaction scope does not have an active transaction.
     */
    void flushNow(bool sync);

    /**
    Sets that the layout became dirty during the ViewNodeTree update pass.
     */
    void setLayoutDidBecomeDirty(bool layoutDidBecomeDirty);

    /**
    Return a valid ViewTransactionScope that is associated with the given view manager.
    Will create one if needed.
     */
    ViewTransactionScope& withViewManager(IViewManager& viewManager);

    /**
    Return an active transaction, allowing the caller to interact with the view hierarchy.
    The transaction will be created if needed.
     */
    IViewTransaction& transaction();

    const Ref<MainThreadManager>& getMainThreadManager() const;

private:
    IViewManager* _viewManager;
    Ref<View> _rootView;
    Ref<MainThreadManager> _mainThreadManager;
    Ref<IViewTransaction> _transaction;
    // The ViewTransactionScope will be set as a linked list if we need
    // a different IViewManager instance
    Ref<ViewTransactionScope> _next;
    bool _layoutDidBecomeDirty;
    bool _useMainThreadTransaction;

    void createTransaction();
};

} // namespace Valdi
