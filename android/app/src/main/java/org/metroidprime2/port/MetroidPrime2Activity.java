package org.metroidprime2.port;

import android.content.Context;
import android.os.Bundle;
import android.util.Log;

import dev.encounter.aurora.AuroraSurface;

import org.libsdl.app.SDLActivity;
import org.libsdl.app.SDLSurface;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

/**
 * The whole app at milestone M1: SDL's activity with this port's library, Aurora's surface, and
 * the port's directories written down before the native entry runs.
 *
 * <p>What is deliberately absent: touch controls (M3), the file picker (M2) and the debug
 * overlay's settings. ../MetroidPrimePort's MetroidPrimeActivity is the reference for all of
 * them when they land.
 */
public final class MetroidPrime2Activity extends SDLActivity {
    private static final String TAG = "MetroidPrime2Port";

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
        Log.i(TAG, "activity created; files dir " + getFilesDir());
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
}
