
// using namespace Valdi::IOS;
#include <gtest/gtest.h>
#import "valdi_core/SCValdiReferenceTable+CPP.h"

@interface SCValdiCallBlockOnDealloc: NSObject
@end

@implementation SCValdiCallBlockOnDealloc {
    dispatch_block_t _block;
}

- (instancetype)initWithBlock:(dispatch_block_t)block
{
    self = [super init];

    if (self) {
        _block = block;
    }

    return self;
}

- (void)dealloc
{
    _block();
}

@end

namespace {

using namespace Valdi::IOS;

TEST(SCValdiReferenceTable, strongRefTableRetainsAndReleasesObject) {
    @autoreleasepool {
        auto table = Valdi::makeShared<ObjCStrongReferenceTable>();
        id object = [NSObject new];
        ASSERT_EQ(1, CFGetRetainCount((__bridge CFTypeRef)object));

        auto refId = table->store(object);

        ASSERT_EQ(2, CFGetRetainCount((__bridge CFTypeRef)object));

        table->remove(refId);

        ASSERT_EQ(1, CFGetRetainCount((__bridge CFTypeRef)object));
    }
}

TEST(SCValdiReferenceTable, strongRefTableReturnsRetainedInstance) {
    @autoreleasepool {
        auto table = Valdi::makeShared<ObjCStrongReferenceTable>();
        id object = [NSObject new];
        auto refId = table->store(object);

        ASSERT_EQ(2, CFGetRetainCount((__bridge CFTypeRef)object));

        id retrievedObject = table->load(refId);
        ASSERT_TRUE(object == retrievedObject);

        ASSERT_EQ(3, CFGetRetainCount((__bridge CFTypeRef)object));
        retrievedObject = nil;

        ASSERT_EQ(2, CFGetRetainCount((__bridge CFTypeRef)object));
    }
}

TEST(SCValdiReferenceTable, strongRefTableReleasesObjectsOnDestruction) {
    @autoreleasepool {
        auto table = Valdi::makeShared<ObjCStrongReferenceTable>();
        id object = [NSObject new];
        table->store(object);

        ASSERT_EQ(2, CFGetRetainCount((__bridge CFTypeRef)object));

        table = nullptr;

        ASSERT_EQ(1, CFGetRetainCount((__bridge CFTypeRef)object));
    }
}

TEST(SCValdiReferenceTable, strongRefTableCanReleaseNestedObject) {
    @autoreleasepool {
        auto table = Valdi::makeShared<ObjCStrongReferenceTable>();
        id object = [NSObject new];
        auto refId = table->store(object);

        __block BOOL blockCalled = NO;

        auto nestedRefId = table->store([[SCValdiCallBlockOnDealloc alloc] initWithBlock:^{
            blockCalled = YES;
            table->remove(refId);
        }]);

        ASSERT_EQ(2, CFGetRetainCount((__bridge CFTypeRef)object));

        table->remove(nestedRefId);

        ASSERT_TRUE(blockCalled);
        // Should have released the ref
        ASSERT_EQ(1, CFGetRetainCount((__bridge CFTypeRef)object));
    }
}

TEST(SCValdiReferenceTable, weakRefTableDoesntRetainAndReleaseObject) {
    @autoreleasepool {
        auto table = Valdi::makeShared<ObjCWeakReferenceTable>();
        id object = [NSObject new];

        ASSERT_EQ(1, CFGetRetainCount((__bridge CFTypeRef)object));

        auto refId = table->store(object);

        ASSERT_EQ(1, CFGetRetainCount((__bridge CFTypeRef)object));

        @autoreleasepool {
            table->remove(refId);
        }

        ASSERT_EQ(1, CFGetRetainCount((__bridge CFTypeRef)object));
    }
}

TEST(SCValdiReferenceTable, weakRefTableReturnsRetainedInstance) {
    @autoreleasepool {
        auto table = Valdi::makeShared<ObjCWeakReferenceTable>();
        id object = [NSObject new];
        auto refId = table->store(object);

        ASSERT_EQ(1, CFGetRetainCount((__bridge CFTypeRef)object));

        @autoreleasepool {
            id retrievedObject = table->load(refId);
            ASSERT_TRUE(object == retrievedObject);

            // Weak table currently returns an autoreleased object, so the ref count
            // will be +2
            ASSERT_EQ(3, CFGetRetainCount((__bridge CFTypeRef)object));
            retrievedObject = nil;
        }

        ASSERT_EQ(1, CFGetRetainCount((__bridge CFTypeRef)object));
    }
}

TEST(SCValdiReferenceTable, weakRefReturnsNilOnDestroyedObject) {
    @autoreleasepool {
        auto table = Valdi::makeShared<ObjCWeakReferenceTable>();
        id object = [NSObject new];
        auto refId = table->store(object);
        
        ASSERT_EQ(1, CFGetRetainCount((__bridge CFTypeRef)object));
        
        object = nil;
        
        id retrievedObject = table->load(refId);
        ASSERT_TRUE(retrievedObject == nil);
    }
}

}
