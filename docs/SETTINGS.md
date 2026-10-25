# Controls and settings

- F1: the settings overlay (the Prime 1 port's `platform/debug_ui/` layout). Pages: Game,
  Controls (Options, Mouse settings, Controller settings, Touch & gyro, Remap), Video (Display,
  Quality, Frame rate), Randomizer, Cheats, Save states, System, Debug.
  `MP2_DEBUG_TAB=<page>[/<sub-tab>]` opens the overlay on that page; console `overlay <page>`
  does the same in a running game.
- Settings search: the box at the top of the overlay (Ctrl+F) lists the settings whose name,
  page, section or keywords contain every typed word; picking one opens its page, sub-tab and
  section, scrolls to it and flashes it. The index is `platform/port_settings_index.inc` (one row
  per widget, in draw order; ctest `port_settings_index`, `tools/check_settings_index.py`, checks
  it against the sources, so a new widget must be added there). `MP2_DEBUG_SEARCH=<query>` types a
  query into the box when the overlay first opens; `MP2_DEBUG_SEARCH_AUDIT=1` opens every row in
  turn and logs `search audit: found|MISSED: <row>`. Rows drawn on one platform only sit in an
  `#if` block of the index.
- Settings changed in the overlay are saved to `<user>/settings.cfg` and restored on the next
  launch; System > Settings has **Save settings now**. Each key is one row of `kSettingKeys` in
  `platform/debug_ui/settings.cpp`, which both reads and writes it with two sample values;
  console `settings self-test` checks that every key reads back as written. An `MP2_<KEY>`
  environment variable (the key upper-cased, unless listed otherwise below) overrides the file
  for that run. A new key needs a mention here as `key`, or the index check fails.
  `metroid_prime2_port --settings-selftest` runs the same check without starting the game
  (ctest `port_settings_roundtrip`).
- F1 > System > Settings > **Reset configuration...** puts every port setting and control mapping
  back to its default: after a confirmation it saves the current settings, moves `settings.cfg`,
  `touch.cfg`, `controls.toml`, `imgui.ini`, `controller_ports.dat`, `keyboard_bindings.dat` and
  the `*.controller` mapping files into a new `config-backup-<date>-<time>/` folder inside the
  user folder (older backups are never reused), and exits; the next launch starts from defaults
  (`platform/include/port_config_reset.h`, test `port_config_reset_tests`). Saves, save states and
  the card are not touched.
- `MP2_GODMODE=1` starts with Cheats > God mode on (not saved).

## Keys

Save states page:

- `quick_resume`: Save states > Quick resume (default on); env `MP2_QUICK_RESUME`. On quitting during
  play (and as the game keeps safe points, see Resume below) `savestates/resume.mp2ss` is written; the
  next launch asks "Resume where you left off?". Console: `resume [status|save|load|clear]`.

Game page:

- `skip_cinematics`: Cutscenes > Skippable cutscenes.
- `boot_logos`: Cutscenes > Boot logos (0 show, 1 skippable, 2 off).
- `world_name_card`: Cutscenes > World name on load (same values); env `MP2_WORLD_NAME`.
- `walk_speed`: Cutscenes > Walk speed (50 to 200 percent, default 100 = retail); env
  `MP2_WALK_SPEED`, console `walkspeed`. Scales how fast Samus moves on foot: the top speed,
  acceleration and friction of walking and strafing (on the ground, in the air and in water) and the
  orbit strafe and approach speeds are all multiplied by it, so she reaches k times the retail speed
  in the same time and stops in the same time. Jump height, sideways dashes, turning, the grapple
  and the Morph Ball are unchanged; speed carries into a jump. At 100 movement is exactly retail.
- `hint_delay`: Cutscenes > Hint delay (0 to 30 minutes, default 0 = the game's own timing); env
  `MP2_HINT_DELAY`. Caps the wait before a hint first shows, also for hints already waiting in a
  loaded save. Needs the in-game hint system on. Show current hint (Cutscenes button; console
  `hintshow`) shows the hint the game would show next: a hint already on screen comes back,
  otherwise the waiting hint due soonest shows now.
- `livesplit`, `livesplit_address`, `livesplit_split_upgrades`: Speedrun > LiveSplit, Server
  (host:port), Split on upgrades; the address's env is `MP2_LIVESPLIT_ADDR`.
- `game_options`: the game's own Options menu (volumes, sound mode, brightness...), stored as a
  hex blob so they survive without a memory card; not an overlay row.

Controls > Options:

- `lockon_toggle`: Buttons > Toggle Lock-On; env `MP2_LOCK_ON_TOGGLE`.
- `sticky_charge`, `rapid_charge`: Buttons > Sticky Charge, Remastered charge (rapid fire).
- `auto_aim`: Aiming > Auto-aim.
- `pointer_aim`, `pointer_aim_box`, `pointer_aim_turn`: Aiming > Pointer aim (Trilogy style),
  Dead box, Edge turn speed.
- `lock_on_free_aim`: Aiming > Lock-on free aim (default off; needs Mouse aim, Twin stick or
  Touch aim), as in Remastered; env `MP2_LOCK_ON_FREE_AIM`. Under a combat lock the mouse, the
  right stick, gyro aim or a touch aim drag moves a crosshair over the view and shots leave along
  the ray through it, as with pointer aim, while the lock steers the view (no edge turn). Without
  pointer aim the crosshair is there only during the lock. It starts and ends each lock at the
  centre, where shots and homing go for the locked target as before; off the centre beams and
  missiles head for the crosshair and do not home. Under pointer aim with gyro aim the right stick
  moves the crosshair too while locked. Scan locks and grapples still aim through the centre. A
  touch source re-centres it while no finger is down.
- `reset_aim_both`: Aiming > Reset aim resets pointer and camera (the Reset aim binding, unbound
  by default, recentres the pointer under pointer aim, else levels the view); env
  `MP2_RESET_AIM_BOTH`.
- `pointer_menus`, `map_swap_fingers`: Options > Menus > Point and click menus (mouse and touch in
  the pause screens and the map; default on; env `MP2_POINTER_MENUS`), Swap map finger gestures
  (touch: one finger pans the map, two rotate; env `MP2_MAP_SWAP_FINGERS`).
- `fast_morph`, `spring_ball`: Morph ball > Fast Morph, Spring Ball (C-stick up).

Controls > Mouse settings:

- `mouse_aim`, `mouse_invert_x`, `mouse_invert_y`: Mouse aim > Mouse aim, Invert X, Invert Y.
- `mouse_sensitivity`: Mouse aim > Sensitivity (radians per count; the row shows MP1's
  Quake/Source scale, 0.022 degrees per unit); env `MP2_MOUSE_SENS`.
- `mouse_sensitivity_split`, `mouse_sensitivity_y`: Mouse aim > Separate vertical speed, Vertical
  sensitivity (same units); env `MP2_MOUSE_SENS_SPLIT`, `MP2_MOUSE_SENS_Y`.
- `mouse_dpi`: Mouse aim > Mouse DPI (100..64000, default 800), only for the cm/360 readout.
- `mouse_buttons`: Mouse aim > Weapon buttons.
- `aim_crosshair`, `crosshair_size`: Mouse aim > Crosshair, Size.

Controls > Controller settings:

- `twin_stick`: Stick aim > Twin stick (right stick aims).
- `stick_aim_rate`, `stick_aim_split`, `stick_aim_rate_y`: Stick aim > Stick aim speed,
  Separate vertical speed, Vertical stick aim speed (px/s, 50..10000; the slider runs
  100..8100).

Controls > Remap:

- Presets... (beside Reset...): one click applies Classic or Mouse & keyboard to the keyboard, or
  GameCube, Remastered, Modern or Southpaw to all controllers, replacing your own bindings
  there; a selected kind or model keeps its own on top. Reset... removes a device's own
  bindings (after a confirmation), bringing back the Classic and GameCube layouts.
- Morph Ball rows (issue #167: Bomb, Power Bomb, Boost Ball, Spider Ball, Unmorph;
  `ball_bomb`, `ball_power_bomb`, `ball_boost`, `ball_spider`, `ball_unmorph` in
  controls.toml) press A, Y, B, R and X only in Morph Ball, so the ball's controls are
  separate from gameplay's; the stick, Start, Z (map) and the C-stick are shared. On
  keyboard & mouse and controller they start unbound, and an unbound row follows gameplay:
  the game button's inputs do it in the ball. Once a row is set in one of those kinds, it
  replaces that button in the ball for that kind only (setting Bomb on the controller stops
  the pad's A bombing; the keyboard's A key still does). An input bound to a row does only
  that in the ball, held layers included (the same mouse button under any of its codes, an
  axis at any threshold); outside the ball its other bindings work as before, and the
  prompts in the ball follow it. A combo on a row replaces the same combo; its buttons alone
  keep what they do. A row given layers in controls.toml works in the ball while one is held
  and adds to the game button; without the layer its input does what it did. Touch can't
  take them (the overlay keeps its own morph ball buttons). A touch: binding on a control
  counts as taking it over only where it applies.
- `menu_standard_buttons`: Separate menu buttons (menus and the map read the "Menus and map"
  rows; ignored while the touch controls are on); env `MP2_MENU_STANDARD_BUTTONS`.

Controls > Touch & gyro (Gyro aim):

- `gyro_aim`, `gyro_mode`: Mode (Off, or one of the four modes; Off clears `gyro_aim`).
- `gyro_source`, `gyro_space`, `gyro_auto_calibrate`: Source, Space, Auto calibrate.
- `gyro_sensitivity`, `gyro_cursor_angle`, `gyro_cursor_camera`: Sensitivity, Pointer speed,
  Gyro also turns the camera.
- `gyro_split`, `gyro_sensitivity_y`: Separate vertical speed, Vertical sensitivity (under
  pointer aim it scales the pointer's vertical travel); env `MP2_GYRO_SPLIT`,
  `MP2_GYRO_SENSITIVITY_Y`.
- `gyro_smoothing`, `gyro_tightening`: Smoothing, Tightening.

Gyro aim needs mouse aim, twin stick or the touch controls' aim; without pointer aim it turns the
camera at Sensitivity. Touch aim's Separate vertical speed and Vertical touch aim speed are
touch.cfg's `aim_split` and `aim_speed_y` (env `MP2_TOUCH_AIM_SPLIT`, `MP2_TOUCH_AIM_SPEED_Y`).

Video > Display:

- `fullscreen`, `vsync`, `aspect`: Display > Fullscreen (F11), Vsync, Aspect ratio. `MP2_ASPECT`
  takes `4:3`, `16:9` or `window`; `MP2_WIDESCREEN=1` also gives 16:9.
- `hud_wide`, `hud_scale`: HUD and view > Widescreen HUD, HUD scale.
- `crosshair_style`, `crosshair_opacity`: HUD and view > Crosshair style, Crosshair opacity.
- `crosshair_color`, `crosshair_custom_color`: the crosshair's colour (env `MP2_CROSSHAIR_COLOR`);
  no overlay row yet.
- `no_helmet`, `no_visor_fx`: HUD and view > Hide helmet, Hide visor effects; env
  `MP2_NO_HELMET`, `MP2_NO_VISOR_FX`.
- `fov`: HUD and view > Field of view (30..120; 0, the **Retail** button, keeps the game's own);
  env `MP2_FOV`.
- `gun_offset`: HUD and view > Arm cannon right/forward/up, written `right,forward,up`; env
  `MP2_GUN_OFFSET`.

Video > Quality and Frame rate:

- `msaa`, `aniso`, `render_scale`: Quality > Anti-aliasing (1 or 4), Anisotropic filtering (env
  `MP2_ANISO`), Resolution scale (0 = Auto render scale).
- `frame_limit`: Frame rate > 60 FPS cap (target).
- `smooth_frames`, `pose_interpolation`, `particle_interpolation`, `look_interpolation`: Frame
  rate > Smooth uncapped frames, Smooth animation, Smooth effects, Smooth look.
- `show_shader_compilation`, `show_menu_hint`: Quality > Show shader compilation (a toast while
  the startup pipeline queue compiles), Show F1 hint at launch; env `MP2_SHOW_SHADER_COMPILATION`,
  `MP2_SHOW_MENU_HINT`.
- `per_pixel_lighting`: Quality > Per-pixel lighting (Aurora lights per pixel); env
  `MP2_PER_PIXEL_LIGHTING`.
- `dynamic_res`, `dynamic_res_target`, `dynamic_res_min`: Quality > Dynamic resolution (Off, Auto,
  User; the mode is `dynamic_res` with `render_scale`: Auto goes up to the window's own scale
  (`render_scale` 0), User up to the "Maximum resolution scale" set), Frame rate target (0 =
  Display, the display's rate, else 30..500 fps; never above the frame rate cap), Minimum
  resolution scale (0.25..the top, 4 at most; the sliders show the lines drawn, 1x being 480p);
  env `MP2_DYNAMIC_RES`, `MP2_DYNAMIC_RES_TARGET`, `MP2_DYNAMIC_RES_MIN`. Auto starts over when the
  window is resized and does nothing while its top is not above the minimum (F1 says so).
  `MP2_DYNRES_GPU_TIMES=0` steers on the frame rate alone, without GPU timestamps.
- `upscale_sharpen` (`off`, `dynamic`, `always`), `upscale_sharpen_amount` (0..100): Quality >
  Sharpen when upscaled, Sharpening; env `MP2_UPSCALE_SHARPEN`, `MP2_UPSCALE_SHARPEN_AMOUNT`.
- `fps_cap`: Frame rate > FPS cap with the 60 FPS cap off (0 = Unlimited, else 30..500); env
  `MP2_FPS_CAP`.

Video > Compatibility (read once, when the GPU starts: a change applies after a restart):

- `opengles`: Use OpenGL ES (shown only where Aurora has the backend); env `MP2_OPENGLES`. A start
  that crashes leaves the marker `opengles_starting` in the user folder and the next start goes
  back to the default backend.
- `storage_clamp`: Adreno shader fix, -1 Auto, 0 Off, 1 On; sets Aurora's `MP2_STORAGE_CLAMP`
  unless that is already set.
- `low_power_gpu`: Low-power GPU (no MP1 row); sets Aurora's `MP2_GPU_POWER=low` unless set.
- `driver_workarounds`: Driver workarounds, comma-separated `keep_depth`, `gpu_constants`,
  `unmonitored_fence`, `validation` (validation lasts one start). `MP2_DRIVER_WORKAROUNDS` replaces
  them for a run; a start that fails leaves `driver_workarounds_starting` and the next start
  clears them.
- GPU self-test (button, no key): logs one PASS/FAIL line per feature; `MP2_GPU_SELFTEST=1` runs
  it at start. Console `gpuselftest` runs it too.

System and Debug:

- `overlay_windowed`, `ui_theme`, `ui_scale`: System > Overlay > Overlay as a floating window
  (desktop), Menu theme, UI scale.
- `card_backup`: System > Memory card > Back up the card: copies the save file into
  `save-backups` before each card write (console `save import latest` restores one).
- `log_file`: System > Log > Write the log to a file.
- `photo_supersample`, `photo_speed`: Debug > Photo > Supersample, Camera speed.
