package org.metroidprime2.port;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.provider.MediaStore;
import android.util.Log;

import dev.encounter.aurora.AuroraSurface;

import org.libsdl.app.SDLActivity;
import org.libsdl.app.SDLSurface;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * The app: SDL's activity with this port's library, Aurora's surface, the port's directories
 * written down before the native entry runs, and - from milestone M3 - the on-screen
 * controller drawn over the game.
 *
 * <p>What is deliberately absent: the port's F1 settings window has no touch affordance, so
 * the layout editor is reached from the overlay's own edit pill (see TouchControlsView).
 */
public final class MetroidPrime2Activity extends SDLActivity {
    private static final String TAG = "MetroidPrime2Port";

    // The overlay, added to SDL's own layout so it sits over the surface. Null if SDL did not
    // build a layout, which only happens if library loading failed.
    private TouchControlsView touchControls;

    // The library the native build produces: CMakeLists.txt's `metroid_prime2_port`, which on
    // Android is libmetroid_prime2_port.so and exports SDL_main.
    @Override
    protected String[] getLibraries() {
        return new String[]{"metroid_prime2_port"};
    }

    // Aurora presents through its own surface, which is where the WebGPU swapchain attaches;
    // SDL's plain SDLSurface has nowhere to put it.
    @Override
    protected SDLSurface createSDLSurface(Context context) {
        return new AuroraSurface(context);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // Before super.onCreate: that is where SDLActivity loads the library, and the native
        // entry reads this file as one of its first acts (platform/main.cpp,
        // ResolveAndroidPaths).
        writePortPaths();
        super.onCreate(savedInstanceState);
        // SDLActivity.onCreate sets the window to non-fullscreen; the game wants the whole screen.
        setWindowStyle(true);
        addTouchControls();
        Log.i(TAG, "activity created; files dir " + getFilesDir());
    }

    /**
     * SDL keeps its surface in {@code mLayout}; adding the overlay to the same layout is what
     * MP1's activity does, and it is the only way to draw over the game without touching SDL.
     * The view covers the whole layout and consumes the touches it does not pass on, so SDL
     * never sees them - which is why the native side feeds the game a virtual pad from the JNI
     * callbacks instead of trying to synthesise input events.
     */
    private void addTouchControls() {
        if (mLayout == null) {
            Log.w(TAG, "SDL has no layout; no touch controls");
            return;
        }
        touchControls = new TouchControlsView(this);
        mLayout.addView(touchControls, new android.widget.RelativeLayout.LayoutParams(
            android.widget.RelativeLayout.LayoutParams.MATCH_PARENT,
            android.widget.RelativeLayout.LayoutParams.MATCH_PARENT));
    }

    /**
     * The native side resolves the port's directories beneath SDL's preference directory -
     * which on Android is this same app-private files directory - and reads user=, cache= and
     * disc= overrides from a port_paths.txt beside them. Writing the file here makes the layout
     * visible in the log from the first launch and leaves the place the file picker will put
     * the disc it copies in the next milestone. An existing file is left alone, so a pushed
     * port_paths.txt still wins, which is what a debugging session wants.
     */
    private void writePortPaths() {
        File marker = new File(getFilesDir(), "port_paths.txt");
        if (marker.exists()) {
            return;
        }
        String listed = "user=" + new File(getFilesDir(), "user") + "\n"
            + "cache=" + new File(getFilesDir(), "cache") + "\n"
            + "disc=" + new File(getFilesDir(), "disc.iso") + "\n";
        try (FileOutputStream output = new FileOutputStream(marker)) {
            output.write(listed.getBytes(StandardCharsets.UTF_8));
        } catch (IOException e) {
            // Not fatal: the native side falls back to those same directories.
            Log.w(TAG, "Could not write " + marker, e);
        }
    }

    private static final int REQUEST_TEXTURE_PACK = 0x7e57;
    // Ask the system picker to list the phone's storage root alongside its shortcuts.
    private static final String SHOW_ADVANCED = "android.content.extra.SHOW_ADVANCED";
    // A disc picked after Android destroyed the activity behind the picker reaches a new
    // launch, which drops it: SDL's dialog state died with the old process. The picker's
    // request code is kept here, and such a pick is left in PENDING_DISC_FILE for the
    // native side's next start (platform/main.cpp).
    private static final String DISC_PICK_PREFS = "disc_pick";
    private static final String PENDING_DISC_FILE = "pending_disc_uri";

    // Texture pack picker (platform/debug_ui/game_sections.cpp, nativeTexturePack* in
    // platform/port_touch_jni.cpp): the folder is copied to <user>/user_textures.new, which
    // PortTextures swaps in on its next frame.
    private static native void nativeTexturePackStatus(String status);
    private static native void nativeTexturePackReady();

    private final AtomicBoolean texturePackCopying = new AtomicBoolean();
    private volatile String texturePackFolder;

    // Called from the debug overlay, on the SDL thread, with the user folder the pack goes into.
    public void pickTexturePack(String folder) {
        runOnUiThread(() -> {
            if (texturePackCopying.get()) {
                nativeTexturePackStatus("A texture pack is still being copied.");
                return;
            }
            texturePackFolder = folder;
            try {
                startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE)
                                           .putExtra(SHOW_ADVANCED, true),
                                       REQUEST_TEXTURE_PACK);
            } catch (android.content.ActivityNotFoundException e) {
                nativeTexturePackStatus("This device has no folder picker.");
            }
        });
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == REQUEST_TEXTURE_PACK) {
            Uri tree = data != null && resultCode == RESULT_OK ? data.getData() : null;
            if (tree != null) {
                copyTexturePack(tree);
            }
            return;
        }
        android.content.SharedPreferences discPick = getSharedPreferences(DISC_PICK_PREFS, MODE_PRIVATE);
        if (discPick.getInt("request", Integer.MIN_VALUE) == requestCode) {
            discPick.edit().remove("request").commit();
            Uri disc = data != null && resultCode == RESULT_OK ? data.getData() : null;
            if (disc != null && !isFileDialogPending(requestCode)) {
                try (OutputStream out = new FileOutputStream(new File(getFilesDir(), PENDING_DISC_FILE))) {
                    out.write(disc.toString().getBytes(StandardCharsets.UTF_8));
                    Log.i(TAG, "Kept a disc picked before the activity was recreated: " + disc);
                } catch (IOException e) {
                    Log.w(TAG, "Could not keep the disc picked before the activity was recreated", e);
                }
                return;
            }
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    // Remembers which dialog is the disc picker (opening, with filters; the texture pack
    // uses its own intent) in storage that outlives the process. commit, not apply: the
    // process can be gone before an apply is written.
    @Override
    protected void onFileDialogShown(String[] filters, boolean forWrite, int requestCode) {
        android.content.SharedPreferences.Editor edit = getSharedPreferences(DISC_PICK_PREFS, MODE_PRIVATE).edit();
        if (!forWrite && filters != null && filters.length > 0) {
            edit.putInt("request", requestCode);
        } else {
            edit.remove("request");
        }
        edit.commit();
    }

    // A copy rather than reading through the URI: the grant can be revoked, and the loader needs
    // real paths. The built-in set is never touched.
    private void copyTexturePack(Uri tree) {
        if (!texturePackCopying.compareAndSet(false, true)) {
            return;
        }
        nativeTexturePackStatus("Copying the texture pack...");
        new Thread(() -> {
            String folder = texturePackFolder;
            File root = folder != null && !folder.isEmpty() ? new File(folder) : getFilesDir();
            File staging = new File(root, "user_textures.partial");
            File ready = new File(root, "user_textures.new");
            try {
                deleteTree(staging);
                int[] copied = {0};
                String rootId = DocumentsContract.getTreeDocumentId(tree);
                copyDocumentTree(tree, rootId, staging, copied);
                if (copied[0] == 0) {
                    deleteTree(staging);
                    nativeTexturePackStatus("No .png or .dds textures in that folder; nothing changed.");
                    return;
                }
                deleteTree(ready);
                if (!staging.renameTo(ready)) {
                    throw new IOException("Failed to rename " + staging + " to " + ready);
                }
                nativeTexturePackStatus(String.format(Locale.ROOT, "Copied %d textures.", copied[0]));
                nativeTexturePackReady();
            } catch (IOException | RuntimeException e) {
                Log.e(TAG, "Failed to copy the texture pack", e);
                deleteTree(staging);
                nativeTexturePackStatus("Copying the texture pack failed: " + e.getMessage());
            } finally {
                texturePackCopying.set(false);
            }
        }, "texture-pack-copy").start();
    }

    private void copyDocumentTree(Uri tree, String parentId, File destination, int[] copied)
            throws IOException {
        Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(tree, parentId);
        String[] columns = {
            DocumentsContract.Document.COLUMN_DOCUMENT_ID,
            DocumentsContract.Document.COLUMN_DISPLAY_NAME,
            DocumentsContract.Document.COLUMN_MIME_TYPE,
        };
        try (Cursor cursor = getContentResolver().query(children, columns, null, null, null)) {
            if (cursor == null) {
                throw new IOException("Cannot list " + children);
            }
            while (cursor.moveToNext()) {
                String id = cursor.getString(0);
                String name = cursor.getString(1);
                String mime = cursor.getString(2);
                // A name from the provider becomes a path component here.
                if (name == null || name.isEmpty() || name.equals(".") || name.equals("..")
                        || name.contains("/")) {
                    continue;
                }
                File target = new File(destination, name);
                if (DocumentsContract.Document.MIME_TYPE_DIR.equals(mime)) {
                    copyDocumentTree(tree, id, target, copied);
                    continue;
                }
                String lower = name.toLowerCase(Locale.ROOT);
                if (!lower.endsWith(".png") && !lower.endsWith(".dds")) {
                    continue;
                }
                if (!destination.isDirectory() && !destination.mkdirs()) {
                    throw new IOException("Failed to create " + destination);
                }
                Uri document = DocumentsContract.buildDocumentUriUsingTree(tree, id);
                try (InputStream input = getContentResolver().openInputStream(document);
                     FileOutputStream output = new FileOutputStream(target)) {
                    if (input == null) {
                        throw new IOException("Cannot open " + document);
                    }
                    byte[] buffer = new byte[64 * 1024];
                    int count;
                    while ((count = input.read(buffer)) != -1) {
                        output.write(buffer, 0, count);
                    }
                }
                if (++copied[0] % 100 == 0) {
                    nativeTexturePackStatus(String.format(Locale.ROOT,
                        "Copying the texture pack... %d textures", copied[0]));
                }
            }
        }
    }

    private static void deleteTree(File file) {
        File[] children = file.listFiles();
        if (children != null) {
            for (File child : children) {
                deleteTree(child);
            }
        }
        file.delete();
    }

    // Called from photo mode (port_photo.cpp), on the SDL thread, once a photo is
    // written: moves it into the shared Pictures/Metroid Prime 2 folder, where the
    // gallery sees it. Returns where it went, or null (the file is then kept).
    // (MP1 MetroidPrimeActivity.exportPhoto.)
    public String exportPhoto(String path) {
        File source = new File(path);
        String folder = Environment.DIRECTORY_PICTURES + "/Metroid Prime 2";
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                ContentResolver resolver = getContentResolver();
                ContentValues values = new ContentValues();
                values.put(MediaStore.Images.Media.DISPLAY_NAME, source.getName());
                values.put(MediaStore.Images.Media.MIME_TYPE, "image/png");
                values.put(MediaStore.Images.Media.RELATIVE_PATH, folder);
                values.put(MediaStore.Images.Media.IS_PENDING, 1);
                Uri uri = resolver.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, values);
                if (uri == null) {
                    return null;
                }
                try (InputStream input = new FileInputStream(source);
                     OutputStream output = resolver.openOutputStream(uri)) {
                    if (output == null) {
                        throw new IOException("No stream for " + uri);
                    }
                    copyStream(input, output);
                } catch (IOException e) {
                    resolver.delete(uri, null, null);
                    throw e;
                }
                values.clear();
                values.put(MediaStore.Images.Media.IS_PENDING, 0);
                resolver.update(uri, values, null, null);
            } else {
                // Android 9 has no MediaStore folders: a plain file (WRITE_EXTERNAL_STORAGE,
                // maxSdkVersion 28), then a scan for the gallery.
                File dir = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES),
                                    "Metroid Prime 2");
                if (!dir.isDirectory() && !dir.mkdirs()) {
                    throw new IOException("Failed to create " + dir);
                }
                File destination = new File(dir, source.getName());
                try (InputStream input = new FileInputStream(source);
                     OutputStream output = new FileOutputStream(destination)) {
                    copyStream(input, output);
                }
                MediaScannerConnection.scanFile(this, new String[]{destination.getPath()},
                                                new String[]{"image/png"}, null);
            }
        } catch (IOException | RuntimeException e) {
            Log.w(TAG, "Could not move " + source + " to " + folder, e);
            return null;
        }
        source.delete();
        return folder + "/" + source.getName();
    }

    private static void copyStream(InputStream input, OutputStream output) throws IOException {
        byte[] buffer = new byte[64 * 1024];
        int count;
        while ((count = input.read(buffer)) != -1) {
            output.write(buffer, 0, count);
        }
    }

}
