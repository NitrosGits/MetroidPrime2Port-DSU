# lum2 dead: Dead's Activate with each cause-of-death weapon (comboed Dark / Annihilator skip the spin, anything else
# runs it) and Update with the previous-on-ground flag both ways. Last step: it leaves this Lumite in the Dead pose.
set $t = (CLumite*)$rdi
set $m = (CStateManager*)$rsi
set $lm_hi = (CHealthInfo*)$t->GetHealthInfo()
python
for comboed, wt in ((1, 'kWT_Dark'), (1, 'kWT_Annihilator'), (0, 'kWT_Power'), (1, 'kWT_Power')):
    gdb.execute('set var $lm_hi->mDeathWeaponMode.mComboed = %d' % comboed)
    gdb.execute('set var $lm_hi->mDeathWeaponMode.mWeaponType = %s' % wt)
    gdb.execute('call $t->Dead(*$m, kStateMsg_Activate, 0.016f)')
for prev in (0, 1):
    gdb.execute('set var $t->mPrevOnGround = %d' % prev)
    gdb.execute('call $t->Dead(*$m, kStateMsg_Update, 0.016f)')
end
printf "dead done\n"
