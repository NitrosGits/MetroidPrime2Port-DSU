package org.metroidprime2.port;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.DashPathEffect;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.view.MotionEvent;
import android.view.View;

import org.libsdl.app.SDLActivity;

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

/**
 * On-screen controller drawn over the game surface.
 *
 * It feeds an SDL virtual gamepad rather than synthesising keyboard keys, so the
 * game's own controller mapping, prompts and rebinding treat it as a real pad,
 * and its sticks are sticks rather than four keys pretending to be one.
 *
 * Two layouts, switched from the layout editor. Default: the classic GameCube pad with the
 * C-stick and the D-pad, because MP2 switches beams and visors with the C-stick. Twin stick
 * is a placement variant (Remastered's Dual Sticks labels and positions): its right stick is
 * still the C-stick, and no PortMouse stick source is behind it. Drag-to-aim on the free
 * screen area is opt-in on the native side (`MP2_TOUCH_AIM=1` via the app's port_paths.txt).
 * The game assigns the actions.
 *
 * This MP2 port keeps MP1's layout editor and removed the beam and visor wheels, the map
 * (minimap tap, pan and pinch), the pointer menus, Turbo, beam shift, the Remastered update
 * toast and the F1 overlay button: MP2 has none of those subsystems yet. MP2's F1 window takes
 * no touch, so the editor opens from the Edit button beside the eye.
 */
final class TouchControlsView extends View {
    // Touch target types.
    private static final int LEFT_STICK = 1;
    private static final int RIGHT_STICK = 2;
    private static final int BUTTON = 3;
    private static final int HIDE = 4;
    // A drag on the free screen area that turns the view by its travel.
    private static final int AIM = 5;
    // The bottom-row Edit button, which opens the layout editor.
    private static final int EDIT = 6;

    // Axis-held controls are tracked with ids above this, to share one press map.
    private static final int AXIS_ID_BASE = 100;

    // SDL gamepad ids. The port maps these onto the GameCube pad itself: SOUTH is
    // A, EAST is B, WEST is X, NORTH is Y, Start is Start, the right shoulder is
    // Z, the D-pad is the D-pad, the left stick is the main stick, the right
    // stick is the C-stick and the triggers are L and R.
    private static final int BTN_SOUTH = 0;
    private static final int BTN_EAST = 1;
    private static final int BTN_WEST = 2;
    private static final int BTN_NORTH = 3;
    private static final int BTN_START = 6;
    private static final int BTN_RIGHT_SHOULDER = 10;
    private static final int BTN_DPAD_UP = 11;
    private static final int BTN_DPAD_DOWN = 12;
    private static final int BTN_DPAD_LEFT = 13;
    private static final int BTN_DPAD_RIGHT = 14;

    private static final int AXIS_LEFTX = 0;
    private static final int AXIS_LEFTY = 1;
    private static final int AXIS_RIGHTX = 2;
    private static final int AXIS_RIGHTY = 3;
    private static final int AXIS_TRIGGER_L = 4;
    private static final int AXIS_TRIGGER_R = 5;

    // The layout's fractions of the view height stop growing at this many dp
    // (a 500 dp phone is unchanged), so a tablet gets phone-sized controls
    // that keep their distance to the nearest edge.
    private static final float MAX_LAYOUT_DP = 520f;
    // Sticks are drawn and read through these, so the two cannot disagree.
    private static final float STICK_RADIUS = 0.16f;
    private static final float LEFT_STICK_REACH = 1.5f; // grab area, in stick radii
    private static final float STICK_DEAD_ZONE = 0.12f;
    // The GameCube C-stick is smaller than the main stick and sits just below
    // and left of the face buttons: its centre is this far from the right edge,
    // in view heights, like the face buttons' anchor.
    private static final float GC_CSTICK_FROM_RIGHT = 0.60f;
    private static final float GC_CSTICK_Y = 0.76f;
    private static final float GC_CSTICK_RADIUS = 0.115f;

    // The GameCube pad's colours, as RGB; the fill alpha follows the press state.
    private static final int GC_GREEN = 0x2FA864;
    private static final int GC_RED = 0xC8343A;
    private static final int GC_GREY = 0x8A8A94;
    private static final int GC_YELLOW = 0xE0C020;
    private static final int GC_PURPLE = 0x6A4FB0;

    // A's centre: from the right edge and from the top, in view heights. The
    // GameCube cluster is laid out in heights around it, so it keeps the pad's
    // shape whatever the screen's aspect.
    private static final float GC_A_FROM_RIGHT = 0.235f;
    private static final float GC_A_Y = 0.700f;

    // The GameCube pad's face, so the overlay
    // matches the pad the game was authored for: a big green A, a small red B
    // at its lower left, and X and Y as kidneys curving round A's right and top.
    private static final ControlButton[] GAMECUBE_FACE = {
        ControlButton.round("A", BTN_SOUTH, 0f, 0f, 0.085f, GC_GREEN),
        ControlButton.round("B", BTN_EAST, -0.123f, 0.103f, 0.050f, GC_RED),
        ControlButton.kidney("X", BTN_WEST, 0.158f, 0.040f, -37f, 55f, GC_GREY),
        ControlButton.kidney("Y", BTN_NORTH, 0.158f, 0.040f, -150f, 55f, GC_GREY),
    };
    // What each GameCube face button does, drawn small under its letter.
    private static final String[] GAMECUBE_FUNCTIONS = {"Fire", "Jump", "Morph", "Missile"};

    // The D-pad is one cross, as on the GameCube pad: its centre as fractions of
    // the view, its arms in view heights so it stays square.
    private static final float DPAD_X = 0.115f;
    private static final float DPAD_Y = 0.335f;
    private static final float DPAD_ARM = 0.130f;  // centre to an arm's end
    private static final float DPAD_HALF = 0.045f; // an arm's half width
    // Up, down, left, right: ids, labels, and each arm's direction.
    private static final int[] DPAD_BUTTONS = {
        BTN_DPAD_UP, BTN_DPAD_DOWN, BTN_DPAD_LEFT, BTN_DPAD_RIGHT,
    };
    private static final String[] DPAD_LABELS = {"\u25B2", "\u25BC", "\u25C0", "\u25B6"};
    private static final int[] DPAD_DX = {0, 0, -1, 1};
    private static final int[] DPAD_DY = {-1, 1, 0, 0};

    // L and R are the pad's analog triggers; Z is its digital shoulder. Z sits in
    // front of R on the GameCube pad, so Z goes under R here. L and R are tall so
    // a quick lock-on is hard to miss; L stops just above the D-pad. START
    // ignores its rect: see cornerSlot.
    // Twin stick (Remastered): a diamond in Xbox positions, round A's spot, with
    // Remastered's Dual Sticks functions on the GameCube buttons they send: Jump
    // is B, Fire is A, Morph is X and Missile is Y (the RB pill sends Y too).
    // Bottom, right, left, top, so
    // the controls are C_JUMP + index. Coloured, each takes the colour of the
    // GameCube button it sends.
    private static final float TWIN_DIAMOND = 0.095f;
    private static final ControlButton[] TWIN_FACE = {
        ControlButton.round("Jump", BTN_EAST, 0f, TWIN_DIAMOND, 0.055f, GC_RED),
        ControlButton.round("Fire", BTN_SOUTH, TWIN_DIAMOND, 0f, 0.055f, GC_GREEN),
        ControlButton.round("Morph", BTN_WEST, -TWIN_DIAMOND, 0f, 0.055f, GC_GREY),
        ControlButton.round("Missile", BTN_NORTH, 0f, -TWIN_DIAMOND, 0.055f, GC_GREY),
    };

    private static final PillButton[] GAMECUBE_PILLS = {
        new PillButton("L Lock", AXIS_TRIGGER_L, -1, 0.020f, 0.030f, 0.150f, 0.190f, GC_GREY),
        new PillButton("R Look", AXIS_TRIGGER_R, -1, 0.850f, 0.030f, 0.980f, 0.190f, GC_GREY),
        new PillButton("Z Map", -1, BTN_RIGHT_SHOULDER, 0.870f, 0.205f, 0.960f, 0.265f, GC_PURPLE),
        new PillButton("START", -1, BTN_START, 0f, 0f, 0f, 0f),
    };

    // Twin stick's shoulders, like Remastered's: LT locks on (the L trigger), RT
    // fires, LB jumps and RB fires missiles. Z (the map) sits inboard of the
    // right pair; R (free look) has no use with a right stick that aims. START as in the GameCube layout.
    // The Xbox letter drawn big on each diamond button, matching the in-game hint.
    private static final String[] TWIN_LETTERS = {"A", "B", "X", "Y"};

    private static final PillButton[] TWIN_PILLS = {
        new PillButton("LT Lock", AXIS_TRIGGER_L, -1, 0.020f, 0.030f, 0.150f, 0.100f, GC_GREY,
                       TouchControlsView.C_LT),
        new PillButton("LB Jump", -1, BTN_EAST, 0.020f, 0.115f, 0.150f, 0.185f,
                       GC_RED, TouchControlsView.C_LB),
        new PillButton("RT Fire", -1, BTN_SOUTH, 0.850f, 0.030f, 0.980f, 0.100f,
                       GC_GREEN, TouchControlsView.C_RT),
        new PillButton("RB Missile", -1, BTN_NORTH, 0.850f, 0.115f, 0.980f, 0.185f, GC_GREY,
                       TouchControlsView.C_RB),
        new PillButton("Map", -1, BTN_RIGHT_SHOULDER, 0.730f, 0.030f, 0.830f, 0.100f, GC_PURPLE,
                       TouchControlsView.C_TZ),
        new PillButton("START", -1, BTN_START, 0f, 0f, 0f, 0f),
    };

    // The round buttons along the bottom: START on the left, the eye on the
    // right. Inset from the sides and bottom so rounded
    // display corners don't cut them off.
    private static final float BOTTOM_BUTTON_RADIUS_DP = 22f;
    private static final float BOTTOM_BUTTON_SIDE_DP = 28f;
    private static final float BOTTOM_BUTTON_BOTTOM_DP = 12f;
    private static final float BOTTOM_BUTTON_GAP_DP = 8f;

    // A flick up that counts as a swipe (native SpringSwipe holds the aim): at least this far,
    // in dp, within this many ms of the stroke's start (about 270 dp/s or faster).
    private static final float SWIPE_UP_DP = 40f;
    private static final long SWIPE_UP_MS = 150;

    // Finger spread, in dp, under which a pinch is ignored (the ratio blows up).
    private static final float MAP_PINCH_MIN_DP = 20f;
    // Travel, in dp, past which a touch is a drag and not a tap (Fire, Jump).
    private static final float MAP_TAP_SLOP_DP = 12f;

    private static final long PHYSICAL_INPUT_POLL_MS = 250;

    // Every control is one movable unit. Its override is an offset (in layoutU
    // units, so it follows the screen's size) and a size scale, both applied on
    // top of the default position, which still follows the F1 margin settings.
    // MP2 draws no Visor, Beam, Menu, Map or Turbo control: their ids stay, retired
    // like C_TR, so the saved ids don't shift (see editShown).
    private static final int C_LSTICK = 0;
    private static final int C_CSTICK = 1;
    private static final int C_DPAD = 2;
    private static final int C_A = 3; // A, B, X, Y follow in GAMECUBE_FACE's order
    private static final int C_B = 4;
    private static final int C_X = 5;
    private static final int C_Y = 6;
    private static final int C_L = 7;
    private static final int C_R = 8;
    private static final int C_Z = 9;
    private static final int C_VISOR = 10; // the beam button follows
    private static final int C_BEAM = 11;
    private static final int C_START = 12;
    private static final int C_MENU = 13;
    private static final int C_MAP = 14;
    private static final int C_EYE = 15;
    // Twin stick's own controls, so a layout made there doesn't move the
    // default or classic layout's buttons. The four face buttons are bottom,
    // right, left, top (TWIN_FACE's order).
    private static final int C_RSTICK = 16;
    private static final int C_JUMP = 17;
    private static final int C_FIRE = 18;
    private static final int C_MORPH = 19;
    private static final int C_MISSILE = 20;
    private static final int C_LT = 21;
    private static final int C_LB = 22;
    private static final int C_RT = 23;
    private static final int C_RB = 24;
    private static final int C_TZ = 25;
    private static final int C_TR = 26;
    // MP1's Turbo button, once for the default/classic layouts and once for twin.
    private static final int C_TURBO = 27;
    private static final int C_TTURBO = 28;
    // The Edit pill beside the eye; appended so no saved id shifts.
    private static final int C_EDIT = 29;
    private static final int CONTROLS = 30;
    // The ids the saved layout uses; never rename one.
    private static final String[] CONTROL_IDS = {
        "lstick", "cstick", "dpad", "a", "b", "x", "y", "l", "r", "z", "visor", "beam", "start",
        "menu", "map", "eye", "rstick", "jump", "fire", "morph", "missile", "lt", "lb", "rt", "rb",
        "tz", "tr", "turbo", "tturbo", "edit",
    };
    private static final String[] CONTROL_NAMES = {
        "Left stick", "C-stick", "D-pad", "A", "B", "X", "Y", "L", "R", "Z", "Visor", "Beam",
        "Start", "Menu", "Map", "Hide", "Right stick", "Jump", "Fire", "Morph", "Missile",
        "LT Lock", "LB Jump", "RT Fire", "RB Missile", "Map (Z)", "R",
        "Turbo", "Turbo", "Edit",
    };
    private static final float MIN_SCALE = 0.5f;
    private static final float MAX_SCALE = 2.5f;
    private static final float SCALE_STEP = 0.1f;
    // A saved offset beyond this many layout heights is junk, not a layout.
    private static final float MAX_OFFSET = 4f;
    // The editor's toolbar: -, +, Reset, Reset all, Layout (GameCube <-> twin stick), Done.
    private static final String[] EDIT_LABELS = {"−", "+", "Reset", "Reset all", "Layout",
                                                 "Done"};
    private static final float[] EDIT_WIDTHS_DP = {48f, 48f, 72f, 96f, 80f, 72f};
    private static final float EDIT_BAR_HEIGHT_DP = 44f;
    private static final float EDIT_BAR_GAP_DP = 6f;
    private static final float EDIT_GRAB_DP = 8f; // slack round a control's bounds
    private static final int EDIT_MINUS = 0;
    private static final int EDIT_PLUS = 1;
    private static final int EDIT_RESET = 2;
    private static final int EDIT_RESET_ALL = 3;
    private static final int EDIT_LAYOUT = 4;
    private static final int EDIT_DONE = 5;

    private final float[] ovDx = new float[CONTROLS];
    private final float[] ovDy = new float[CONTROLS];
    private final float[] ovScale = new float[CONTROLS];
    // The layout string last read from or written to the native config.
    private String layoutText = "";
    // F1's "Edit layout" was pressed; the editor opens once the overlay is gone.
    private boolean editPending;
    private boolean editing;
    private int selected = -1;
    private int dragPointer = -1;
    private int pinchPointer = -1;
    private int editToolPointer = -1;
    private float dragLastX;
    private float dragLastY;
    private float pinchStartDist;
    private float pinchStartScale;
    private final RectF[] editBar = new RectF[EDIT_LABELS.length];
    private final RectF editRect = new RectF();
    private final RectF clampRect = new RectF();
    private final Paint editPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private DashPathEffect editDashes;

    private ControlButton[] face = GAMECUBE_FACE;
    private PillButton[] pills = GAMECUBE_PILLS;
    // The GameCube pad's colours, off by default (plain translucent buttons);
    // an F1 setting, so it's re-read every draw.
    private boolean colored;
    // Each button's function (Fire, Jump...) drawn with its letter.
    private boolean labels = true;
    // F1's "Floating left stick", re-read every draw: the left stick is hidden
    // until a free touch on the left half, then centred at (floatX, floatY), where
    // that finger came down (pulled in so the ring stays on screen).
    private boolean floating;
    private float floatX;
    private float floatY;
    // The held left stick is a floating one (stays so if the map opens meanwhile).
    private boolean leftFloated;
    // F1 settings too, in px: every control's gap to the side edges, and the
    // left stick's on top of it.
    private float sideMargin;
    private float stickInset;
    private float buttonInset;
    // Classic GameCube layout (F1 setting, re-read every draw): the C-stick is
    // drawn and grabs presses in its zone.
    private boolean cStick;
    // Twin stick (Remastered) layout, also an F1 setting re-read every draw: the
    // right stick is drawn (cStick is on too) but aims, with Jump/Fire/Morph/Missile.
    private boolean twin;
    // A drag on the free area aims (mouse-style). Opt-in on the native side on
    // MP2 (MP2_TOUCH_AIM=1), re-read every draw.
    private boolean aim;

    private final Paint fillPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint strokePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Map<Integer, TouchTarget> targets = new HashMap<>();
    private final Map<Integer, Integer> held = new HashMap<>();
    // Fingers passed on to SDL; see forwardToSdl.
    private final Set<Integer> forwarded = new HashSet<>();
    private final RectF hideBounds = new RectF();
    private final RectF editBounds = new RectF();
    private final Path shapePath = new Path();
    private final Path crossArmPath = new Path();
    private static native boolean nativeDebugOverlayVisible();
    private static native boolean nativeTouchClassic();
    private static native boolean nativeTouchTwinStick();
    private static native boolean nativeTouchColors();
    private static native boolean nativeTouchLabels();
    private static native boolean nativeTouchFloatingStick();
    // F1's side margin (every control) and the left stick's extra inset, in dp.
    private static native float nativeTouchSideMarginDp();
    private static native float nativeTouchStickInsetDp();
    // The face buttons' and C-stick's extra inset from the right edge, in dp.
    private static native float nativeTouchButtonInsetDp();
    // F1's opacity for the gameplay controls, in percent (10-100).
    private static native float nativeTouchOpacity();
    // The saved per-control layout (`<id>:<dx>,<dy>,<scale>;...`), and its writer, which
    // also saves F1's config.
    private static native String nativeTouchLayout();
    private static native void nativeSetTouchLayout(String layout);
    // True once after F1's "Edit layout" was pressed.
    private static native boolean nativeTouchEditRequested();
    // Asks for the same editor F1's "Edit layout" does, for a phone with no F1 touch.
    private static native void nativeRequestTouchEdit();
    // Switches GameCube <-> twin stick and clears the per-control offsets.
    private static native void nativeCycleTouchLayout();
    private static native boolean nativeTouchAimEnabled();
    private static native void nativeTouchAim(float dxDp, float dyDp);
    private static native void nativeTouchAimDown(boolean down);
    // A quick swipe on the aim drag (trackSwipe); the native side keeps the aim live a moment.
    private static native void nativeTouchSpringSwipe();
    private static native void nativeSetTouchDevice(boolean xboxLayout);
    private static native void nativeVirtualButton(int button, boolean down);
    private static native void nativeVirtualAxis(int axis, float value);
    private static native boolean nativeTakePhysicalInput();
    private int leftPointer = -1;
    private int rightPointer = -1;
    private int aimPointer = -1;
    private boolean hidden;
    // Hidden because a real pad, keyboard or mouse was used. Unlike HIDE, which
    // leaves a SHOW button, nothing is drawn and any touch brings them back.
    private boolean autoHidden;
    private boolean lastOverlayVisible;

    private final Runnable physicalInputPoll = new Runnable() {
        @Override
        public void run() {
            // Always drained, so input from while the controls were already
            // hidden cannot hide them again the moment they come back.
            if (nativeTakePhysicalInput() && !hidden && !autoHidden && !editing) {
                autoHidden = true;
                releaseAll();
            }
            if (nativeTouchEditRequested()) {
                editPending = true;
            }
            // The overlay can open without this view hearing of it (a pad, or
            // F1 while the first frames are slow enough that a one-off
            // redraw ran before the toggle landed), so redraw on any change.
            final boolean overlayVisible = nativeDebugOverlayVisible();
            if (overlayVisible != lastOverlayVisible) {
                lastOverlayVisible = overlayVisible;
                invalidate();
            }
            // F1 closes itself on the request; open the editor once it has.
            if (editPending && !overlayVisible) {
                editPending = false;
                beginEdit();
            } else if (editing && overlayVisible) {
                // The overlay was opened some other way (a pad's chord): keep what was done.
                endEdit(true);
            }
            postDelayed(this, PHYSICAL_INPUT_POLL_MS);
        }
    };

    TouchControlsView(Context context) {
        super(context);
        setClickable(true);
        setFocusable(false);
        fillPaint.setStyle(Paint.Style.FILL);
        strokePaint.setStyle(Paint.Style.STROKE);
        strokePaint.setStrokeWidth(dp(2));
        textPaint.setColor(Color.WHITE);
        textPaint.setTextAlign(Paint.Align.CENTER);
        textPaint.setFakeBoldText(true);
        editPaint.setStyle(Paint.Style.STROKE);
        editPaint.setStrokeWidth(dp(2));
        Arrays.fill(ovScale, 1f);
        for (int i = 0; i < editBar.length; ++i) {
            editBar[i] = new RectF();
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (!editing) {
            syncLayout();
        }
        // The debug overlay is drawn into the surface below; stay out of its way.
        // It is usually closed from its own Close button, which this view never
        // hears about, so keep checking; otherwise the controls stayed invisible
        // until the next touch happened to redraw them.
        if (nativeDebugOverlayVisible() && !editing) {
            postInvalidateDelayed(150);
            return;
        }
        if ((autoHidden || hidden) && !editing) {
            return;
        }

        float width = getWidth();
        float height = getHeight();
        // The classic setting can change mid-session, so re-check it every draw.
        final boolean classic = nativeTouchClassic();
        colored = nativeTouchColors();
        labels = nativeTouchLabels();
        floating = nativeTouchFloatingStick();
        sideMargin = dp(nativeTouchSideMarginDp());
        stickInset = dp(nativeTouchStickInsetDp());
        buttonInset = dp(nativeTouchButtonInsetDp());
        // F1's opacity for the on-screen controls: the gameplay controls (and the
        // layout editor over them) fade together.
        final float touchOpacity = editing ? 100f : nativeTouchOpacity();
        final int controlsAlpha = Math.round(touchOpacity * 2.55f);
        // 255 means no layer at all, the default, so nothing changes (and nothing
        // is paid for) at 100%.
        final int savedLayer = controlsAlpha < 255
                ? canvas.saveLayerAlpha(0f, 0f, width, height, controlsAlpha) : -1;
        try {
            drawControls(canvas, width, height, classic);
        } finally {
            // Every path back out of the controls, including an exception, puts the
            // canvas back.
            if (savedLayer >= 0) {
                canvas.restoreToCount(savedLayer);
            }
        }
    }

    // The gameplay controls and the layout editor.
    private void drawControls(Canvas canvas, float width, float height, boolean classic) {
        // Classic and twin stick are exclusive (the native side keeps them so).
        twin = !classic && nativeTouchTwinStick();
        face = twin ? TWIN_FACE : GAMECUBE_FACE;
        pills = twin ? TWIN_PILLS : GAMECUBE_PILLS;
        aim = nativeTouchAimEnabled();
        // MP2 looks and switches beams and visors with the C-stick, so without
        // drag-to-aim the default layout draws it as well.
        cStick = classic || twin || !aim;
        if (!editing) {
            // A layout made on another screen, or before a rotation or resize,
            // must not leave a control off screen where it can't be grabbed.
            for (int i = 0; i < CONTROLS; ++i) {
                if (editShown(i) && (ovDx[i] != 0f || ovDy[i] != 0f || ovScale[i] != 1f)) {
                    clampToScreen(i, width, height);
                }
            }
        }
        bottomButtonRect(0, true, width, height, hideBounds);
        bottomButtonRect(1, true, width, height, editBounds);
        if (leftPointer != -1 && leftFloated) {
            drawStick(canvas, floatX, floatY, leftStickRadius(height), leftPointer, 0);
        } else if (!floatingStick()) {
            drawStick(canvas, leftStickX(width, height), leftStickY(width, height),
                      leftStickRadius(height), leftPointer, 0);
        }
        // The right stick is the C-stick, yellow on the GameCube pad, in the classic
        // layout; in twin stick it aims, plain like the left. Otherwise a drag
        // anywhere free aims.
        if (cStick) {
            drawStick(canvas, rightStickX(width, height), rightStickY(height),
                      rightStickRadius(height), rightPointer, colored && !twin ? GC_YELLOW : 0);
        }

        for (PillButton pill : pills) {
            drawPillButton(canvas, pill, width, height);
        }
        for (ControlButton button : face) {
            drawButton(canvas, button, width, height);
        }
        drawDpad(canvas, width, height);
        drawEye(canvas, hideBounds);
        drawEditButton(canvas, editBounds);
        if (editing) {
            drawEditOverlay(canvas, width, height);
        }
    }

    // The square around bottom button `slot`, counted from the side edge: 0 is
    // the eye on the right and START on the left.
    private void bottomButtonRect(int slot, boolean right, float width, float height,
                                  RectF out) {
        final float d = 2f * dp(BOTTOM_BUTTON_RADIUS_DP);
        final float fromSide = sideMargin + dp(BOTTOM_BUTTON_SIDE_DP) + slot * (d + dp(BOTTOM_BUTTON_GAP_DP));
        final float left = right ? width - fromSide - d : fromSide;
        final float bottom = height - dp(BOTTOM_BUTTON_BOTTOM_DP);
        out.set(left, bottom - d, left + d, bottom);
        // The Edit pill (right, slot 1) has its own override so it can be moved and scaled
        // without dragging the eye along.
        applyOverride(!right ? C_START : slot == 0 ? C_EYE : C_EDIT, out, height);
    }

    // Moves and scales a control's default rect by its override, round its centre.
    private void applyOverride(int control, RectF rect, float height) {
        if (ovDx[control] == 0f && ovDy[control] == 0f && ovScale[control] == 1f) {
            return; // the default layout stays bit-for-bit what it was
        }
        final float u = layoutU(height);
        final float cx = rect.centerX() + ovDx[control] * u;
        final float cy = rect.centerY() + ovDy[control] * u;
        final float halfW = rect.width() * 0.5f * ovScale[control];
        final float halfH = rect.height() * 0.5f * ovScale[control];
        rect.set(cx - halfW, cy - halfH, cx + halfW, cy + halfH);
    }

    // A mouse is not a finger on the overlay. Its clicks are dispatched as
    // touches, and hover goes to the topmost hoverable view (this one, being
    // clickable), so without these the SDL surface below saw neither and a
    // click landed on whatever on-screen button was under the cursor. Declining
    // passes them down to the surface. Once SDL captures the pointer for mouse
    // aim, events go to the focused surface and never reach here.
    private static boolean fromMouse(MotionEvent event) {
        return event.getToolType(event.getActionIndex()) == MotionEvent.TOOL_TYPE_MOUSE;
    }

    @Override
    public boolean onHoverEvent(MotionEvent event) {
        return !fromMouse(event) && super.onHoverEvent(event);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        // The editor takes every touch, a mouse's included: nothing reaches the game.
        if (editing) {
            return onEditTouch(event);
        }
        if (fromMouse(event)) {
            return false;
        }
        int action = event.getActionMasked();
        int actionIndex = event.getActionIndex();

        // While the debug overlay is open the game is paused and the touches
        // are for it, so decline them and let the SDL surface below have them.
        // Anything still held has to go first: once this view stops claiming
        // touches the matching releases never arrive, which left whatever was
        // down (a trigger, say) held for the rest of the session.
        final boolean overlayVisible = nativeDebugOverlayVisible();
        if (overlayVisible && (!targets.isEmpty() || !held.isEmpty())) {
            releaseAll();
        }
        if (action == MotionEvent.ACTION_DOWN) {
            cancelForwarded(event);
            if (overlayVisible) {
                return false;
            }
        } else if (forwardToSdl(event, action, actionIndex, overlayVisible) || overlayVisible) {
            return true;
        }

        if (autoHidden) {
            // The touch that brings them back is not also a press.
            if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
                autoHidden = false;
                nativeTakePhysicalInput();
                nativeSetTouchDevice(twin);
                invalidate();
            }
            return true;
        }

        if (hidden) {
            // Any touch brings the controls back, and is not also a press.
            if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
                setHidden(false);
                performClick();
            }
            return true;
        }

        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            // The in-game prompts follow the input the player reached for, so
            // tell the port which set this overlay is showing.
            nativeSetTouchDevice(twin);
            int pointerId = event.getPointerId(actionIndex);
            float x = event.getX(actionIndex);
            float y = event.getY(actionIndex);
            if (hideBounds.contains(x, y)) {
                targets.put(pointerId, new TouchTarget(HIDE, 0));
                return true;
            }
            if (editBounds.contains(x, y)) {
                targets.put(pointerId, new TouchTarget(EDIT, 0));
                return true;
            }
            assignPointer(pointerId, x, y);
        } else if (action == MotionEvent.ACTION_MOVE) {
            for (int i = 0; i < event.getPointerCount(); ++i) {
                TouchTarget target = targets.get(event.getPointerId(i));
                if (target == null) {
                    continue;
                }
                if (target.type == LEFT_STICK || target.type == RIGHT_STICK) {
                    updateStick(target, event.getX(i), event.getY(i));
                } else if (target.type == AIM || target.aiming) {
                    updateAim(target, event, i);
                } else if (target.type == BUTTON && aimsWhileHeld(target) && aim &&
                           aimPointer == -1) {
                    startButtonAim(event.getPointerId(i), target, event, i);
                }
            }
        } else if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) {
            releasePointer(event.getPointerId(actionIndex));
        } else if (action == MotionEvent.ACTION_CANCEL) {
            releaseAll();
        }
        invalidate();
        return true;
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        postDelayed(physicalInputPoll, PHYSICAL_INPUT_POLL_MS);
    }

    @Override
    protected void onDetachedFromWindow() {
        removeCallbacks(physicalInputPoll);
        // No more touch events will arrive to release what is held.
        releaseAll();
        super.onDetachedFromWindow();
    }

    @Override
    public boolean performClick() {
        super.performClick();
        return true;
    }

    void releaseAll() {
        for (TouchTarget target : targets.values()) {
            releaseTarget(target);
        }
        targets.clear();
        held.clear();
        leftPointer = -1;
        rightPointer = -1;
        aimPointer = -1;
        invalidate();
    }

    // Android sends a new finger to the view that already owns the gesture,
    // without asking whether it wants it. So while any finger that went down on
    // these controls is still held (a thumb resting on the stick, or the one that
    // tapped START), every tap on the debug overlay lands here rather than on the
    // SDL surface, and the overlay seems frozen until that finger lifts. Fingers
    // that go down while the overlay is open are handed to SDL directly instead,
    // and keep going there until they lift, even if the overlay closes first:
    // SDL drives its touch mouse from the first finger down and ignores every
    // other finger until that one comes up.
    //
    // Returns whether the event was entirely such fingers.
    private boolean forwardToSdl(MotionEvent event, int action, int actionIndex,
                                 boolean toGame) {
        final int pointerId = event.getPointerId(actionIndex);
        switch (action) {
            case MotionEvent.ACTION_POINTER_DOWN:
                if (!toGame) {
                    return false;
                }
                forwarded.add(pointerId);
                sendToSdl(event, actionIndex, MotionEvent.ACTION_DOWN);
                return true;
            case MotionEvent.ACTION_POINTER_UP:
            case MotionEvent.ACTION_UP:
                if (!forwarded.remove(pointerId)) {
                    return false;
                }
                sendToSdl(event, actionIndex, MotionEvent.ACTION_UP);
                return true;
            case MotionEvent.ACTION_MOVE: {
                boolean all = true;
                for (int i = 0; i < event.getPointerCount(); ++i) {
                    if (forwarded.contains(event.getPointerId(i))) {
                        sendToSdl(event, i, MotionEvent.ACTION_MOVE);
                    } else {
                        all = false;
                    }
                }
                return all;
            }
            case MotionEvent.ACTION_CANCEL:
                cancelForwarded(event);
                return false;
            default:
                return false;
        }
    }

    // A gesture that ends without its releases (cancelled, or a new one begun)
    // must still lift SDL's fingers, or its touch mouse stays stuck on them.
    private void cancelForwarded(MotionEvent event) {
        if (forwarded.isEmpty()) {
            return;
        }
        for (int pointerId : forwarded) {
            SDLActivity.onNativeTouch(event.getDeviceId(), pointerId, MotionEvent.ACTION_CANCEL,
                                      0f, 0f, 0f);
        }
        forwarded.clear();
    }

    private void sendToSdl(MotionEvent event, int index, int action) {
        // SDL wants the position normalised to its surface, which fills this
        // view's parent; this view does not while it is shrunk to SHOW.
        View parent = (View) getParent();
        if (parent == null) {
            return; // a late touch during teardown
        }
        float x = (getLeft() + event.getX(index)) / Math.max(1, parent.getWidth() - 1);
        float y = (getTop() + event.getY(index)) / Math.max(1, parent.getHeight() - 1);
        float pressure = Math.min(event.getPressure(index), 1f);
        SDLActivity.onNativeTouch(event.getDeviceId(), event.getPointerId(index), action, x, y,
                                  pressure);
    }

    // Touch-down routing, first match wins: pills, face buttons, the D-pad, the
    // left stick, the C-stick zone, and the free area aims.
    private void assignPointer(int pointerId, float x, float y) {
        float width = getWidth();
        float height = getHeight();
        for (PillButton pill : pills) {
            pillRect(pill, width, height, pillHit);
            if (pillHit.contains(x, y)) {
                final TouchTarget target = new TouchTarget(BUTTON, pill.id());
                target.control = pill;
                targets.put(pointerId, target);
                pressControl(target.id);
                return;
            }
        }
        for (ControlButton button : face) {
            if (hitButton(button, x, y, width, height)) {
                final TouchTarget target = TouchTarget.begin(BUTTON, button.button, x, y);
                target.control = button;
                targets.put(pointerId, target);
                pressControl(target.id);
                return;
            }
        }
        int dpadButton = dpadButtonAt(x, y, width, height);
        if (dpadButton != -1) {
            targets.put(pointerId, new TouchTarget(BUTTON, dpadButton));
            pressControl(dpadButton);
            return;
        }

        final boolean floatHere = floatingStick() && x < width * 0.5f;
        if ((floatHere || (!floatingStick() && onLeftStick(x, y, width, height))) &&
            leftPointer == -1) {
            if (floatHere) {
                final float radius = leftStickRadius(height);
                floatX = Math.max(radius, Math.min(width - radius, x));
                floatY = Math.max(radius, Math.min(height - radius, y));
            }
            TouchTarget target = new TouchTarget(LEFT_STICK, 0);
            leftPointer = pointerId;
            leftFloated = floatHere;
            targets.put(pointerId, target);
            updateStick(target, x, y);
        } else if (cStick && inRightStickGrab(x, y, width, height)) {
            if (rightPointer == -1) {
                TouchTarget target = new TouchTarget(RIGHT_STICK, 0);
                rightPointer = pointerId;
                targets.put(pointerId, target);
                updateStick(target, x, y);
            }
        } else if (aim && aimPointer == -1) {
            // Everything else that is free: one finger at a time aims.
            aimPointer = pointerId;
            targets.put(pointerId, TouchTarget.begin(AIM, 0, x, y));
            nativeTouchAimDown(true);
        }
    }

    // The floating left stick, except in the layout editor, which places the fixed one.
    private boolean floatingStick() {
        return floating && !editing;
    }

    // A press within half a stick radius outside the left stick's ring grabs it.
    private boolean onLeftStick(float x, float y, float width, float height) {
        final float dx = x - leftStickX(width, height);
        final float dy = y - leftStickY(width, height);
        final float reach = leftStickRadius(height) * LEFT_STICK_REACH;
        return dx * dx + dy * dy <= reach * reach;
    }

    // The layout height: the view's, capped at a phone's. Fractions of the
    // height are sizes and offsets from it; positions go through the layout
    // helpers below, which keep a fraction's distance to the nearest edge,
    // scaled by u / height.
    private float layoutU(float height) {
        return Math.min(height, dp(MAX_LAYOUT_DP));
    }

    private float layoutScale(float height) {
        return height > 0f ? layoutU(height) / height : 1f;
    }

    // A horizontal fraction of the width, from the nearest side edge, plus the
    // side margin.
    private float layoutX(float fraction, float width, float height) {
        final float s = layoutScale(height);
        return fraction < 0.5f ? sideMargin + fraction * width * s
                               : fraction * width + (1f - fraction) * width * (1f - s) - sideMargin;
    }

    // A vertical fraction of the height. The layout sits in a phone-height band
    // at the bottom of the screen, so the top half is still in reach.
    private float layoutY(float fraction, float height) {
        return fraction < 0.5f ? (height - layoutU(height)) + fraction * layoutU(height)
                               : layoutYFromBottom(fraction, height);
    }

    private float layoutYFromBottom(float fraction, float height) {
        return fraction * height + (1f - fraction) * (height - layoutU(height));
    }

    private final RectF stickFaceRect = new RectF();

    // Mirrors the face cluster's gap to the right edge (side margin included,
    // button inset not), plus the stick inset, so the two thumbs' controls look
    // balanced.
    // The default position, without the override: the Visor button, the face
    // cluster and the pill shift hang off it, so moving the stick moves none of them.
    private float baseLeftStickX(float width, float height) {
        faceBounds(width, height, stickFaceRect);
        return (width - stickFaceRect.right - buttonInset) + stickInset +
               STICK_RADIUS * layoutU(height);
    }

    // Level with the face cluster's middle.
    private float baseLeftStickY(float width, float height) {
        faceBounds(width, height, stickFaceRect);
        return stickFaceRect.centerY();
    }

    private float leftStickX(float width, float height) {
        return baseLeftStickX(width, height) + ovDx[C_LSTICK] * layoutU(height);
    }

    private float leftStickY(float width, float height) {
        return baseLeftStickY(width, height) + ovDy[C_LSTICK] * layoutU(height);
    }

    private float leftStickRadius(float height) {
        return layoutU(height) * STICK_RADIUS * ovScale[C_LSTICK];
    }

    // The control id the right stick's override is stored under.
    private int rightControl() {
        return twin ? C_RSTICK : C_CSTICK;
    }

    private float baseRightStickX(float width, float height) {
        return width - sideMargin - buttonInset - layoutU(height) * GC_CSTICK_FROM_RIGHT;
    }

    private float rightStickX(float width, float height) {
        return baseRightStickX(width, height) + ovDx[rightControl()] * layoutU(height);
    }

    // Twin's stick is level with the left one, which sits at the face cluster's
    // middle: A's spot, the diamond's centre. The C-stick sits lower, as on the pad.
    private float baseRightStickY(float height) {
        return layoutY(twin ? GC_A_Y : GC_CSTICK_Y, height);
    }

    private float rightStickY(float height) {
        return baseRightStickY(height) + ovDy[rightControl()] * layoutU(height);
    }

    // The default radius in layout units: twin's aiming stick matches the left
    // stick, the C-stick is the GameCube pad's small one.
    private float rightStickBaseRadius() {
        return twin ? STICK_RADIUS : GC_CSTICK_RADIUS;
    }

    private float rightStickRadius(float height) {
        return layoutU(height) * rightStickBaseRadius() * ovScale[rightControl()];
    }

    // Whether (x, y) is in the area that grabs the right stick. By default the
    // right edge is the stick's reach (face buttons are hit-tested first, so it
    // can pass them), the left edge a phone's 0.38 of the width with the reach
    // scaled down on a big screen, and the top 0.43 of the height from the
    // bottom. The moved stick takes the whole area along, scaled by its size, so
    // no grab is left at the old place.
    private boolean inRightStickGrab(float x, float y, float width, float height) {
        final float u = layoutU(height);
        final float scale = ovScale[rightControl()];
        final float baseCy = baseRightStickY(height);
        final float baseRight = baseRightStickX(width, height) + u * rightStickBaseRadius() * 1.6f;
        final float baseLeft = baseRight - (baseRight - width * 0.38f) * layoutScale(height);
        final float baseTop = layoutYFromBottom(0.43f, height);
        final float cy = rightStickY(height);
        final float right = rightStickX(width, height) + rightStickRadius(height) * 1.6f;
        return x >= right - (baseRight - baseLeft) * scale && x < right &&
               y > cy - (baseCy - baseTop) * scale && y <= cy + (height - baseCy) * scale;
    }

    // Fire and Jump face buttons aim when slid (see startButtonAim). Pills
    // and the D-pad don't: their targets carry no start point.
    private static boolean aimsWhileHeld(TouchTarget target) {
        return target.control instanceof ControlButton &&
               (target.id == BTN_SOUTH || target.id == BTN_EAST);
    }

    // A held Fire (charging) or Jump that slides past the tap slop aims
    // too, so one thumb can press and aim; the button stays held until the
    // finger lifts.
    private void startButtonAim(int pointerId, TouchTarget target, MotionEvent event, int index) {
        if (Math.hypot(event.getX(index) - target.startX, event.getY(index) - target.startY) <
            dp(MAP_TAP_SLOP_DP)) {
            return;
        }
        target.aiming = true;
        target.x = event.getX(index);
        target.y = event.getY(index);
        aimPointer = pointerId;
        nativeTouchAimDown(true);
    }

    // Sends the finger's travel since the last event, in dp, through every
    // historical sample so a fast swipe stays smooth.
    private void updateAim(TouchTarget target, MotionEvent event, int index) {
        final float density = getResources().getDisplayMetrics().density;
        float lastX = target.x;
        float lastY = target.y;
        final int history = event.getHistorySize();
        for (int h = 0; h <= history; ++h) {
            final float x = h < history ? event.getHistoricalX(index, h) : event.getX(index);
            final float y = h < history ? event.getHistoricalY(index, h) : event.getY(index);
            nativeTouchAim((x - lastX) / density, (y - lastY) / density);
            trackSwipe(target, x / density, y / density,
                       h < history ? event.getHistoricalEventTime(h) : event.getEventTime());
            lastX = x;
            lastY = y;
        }
        target.x = lastX;
        target.y = lastY;
    }

    // A quick swipe up while aiming tells the native side to keep the aim live for a moment
    // (PortMouse::SpringSwipe), so a flick turns the view by its whole travel. On MP2 it does
    // not spring the morph ball. A stroke starts where the finger last moved down or stalled;
    // it counts once it has gone SWIPE_UP_DP up, more up than sideways, within SWIPE_UP_MS.
    // Positions are in dp.
    private void trackSwipe(TouchTarget target, float x, float y, long timeMs) {
        if (target.swipeMs < 0 || y >= target.swipeY || timeMs - target.swipeMs > SWIPE_UP_MS) {
            target.swipeX = x;
            target.swipeY = y;
            target.swipeMs = timeMs;
            target.swiped = false;
            return;
        }
        final float up = target.swipeY - y;
        if (!target.swiped && up >= SWIPE_UP_DP && Math.abs(x - target.swipeX) < up) {
            target.swiped = true;
            nativeTouchSpringSwipe();
        }
    }

    private void updateStick(TouchTarget target, float x, float y) {
        target.x = x;
        target.y = y;
        final boolean left = target.type == LEFT_STICK;
        float width = getWidth();
        float height = getHeight();
        // A floating stick keeps the centre it was grabbed at, even if the map
        // opens while it is held.
        final boolean floated = left && leftFloated;
        float centreX = floated ? floatX : left ? leftStickX(width, height) : rightStickX(width, height);
        float centreY = floated ? floatY : left ? leftStickY(width, height) : rightStickY(height);
        float radius = left ? leftStickRadius(height) : rightStickRadius(height);
        // SDL's gamepad axes are +X right and +Y *down* (Aurora inverts Y for the
        // GameCube stick, whose +Y is up), so screen coordinates apply as-is.
        float dx = (x - centreX) / radius;
        float dy = (y - centreY) / radius;
        float length = (float) Math.hypot(dx, dy);
        if (length > 1f) {
            dx /= length;
            dy /= length;
            length = 1f;
        }
        if (length < STICK_DEAD_ZONE) {
            dx = 0f;
            dy = 0f;
        }
        nativeVirtualAxis(left ? AXIS_LEFTX : AXIS_RIGHTX, dx);
        nativeVirtualAxis(left ? AXIS_LEFTY : AXIS_RIGHTY, dy);
    }

    private void releasePointer(int pointerId) {
        TouchTarget target = targets.remove(pointerId);
        if (target == null) {
            return;
        }
        if (target.type == HIDE) {
            setHidden(true);
            performClick();
            return;
        }
        if (target.type == EDIT) {
            // Poll now rather than wait an interval; beginEdit releases everything held.
            nativeRequestTouchEdit();
            if (nativeTouchEditRequested()) {
                editPending = true;
            }
            return;
        }
        releaseTarget(target);
        if (pointerId == leftPointer) {
            leftPointer = -1;
        }
        if (pointerId == rightPointer) {
            rightPointer = -1;
        }
        if (pointerId == aimPointer) {
            aimPointer = -1;
        }
    }

    private void releaseTarget(TouchTarget target) {
        // Nothing to zero for a hide tap or an aim drag (a distance).
        if (target.type == AIM) {
            nativeTouchAimDown(false);
            return;
        }
        if (target.type == HIDE || target.type == EDIT) {
            return;
        }
        if (target.type == BUTTON) {
            releaseControl(target.id);
            if (target.aiming) {
                nativeTouchAimDown(false);
            }
            return;
        }
        final boolean left = target.type == LEFT_STICK;
        nativeVirtualAxis(left ? AXIS_LEFTX : AXIS_RIGHTX, 0f);
        nativeVirtualAxis(left ? AXIS_LEFTY : AXIS_RIGHTY, 0f);
    }

    private void pressControl(int id) {
        int count = held.containsKey(id) ? held.get(id) : 0;
        if (count == 0) {
            if (id >= AXIS_ID_BASE) {
                nativeVirtualAxis(id - AXIS_ID_BASE, 1f);
            } else {
                nativeVirtualButton(id, true);
            }
        }
        held.put(id, count + 1);
    }

    private void releaseControl(int id) {
        Integer current = held.get(id);
        if (current == null) {
            return;
        }
        if (current <= 1) {
            held.remove(id);
            if (id >= AXIS_ID_BASE) {
                // Triggers are axes, and SDL's joystick axes run -32768..32767
                // with a trigger resting at the minimum: releasing with 0 left
                // the trigger half pressed, so the game stayed locked on (and
                // strafing) after the player let go.
                nativeVirtualAxis(id - AXIS_ID_BASE, -1f);
            } else {
                nativeVirtualButton(id, false);
            }
        } else {
            held.put(id, current - 1);
        }
    }

    // Hidden, the view stays full screen but draws nothing, and the next
    // touch anywhere brings the controls back (as after a physical pad).
    private void setHidden(boolean hide) {
        releaseAll();
        hidden = hide;
        invalidate();
    }

    // color is the base's RGB, or 0 for the overlay's own.
    private void drawStick(Canvas canvas, float x, float y, float radius, int pointerId,
                           int color) {
        boolean active = pointerId != -1;
        if (color != 0) {
            fillPaint.setColor((active ? 0x88000000 : 0x55000000) | color);
        } else {
            fillPaint.setColor(active ? 0x8848C8E8 : 0x66081218);
        }
        strokePaint.setColor(active ? 0xFFE1F8FF : 0xAAFFFFFF);
        canvas.drawCircle(x, y, radius, fillPaint);
        canvas.drawCircle(x, y, radius, strokePaint);
        float knobX = x;
        float knobY = y;
        TouchTarget target = targets.get(pointerId);
        if (target != null) {
            float dx = target.x - x;
            float dy = target.y - y;
            float distance = (float) Math.hypot(dx, dy);
            float limit = radius * 0.58f;
            if (distance > limit) {
                dx *= limit / distance;
                dy *= limit / distance;
            }
            knobX += dx;
            knobY += dy;
        }
        canvas.drawCircle(knobX, knobY, radius * 0.42f, strokePaint);
    }

    private float centreX(ControlButton button, float width, float height) {
        return button.anchored
                   ? width - sideMargin - buttonInset - (GC_A_FROM_RIGHT - button.x) * layoutU(height)
                   : layoutX(button.x, width, height);
    }

    private float centreY(ControlButton button, float height) {
        return layoutY(button.anchored ? GC_A_Y + button.y : button.y, height);
    }

    private int controlOf(ControlButton button) {
        final int first = twin ? C_JUMP : C_A;
        for (int i = 0; i < face.length; ++i) {
            if (face[i] == button) {
                return first + i;
            }
        }
        return first;
    }

    // The button's centre, radius (a kidney's is its arc's) and band half width
    // into out: x, y, radius, half. `moved` applies the override, which scales a
    // kidney round its own middle rather than round its arc's centre; draw and hit
    // test both go through here.
    private void faceGeometry(ControlButton button, float width, float height, boolean moved,
                              float[] out) {
        final float u = layoutU(height);
        float x = centreX(button, width, height);
        float y = centreY(button, height);
        float radius = button.radius * u;
        float half = button.halfWidth * u;
        if (moved) {
            final int control = controlOf(button);
            final float scale = ovScale[control];
            if (button.isKidney()) {
                final double middle = Math.toRadians(button.arcStart + button.arcSweep / 2);
                x += radius * (1f - scale) * (float) Math.cos(middle);
                y += radius * (1f - scale) * (float) Math.sin(middle);
            }
            x += ovDx[control] * u;
            y += ovDy[control] * u;
            radius *= scale;
            half *= scale;
        }
        out[0] = x;
        out[1] = y;
        out[2] = radius;
        out[3] = half;
    }

    private final float[] faceGeo = new float[4];

    private boolean hitButton(ControlButton button, float x, float y, float width, float height) {
        faceGeometry(button, width, height, true, faceGeo);
        float dx = x - faceGeo[0];
        float dy = y - faceGeo[1];
        if (!button.isKidney()) {
            float radius = faceGeo[2];
            return dx * dx + dy * dy <= radius * radius;
        }
        // Distance to the nearest point of the kidney's centre arc.
        double angle = Math.toDegrees(Math.atan2(dy, dx));
        double along = ((angle - button.arcStart) % 360 + 360) % 360;
        if (along > button.arcSweep) {
            along = along - button.arcSweep < 360 - along ? button.arcSweep : 0;
        }
        double nearest = Math.toRadians(button.arcStart + along);
        float ring = faceGeo[2];
        float px = dx - ring * (float) Math.cos(nearest);
        float py = dy - ring * (float) Math.sin(nearest);
        float half = faceGeo[3];
        return px * px + py * py <= half * half;
    }

    // The D-pad direction under (x, y), or -1. The whole square round the cross
    // counts, by the dominant axis, so a thumb that slips off an arm's side or
    // into a corner still presses something; only a small centre is dead.
    private int dpadButtonAt(float x, float y, float width, float height) {
        float dx = x - dpadX(width, height);
        float dy = y - dpadY(height);
        float arm = DPAD_ARM * layoutU(height) * ovScale[C_DPAD];
        float dead = DPAD_HALF * layoutU(height) * ovScale[C_DPAD] * 0.4f;
        if (Math.abs(dx) > arm || Math.abs(dy) > arm || dx * dx + dy * dy < dead * dead) {
            return -1;
        }
        if (Math.abs(dx) > Math.abs(dy)) {
            return dx < 0 ? BTN_DPAD_LEFT : BTN_DPAD_RIGHT;
        }
        return dy < 0 ? BTN_DPAD_UP : BTN_DPAD_DOWN;
    }

    private final RectF armRect = new RectF();

    private float dpadX(float width, float height) {
        return layoutX(DPAD_X, width, height) + ovDx[C_DPAD] * layoutU(height);
    }

    private float dpadY(float height) {
        return layoutY(DPAD_Y, height) + ovDy[C_DPAD] * layoutU(height);
    }

    private void drawDpad(Canvas canvas, float width, float height) {
        float cx = dpadX(width, height);
        float cy = dpadY(height);
        float arm = DPAD_ARM * layoutU(height) * ovScale[C_DPAD];
        float half = DPAD_HALF * layoutU(height) * ovScale[C_DPAD];
        float corner = half * 0.35f;
        shapePath.reset();
        shapePath.addRoundRect(cx - arm, cy - half, cx + arm, cy + half, corner, corner,
                               Path.Direction.CW);
        crossArmPath.reset();
        crossArmPath.addRoundRect(cx - half, cy - arm, cx + half, cy + arm, corner, corner,
                                  Path.Direction.CW);
        shapePath.op(crossArmPath, Path.Op.UNION);
        fillPaint.setColor(colored ? padFill(GC_GREY, false) : 0x77081218);
        canvas.drawPath(shapePath, fillPaint);
        // A held arm lights from the centre out.
        for (int i = 0; i < DPAD_BUTTONS.length; ++i) {
            if (!held.containsKey(DPAD_BUTTONS[i])) {
                continue;
            }
            float endX = cx + DPAD_DX[i] * arm;
            float endY = cy + DPAD_DY[i] * arm;
            armRect.set(
                Math.min(cx, endX) - (DPAD_DX[i] == 0 ? half : 0),
                Math.min(cy, endY) - (DPAD_DY[i] == 0 ? half : 0),
                Math.max(cx, endX) + (DPAD_DX[i] == 0 ? half : 0),
                Math.max(cy, endY) + (DPAD_DY[i] == 0 ? half : 0));
            fillPaint.setColor(colored ? padFill(GC_GREY, true) : 0xCC48C8E8);
            canvas.drawRoundRect(armRect, corner, corner, fillPaint);
        }
        strokePaint.setColor(0xBBFFFFFF);
        canvas.drawPath(shapePath, strokePaint);
        for (int i = 0; i < DPAD_BUTTONS.length; ++i) {
            drawCenteredLabel(canvas, DPAD_LABELS[i], cx + DPAD_DX[i] * arm * 0.62f,
                              cy + DPAD_DY[i] * arm * 0.62f, dp(12) * ovScale[C_DPAD]);
        }
    }

    private final RectF arcRect = new RectF();

    // A band round (cx, cy) along the arc, with round ends: GameCube X and Y.
    private void kidneyPath(Path path, float cx, float cy, float ring, float half,
                                   float start, float sweep) {
        float end = start + sweep;
        float endX = cx + ring * (float) Math.cos(Math.toRadians(end));
        float endY = cy + ring * (float) Math.sin(Math.toRadians(end));
        float startX = cx + ring * (float) Math.cos(Math.toRadians(start));
        float startY = cy + ring * (float) Math.sin(Math.toRadians(start));
        path.reset();
        arcRect.set(cx - ring - half, cy - ring - half, cx + ring + half, cy + ring + half);
        path.arcTo(arcRect, start, sweep, true);
        arcRect.set(endX - half, endY - half, endX + half, endY + half);
        path.arcTo(arcRect, end, 180f, false);
        arcRect.set(cx - ring + half, cy - ring + half, cx + ring - half, cy + ring - half);
        path.arcTo(arcRect, end, -sweep, false);
        arcRect.set(startX - half, startY - half, startX + half, startY + half);
        path.arcTo(arcRect, start + 180f, 180f, false);
        path.close();
    }

    // A GameCube colour as a fill: translucent at rest, lighter and more solid
    // while held.
    private static int padFill(int rgb, boolean active) {
        if (!active) {
            return 0x99000000 | rgb;
        }
        int r = (rgb >> 16) & 0xFF;
        int g = (rgb >> 8) & 0xFF;
        int b = rgb & 0xFF;
        r += (255 - r) * 2 / 5;
        g += (255 - g) * 2 / 5;
        b += (255 - b) * 2 / 5;
        return 0xDD000000 | (r << 16) | (g << 8) | b;
    }

    private final Path facePath = new Path();
    private final RectF faceRect = new RectF();
    private final RectF faceTmp = new RectF();

    // The face buttons' real bounds, kidney arcs included.
    private void faceBounds(float width, float height, RectF out) {
        final float u = layoutU(height);
        boolean first = true;
        for (ControlButton button : face) {
            final float x = centreX(button, width, height);
            final float y = centreY(button, height);
            final float r = button.radius * u;
            if (button.isKidney()) {
                kidneyPath(facePath, x, y, r, button.halfWidth * u, button.arcStart,
                           button.arcSweep);
                facePath.computeBounds(faceTmp, true);
            } else {
                faceTmp.set(x - r, y - r, x + r, y + r);
            }
            if (first) {
                out.set(faceTmp);
                first = false;
            } else {
                out.union(faceTmp);
            }
        }
    }

    // START sits in the bottom-left corner (slot 0); -1 for the pills placed
    // from their rects. The triggers are axes with button -1, hence the axis check.
    private static int cornerSlot(PillButton pill) {
        if (pill.axis >= 0) {
            return -1;
        }
        return pill.button == BTN_START ? 0 : -1;
    }

    // A pill's rect, from its side edge and the top of the layout band, or its
    // corner slot: round buttons mirroring the map button by the eye.
    private void pillRect(PillButton pill, float width, float height, RectF out) {
        final int slot = cornerSlot(pill);
        if (slot >= 0) {
            bottomButtonRect(slot, false, width, height, out);
            return;
        }
        out.set(layoutX(pill.left, width, height), layoutY(pill.top, height),
                layoutX(pill.right, width, height), layoutY(pill.bottom, height));
        applyOverride(pillControl(pill), out, height);
    }

    private static int pillControl(PillButton pill) {
        final int slot = cornerSlot(pill);
        if (slot >= 0) {
            return C_START;
        }
        if (pill.control >= 0) {
            return pill.control;
        }
        if (pill.axis == AXIS_TRIGGER_L) {
            return C_L;
        }
        if (pill.axis == AXIS_TRIGGER_R) {
            return C_R;
        }
        return pill.button == BTN_RIGHT_SHOULDER ? C_Z : C_L;
    }

    // Whether a finger is down on this control itself.
    private boolean pressedControl(Object control) {
        for (TouchTarget target : targets.values()) {
            if (target.control == control) {
                return true;
            }
        }
        return false;
    }

    private void drawPillButton(Canvas canvas, PillButton pill, float width, float height) {
        pillRect(pill, width, height, pillHit);
        boolean active = pressedControl(pill);
        if (colored && pill.color != 0) {
            fillPaint.setColor(padFill(pill.color, active));
        } else {
            fillPaint.setColor(active ? 0xCC48C8E8 : 0x99081218);
        }
        strokePaint.setColor(active ? 0xFFE1F8FF : 0xCCFFFFFF);
        final int slot = cornerSlot(pill);
        if (slot >= 0) {
            drawCornerButton(canvas, pillHit);
            return;
        }
        final RectF bounds = pillHit;
        float radius = Math.min(bounds.width(), bounds.height()) * 0.28f;
        canvas.drawRoundRect(bounds, radius, radius, fillPaint);
        canvas.drawRoundRect(bounds, radius, radius, strokePaint);
        if (twin && pill.axis < 0 && pill.button == BTN_RIGHT_SHOULDER) {
            // The Map pill wears Xbox's Menu glyph (three lines), as the hint does.
            final float h = bounds.height();
            final float gx = labels ? bounds.left + bounds.width() * 0.26f : bounds.centerX();
            final float lineW = h * 0.34f;
            final float lineH = Math.max(1.5f, h * 0.06f);
            final int saved = fillPaint.getColor();
            fillPaint.setColor(strokePaint.getColor());
            for (int i = -1; i <= 1; i++) {
                final float ly = bounds.centerY() + i * h * 0.16f;
                canvas.drawRect(gx - lineW / 2f, ly - lineH / 2f, gx + lineW / 2f,
                                ly + lineH / 2f, fillPaint);
            }
            fillPaint.setColor(saved);
            if (labels) {
                drawCenteredLabel(canvas, pill.label, bounds.left + bounds.width() * 0.62f,
                                  bounds.centerY(), dp(11));
            }
            return;
        }
        // "LB Jump": without descriptions, just the button's name.
        final String label = labels ? pill.label : pill.label.split(" ")[0];
        drawCenteredLabel(canvas, label, bounds.centerX(), bounds.centerY(),
                          label.length() > 2 ? dp(11) : dp(13));
    }

    // A face button's function, drawn small under its letter.
    private String faceFunction(ControlButton button) {
        if (twin) {
            return button.label;
        }
        return GAMECUBE_FUNCTIONS[controlOf(button) - C_A];
    }

    private void drawButton(Canvas canvas, ControlButton button, float width, float height) {
        faceGeometry(button, width, height, true, faceGeo);
        float x = faceGeo[0];
        float y = faceGeo[1];
        float radius = faceGeo[2];
        final float scale = ovScale[controlOf(button)];
        boolean active = pressedControl(button);
        if (colored && button.color != 0) {
            fillPaint.setColor(padFill(button.color, active));
        } else {
            fillPaint.setColor(active ? 0xCC48C8E8 : 0x77081218);
        }
        strokePaint.setColor(active ? 0xFFE1F8FF : 0xBBFFFFFF);
        if (button.isKidney()) {
            kidneyPath(shapePath, x, y, radius, faceGeo[3], button.arcStart, button.arcSweep);
            canvas.drawPath(shapePath, fillPaint);
            canvas.drawPath(shapePath, strokePaint);
            double middle = Math.toRadians(button.arcStart + button.arcSweep / 2);
            x += radius * (float) Math.cos(middle);
            y += radius * (float) Math.sin(middle);
        } else {
            canvas.drawCircle(x, y, radius, fillPaint);
            canvas.drawCircle(x, y, radius, strokePaint);
        }
        // The letter big (twin: the Xbox one), the function small under it.
        final String letter = twin ? TWIN_LETTERS[controlOf(button) - C_JUMP] : button.label;
        final float letterSize = (twin ? dp(17) : dp(15)) * scale;
        if (!labels) {
            drawCenteredLabel(canvas, letter, x, y, letterSize);
            return;
        }
        final String function = faceFunction(button);
        // The function goes below the letter. A kidney is sized by its half width,
        // and a mostly level one (Y) has its text slanted along the band: upright,
        // "Missile" ran into the border of Y's tilted band.
        float size = radius;
        float slant = 0f;
        if (button.isKidney()) {
            size = faceGeo[3] * 1.4f;
            float tangent = button.arcStart + button.arcSweep / 2 + 90f;
            tangent = ((tangent % 180f) + 270f) % 180f - 90f; // into [-90, 90)
            if (Math.abs(tangent) <= 45f) {
                slant = tangent;
            }
        }
        canvas.save();
        canvas.rotate(slant, x, y);
        drawCenteredLabel(canvas, letter, x, y - size * 0.2f, letterSize);
        drawCenteredLabel(canvas, function, x, y + size * 0.52f, dp(8) * scale);
        canvas.restore();
    }

    // The hide button: an eye.
    // START as a pause glyph, in a circle like the eye's. The paints are already
    // set for the held state.
    private void drawCornerButton(Canvas canvas, RectF bounds) {
        final float cx = bounds.centerX();
        final float cy = bounds.centerY();
        final float radius = bounds.width() * 0.5f;
        canvas.drawCircle(cx, cy, radius, fillPaint);
        canvas.drawCircle(cx, cy, radius, strokePaint);
        final float s = radius * 0.36f;
        final int fill = fillPaint.getColor();
        fillPaint.setColor(strokePaint.getColor());
        final float barW = s * 0.55f;
        final float corner = barW * 0.3f;
        canvas.drawRoundRect(cx - s * 0.75f - barW / 2f, cy - s, cx - s * 0.75f + barW / 2f,
                             cy + s, corner, corner, fillPaint);
        canvas.drawRoundRect(cx + s * 0.75f - barW / 2f, cy - s, cx + s * 0.75f + barW / 2f,
                             cy + s, corner, corner, fillPaint);
        fillPaint.setColor(fill);
    }

    private void drawEye(Canvas canvas, RectF bounds) {
        fillPaint.setColor(0x99081218);
        strokePaint.setColor(0xCCFFFFFF);
        float cx = bounds.centerX();
        float cy = bounds.centerY();
        float r = bounds.width() * 0.5f;
        canvas.drawCircle(cx, cy, r, fillPaint);
        canvas.drawCircle(cx, cy, r, strokePaint);
        float halfW = r * 0.62f;
        float lid = r * 0.36f;
        Path eye = shapePath;
        eye.reset();
        eye.moveTo(cx - halfW, cy);
        eye.quadTo(cx, cy - lid * 2f, cx + halfW, cy);
        eye.quadTo(cx, cy + lid * 2f, cx - halfW, cy);
        eye.close();
        canvas.drawPath(eye, strokePaint);
        fillPaint.setColor(0xCCFFFFFF);
        canvas.drawCircle(cx, cy, lid * 0.55f, fillPaint);
    }

    private void drawEditButton(Canvas canvas, RectF bounds) {
        fillPaint.setColor(0x99081218);
        strokePaint.setColor(0xCCFFFFFF);
        float r = bounds.width() * 0.5f;
        canvas.drawCircle(bounds.centerX(), bounds.centerY(), r, fillPaint);
        canvas.drawCircle(bounds.centerX(), bounds.centerY(), r, strokePaint);
        drawCenteredLabel(canvas, "Edit", bounds.centerX(), bounds.centerY(), dp(11));
    }

    private void drawCenteredLabel(Canvas canvas, String label, float x, float y, float size) {
        textPaint.setTextSize(size);
        canvas.drawText(label, x, y - (textPaint.ascent() + textPaint.descent()) / 2, textPaint);
    }

    // The layout is stored natively (the F1 config), so a reset there, or the
    // editor's save, reaches the next draw. Re-parsed only when the text changes.
    private void syncLayout() {
        final String text = nativeTouchLayout();
        if (text != null && !text.equals(layoutText)) {
            layoutText = text;
            loadLayout(text);
        }
    }

    // `id:dx,dy,scale;...`. Unknown ids and malformed entries are skipped, so a
    // layout from a newer version still loads what this one knows.
    private void loadLayout(String text) {
        Arrays.fill(ovDx, 0f);
        Arrays.fill(ovDy, 0f);
        Arrays.fill(ovScale, 1f);
        for (String entry : text.split(";")) {
            final int colon = entry.indexOf(':');
            if (colon <= 0) {
                continue;
            }
            final String id = entry.substring(0, colon).trim();
            int control = -1;
            for (int i = 0; i < CONTROLS; ++i) {
                if (CONTROL_IDS[i].equals(id)) {
                    control = i;
                    break;
                }
            }
            final String[] parts = entry.substring(colon + 1).split(",");
            if (control < 0 || parts.length != 3) {
                continue;
            }
            try {
                final float dx = Float.parseFloat(parts[0].trim());
                final float dy = Float.parseFloat(parts[1].trim());
                final float scale = Float.parseFloat(parts[2].trim());
                if (Float.isNaN(dx) || Float.isInfinite(dx) || Float.isNaN(dy) ||
                    Float.isInfinite(dy) || Float.isNaN(scale) || Float.isInfinite(scale)) {
                    continue;
                }
                ovDx[control] = Math.max(-MAX_OFFSET, Math.min(MAX_OFFSET, dx));
                ovDy[control] = Math.max(-MAX_OFFSET, Math.min(MAX_OFFSET, dy));
                ovScale[control] = Math.max(MIN_SCALE, Math.min(MAX_SCALE, scale));
            } catch (NumberFormatException e) {
                // A malformed entry: leave the control at its default.
            }
        }
    }

    // Only the controls that were changed, so a default layout is the empty string.
    private String serializeLayout() {
        final StringBuilder out = new StringBuilder();
        for (int i = 0; i < CONTROLS; ++i) {
            if (ovDx[i] == 0f && ovDy[i] == 0f && ovScale[i] == 1f) {
                continue;
            }
            if (out.length() > 0) {
                out.append(';');
            }
            out.append(String.format(Locale.ROOT, "%s:%.4f,%.4f,%.3f", CONTROL_IDS[i], ovDx[i],
                                     ovDy[i], ovScale[i]));
        }
        return out.toString();
    }

    private void resetControl(int control) {
        ovDx[control] = 0f;
        ovDy[control] = 0f;
        ovScale[control] = 1f;
    }

    // The editor: every touch is consumed, so nothing reaches the game (F1 does
    // not pause it, so it keeps running underneath). Inputs held on entry are
    // released first so a stick or button isn't left pressed.
    private void beginEdit() {
        releaseAll();
        // Fingers handed to SDL (over the closing overlay) must still be lifted.
        for (int pointerId : forwarded) {
            SDLActivity.onNativeTouch(0, pointerId, MotionEvent.ACTION_CANCEL, 0f, 0f, 0f);
        }
        forwarded.clear();
        hidden = false;
        autoHidden = false;
        editing = true;
        selected = -1;
        dragPointer = -1;
        pinchPointer = -1;
        editToolPointer = -1;
        invalidate();
    }

    private void endEdit(boolean save) {
        if (save) {
            layoutText = serializeLayout();
            nativeSetTouchLayout(layoutText);
        }
        editing = false;
        editPending = false;
        selected = -1;
        dragPointer = -1;
        pinchPointer = -1;
        editToolPointer = -1;
        invalidate();
    }

    // Whether the current settings draw this control.
    private boolean editShown(int control) {
        switch (control) {
            case C_CSTICK:
                return cStick && !twin;
            case C_A:
            case C_B:
            case C_X:
            case C_Y:
            case C_L:
            case C_R:
                return !twin;
            case C_RSTICK:
            case C_JUMP:
            case C_FIRE:
            case C_MORPH:
            case C_MISSILE:
            case C_LT:
            case C_LB:
            case C_RT:
            case C_RB:
                return twin;
            case C_TR:
                return false; // retired: twin's R pill, kept so saved ids don't shift
            case C_TURBO:
            case C_TTURBO:
            case C_VISOR:
            case C_BEAM:
            case C_MENU:
            case C_MAP:
                return false; // MP1's, not drawn on MP2; kept so saved ids don't shift
            case C_Z:
                return !twin;
            case C_TZ:
                return twin;
            default:
                return true;
        }
    }

    private PillButton pillOf(int control) {
        for (PillButton pill : pills) {
            if (cornerSlot(pill) < 0 && pillControl(pill) == control) {
                return pill;
            }
        }
        return null;
    }

    // A control's current bounds: the same geometry the draw and hit test use.
    private void controlBounds(int control, float width, float height, RectF out) {
        final float u = layoutU(height);
        switch (control) {
            case C_LSTICK: {
                final float r = leftStickRadius(height);
                final float x = leftStickX(width, height);
                final float y = leftStickY(width, height);
                out.set(x - r, y - r, x + r, y + r);
                return;
            }
            case C_CSTICK:
            case C_RSTICK: {
                final float r = rightStickRadius(height);
                final float x = rightStickX(width, height);
                final float y = rightStickY(height);
                out.set(x - r, y - r, x + r, y + r);
                return;
            }
            case C_DPAD: {
                final float arm = DPAD_ARM * u * ovScale[C_DPAD];
                final float x = dpadX(width, height);
                final float y = dpadY(height);
                out.set(x - arm, y - arm, x + arm, y + arm);
                return;
            }
            case C_A:
            case C_B:
            case C_X:
            case C_Y:
            case C_JUMP:
            case C_FIRE:
            case C_MORPH:
            case C_MISSILE: {
                final ControlButton button =
                    face[control >= C_JUMP ? control - C_JUMP : control - C_A];
                if (button == null) {
                    out.setEmpty();
                    return;
                }
                faceGeometry(button, width, height, true, faceGeo);
                if (button.isKidney()) {
                    kidneyPath(facePath, faceGeo[0], faceGeo[1], faceGeo[2], faceGeo[3],
                               button.arcStart, button.arcSweep);
                    facePath.computeBounds(out, true);
                } else {
                    out.set(faceGeo[0] - faceGeo[2], faceGeo[1] - faceGeo[2],
                            faceGeo[0] + faceGeo[2], faceGeo[1] + faceGeo[2]);
                }
                return;
            }
            case C_L:
            case C_R:
            case C_Z:
            case C_LT:
            case C_LB:
            case C_RT:
            case C_RB:
            case C_TZ:
            case C_TR: {
                final PillButton pill = pillOf(control);
                if (pill != null) {
                    pillRect(pill, width, height, out);
                } else {
                    out.setEmpty();
                }
                return;
            }
            case C_START:
                bottomButtonRect(0, false, width, height, out);
                return;
            case C_VISOR:
            case C_BEAM:
            case C_MENU:
            case C_MAP:
            case C_TURBO:
            case C_TTURBO:
                out.setEmpty();
                return;
            case C_EDIT:
                bottomButtonRect(1, true, width, height, out);
                return;
            default:
                bottomButtonRect(0, true, width, height, out);
        }
    }

    // Keeps at least a control's centre on screen, and its offset sane.
    private void clampToScreen(int control, float width, float height) {
        final float u = layoutU(height);
        if (!(u > 0f) || !(width > 0f)) {
            return; // no size yet: dividing by u would put NaN in the layout
        }
        ovDx[control] = Math.max(-MAX_OFFSET, Math.min(MAX_OFFSET, ovDx[control]));
        ovDy[control] = Math.max(-MAX_OFFSET, Math.min(MAX_OFFSET, ovDy[control]));
        controlBounds(control, width, height, clampRect);
        final float cx = clampRect.centerX();
        final float cy = clampRect.centerY();
        ovDx[control] += (Math.max(0f, Math.min(width, cx)) - cx) / u;
        ovDy[control] += (Math.max(0f, Math.min(height, cy)) - cy) / u;
    }

    private void layoutEditBar(float width) {
        float total = 0f;
        for (float w : EDIT_WIDTHS_DP) {
            total += dp(w);
        }
        final float gap = dp(4);
        total += gap * (EDIT_WIDTHS_DP.length - 1);
        final float fit = Math.min(1f, (width - 2f * dp(8)) / total);
        float x = (width - total * fit) / 2f;
        final float top = dp(8);
        for (int i = 0; i < EDIT_WIDTHS_DP.length; ++i) {
            final float w = dp(EDIT_WIDTHS_DP[i]) * fit;
            editBar[i].set(x, top, x + w, top + dp(EDIT_BAR_HEIGHT_DP));
            x += w + gap * fit;
        }
    }

    private int editBarAt(float x, float y) {
        for (int i = 0; i < editBar.length; ++i) {
            if (editBar[i].contains(x, y)) {
                return i;
            }
        }
        return -1;
    }

    private void editAction(int action, float width, float height) {
        switch (action) {
            case EDIT_MINUS:
            case EDIT_PLUS:
                if (selected >= 0) {
                    final float step = action == EDIT_PLUS ? SCALE_STEP : -SCALE_STEP;
                    // Rounded so repeated steps land on tidy percentages.
                    ovScale[selected] = Math.max(MIN_SCALE, Math.min(MAX_SCALE,
                        Math.round((ovScale[selected] + step) * 100f) / 100f));
                    clampToScreen(selected, width, height);
                }
                break;
            case EDIT_RESET:
                if (selected >= 0) {
                    resetControl(selected);
                }
                break;
            case EDIT_RESET_ALL:
                for (int i = 0; i < CONTROLS; ++i) {
                    resetControl(i);
                }
                break;
            case EDIT_LAYOUT: {
                // The native side switches GameCube <-> twin stick and clears the
                // offsets; re-read them so the new placement shows at once.
                nativeCycleTouchLayout();
                final String text = nativeTouchLayout();
                layoutText = text != null ? text : "";
                loadLayout(layoutText);
                selected = -1;
                break;
            }
            default:
                endEdit(true);
                return;
        }
        invalidate();
    }

    private boolean onEditTouch(MotionEvent event) {
        final float width = getWidth();
        final float height = getHeight();
        final int action = event.getActionMasked();
        final int index = event.getActionIndex();
        layoutEditBar(width);
        switch (action) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN: {
                final int id = event.getPointerId(index);
                final float x = event.getX(index);
                final float y = event.getY(index);
                final int tool = editBarAt(x, y);
                if (tool >= 0) {
                    if (editToolPointer == -1) {
                        editToolPointer = id;
                        editAction(tool, width, height);
                    }
                } else if (dragPointer != -1 && pinchPointer == -1 && selected >= 0) {
                    final int other = event.findPointerIndex(dragPointer);
                    if (other >= 0) {
                        final float dist = (float) Math.hypot(x - event.getX(other),
                                                              y - event.getY(other));
                        if (dist >= dp(MAP_PINCH_MIN_DP)) {
                            pinchPointer = id;
                            pinchStartDist = dist;
                            pinchStartScale = ovScale[selected];
                        }
                    }
                } else if (dragPointer == -1) {
                    final int hit = editHit(x, y, width, height);
                    selected = hit;
                    if (hit >= 0) {
                        dragPointer = id;
                        dragLastX = x;
                        dragLastY = y;
                    }
                    invalidate();
                }
                return true;
            }
            case MotionEvent.ACTION_MOVE: {
                if (selected < 0) {
                    return true;
                }
                final int drag = event.findPointerIndex(dragPointer);
                final int pinch = event.findPointerIndex(pinchPointer);
                if (drag >= 0 && pinch >= 0) {
                    final float dist = (float) Math.hypot(event.getX(drag) - event.getX(pinch),
                                                          event.getY(drag) - event.getY(pinch));
                    ovScale[selected] = Math.max(MIN_SCALE, Math.min(MAX_SCALE,
                        pinchStartScale * dist / pinchStartDist));
                    // Keep the drag anchor current so lifting one finger doesn't jump.
                    dragLastX = event.getX(drag);
                    dragLastY = event.getY(drag);
                    clampToScreen(selected, width, height);
                } else if (drag >= 0) {
                    final float u = layoutU(height);
                    ovDx[selected] += (event.getX(drag) - dragLastX) / u;
                    ovDy[selected] += (event.getY(drag) - dragLastY) / u;
                    dragLastX = event.getX(drag);
                    dragLastY = event.getY(drag);
                    clampToScreen(selected, width, height);
                }
                invalidate();
                return true;
            }
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP: {
                final int id = event.getPointerId(index);
                if (id == editToolPointer) {
                    editToolPointer = -1;
                } else if (id == pinchPointer) {
                    pinchPointer = -1;
                } else if (id == dragPointer) {
                    // The pinch finger carries on as the drag.
                    dragPointer = pinchPointer;
                    pinchPointer = -1;
                    final int next = event.findPointerIndex(dragPointer);
                    if (next >= 0) {
                        dragLastX = event.getX(next);
                        dragLastY = event.getY(next);
                    }
                }
                if (action == MotionEvent.ACTION_UP) {
                    dragPointer = -1;
                    pinchPointer = -1;
                    editToolPointer = -1;
                }
                return true;
            }
            case MotionEvent.ACTION_CANCEL:
                dragPointer = -1;
                pinchPointer = -1;
                editToolPointer = -1;
                return true;
            default:
                return true;
        }
    }

    // The smallest shown control under (x, y), so a small button on a big one
    // (B near A) is still reachable.
    private int editHit(float x, float y, float width, float height) {
        int best = -1;
        float bestArea = Float.MAX_VALUE;
        final float slack = dp(EDIT_GRAB_DP);
        for (int c = 0; c < CONTROLS; ++c) {
            if (!editShown(c)) {
                continue;
            }
            controlBounds(c, width, height, editRect);
            if (editRect.isEmpty()) {
                continue;
            }
            editRect.inset(-slack, -slack);
            final float area = editRect.width() * editRect.height();
            if (editRect.contains(x, y) && area < bestArea) {
                best = c;
                bestArea = area;
            }
        }
        return best;
    }

    private void drawEditOverlay(Canvas canvas, float width, float height) {
        final float slack = dp(EDIT_GRAB_DP) * 0.5f;
        if (editDashes == null) {
            editDashes = new DashPathEffect(new float[] {dp(6), dp(4)}, 0f);
        }
        final DashPathEffect dashes = editDashes;
        for (int c = 0; c < CONTROLS; ++c) {
            if (!editShown(c)) {
                continue;
            }
            controlBounds(c, width, height, editRect);
            if (editRect.isEmpty()) {
                continue;
            }
            editRect.inset(-slack, -slack);
            final float corner = dp(8);
            if (c == selected) {
                fillPaint.setColor(0x5548C8E8);
                canvas.drawRoundRect(editRect, corner, corner, fillPaint);
                editPaint.setPathEffect(null);
                editPaint.setColor(0xFFFFE04A);
            } else {
                editPaint.setPathEffect(dashes);
                editPaint.setColor(0xCCFFFFFF);
            }
            canvas.drawRoundRect(editRect, corner, corner, editPaint);
        }
        editPaint.setPathEffect(null);

        layoutEditBar(width);
        for (int i = 0; i < editBar.length; ++i) {
            fillPaint.setColor(i == EDIT_DONE ? 0xDD2E8B57 : 0xDD081218);
            strokePaint.setColor(0xCCFFFFFF);
            final float corner = dp(8);
            canvas.drawRoundRect(editBar[i], corner, corner, fillPaint);
            canvas.drawRoundRect(editBar[i], corner, corner, strokePaint);
            drawCenteredLabel(canvas, EDIT_LABELS[i], editBar[i].centerX(), editBar[i].centerY(),
                              i < 2 ? dp(22) : dp(14));
        }

        final String hint = selected >= 0
            ? CONTROL_NAMES[selected] + "  " + Math.round(ovScale[selected] * 100f) + "%"
            : "Drag a control to move it; pinch to resize";
        textPaint.setTextSize(dp(13));
        final float textW = textPaint.measureText(hint);
        final float top = editBar[0].bottom + dp(EDIT_BAR_GAP_DP);
        final float cx = width / 2f;
        editRect.set(cx - textW / 2f - dp(10), top, cx + textW / 2f + dp(10), top + dp(26));
        fillPaint.setColor(0xCC081218);
        canvas.drawRoundRect(editRect, dp(8), dp(8), fillPaint);
        drawCenteredLabel(canvas, hint, cx, editRect.centerY(), dp(13));
    }

    private final RectF pillHit = new RectF();

    private int dp(float value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private static final class ControlButton {
        final String label;
        final int button;
        // A plain button's x is a fraction of the width and its y one of the
        // height. An anchored one's are offsets from GameCube A, in heights.
        final boolean anchored;
        final float x;
        final float y;
        // A kidney's radius is its centre arc's, round its x/y.
        final float radius;
        // Kidneys only: the band's half width, and its arc in degrees,
        // clockwise from +x. A zero sweep is a round button.
        final float halfWidth;
        final float arcStart;
        final float arcSweep;
        // An RGB fill, or 0 for the overlay's own.
        final int color;

        private ControlButton(String label, int button, boolean anchored, float x, float y,
                              float radius, float halfWidth, float arcStart, float arcSweep,
                              int color) {
            this.label = label;
            this.button = button;
            this.anchored = anchored;
            this.x = x;
            this.y = y;
            this.radius = radius;
            this.halfWidth = halfWidth;
            this.arcStart = arcStart;
            this.arcSweep = arcSweep;
            this.color = color;
        }

        static ControlButton round(String label, int button, float x, float y, float radius,
                                   int color) {
            return new ControlButton(label, button, true, x, y, radius, 0f, 0f, 0f, color);
        }

        static ControlButton kidney(String label, int button, float ring, float halfWidth,
                                    float arcStart, float arcSweep, int color) {
            return new ControlButton(label, button, true, 0f, 0f, ring, halfWidth, arcStart,
                                     arcSweep, color);
        }

        boolean isKidney() {
            return arcSweep != 0f;
        }
    }

    private static final class PillButton {
        final String label;
        final int axis;
        final int button;
        final float left;
        final float top;
        final float right;
        final float bottom;
        // An RGB fill, or 0 for the overlay's own.
        final int color;
        // The editor control this pill is, or -1 to go by its axis/button (pillControl).
        final int control;
        PillButton(String label, int axis, int button, float left, float top, float right,
                   float bottom) {
            this(label, axis, button, left, top, right, bottom, 0, -1);
        }

        PillButton(String label, int axis, int button, float left, float top, float right,
                   float bottom, int color) {
            this(label, axis, button, left, top, right, bottom, color, -1);
        }

        PillButton(String label, int axis, int button, float left, float top, float right,
                   float bottom, int color, int control) {
            this.control = control;
            this.color = color;
            this.label = label;
            this.axis = axis;
            this.button = button;
            this.left = left;
            this.top = top;
            this.right = right;
            this.bottom = bottom;
        }

        // One identity for the press map and the held-highlight, whether this is
        // a button or an analog trigger sent as an axis.
        int id() {
            return axis >= 0 ? AXIS_ID_BASE + axis : button;
        }
    }

    private static final class TouchTarget {
        final int type;
        final int id;
        float x;
        float y;
        float startX;
        float startY;
        // A held A button that also aims; see startButtonAim.
        boolean aiming;
        // The pill or face button a BUTTON target pressed. Twin sends one GC button
        // from two controls, so the highlight follows the control, not `held`.
        Object control;
        // The aiming finger's current upward stroke (see trackSwipe): where and
        // when it began (swipeMs -1: none yet), and whether it already counted as a swipe.
        float swipeX;
        float swipeY;
        long swipeMs = -1;
        boolean swiped;

        TouchTarget(int type, int id) {
            this.type = type;
            this.id = id;
        }

        // A target whose finger went down at (x, y).
        static TouchTarget begin(int type, int id, float x, float y) {
            final TouchTarget target = new TouchTarget(type, id);
            target.x = x;
            target.y = y;
            target.startX = x;
            target.startY = y;
            return target;
        }
    }
}
