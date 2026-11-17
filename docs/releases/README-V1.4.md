# smolnes Windows-native V1.4 candidate

The V1.x branch has a local V1.4 dual-timer candidate prepared under [`dist/V1.4/candidate/`](../../dist/V1.4/candidate/). The checked x64/x86 pair is the recommended candidate; the `extreme/` pair is an optional size experiment that omits timer failure handling and shutdown cleanup.

The nominal timer cadence is 16/16/18 ms. Runtime pacing, game compatibility, and gameplay have not been validated for this candidate. It is not a published release.
