# dtr6: Pursue (CDarkTrooper.cpp:818) `!PathShagged && !IsOver()`: the IsOver operand needs the
# search cursor past the last waypoint, which open floors never reach before the state changes.
# The cursor is pushed past the end once the trooper has been pursuing for a while.
CLASS = 'CDarkTrooper'
FORCE = [
  ('PursueIsOver', 'loc:CDarkTrooper.cpp:818', [],
   ['$this->mPathFindSearch.mCurWaypoint = 100000'], 50,
   'forced: path cursor past the end so IsOver() is true'),
]
