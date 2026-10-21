# Forcing spec (tools/cov/force.sh) for SpacePirate WarpIn 7078's two fade arms the fights never take.
# One fade update each, then the cached time is put back:
#   1. cached time (0xbd0) <= FLT_EPSILON (rodata BDC, asm 0x74B8-0x74C8): 0xbd0 = 0 -> no fade write.
#   2. f = 1 - remaining / cached <= 0 (asm 0x74F8-0x7504): 0xbd0 = 1e-6 -> alpha clamped to 0.
# $wi_fade: a fade update (msg 1, warp point 1, 0x8fa bit 0x08 set, a real cached time).
# (Not $fs/$gs: those are x86 registers in gdb, not convenience variables.)
CLASS = 'CSpacePirate'
E = '$rdx == 1 && $this->xc04_warpPoint == 1 && $this->x8fa_28_ && $this->xbd0_ > 1e-3'
FORCE = [
    ('7078 WarpIn', 'CSpacePirate::WarpIn(',
     ['$rdx', '$this->xc04_warpPoint', '$this->x8fa_28_', '$this->xbd0_ > 1.2e-7', '$this->mColor.mA'],
     ['$wi_eps = $_isvoid($wi_eps) ? 0 : $wi_eps', '$wi_clamp = $_isvoid($wi_clamp) ? 0 : $wi_clamp', '$wi_save = $_isvoid($wi_save) ? 0.0 : $wi_save',
      '$this->xbd0_ = $wi_eps == 1 ? $wi_save : $this->xbd0_', '$wi_eps = $wi_eps == 1 ? 2 : $wi_eps',
      '$this->xbd0_ = $wi_clamp == 1 ? $wi_save : $this->xbd0_', '$wi_clamp = $wi_clamp == 1 ? 2 : $wi_clamp',
      '$wi_fade = (' + E + ') ? 1 : 0',
      '$wi_save = ($wi_eps == 0 && $wi_fade) ? $this->xbd0_ : $wi_save',
      '$this->xbd0_ = ($wi_eps == 0 && $wi_fade) ? 0 : $this->xbd0_',
      '$wi_eps = ($wi_eps == 0 && $wi_fade) ? 1 : $wi_eps',
      '$this->xbd0_ = ($wi_clamp == 0 && $wi_eps == 2 && $wi_fade) ? 1e-6 : $this->xbd0_',
      '$wi_clamp = ($wi_clamp == 0 && $wi_eps == 2 && $wi_fade) ? 1 : $wi_clamp'],
     0, 'one fade update with 0xbd0 = 0 (FLT_EPSILON arm), the next with 0xbd0 = 1e-6 (clamp)'),
]
