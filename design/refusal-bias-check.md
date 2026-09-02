# Are pacing refusals neutral? The procedure, fixed before the data was looked at

A run refused by a pre-declared gate is discarded, and every campaign in this work has
discarded some. The assumption underneath that discard is that a refusal is a fault of the
instrument: the load generator failed to keep its schedule, so the run says nothing about the
server and removing it costs nothing.

At one cell that assumption turned out to be false. In the network campaign, three runs at 400
establishments a second were refused for pacing. Their own records show the server's latency
distribution moved with the refusal: each of the three has a median at or above the maximum
median of all forty-seven accepted runs of the same cell, a 99th percentile three to five times
the worst accepted one, and an establishment excursion of about eight milliseconds, while
establishing every connection with no handshake failures and ordinary CPU. The server did not
fail. It went slow, in exactly the runs the gate removed. There, the discarded runs were the
measurement.

If that is a property of pacing refusals rather than of that cell, then every accepted set in
this work is biased fast at the tail and every reported 99th and 99.9th percentile is
optimistic. This document fixes how that question is answered, before any record is inspected,
so that the answer cannot be fitted to what the records happen to contain.

## The procedure

1. **Population.** Every pacing refusal in every filed design. Refusals by other gates, such as
   CPU frequency drift, are not pacing refusals and are excluded.
2. **Cell.** The full factor combination the design varies, read from the records rather than
   assumed. A refusal is compared only against accepted runs identical to it on every factor.
   No factor is ever pooled to enlarge the comparison set. Pooling the transport arms at one
   rate, for instance, would compare a refusal against faster runs of the other arm and would
   move the bound in the direction of finding refusals innocent. A test that can be weakened by
   choosing a larger comparison set is not a test.
3. **Criterion.** One statistic, one direction: a refusal is OUTSIDE if its latency 99th
   percentile exceeds the maximum latency 99th percentile of the accepted runs in its cell.
   Otherwise INSIDE.
4. **Undefined.** A cell with no accepted runs provides no bound. Those rows are UNDEFINED and
   contribute to neither side.
5. **Weak bound.** A cell with fewer than five accepted runs is marked weak. The direction of
   that bias is towards OUTSIDE, because a maximum over four runs is easier to exceed than a
   maximum over forty-seven. The marking therefore cuts in favour of the finding, not against
   it.
6. **The decisive rule.** The systemic conclusion is drawn only if the OUTSIDE verdicts survive
   removing every weak-bound row. Both counts are reported.
7. **Where the test is undefined by construction.** Refusals at an offered rate already declared
   past the arrangement's reach are reported but excluded from the verdict. At such a rate the
   server is expected to be slow, so the comparison cannot distinguish a stall from a ceiling
   behaving as a ceiling.
8. **Beside every verdict**, the rank of the refused run among the accepted runs of its cell on
   latency 50th and 99th percentiles and establishment 50th and 99th, and the accepted count.
   "Above all forty-seven" and "twelfth of forty-seven" are different facts. Where a design does
   not carry establishment timings those columns are reported absent. No column is synthesised.
9. **Circularity guard.** The three refusals from which this hypothesis was formed are reported
   but flagged as the defining cases, and the verdict is read on the others. Three known
   positives are not evidence for a pattern they were used to define.
10. **Power.** The informative population is ten runs and the table says so. Nothing is claimed
    about statistical power.

## The two permitted conclusions, worded in advance

**If OUTSIDE survives rule 6:** refusal is not demonstrably neutral, and at rates the
arrangement handles comfortably it preferentially removes the runs in which the server was
slowest. This obliges the text to state that reported tails are optimistic by an amount that has
not been bounded. It is a flag on the method. It is not a measurement of the bias and must not
be written as one.

**If INSIDE:** on the refusals available, at the rates that can be tested, the discard shows no
sign of removing slow runs. Not that it is harmless in general. The one established case stands
regardless, because it rests on the server's own latency distribution rather than on this test.

## Why this file exists

A procedure declared in advance and then reported afterwards is only as good as the evidence
that it really was declared in advance. This file is committed before the check is run, so its
timestamp carries that evidence rather than an assurance.
