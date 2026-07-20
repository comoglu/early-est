# Patches & operational notes — this deployment

This tree is an unmodified copy of Early-est `1.2.9xDEV` (2023.05.05) — no source
patches have been applied. This file documents, precisely, *why* Early-est's built-in
real-time acquisition and web-service queries struggle against current
IRIS/EarthScope services, for A. Lomax and anyone else running into the same issue.

There are two distinct EarthScope-side changes that can each independently cause
"trouble getting data and using web services" — worth checking both.

## 1. SeedLink real-time streaming: address migration (affects "getting data")

EarthScope moved the NSF National Geophysical Facility's public SeedLink service from
`rtserve.iris.washington.edu` to `rtserve.earthscope.org` as part of a cloud
transition ([announcement](https://www.earthscope.org/news/seedlink-service-is-moving-as-part-of-our-cloud-transition/)).
Both addresses ran in parallel for about a month, with a **forced cutover on 2026-06-08**:
the legacy address's DNS now redirects to the new service, dropping any client still
connected there. Critically, **the new service is an independent implementation that
does not share connection/state with the legacy server** — a client resuming with an
old statefile against the new address can stall, gap, or (per EarthScope's own notice)
attempt to re-download large amounts of data it thinks is missing.

If `seedlink_monitor` (or an upstream SeisComP/seedlink chain feeding it) is configured
to connect directly to `rtserve.iris.washington.edu:18000`, **that is very likely a
direct cause of "getting data" trouble** — independent of the TLS issue below.
**Fix:** point the SeedLink client at `rtserve.earthscope.org:18000` instead, and
discard/rename any saved SeedLink statefile before reconnecting (do not resume old
state against the new address). This deployment's local SeisComP seedlink chain
already uses `rtserve.earthscope.org:18000` and is unaffected.

## 2. FDSN web services: no TLS support in the built-in HTTP client (affects "web services")

Early-est's own HTTP client (`net/net_lib.c`, `net/htmlget.c`, `response/response_lib.c`)
is a **plain-socket, unencrypted HTTP/1.x client** — there is no OpenSSL/mbedTLS/any TLS
library anywhere in the networking code. Every internet-query call site
(`response_lib.c`, `net/htmlget.c`) opens the connection with:

```c
get_socket_connection(host_name, "http")
```

`get_socket_connection()` resolves `"http"` via `getaddrinfo()`, which always maps to
**port 80, plain text**. There is no path in this code to negotiate TLS or reach an
HTTPS-only endpoint — the built-in `-sta-query-host` / `-pz-query-host` options
(`applications/app_lib.c` usage text: *"host name for station web service (e.g.
www.iris.edu)"*) were designed for an era when FDSN web services were served over plain
HTTP.

Since IRIS/EarthScope (and most other FDSN data centres) now require HTTPS, **any
Early-est built-in internet query (`-sta-query`, `-pz-query`, or the internet gain/
station lookups in `response_lib.c`) will fail against those endpoints** — not because
of a certificate mismatch Early-est is mishandling, but because it never attempts TLS
at all; the connection is refused/reset at the protocol level, or blocked if EarthScope
redirects HTTP→HTTPS.

## Note: this is a site-specific stopgap, not a fix

`seedlink_monitor` already has its own native SeedLink client and its own built-in
web-service queries (`-sta-query`, `-pz-query`) — it's designed to acquire real-time
data and metadata directly, with nothing in between. The TLS gap above is a genuine
bug in that direct path, and the right fix is to close the gap (e.g. by adding TLS
support to `net/net_lib.c`, most easily via **libcurl** rather than hand-rolled
OpenSSL), so those built-in mechanisms work again as intended.

For reference only: this *particular* deployment happens to sit behind a local
SeisComP instance for reasons unrelated to this bug (multi-network aggregation across
several upstream providers), and that incidentally routes around the TLS gap too,
since Early-est only ever talks to `localhost` in that setup. That's specific to this
one deployment, not a general recommendation — happy to share the details if useful
to someone, but it shouldn't be read as "how Early-est should be run."

## Other environment-compatibility notes

- **`work/gmt_bin/`** — thin wrapper scripts (one per GMT5 module name, e.g. `psmeca`,
  `pscoast`, `grdimage`) that each call `gmt <module>`, for running the GMT5-era
  report-plotting scripts against a modern GMT6 install (GMT6 consolidated all module
  binaries under a single `gmt` command). Not an Early-est code change — purely an
  environment shim.

- **Known bug, worked around via config, not patched:** with
  `save.plotfiles.geojson = 1` (and/or `[WaveformExport] enable = 1`), the report
  writer's `simple_distance()` call inside `td_writeTimedomainProcessingReport()`
  (`timedomain_processing_report.c`) can enter an infinite loop / crash under a
  continuous, global, multi-network real-time feed. This deployment runs with both
  disabled (`save.plotfiles.geojson = 0`, `[WaveformExport] enable = 0`) in
  `seedlink_monitor.prop`, which is stable. Flagging in case it's a quick fix upstream.
