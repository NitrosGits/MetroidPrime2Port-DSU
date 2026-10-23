# Parasite REL (module 47: CParasite for the Brizgee and Crystallite loaders) on the coverage build:
# the gcov profile is the evidence; these breakpoints only count a few entries and the types.
# Shipped data has no PARA placement; BRZG (kPT_IceZoomer 3) and CRLT (kPT_Crystallite 10) spawn.
# Run: BUILD=build-port-cov tools/cov/run.sh tools/cov/specs/parasite.py <out> <world,area>
#      --con tools/cov/specs/parasite_<x>.con --fast --frames <n>
#      then tools/cov/gcov_report.py build-port-cov <out>/gcov --module Parasite
CLASS = 'CParasite'
SPEC = [
    ('AcceptScriptMsg 2C2C', 'CParasite::AcceptScriptMsg(', ['$this->mType', '(int)$rdx'], ''),
    ('Death', 'CParasite::Death(', ['$this->mType'], ''),
    ('UpdateShell', 'CParasite::UpdateShell(', ['(int)$rdx'], ''),
    ('Attack', 'CParasite::Attack(', ['(int)$rdx'], ''),
    ('Retreat', 'CParasite::Retreat(', ['(int)$rdx'], ''),
    ('Halt', 'CParasite::Halt(', ['(int)$rdx'], ''),
    ('Crouch', 'CParasite::Crouch(', ['$this->mType', '(int)$rdx'], ''),
    ('GetUp', 'CParasite::GetUp(', ['$this->mType', '(int)$rdx'], ''),
]
