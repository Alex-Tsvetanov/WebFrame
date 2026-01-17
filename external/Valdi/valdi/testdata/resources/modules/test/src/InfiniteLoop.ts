
function doCallIndefinitey(cb: () => boolean) {
    for (;;) {
        if (!cb()) {
            return;
        }
    }
}

export function callIndefinitely(cb: () => boolean) {
    doCallIndefinitey(cb);
}
