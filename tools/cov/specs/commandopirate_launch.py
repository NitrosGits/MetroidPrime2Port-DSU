# CommandoPirate (module 9) spec for the launching-message run (tools/cov/run.sh): the module's
# jump-back velocity helper (fn_9_3900), which the module's kSM_Launching case calls
# (fn_9_CB40, the call at 0x0000D040).  The driver sends the module's own launching message
# (tools/drivers/commandopirate_launch.py) twice, so the helper's early out (the 0x08 bit of the
# flag byte at guest this+0xC63, 0x0000393C) runs too.
CLASS = 'CCommandoPirate'
SPEC = [
    ('3900 JumpBackVelocity', 'CCommandoPirate::x3900_JumpBackVelocity(', [], 'max=50'),
    ('3090 Drown', 'CCommandoPirate::x3090_Drown(', [], 'max=50'),
]
