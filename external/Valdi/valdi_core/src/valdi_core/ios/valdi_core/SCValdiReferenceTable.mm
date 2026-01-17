//
//  SCValdiReferenceTable.mm
//  Valdi
//
//  Created by Simon Corsin on 12/12/23.
//  Copyright © 2023 Snap Inc. All rights reserved.
//

#import "valdi_core/SCValdiReferenceTable+CPP.h"

namespace Valdi::IOS {

ObjCReferenceTable::ObjCReferenceTable(bool retainsObjectsAsStrongReference): _retainsObjectsAsStrongReference(retainsObjectsAsStrongReference) {}
ObjCReferenceTable::~ObjCReferenceTable() = default;

SequenceID ObjCReferenceTable::store(__unsafe_unretained id object) {
    auto writeAccess = _table.writeAccess();
    auto ref = writeAccess.makeRef(nullptr);
    auto index = ref.getIndex();

    onStore((__bridge void *)object, index);

    return ref.id;
}

void ObjCReferenceTable::remove(SequenceID objectId) {
    // Keeping as a reference outside of the write access
    // so that releasing the objective-c object occurs
    // without the ref table write lock
    const void *oldObject = nullptr;
    {
        auto writeAccess = _table.writeAccess();
        if (!writeAccess.releaseRef(objectId)) {
            oldObject = onRemove(objectId.getIndex());
        }
    }

    if (oldObject != nullptr) {
        CFRelease(oldObject);
    }
}

id ObjCReferenceTable::load(SequenceID objectId) __attribute__((ns_returns_retained)) {
    auto readAccess = _table.readAccess();
    auto ref = readAccess.getEntry(objectId);

    const auto *object = onLoad(ref.getIndex());

    if (object == nullptr) {
        return nil;
    }

    CFRetain(object);
    return (__bridge_transfer id)object;
}

bool ObjCReferenceTable::retainsObjectsAsStrongReference() const {
    return _retainsObjectsAsStrongReference;
}

ObjCWeakReferenceTable::ObjCWeakReferenceTable(): ObjCReferenceTable(false), _objects([NSPointerArray weakObjectsPointerArray]) {}

ObjCWeakReferenceTable::~ObjCWeakReferenceTable() = default;

void ObjCWeakReferenceTable::onStore(void *object, NSUInteger index) {
    while (index >= _objects.count) {
        [_objects addPointer:nullptr];
    }

    [_objects replacePointerAtIndex:index withPointer:object];
}

const void *ObjCWeakReferenceTable::onRemove(NSUInteger index) {
    [_objects replacePointerAtIndex:index withPointer:nullptr];
    // No need to return old objects for weak references
    return nullptr;
}

const void *ObjCWeakReferenceTable::onLoad(NSUInteger index) {
    return [_objects pointerAtIndex:index];
}

ObjCWeakReferenceTable *ObjCWeakReferenceTable::getGlobal() {
    static auto *kInstance = new ObjCWeakReferenceTable();
    return kInstance;
}

ObjCStrongReferenceTable::ObjCStrongReferenceTable(): ObjCReferenceTable(true), _objects((__bridge_retained CFMutableArrayRef)[NSMutableArray new]), _tombstone([NSObject new]) {

}

ObjCStrongReferenceTable::~ObjCStrongReferenceTable() {
    _tombstone = nil;
    CFRelease(_objects);
}

void ObjCStrongReferenceTable::onStore(void *object, NSUInteger index) {
    CFArraySetValueAtIndex(_objects, index, object);
}

const void *ObjCStrongReferenceTable::onRemove(NSUInteger index) {
    const void *oldObject = CFArrayGetValueAtIndex(_objects, index);
    CFRetain(oldObject);
    CFArraySetValueAtIndex(_objects, index, (__bridge const void *)_tombstone);
    return oldObject;
}

const void *ObjCStrongReferenceTable::onLoad(NSUInteger index) {
    return CFArrayGetValueAtIndex(_objects, index);
}

ObjCStrongReferenceTable *ObjCStrongReferenceTable::getGlobal() {
    static auto *kInstance = new ObjCStrongReferenceTable();
    return kInstance;
}

NSMutableArray *ObjCStrongReferenceTable::unsafeGetInnerTable() const {
    return (__bridge NSMutableArray *)_objects;
}

NSArray<id> *ObjCStrongReferenceTable::getObjects() const {
    auto readAccess = _table.readAccess();
    NSMutableArray<id> *output = [NSMutableArray array];

    for (id obj in unsafeGetInnerTable()) {
        if (obj != _tombstone) {
            [output addObject:obj];
        }
    }

    return [output copy];
}

}
