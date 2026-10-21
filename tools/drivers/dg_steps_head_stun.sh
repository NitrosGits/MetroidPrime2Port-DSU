# Steps for dg_area53.sh: dg_steps_beacon_rounds.sh with no rounds (knees down, beacon and echo
# targets found), then every echo target destroyed (the head's armour goes, bit 0x10 of 0x10F4)
# and `fsm <head> StunnedReaction` twice; the head's state printed after each step.
ROUNDS=0 source "$(dirname "${BASH_SOURCE[0]}")/dg_steps_beacon_rounds.sh"
for i in $(seq 1 30); do
  C "send $BODY 0x4f50454e" >/dev/null; sleep 2; echo "E$i beacon $(C "damage $BEACON 500" | head -1)"; sleep 2
  for e in $ECHO; do echo "E$i $e $(C "damage $e 1000" | head -1)"; done; sleep 6
  [ -z "$(C "objs EchoTarget" | grep -o '^u[0-9a-f]*')" ] && break
done
echo "ECHO LEFT: $(C "objs EchoTarget" | grep -c '^u')"
C "obj $HEAD" | grep -i fsm
for r in 1 2; do C "fsm $HEAD StunnedReaction"; sleep 15; C "obj $HEAD" | grep -i fsm; done
sleep 30; C "obj $HEAD" | grep -i fsm
