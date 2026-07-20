# Patches & operational notes — this deployment

This tree is otherwise an unmodified copy of Early-est `1.2.9xDEV` (2023.05.05). No
source patches have been applied to the networking code in this deployment; instead,
the internet-query code path is avoided entirely (see below). This file exists to
document *why* Early-est struggles against modern IRIS/EarthScope services, and the
workaround used here, precisely — for A. Lomax and anyone else running into the same
issue.

## Root cause: no TLS support in the built-in web-service client

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
