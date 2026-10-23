# MediumIng (module 41) upstream trigger/helper spec (lane ming1): the states spec plus every bool
# trigger with its result (`ret`), so both answers of a trigger show before it is marked. Same drivers
# as mediuming_states.py (mediuming_play/forced/forced2/forced3/dead/zone/dash.con).
import os, runpy
SPEC = list(runpy.run_path(os.path.join(os.path.dirname(os.path.abspath(os.environ['COV_SPEC'])), 'mediuming_states.py'))['SPEC'])
CLASS = 'CMediumIng'
_T = [('2328', 'ShouldBackUp'), ('2390', 'ShouldDoubleDash'), ('23E4', 'ShouldTaunt'),
      ('2438', 'PatternOver'), ('2454', 'HasApproachPattern'), ('24C4', 'HasAttackPattern'),
      ('2534', 'CanBeShot'), ('2B74', 'StillLuredBySafeZone'), ('2B94', 'IsLuredBySafeZone'),
      ('2BCC', 'ShouldEvaporate'), ('2BD8', 'ShouldDoSafezoneReaction'), ('2C2C', 'IsFrustated'),
      ('2E04', 'IsOffPath'), ('2E50', 'HasJumpPoint'), ('3268', 'HasLineOfSight'),
      ('5BD8', 'ShouldGenerate'), ('5C68', 'IsAggressive'), ('5CAC', 'IsMisting'), ('5CCC', 'Leash'),
      ('5D7C', 'PathShagged'), ('2C6C', 'IsDestInsideCurrentRegion'), ('337C', 'ShouldMistAttack'),
      ('3518', 'ShouldMeleeAttack'), ('3640', 'ShouldArmAttack'), ('1E4C', 'IsInSafeZone'),
      ('1E28', 'IsPointInSafeZone')]
SPEC += [('%s %s' % (o, n), 'CMediumIng::%s(' % n, [], 'ret max=4000') for o, n in _T]
SPEC += [('2548 IncrementFrustatedCounter', 'CMediumIng::IncrementFrustatedCounter(', ['$this->mFrustratedCounter'], ''),
         ('2558 ResetFrustatedCounter', 'CMediumIng::ResetFrustatedCounter(', ['$this->mFrustratedCounter'], ''),
         ('4628 TakeDamage', 'CMediumIng::TakeDamage(', [], ''),
         ('1378 ValidateSafeZone', 'CMediumIng::ValidateSafeZone(', [], 'max=4000'),
         ('1ED0 QuitTeam', 'CMediumIng::QuitTeam(', ['$this->mTeamAiMgrId.value != 0xffff'], ''),
         ('1F78 JoinTeam', 'CMediumIng::JoinTeam(', ['$this->mTeamAiMgrId.value != 0xffff'], '')]
