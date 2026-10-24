# dtr9: MarkPathRegion at a Think entry with >1 waypoints (> 4 in the unknownenemies pursue): the last
# waypoint copied from the one before (the midpoint region is the last waypoint's region), then sent far
# away (last region null; the next midpoint marks), then once more (the marked region has avoidance flags).
set $dM = 1
set $t = (CDarkTrooper*)$rdi
set $m = (CStateManager*)$rsi
set $wp = (CVector3f*)$t->mPathFindSearch.mWaypoints.mData
set $nl = $t->mPathFindSearch.mWaypoints.mCount - 1
printf "mark: this %p waypoints %d\n", $t, $nl + 1
set $mi = $t->mMarkedRegionIndex
set $lx = $wp[$nl].mX
set $ly = $wp[$nl].mY
set $lz = $wp[$nl].mZ
set var $wp[$nl].mX = $wp[$nl-1].mX
set var $wp[$nl].mY = $wp[$nl-1].mY
set var $wp[$nl].mZ = $wp[$nl-1].mZ
call $t->MarkPathRegion(*$m)
printf "mark same index %d\n", $t->mMarkedRegionIndex
set var $wp[$nl].mX = 100000.0f
call $t->MarkPathRegion(*$m)
printf "mark far index %d\n", $t->mMarkedRegionIndex
call $t->MarkPathRegion(*$m)
set var $wp[$nl].mX = $lx
set var $wp[$nl].mY = $ly
set var $wp[$nl].mZ = $lz
call $t->UnmarkPathRegion(*$m)
set var $t->mMarkedRegionIndex = $mi
printf "mark done\n"
