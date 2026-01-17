//
//  SCMacros.h
//  ValdiIOS
//
//  Created by Simon Corsin on 5/22/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#define VALDI_NO_INIT                                                                                                  \
    -(instancetype)init __attribute__((unavailable("init is not available.")));                                        \
    +(instancetype) new __attribute__((unavailable("new is not available")));

#define VALDI_NO_VIEW_INIT                                                                                             \
    VALDI_NO_INIT                                                                                                      \
    -(instancetype)initWithFrame : (CGRect)frame __attribute__((unavailable("initWithFrame: is not available.")));     \
    -(instancetype)initWithCoder : (NSCoder*)aDecoder __attribute__((unavailable("initWithCoder: is not "              \
                                                                                 "available")));

#define ObjectAs(INSTANCE, CLASSNAME)                                                                                  \
    ({                                                                                                                 \
        id __CAST_INSTANCE_TO_CLASS_VARIABLE = INSTANCE;                                                               \
        [__CAST_INSTANCE_TO_CLASS_VARIABLE isKindOfClass:[CLASSNAME class]] ?                                          \
            (CLASSNAME*)__CAST_INSTANCE_TO_CLASS_VARIABLE :                                                            \
            nil;                                                                                                       \
    })

#define ProtocolAs(INSTANCE, PROTOCOLNAME)                                                                             \
    ({                                                                                                                 \
        id __CAST_INSTANCE_TO_PROTOCOL_VARIABLE = INSTANCE;                                                            \
        [__CAST_INSTANCE_TO_PROTOCOL_VARIABLE conformsToProtocol:@protocol(PROTOCOLNAME)] ?                            \
            (id<PROTOCOLNAME>)__CAST_INSTANCE_TO_PROTOCOL_VARIABLE :                                                   \
            nil;                                                                                                       \
    })

#ifdef __cplusplus
#define SC_EXTERN_C_BEGIN extern "C" {
#define SC_EXTERN_C_END }
#else
#define SC_EXTERN_C_BEGIN
#define SC_EXTERN_C_END
#endif
