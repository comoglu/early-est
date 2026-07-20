# Patches & operational notes — this deployment

This tree is otherwise an unmodified copy of Early-est `1.2.9xDEV` (2023.05.05). No
source patches have been applied to the networking code in this deployment; instead,
the internet-query code path is avoided entirely (see below). This file exists to
document *why* Early-est struggles against modern IRIS/EarthScope services, and the
workaround used here, precisely — for A. Lomax and anyone else running into the same
issue.

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

## Workaround used in this deployment (no source patch)

Rather than patch the networking layer, this deployment avoids it entirely:

1. **Station coordinates & gains are supplied as static local files** (`-g geogfile`,
   `-pz gainfile`) instead of the runtime `-sta-query`/`-pz-query` internet lookups.
   These files are regenerated periodically from a **local** SeisComP `fdsnws`
   instance (plain HTTP, localhost — no TLS needed) rather than fetched by Early-est
   itself over the internet.
2. **Continuous waveform data is fed from a local SeisComP SeedLink server**
   (`seedlink_monitor :18000 -l streams.list …`) rather than Early-est connecting to a
   remote SeedLink/DAS server directly. SeisComP itself handles acquisition from
   GEOFON/EarthScope/RESIF/etc. upstream (over whatever protocol those require), and
   Early-est only ever talks to `localhost`.

This sidesteps the TLS gap completely: Early-est's own networking code is never asked
to reach an HTTPS endpoint. A proper fix would add TLS support to `net/net_lib.c`
(e.g. via OpenSSL) so `-sta-query`/`-pz-query` and the internet gain/station lookups
can work again directly against modern FDSN web services.

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
