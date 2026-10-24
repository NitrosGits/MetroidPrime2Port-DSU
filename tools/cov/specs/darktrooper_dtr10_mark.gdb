# dtr10: MarkPathRegion with waypoints built from real region centroids of the trooper's path area:
# last waypoint = centroid of region A, previous = 2*centroid(B) - centroid(A) (midpoint = B's centroid).
# Arms: B marked (flags 0); B already flagged (flags != 0, nothing marked); then restore.
set $dM = 1
set $t = (CDarkTrooper*)$rdi
set $m = (CStateManager*)$rsi
set $ar = $t->GetPathArea(*$m)
set $n = $ar->mRegions.mSize
set $rg = $ar->mRegions.mData
set $wp = (CVector3f*)$t->mPathFindSearch.mWaypoints.mData
set $nl = $t->mPathFindSearch.mWaypoints.mCount - 1
set $mi = $t->mMarkedRegionIndex
set $lx = $wp[$nl].mX
set $ly = $wp[$nl].mY
set $lz = $wp[$nl].mZ
set $px = $wp[$nl-1].mX
set $py = $wp[$nl-1].mY
set $pz = $wp[$nl-1].mZ
printf "mark10: this %p area %p regions %d waypoints %d\n", $t, $ar, $n, $nl + 1
set $a = 0
set $b = $n / 2
if $b == 0
  set $b = 1
end
set var $wp[$nl].mX = $rg[$a].mCentroid.mX
set var $wp[$nl].mY = $rg[$a].mCentroid.mY
set var $wp[$nl].mZ = $rg[$a].mCentroid.mZ
set var $wp[$nl-1].mX = 2.0f * $rg[$b].mCentroid.mX - $rg[$a].mCentroid.mX
set var $wp[$nl-1].mY = 2.0f * $rg[$b].mCentroid.mY - $rg[$a].mCentroid.mY
set var $wp[$nl-1].mZ = 2.0f * $rg[$b].mCentroid.mZ - $rg[$a].mCentroid.mZ
printf "flags b before %d\n", $rg[$b].mData->mAvoidanceFlags
call $t->MarkPathRegion(*$m)
printf "mark real index %d (b=%d) flags b %d\n", $t->mMarkedRegionIndex, $b, $rg[$b].mData->mAvoidanceFlags
call $t->MarkPathRegion(*$m)
printf "mark again index %d\n", $t->mMarkedRegionIndex
call $t->UnmarkPathRegion(*$m)
set $wc = $t->mPathFindSearch.mWaypoints.mCount
set var $t->mPathFindSearch.mWaypoints.mCount = 0
call $t->MarkPathRegion(*$m)
set var $t->mPathFindSearch.mWaypoints.mCount = $wc
set var $rg[$b].mData->mAvoidanceFlags = 0
set var $wp[$nl].mX = $lx
set var $wp[$nl].mY = $ly
set var $wp[$nl].mZ = $lz
set var $wp[$nl-1].mX = $px
set var $wp[$nl-1].mY = $py
set var $wp[$nl-1].mZ = $pz
set var $t->mMarkedRegionIndex = $mi
printf "mark10 done\n"
