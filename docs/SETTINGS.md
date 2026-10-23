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
- `MP2_GODMODE=1` starts with Cheats > God mode on (not saved).

## Keys

Game page:

- `skip_cinematics`: Cutscenes > Skippable cutscenes.
- `boot_logos`: Cutscenes > Boot logos (0 show, 1 skippable, 2 off).
- `world_name_card`: Cutscenes > World name on load (same values); env `MP2_WORLD_NAME`.
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
- `fast_morph`, `spring_ball`: Morph ball > Fast Morph, Spring Ball (C-stick up).

Controls > Mouse settings:

- `mouse_aim`, `mouse_invert_x`, `mouse_invert_y`: Mouse aim > Mouse aim, Invert X, Invert Y.
- `mouse_sensitivity`: Mouse aim > Sensitivity; env `MP2_MOUSE_SENS`.
- `mouse_buttons`: Mouse aim > Weapon buttons.
- `aim_crosshair`, `crosshair_size`: Mouse aim > Crosshair, Size.

Controls > Controller settings:

- `twin_stick`: Stick aim > Twin stick (right stick aims).
- `stick_aim_rate`, `stick_aim_split`, `stick_aim_rate_y`: Stick aim > Stick aim speed,
  Separate vertical speed, Vertical stick aim speed.

Controls > Touch & gyro (Gyro aim):

- `gyro_aim`, `gyro_mode`: Mode (Off, or one of the four modes; Off clears `gyro_aim`).
- `gyro_source`, `gyro_space`, `gyro_auto_calibrate`: Source, Space, Auto calibrate.
- `gyro_sensitivity`, `gyro_cursor_angle`, `gyro_cursor_camera`: Sensitivity, Pointer speed,
  Gyro also turns the camera.
- `gyro_smoothing`, `gyro_tightening`: Smoothing, Tightening.

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
  `MP2_ANISO`), EFB scale (0 = Auto render scale).
- `frame_limit`: Frame rate > 60 FPS cap (target).
- `smooth_frames`, `pose_interpolation`, `particle_interpolation`, `look_interpolation`: Frame
  rate > Smooth uncapped frames, Smooth animation, Smooth effects, Smooth look.
- `show_shader_compilation`, `show_menu_hint`: Quality > Show shader compilation (a toast while
  the startup pipeline queue compiles), Show F1 hint at launch; env `MP2_SHOW_SHADER_COMPILATION`,
  `MP2_SHOW_MENU_HINT`.
- `per_pixel_lighting`: Quality > Per-pixel lighting (Aurora lights per pixel); env
  `MP2_PER_PIXEL_LIGHTING`.
- `dynamic_res`, `dynamic_res_target`, `dynamic_res_min`: Quality > Dynamic resolution, Target
  (0 = display rate, else 30..240 fps), Lowest scale (0.5..1); env `MP2_DYNAMIC_RES`,
  `MP2_DYNAMIC_RES_TARGET`, `MP2_DYNAMIC_RES_MIN`. `MP2_DYNRES_GPU_TIMES=0` steers on the frame
  rate alone, without GPU timestamps.
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
