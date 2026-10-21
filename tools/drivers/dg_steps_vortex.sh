# Steps for dg_area53.sh: `fsm <body> VortexAttack`, then the vortex sub-machine runs on its own
# (VortexEndAttack, VortexRetreat, ExitVortexAttack); the body's state is printed every 5 s.
C "fsm $BODY VortexAttack"
for i in $(seq 1 ${VPOLLS:-24}); do sleep 5; echo "T$i $(C "obj $BODY" | grep -i 'fsm' | head -1)"; done
