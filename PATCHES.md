# Patches & operational notes — this deployment

This tree is based on Early-est `1.2.9xDEV` (2023.05.05). One small, additive,
optional patch is applied (see "Fix implemented" below) — everything else is
unmodified. This file documents, precisely, *why* Early-est's built-in real-time
acquisition and web-service queries struggle against current IRIS/EarthScope
services, for A. Lomax and anyone else running into the same issue.

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

**Confirmed independently**, against `service.earthscope.org` on port 80 (i.e. without
a real TLS fix, regardless of hostname or HTTP/1.0 vs 1.1): a plain HTTP/1.0 request is
met with CloudFront's standard `301 Moved Permanently` → `https://...` (no
redirect-following logic exists in this code, so the tiny redirect page is all that's
returned); a plain HTTP/1.1 request instead hangs until the 10s socket timeout
(`"Resource temporarily unavailable"`) — because CloudFront, like most servers, defaults
to a *persistent* (keep-alive) connection under HTTP/1.1, and this code's `recv()` loop
relies on the peer closing the socket to know the response is complete (true by default
under HTTP/1.0, not under 1.1). Both are exactly the symptoms of "no TLS, ever, on
port 80" — not two separate bugs.

## Fix implemented: `http_fetch()`, a TLS-capable fetch via libcurl

`net/net_lib.c` / `net/net_lib.h` add a new function, `http_fetch(host, page,
&page_length)`, guarded by `#ifdef USE_LIBCURL` and kept alongside (not replacing) the
original socket-based functions:

- Tries `https://host/page` first; if that fails, automatically retries
  `http://host/page` — no config changes needed at call sites.
- Uses libcurl, so it gets a real TLS handshake, HTTP/1.1 keep-alive handled correctly,
  and redirects followed (`CURLOPT_FOLLOWLOCATION`) — closing all three failure modes
  above at once.
- Returns the **full raw response text, headers included** (`CURLOPT_HEADER, 1L`),
  matching the historical `get_page()` output contract exactly — so `get_gain_xml()`,
  `get_station_coords_xml()`, `get_disp_gain_seed_resp()`, etc. in `response_lib.c`
  needed **no parsing changes**, only the fetch call at each of their 4 call sites
  swapped in (also under `#ifdef USE_LIBCURL`, with the original path preserved in the
  `#else` branch). `net/htmlget.c` (the standalone test tool) is updated the same way.
- Enabled by default via `LIBCURL_DEFS = -D USE_LIBCURL` in the top-level `Makefile`
  (mirroring the existing `AMQP_DEFS`/`JSON_DEFS` opt-in pattern already used in that
  file) — comment out that one line to build without it; everything else is unaffected.
  Requires `libcurl` (`apt install libcurl4-openssl-dev` / `brew install curl`).

**Tested:** compiles cleanly both with and without `USE_LIBCURL` defined (`net_lib.c`,
`response_lib.c`, `htmlget.c`). Run live against `service.earthscope.org`, including the
exact failing query above (`net=AU&sta=EIDS&loc=00&cha=BHE&...&level=channel`) — returns
a proper `HTTP/2 200` with the real `FDSNStationXML` body (gain/instrument sensitivity
data intact), where the unpatched code got the 301 or hung.

Landed via PR #1, kept as its own discrete, reviewable change rather than folded into
other commits. **Confirmed working in the field**: A. Lomax applied this same patch to
his own working tree and reported it "worked immediately", including against his own
exact previously-failing query — see the version sync below.

## Aside: `seedlink_monitor` doesn't need anything in between

Worth restating since it's easy to lose in all this: `seedlink_monitor` already has its
own native SeedLink client and its own built-in web-service queries — it's designed to
acquire real-time data and metadata directly, with nothing in between. The fix above
closes the actual gap in that direct path. This deployment happens to also sit behind a
local SeisComP instance, but that's for unrelated reasons (multi-network aggregation
across several upstream providers) and is specific to this one deployment, not a
general recommendation or a substitute for the fix above — happy to share the details
if useful to someone, but it shouldn't be read as "how Early-est should be run."

## Other environment-compatibility notes

- **`Makefile`** — the `libslink` include path was previously hardcoded to one
  deployment's home directory (`-I/home/comonvgc/early-est_deps/libslink`), left over
  from that deployment's own build. Replaced with a `LIBSLINK_INCLUDE_PATH` variable
  (empty by default, matching how `AMQP_INCLUDE_PATH`/`JSONC_INCLUDE_PATH` already work
  in this file) — set it only if your compiler can't already find libslink's headers.

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
  **Still present, unfixed, as of 1.2.11xDEV** (see version sync below) — the
  workaround remains necessary regardless of version.

## Version sync: `1.2.9xDEV` → `1.2.11xDEV` (2026-07-22)

A. Lomax shared his current working tree (`1.2.11xDEV`, 2026.07.21) directly. It only
contains the early-est-specific modules — `net/`, `response/`, `math/`, `octtree/`,
`picker/`, `settings/`, `feregion/`, `alomax_matrix/`, `matrix_statistics/`, `ran1/`,
`vector/`, `statistics/`, `geometry/`, `miniseed_utils/`, `rabbitmq/` are symlinks to
shared code that lives outside this tree on his own machine (one even points to
`/Users/anthony/soft/settings/`) and weren't sent or touched — so this sync is scoped
to what actually changed. `json/` was byte-identical; not repeated here.

Files updated, verbatim from his tree (this is exactly what he independently confirmed
"worked immediately" on his own machine, incl. the TLS patch above — see PR #1):

- **`timedomain_processing/timedomain_processing.h`** — version bump to `1.2.11xDEV`
  (2026.07.21); `IRIS_METADATA_BASE_URL` changed to `https://` (this is only used to
  build an `<a href>` link in the HTML report for a human to click — not fetched by
  Early-est's own code — so harmless either way, unrelated to the TLS patch above).
- **`timedomain_processing/timedomain_processing_report.c`** — real bug fix (his commit
  title: *"detected non-located (associated only) event"*): events that got associated
  but never actually located (`nassoc < 1` or `prob <= 0`) are now correctly flagged
  `NOT ASSOCIATED` instead of silently mishandled.
- **`timedomain_processing/timedomain_processing.c`** — a commented-out (inactive)
  experimental fallback note for FDSN station queries with an unrecognized location
  code, plus incidental whitespace fixes.
- **`applications/pick_process.c`** — one whitespace fix, one clarified error message
  (now mentions the `-i` ignore flag and checking the station exists on the query
  host). Not a functional change.
- **`timedomain_processing/ttimes/*.h`** — 24 new header files: additional *optional*
  regional velocity models (Longmenshan, two Marmara/`marsite` variants, a TexNet
  `r4-s3-t3-iasp91` regional model, SoCalSN, and TauP-generated AK135 variants) plus
  their travel-time/take-off-angle tables. Purely additive — none of these are the
  default (`TTIMES_AK135_GLOBAL`, unchanged) and none touch the global AK135 tables
  already in this tree.

**Deliberately not touched:**
- Top `Makefile` / `applications/Makefile` / `net/Makefile` — his copies already
  contain this repo's own `LIBCURL_DEFS`/`LDLIBS_CURL` additions verbatim; the only
  further difference is his own local Homebrew/macOS include/lib paths
  (`-L/usr/local/opt/curl/lib`, `-I/usr/local/opt/curl/include`, `-I/usr/local/include`
  on `CC`), which are specific to his machine and don't belong in this generic mirror.
- **`update_distribution.bash`** — this is his own personal release/packaging script
  (builds a distribution zip for his own website), full of hardcoded `/Users/anthony/…`
  paths on both sides of the diff. Not part of the software itself; left as-is.
