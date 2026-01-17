package com.snap.valdi.utils

/**
  Runnable which guarantees that it runs only once. The runnable
  block is ran under a lock, if there are multiple threads calling run(),
  only the first thread will execute the runnable, whereas the other threads
  will wait until the runnable has finished executing.
 */
class SingleRunnable(private var runnable: Runnable?): Runnable {

    override fun run() {
        synchronized(this) {
            val runnable = this.runnable
            if (runnable != null) {
                this.runnable = null
                runnable.run()
            }
        }
    }

}