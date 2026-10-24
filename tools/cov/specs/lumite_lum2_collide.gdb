# lum2 collide: CollidedWith with a fabricated collision list (one entry with the Floor material, or none) on a dead Lumite:
# floor with the bounce speed above 1, below 1, no floor entry (the base call), and alive (the base call).
set $t = (CLumite*)$rdi
set $m = (CStateManager*)$rsi
set $lm_id = (unsigned short*)malloc(16)
set var $lm_id[0] = 0xffff
set $lm_cl = (char*)malloc(3200)
python
gdb.selected_inferior().write_memory(int(gdb.parse_and_eval('(long)$lm_cl')), b'\0' * 3200)
end
set $lm_bs = $t->mBounceSpeed
set $lm_al = $t->mAlive
set var *(int*)$lm_cl = 1
set var *(unsigned long long*)($lm_cl + 8 + 48) = 1ull << kMT_Floor
set var $t->mAlive = 0
set var $t->mBounceSpeed = 7.0f
call $t->CollidedWith(*(TUniqueId*)$lm_id, *(CCollisionInfoList*)$lm_cl, *$m)
set var $t->mBounceSpeed = 0.5f
call $t->CollidedWith(*(TUniqueId*)$lm_id, *(CCollisionInfoList*)$lm_cl, *$m)
set var *(unsigned long long*)($lm_cl + 8 + 48) = 0
call $t->CollidedWith(*(TUniqueId*)$lm_id, *(CCollisionInfoList*)$lm_cl, *$m)
set var $t->mAlive = $lm_al
set var $t->mBounceSpeed = $lm_bs
printf "collide done\n"
