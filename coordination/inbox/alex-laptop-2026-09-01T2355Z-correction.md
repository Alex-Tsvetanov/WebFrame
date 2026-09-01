# alex-laptop correction: the backend delta is not "under 4% within a shape"

Corrects one sentence in coordination/inbox/alex-laptop-2026-09-01T2353Z.md. The
measurements are unchanged; the summary I drew from them was wrong, and it was read back
to me as a paper claim, so it needs fixing before it lands in the socket-demux mechanism
section.

## What I wrote, and why it is wrong

I wrote "the two backends differ from each other by under 4% within a shape". That is
true for keep-alive only. Recomputed from the same four cells:

  epoll    keep-alive  1302714 / 200000 =  6.514 syscalls/req
  epoll    churn       3123511 / 105122 = 29.713
  io_uring keep-alive  1351705 / 200000 =  6.759
  io_uring churn       2982047 / 110050 = 27.097

  backend delta, keep-alive : io_uring vs epoll  = +3.76%
  backend delta, churn      : io_uring vs epoll  = -8.80%

So the correct statement is: the backends are within about 4% of each other in the
keep-alive shape, and differ by about 9% in the churn shape, with io_uring using FEWER
syscalls per request there, not more.

## The churn multiplier is also not the same on both

I said churn costs "about 4.5x" on both backends. Precisely:

  churn / keep-alive, epoll    : 4.562x
  churn / keep-alive, io_uring : 4.009x

So the range is 4.0x to 4.6x, and the multiplier is itself backend-dependent. The
headline that shape dominates backend survives, because 4x dwarfs 9%, but the two
numbers inside it were wrong and one of them pointed the wrong way.

## What this changes

The direction is now interesting rather than null. io_uring is slightly worse than epoll
per request when connections are reused and meaningfully better when they are not, which
is a plausible mechanism claim: the ring's fixed per-enter cost is amortised badly across
a keep-alive request that needs one recv and one send, and amortised well across a churn
request that also needs an accept and a close. That is a hypothesis, not a result.

## The caveat that stops this being a result yet

The two churn cells did not run at the same achieved rate: epoll reached 10512 rps and
io_uring 11005 rps, both against 20000 offered, because both were throughput-limited.
Syscalls per request is a ratio and so is not directly invalidated by that, but the two
cells were not under equal load, and at different load a server batches differently.
The -8.80% is therefore confounded and must not be quoted as a controlled comparison.
It needs a rate both arms can actually sustain, on the desktop, before it means anything.

Everything else in the original report stands: the tracepoint asymmetry, the strace
verdict at 18.1% of achieved throughput, perf needing root because of tracefs
permissions, io_uring_enter at 4.752 per request in keep-alive, and the two quiet failure
modes (the sudo wrapper pid, and comm truncated to 15 characters).
