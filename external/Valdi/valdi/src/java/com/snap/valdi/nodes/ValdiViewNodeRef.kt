package com.snap.valdi.nodes

import com.snap.valdi.utils.Ref

class ValdiViewNodeRef(val viewNode: IValdiViewNode): Ref {

    override fun get(): Any? {
        return viewNode
    }

}
