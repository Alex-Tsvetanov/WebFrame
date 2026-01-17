//
//  SCValdiReferenceTable+CPP.h
//  Valdi
//
//  Created by Simon Corsin on 12/12/23.
//  Copyright © 2023 Snap Inc. All rights reserved.
//

#import "valdi_core/SCValdiReferenceTable.h"
#import "valdi_core/cpp/Utils/Mutex.hpp"
#import "valdi_core/cpp/Utils/ReferenceTable.hpp"
#import "valdi_core/cpp/Utils/Shared.hpp"
#import <Foundation/Foundation.h>

#include <vector>

namespace Valdi::IOS {

class ObjCReferenceTable : public SimpleRefCountable {
public:
    ObjCReferenceTable(bool retainsObjectsAsStrongReference);
    ~ObjCReferenceTable() override;

    /**
     Store the given object as a weak reference and return its id.
     */
    SequenceID store(__unsafe_unretained id object);

    /**
     * Retain a previously stored object. Will increase the ref count by 1.
     */
    void retain(const SequenceID& id);

    /**
     * Remove a previously stored object.
     */
    void remove(SequenceID objectId);

    /**
     * Load an object instance from its id.
     */
    id load(SequenceID objectId) __attribute__((ns_returns_retained));

    bool retainsObjectsAsStrongReference() const;

protected:
    virtual void onStore(void* object, NSUInteger index) = 0;
    virtual const void* onRemove(NSUInteger index) = 0;
    virtual const void* onLoad(NSUInteger index) = 0;

    ReferenceTable _table;
    bool _retainsObjectsAsStrongReference;
};

class ObjCWeakReferenceTable : public ObjCReferenceTable {
public:
    ObjCWeakReferenceTable();
    ~ObjCWeakReferenceTable() final;

    /**
     * Return the global reference table for storing weak references.
     */
    static ObjCWeakReferenceTable* getGlobal();

protected:
    void onStore(void* object, NSUInteger index) final;
    const void* onRemove(NSUInteger index) final;
    const void* onLoad(NSUInteger index) final;

private:
    NSPointerArray* _objects;
};

class ObjCStrongReferenceTable : public ObjCReferenceTable {
public:
    ObjCStrongReferenceTable();
    ~ObjCStrongReferenceTable() final;

    NSMutableArray* unsafeGetInnerTable() const;
    NSArray<id>* getObjects() const;

    /**
     * Return the global reference table for storing strong references.
     */
    static ObjCStrongReferenceTable* getGlobal();

protected:
    void onStore(void* object, NSUInteger index) final;
    const void* onRemove(NSUInteger index) final;
    const void* onLoad(NSUInteger index) final;

private:
    CFMutableArrayRef _objects;
    id _tombstone;
};

} // namespace Valdi::IOS
