# GeomBlobV2 (module 25) spec for tools/cov/run.sh; arms come from the coverage build (gcov_report.py
# --module GeomBlobV2), the breakpoints only count calls. The module's live class is CCollisionTracker:
# every Ing makes one in its create branch (CIng::CreateBlob, fn_29_4830) and the ing-spot form turns
# its emission on (CIng::AlignBlobToSurface, fn_29_4604), so the `ing` smoke family's area drives it:
#   arms: a build-port-cov binary through `tools/smoke/run.sh` family `ing` (1baa96c2,2735236b,
#     driver tools/smoke/drivers/ing_con.py), then gcov_report.py build-port-cov <prefix> --module GeomBlobV2
#   calls: BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/geomblobv2.py <out> 1baa96c2,2735236b \
#     --driver 'MP2_CONSOLE={port} python3 tools/smoke/drivers/ing_con.py <game log> 110' --frames 16000
# gb1b ran the arms route (ing family, 16000 updates): docs/module-analysis/GeomBlobV2.md.
# CSurfaceParticleEffect (the module's second class) is never created: its factory
# (gFactory_SurfaceParticleEffect) is written by SetSurfaceParticleEffectFuncPtrs and read nowhere, in
# the retail DOL, any REL or the port.
CLASS = 'CCollisionTracker'
SPEC = [
    ('27DC Think', 'CCollisionTracker::Think(', [], 'max=4000'),
    ('1FC Trace', 'CSurfacePath::Trace(', [], 'max=4000 ret'),
    ('1A4C FindNearestTriangle', 'CSurfacePath::FindNearestTriangle(', [], 'max=4000 ret'),
    ('1CC8 AddSegment', 'CSurfacePath::AddSegment(', ['$rcx'], 'max=4000'),
    ('0 Simplify', 'CSurfacePath::Simplify(', [], 'max=4000'),
    ('36A0 IsFinished', 'SBlobStrand::IsFinished(', [], 'max=4000 ret'),
    ('2584 AcceptScriptMsg', 'CCollisionTracker::AcceptScriptMsg(', [], 'max=400'),
]
