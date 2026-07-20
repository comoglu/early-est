#!/bin/bash
# -c handler: seedlink_monitor appends (outpath, instance_path, instance_name)
OUTPATH="$1"; INSTANCE_PATH="$2"; INSTANCE_NAME="$3"
INST="$INSTANCE_PATH/$INSTANCE_NAME"
DOC=/home/comonvgc/early-est.comoglu.com
HIST="$DOC/hypolist_history.csv"
mkdir -p "$DOC/events"
cp -p "$INST"/monitor.xml "$INST"/sta.health.html "$INST"/station.map.html "$INST"/station.map.js \
      "$INST"/pickmessage.html "$INST"/hypomessage.html "$INST"/picks.csv "$DOC"/ 2>/dev/null
cp -p "$OUTPATH"/hypolist.csv "$OUTPATH"/hypolist.html "$OUTPATH"/hypolist.xml "$DOC"/ 2>/dev/null
# accumulate event history (dedup by event_id col1, keep latest, cap 300)
if [ -s "$DOC/hypolist.csv" ]; then
  head -1 "$DOC/hypolist.csv" > /tmp/_h.hdr
  { [ -f "$HIST" ] && tail -n +2 "$HIST"; tail -n +2 "$DOC/hypolist.csv"; } 2>/dev/null \
    | awk 'NF>5{ e[$1]=$0 } END{ for(k in e) print e[k] }' | sort -k11 | tail -300 > /tmp/_h.body
  cat /tmp/_h.hdr /tmp/_h.body > "$HIST"
fi
# persist per-event associated picks (events/picks_<id>.csv), dedup by channel(col6), keep latest
if [ -s "$DOC/picks.csv" ]; then
  PHDR=$(head -1 "$DOC/picks.csv")
  for eid in $(awk 'NR>1 && $1!="-1"{print $1}' "$DOC/picks.csv" | sort -u); do
    f="$DOC/events/picks_${eid}.csv"
    { [ -f "$f" ] && tail -n +2 "$f"; awk -v e="$eid" 'NR>1 && $1==e' "$DOC/picks.csv"; } \
      | awk '{ row[$6]=$0 } END{ for(k in row) print row[k] }' | sort -k4n > /tmp/_ep.tmp
    { echo "$PHDR"; cat /tmp/_ep.tmp; } > "$f"
  done
fi
# persist event EVOLUTION: one row per loc_seq_num snapshot (location/magnitude convergence)
if [ -s "$DOC/hypolist.csv" ]; then
  EHDR=$(head -1 "$DOC/hypolist.csv")
  for eid in $(awk 'NR>1 && $1!=""{print $1}' "$DOC/hypolist.csv" | sort -u); do
    f="$DOC/events/seq_${eid}.csv"
    { [ -f "$f" ] && tail -n +2 "$f"; awk -v e="$eid" 'NR>1 && $1==e' "$DOC/hypolist.csv"; } \
      | awk '{ row[$3]=$0 } END{ for(k in row) print row[k] }' | sort -k3n > /tmp/_sq.tmp
    { echo "$EHDR"; cat /tmp/_sq.tmp; } > "$f"
  done
fi
# official focal mechanism via fmamp first-motion polarity inversion (located events, >=8 polarities)
if [ -s "$INST/hypos.csv" ]; then
  export PATH="/home/comonvgc/early-est-1.2.9/fmamp:$PATH"
  mkdir -p "$INST/events"
  ( cd /home/comonvgc/early-est-1.2.9/work && /usr/bin/timeout 40 ./fmamp_driver_early_est_POL.bash "$INST" >/dev/null 2>&1 ) || true
  for mf in "$INST"/events/hypo.*.mech.fmamp_polarity.preferred.meca; do
    [ -f "$mf" ] || continue
    eid=$(basename "$mf" | sed -E 's/^hypo\.([0-9]+)\..*/\1/')
    read mlon mlat mdep mst mdip mrake _ < "$mf"
    nobs=$(grep -c . "$INST/events/hypo.${eid}.mech.fmamp_polarity.preferred.polar" 2>/dev/null || echo 0)
    [ -n "$mrake" ] && printf '{"method":"fmamp_polarity","strike":%s,"dip":%s,"rake":%s,"nobs":%s}\n' \
      "$mst" "$mdip" "$mrake" "$nobs" > "$DOC/events/mech_${eid}.json"
  done
fi

# per-station data-health (latency seconds) for dashboard live/quiet status
if [ -s "$DOC/sta.health.html" ]; then
  sed -E 's:</td>:\t:g; s:<[^>]+>::g; s:&nbsp;::g' "$DOC/sta.health.html" \
  | awk -F'\t' 'BEGIN{printf "{"}
      {net=$1;sta=$2;gsub(/[^A-Za-z0-9]/,"",net);gsub(/[^A-Za-z0-9]/,"",sta);
       if(net ~ /^[A-Z0-9]{1,2}$/ && sta ~ /^[A-Z0-9]{1,8}$/ && $4 ~ /_/){lat="";
         for(i=3;i<=NF;i++){v=$i;gsub(/ /,"",v);if(v ~ /^[0-9.]+s$/){sub(/s$/,"",v);lat=v;break}}
         if(lat!="") printf "%s\"%s_%s\":%s",(c++?",":""),net,sta,lat}}
      END{print "}"}' > "$DOC/station_health.json"
fi

# index of events that have a focal-mechanism file (dashboard fetches only these -> no 404 spam)
ls "$DOC/events"/mech_*.json 2>/dev/null | sed -E "s#.*/mech_([0-9]+)\.json#\1#" | awk "BEGIN{printf \"[\"}{printf \"%s\\\"%s\\\"\",(NR>1?\",\":\"\"),\$0}END{print \"]\"}" > "$DOC/events/mechs.json"

# housekeeping: bound disk + old per-event files
find /home/comonvgc/early-est-1.2.9/work/seedlink_out/local/2026 -mindepth 3 -maxdepth 3 -mmin +180 -exec rm -rf {} + 2>/dev/null
find /home/comonvgc/early-est-1.2.9/work/seedlink_out/local/2026 -name "*.tgz" -mmin +180 -delete 2>/dev/null
find "$DOC/events" -name "picks_*.csv" -mtime +14 -delete 2>/dev/null
exit 0
