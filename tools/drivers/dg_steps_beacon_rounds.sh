# Steps for dg_area53.sh: knees down, beacon-only rounds (each re-enters DeathHeadAttacks, splitters
# shot each round), then every echo target destroyed. Pick counts grep "DGNEW 14470" (a local probe).
for m in 0x494d3031 0x494d3033 0x494d3035 0x494d3037; do C "send $BODY $m"; done
sleep 1
KNEES=""
for i in $(seq $((0x5f0)) $((0x640))); do id=$(printf "u%04x" $i); out=$(C "obj $id"); if echo "$out" | head -1 | grep -q CCollisionActor && ! echo "$out" | grep -q "hp 300/300"; then C "damage $id 10" | head -1; sleep 0.3; fi; done
for i in $(seq $((0x608)) $((0x630))); do id=$(printf "u%04x" $i); out=$(C "obj $id"); if echo "$out" | grep -q "hp 300/300"; then KNEES="$KNEES $id"; fi; done
echo "KNEES $KNEES"
for k in $KNEES; do C "damage $k 100"; done
sleep 1
for r in $(seq 1 12); do LEFT=0; for k in $KNEES; do if C "obj $k" | grep -q 'hp [1-9]'; then LEFT=1; for m in 0x494d3031 0x494d3033 0x494d3035 0x494d3037; do C "send $BODY $m" >/dev/null; done; C "damage $k 500"; sleep 8; fi; done; echo "KNEEROUND $r left=$LEFT"; [ $LEFT = 0 ] && break; done; sleep 15
BEACON=""
for i in $(seq $((0x5f0)) $((0x640))); do id=$(printf "u%04x" $i); out=$(C "obj $id"); if echo "$out" | grep -q "/250"; then BEACON=$id; fi; done
echo "BEACON $BEACON"
ECHO=$(C "objs EchoTarget" | grep -o '^u[0-9a-f]*' | tr '\n' ' '); echo "ECHO $ECHO"
cnt() { echo "picks=$(grep -c 'DGNEW 14470' $LOGF) $(grep 'DGNEW 14470' $LOGF | sort | uniq -c | tr '\n' ' ') splitters=$(C 'objs Splitter' | grep -c '^u')"; }
for r in $(seq 1 ${ROUNDS:-10}); do
  P0=$(grep -c 'DGNEW 14470' $LOGF)
  C "send $BODY 0x4f50454e" >/dev/null; sleep 3
  echo "R$r beacon $(C "damage $BEACON 500" | head -1)"
  for i in $(seq 1 60); do
    [ $((i % 5)) = 1 ] && C "tp ${TPX:-74.46} ${TPY:-10.38} ${TPZ:--14}" >/dev/null
    for s in $(C "objs Splitter" | grep -o '^u[0-9a-f]*'); do C "damage $s 100000" >/dev/null; done
    sleep 2
    [ $(grep -c 'DGNEW 14470' $LOGF) -gt $P0 ] && break
  done
  echo "R$r $(cnt) t=$i vis=$(tail -300 $LOGF | grep "DGC VK.visible" | tail -1) $(C status | grep -o "position.*" | head -1)"
  C "objs Splitter" | head -5
done
echo "PREECHO $(cnt)"
for i in $(seq 1 30); do C "send $BODY 0x4f50454e" >/dev/null; sleep 2; echo "E$i beacon $(C "damage $BEACON 500" | head -1)"; sleep 2; for e in $ECHO; do echo "E$i $e $(C "damage $e 1000" | head -1)"; done; sleep 6; grep -q 'DGHD11FEC fe0=0' $LOGF && break; done
sleep 20
C "objs Guardian"
echo "END $(cnt)"
