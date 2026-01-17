/**
 * Return a promise that resolves when locator returns the result.
 * locator should throw Error if it cannot find the target.
 */
export async function waitFor<T>(locator: () => T | Promise<T>, timeoutMs = 3000, intervalMs = 10): Promise<T> {
  let lastAttemptError: unknown | null = null;

  return new Promise((resolve, reject) => {
    let nextAttemptTimeoutId: number | null = null;

    // Set a timeout to reject the promise if the locator doesn't return a value
    const timeoutId = setTimeout(() => {
      if (nextAttemptTimeoutId !== null) {
        clearTimeout(nextAttemptTimeoutId);
      }
      // Log the stack trace to help debugging
      reject(lastAttemptError ?? new Error('waitFor timed out'));
    }, timeoutMs);

    const check = async () => {
      try {
        const result = await locator();
        clearTimeout(timeoutId);
        resolve(result);
      } catch (e) {
        // ignore, and shedule the next attempt
        nextAttemptTimeoutId = setTimeout(check, intervalMs);
        lastAttemptError = e;
      }
    };

    // Start the first attempt
    void check();
  });
}
