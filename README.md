# Early-est

**EArthquake Rapid Location sYstem with EStimation of Tsunamigenesis**

Real-time earthquake location and tsunami-potential estimation from continuous
seismic data (SeedLink) or event mini-SEED files, including the duration–amplitude
discriminants (T50Ex, Td, T0) and magnitudes (mb, Mwp, Mwpd) described in:

- Lomax, A. & Michelini, A. (2009). *Mwpd: a duration–amplitude procedure for rapid
  determination of earthquake magnitude and tsunamigenic potential from P waveforms.*
  Geophys. J. Int. 176(1), 200–214. doi:10.1111/j.1365-246X.2008.03974.x
- Lomax, A. & Michelini, A. (2009). *Tsunami early warning using earthquake rupture
  duration.* Geophys. Res. Lett. 36, L09306. doi:10.1029/2009GL037223
- Lomax, A. & Michelini, A. (2011). *Tsunami early warning using earthquake rupture
  duration and P-wave dominant period.* Geophys. J. Int. 185(1), 283–291.
  doi:10.1111/j.1365-246X.2010.04916.x
- Lomax, A. & Michelini, A. (2013). *Tsunami early warning within five minutes.*
  Pure Appl. Geophys. 170(9–10), 1385–1395. doi:10.1007/s00024-012-0512-6

Original author: **Anthony Lomax, ALomax Scientific** (www.alomax.net), INGV.

Two programs are included:

- `miniseed_process` — runs Early-est time-domain processing on mini-SEED event files
  (e.g. downloaded from IRIS/EarthScope's Wilber tool).
- `seedlink_monitor` — runs Early-est time-domain processing on continuous data from a
  SeedLink server.

For complete usage information see `work/early-est_users_guide.pdf`, or run either
program with `-h`.

## About this repository

Originally mirrored from the source as deployed and run in production (`1.2.9xDEV`,
2023.05.05), plus one small, optional, additive patch (TLS support for the internet
web-service queries — see [PATCHES.md](PATCHES.md) for a precise diagnosis of why
Early-est's built-in real-time data acquisition and web-service queries can fail
against current IRIS/EarthScope services).

Updated `2026.07.21` to `1.2.11xDEV`, syncing in A. Lomax's own subsequent fixes
(from his working tree, shared directly) to the files that changed: a bug fix for
correctly flagging associated-but-never-located events, an updated report-page link,
and several additional optional regional velocity-model files. `net/`, `response/`,
`math/`, and the other library modules are not part of that sync — they live outside
this specific tree on the author's own machine and weren't touched. See PATCHES.md for
the itemised diff.

**Not included:** compiled/rebuildable artifacts (`*.o`, `*.a`), accumulated runtime
output (`work/seedlink_out/`, `work/msprocess_out/`, `work/msprocess_plots/`), and
site-specific deployment configuration (station/gain lists, `seedlink_monitor.prop`,
`streams.list`) — these are operational, not part of the software itself.

## Licence

Early-est is distributed under the **GNU General Public License v3 or later** — see
[LICENSE](LICENSE).

The `picker/` module carries its own bundled `picker/COPYING`, which is
**GPL v2** — that sub-component retains its original licence as distributed,
unchanged.
