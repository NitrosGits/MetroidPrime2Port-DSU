# DigitalGuardian (module 14), dg31: the charge shot's GetExplosionNormal (fn_14_1AB30). The host
# never sets CEnergyProjectile's explode-pending bit, so every CGuardianProjectile Think sets it
# (forced) and CEnergyProjectile::Think explodes the shot along GetExplosionNormal.
# Run: BUILD=$PWD/build-port-cov tools/cov/force.sh tools/cov/specs/digitalguardian_dg31_explode.py \
#        <out> 1baa96c2,09_cliff_temple_dark --con tools/cov/specs/digitalguardian_chargeshot.con \
#        --fast --frames 420
CLASS = 'CEnergyProjectile'
SPEC = []
FORCE = [
    ('19784 explode pending', '(anonymous namespace)::CGuardianProjectile::Think(', [],
     ['$this->mExplodePending = 1'], 0, 'the explode-pending bit set'),
]
