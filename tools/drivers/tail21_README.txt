Sail: tail21 Rezbit/AIMannedTurret natural-play console scripts

These are tools/drivers/con.py command files for a coverage-build run of the two
upstream modules the forced-FSM survey under-reached (Rezbit 26%, AIMannedTurret 37%):

    SMOKE_OUT=... SMOKE_BUILD=$PWD/build-port-cov SMOKE_DISPLAY=:91 SMOKE_PORT=4811 \
    SMOKE_WORLD="1baa96c2,02_cliff_reactorcore" SMOKE_FRAMES=20000 SMOKE_SECS=400 \
    SMOKE_CMD=tools/drivers/tail21_rezbit.txt SMOKE_SAVE=<card dir> tools/smoke/fam.sh rez
    # and SMOKE_WORLD="42b935e4,06_sand_alamofront" for tail21_mannedturret.txt

Merged result: Rezbit 89.5% lines / 70.4% branches, AIMannedTurret 94.9% / 76.4%.
