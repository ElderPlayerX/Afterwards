mkdir vm
cd vm

call ..\compile  ../g_main.c
@if errorlevel 1 goto quit
call ..\compile  ../g_syscalls.c
@if errorlevel 1 goto quit
call ..\compile  ../bg_misc.c
@if errorlevel 1 goto quit
call ..\compile  ../bg_lib.c
@if errorlevel 1 goto quit
call ..\compile  ../bg_pmove.c
@if errorlevel 1 goto quit
call ..\compile  ../bg_slidemove.c
@if errorlevel 1 goto quit
call ..\compile  ../q_math.c
@if errorlevel 1 goto quit
call ..\compile  ../q_shared.c
@if errorlevel 1 goto quit
call ..\compile  ../ai_dmnet.c
@if errorlevel 1 goto quit
call ..\compile  ../ai_dmq3.c
@if errorlevel 1 goto quit
call ..\compile  ../ai_main.c
@if errorlevel 1 goto quit
call ..\compile  ../ai_chat.c
@if errorlevel 1 goto quit
call ..\compile  ../ai_cmd.c
@if errorlevel 1 goto quit
call ..\compile  ../ai_team.c
@if errorlevel 1 goto quit
call ..\compile  ../g_active.c
@if errorlevel 1 goto quit
call ..\compile  ../g_arenas.c
@if errorlevel 1 goto quit
call ..\compile  ../g_bot.c
@if errorlevel 1 goto quit
call ..\compile  ../g_client.c
@if errorlevel 1 goto quit
call ..\compile  ../g_cmds.c
@if errorlevel 1 goto quit
call ..\compile  ../g_combat.c
@if errorlevel 1 goto quit
call ..\compile  ../g_items.c
@if errorlevel 1 goto quit
call ..\compile  ../g_mem.c
@if errorlevel 1 goto quit
call ..\compile  ../g_misc.c
@if errorlevel 1 goto quit
call ..\compile  ../g_missile.c
@if errorlevel 1 goto quit
call ..\compile  ../g_mover.c
@if errorlevel 1 goto quit
call ..\compile  ../g_session.c
@if errorlevel 1 goto quit
call ..\compile  ../g_spawn.c
@if errorlevel 1 goto quit
call ..\compile  ../g_svcmds.c
@if errorlevel 1 goto quit
call ..\compile  ../g_target.c
@if errorlevel 1 goto quit
call ..\compile  ../g_team.c
@if errorlevel 1 goto quit
call ..\compile  ../g_trigger.c
@if errorlevel 1 goto quit
call ..\compile  ../g_utils.c
@if errorlevel 1 goto quit
call ..\compile  ../g_weapon.c
@if errorlevel 1 goto quit
call ..\compile  ../ai_vcmd.c
@if errorlevel 1 goto quit


..\..\..\bin_nt\q3asm -f ../game
:quit
cd ..
