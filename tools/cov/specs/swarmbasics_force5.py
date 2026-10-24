# SwarmBasics (module 80) swb5: the three next steps from the swb4 note. For tools/cov/force.sh:
#   BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/swarmbasics_force5.py <out> \
#     3dfd2249,0g_swamp_hall --con tools/cov/specs/swarmbasics_swamp.con --fast --frames 1200
# (1) a forged outlier list so Think's outlier loop runs and the tail (UpdateAllBoidMovement) is
#     reached; (2) boid 0's launched path, written through the vector element so it stays `$this`;
# (3) the Render light locals, written by name at the line that tests them (they are const locals
#     read at Render's entry, which is why the entry-time member write in swb4 missed them).
CLASS = 'CSwarmBasics'

FORCE = [
    ('5024 outlier loop', 'CSwarmBasics::Think(', [],
     ['$this->mOutlierBoidList = &$this->mBoids.mItems[0]',
      '$this->mBoids.mItems[0].mNext = 0'], 30,
     "Think's outlier loop (852-857) and the tail call at 867"),
    ('6104 boid 0 launched', 'CSwarmBasics::UpdateBoid(', [],
     ['$this->mBoids.mItems[0].mLaunched = 1'], 1000,
     'the launched path (604-641) on the last frames only, so a never-landing boid cannot crawl the run'),
    ('3B34 render hardware light', 'CSwarmBasics::Render(', [],
     ['$this->mEnableLighting = 1', '$this->mUseSoftwareLight = 0',
      '$this->mOutlierBoidList = &$this->mBoids.mItems[0]',
      '$this->mBoids.mItems[0].mNext = 0'], 590,
     'the hardware-light path on ~2 frames (the outlier-list force makes its loop arms reachable)'),
]
