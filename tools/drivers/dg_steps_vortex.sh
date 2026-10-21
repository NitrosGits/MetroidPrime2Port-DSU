# Steps for dg_area53.sh: `fsm <body> VortexAttack`, then the vortex sub-machine runs on its own
# (VortexEndAttack, VortexRetreat, ExitVortexAttack); the body's state is printed every 5 s.
# SHOOT=1: `shoot <body>` (the console's aim at GetAimPosition) on every poll (`obj` shows the root state only).
C "fsm $BODY VortexAttack"
for i in $(seq 1 ${VPOLLS:-24}); do
  sleep 5; st=$(C "obj $BODY" | grep -i 'fsm' | head -1); echo "T$i $st"
  [ "${SHOOT:-0}" = 1 ] && C "shoot $BODY" | head -1
done
