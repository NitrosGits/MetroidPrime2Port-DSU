# ScriptGui (module 60) spec for tools/cov/run.sh: the arms come from the coverage build
# (gcov_report.py --module ScriptGui); the breakpoints only count calls. Every GUI object is in
# FrontEnd (world 69802220, area cee3af0d), see docs/module-analysis/ScriptGui.md:
#   BUILD=$PWD/build-port-cov tools/cov/run.sh tools/cov/specs/scriptgui.py <out> 69802220,cee3af0d \
#     --con tools/cov/specs/gui_menu_slider.con --frames 700 --env MP2_SKIP_FRONTEND=0
CLASS = 'CScriptGuiFrontEndScreen'
SPEC = [
    ('4544 FE::Think', 'CScriptGuiFrontEndScreen::Think(', [], 'max=40'),
    ('8878 Menu::Think', 'CScriptGuiMenu::Think(', [], 'max=40'),
]
