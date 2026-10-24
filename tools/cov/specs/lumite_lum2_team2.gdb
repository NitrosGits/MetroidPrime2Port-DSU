# lum2 team2: the team arms the joined Lumites do not reach. mTeamAiMgrId = the Lumite's own uid (a live object that is
# no team manager: the null cast of JoinTeam / QuitTeam); IsPartOfTeam forced false (QuitTeam on a manager the Lumite is not
# part of); the team role forced to Unknown = 3 (IsHeckler: CanAttack, CanTaunt); five hop actions forced (ReadyToMove).
set $t = (CLumite*)$rdi
set $m = (CStateManager*)$rsi
set $lm_tid = $t->mTeamAiMgrId.value
set $lm_v = (float*)malloc(64)
set var $t->mTeamAiMgrId.value = $t->mUniqueId.value
call $t->JoinTeam(*$m)
call $t->QuitTeam(*$m)
call $t->StartTeamAction(*$m)
call $t->EndTeamAction(*$m)
set var $t->mTeamAiMgrId.value = $lm_tid
printf "team null done (id %d)\n", $lm_tid
set $lm_rf_part = 0
call $t->QuitTeam(*$m)
set $lm_rf_part = -1
set $lm_rf_role = 3
printf "heckler %d\n", $t->IsHeckler(*$m, *(CTriggerData*)$lm_v)
printf "cantaunt heckler %d\n", $t->CanTaunt(*$m, *(CTriggerData*)$lm_v)
printf "canattack heckler %d\n", $t->CanAttack(*$m, *(CTriggerData*)$lm_v)
set $lm_rf_role = -1
set $lm_rf_acts = 5
printf "readytomove 5 actions %d\n", $t->ReadyToMove(*$m, *(CTriggerData*)$lm_v)
set $lm_rf_acts = -1
printf "team2 done\n"
