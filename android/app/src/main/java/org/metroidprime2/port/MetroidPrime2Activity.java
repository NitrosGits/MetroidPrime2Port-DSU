package org.metroidprime2.port;

import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.util.Log;

import dev.encounter.aurora.AuroraSurface;

import org.libsdl.app.SDLActivity;
import org.libsdl.app.SDLSurface;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
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
                startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE), REQUEST_TEXTURE_PACK);
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
        super.onActivityResult(requestCode, resultCode, data);
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
}
