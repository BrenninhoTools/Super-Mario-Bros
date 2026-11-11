package com.brenninhotools.supermariobros;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.PowerManager;
import android.view.Display;
import android.view.Window;
import android.view.WindowManager;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

public class SmbActivity extends SDLActivity {
    private static final int REQUEST_ROM = 4096;
    private static final String ROM_NAME = "baserom.nes";
    private boolean pickerOpen = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        configureWindow();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            configureWindow();
        }
    }

    private void configureWindow() {
        Window window = getWindow();
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        PowerManager power = (PowerManager) getSystemService(POWER_SERVICE);
        if (power != null && power.isSustainedPerformanceModeSupported()) {
            window.setSustainedPerformanceMode(true);
        }
        selectDisplayMode(window);
    }

    private void selectDisplayMode(Window window) {
        Display display = getWindowManager().getDefaultDisplay();
        if (display == null) {
            return;
        }
        Display.Mode current = display.getMode();
        Display.Mode best = null;
        float bestScore = Float.MAX_VALUE;
        for (Display.Mode mode : display.getSupportedModes()) {
            if (mode.getPhysicalWidth() != current.getPhysicalWidth() || mode.getPhysicalHeight() != current.getPhysicalHeight()) {
                continue;
            }
            float rate = mode.getRefreshRate();
            int multiple = Math.round(rate / 60.0988f);
            if (multiple < 1 || multiple > 2) {
                continue;
            }
            float deviation = Math.abs(rate / multiple - 60.0988f);
            if (deviation > 1.5f) {
                continue;
            }
            float score = deviation + (multiple == 2 ? 0.0f : 0.5f);
            if (score < bestScore) {
                bestScore = score;
                best = mode;
            }
        }
        if (best == null) {
            return;
        }
        WindowManager.LayoutParams params = window.getAttributes();
        if (params.preferredDisplayModeId != best.getModeId()) {
            params.preferredDisplayModeId = best.getModeId();
            window.setAttributes(params);
        }
    }

    public void requestRom() {
        runOnUiThread(() -> {
            if (pickerOpen) {
                return;
            }
            pickerOpen = true;
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("*/*");
            try {
                startActivityForResult(intent, REQUEST_ROM);
            } catch (Exception e) {
                pickerOpen = false;
            }
        });
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode != REQUEST_ROM) {
            super.onActivityResult(requestCode, resultCode, data);
            return;
        }
        pickerOpen = false;
        if (resultCode != RESULT_OK || data == null || data.getData() == null) {
            return;
        }
        Uri uri = data.getData();
        File target = new File(getFilesDir(), ROM_NAME);
        File temp = new File(getFilesDir(), ROM_NAME + ".tmp");
        try (InputStream in = getContentResolver().openInputStream(uri);
             OutputStream out = new FileOutputStream(temp)) {
            if (in == null) {
                return;
            }
            byte[] buffer = new byte[16384];
            int total = 0;
            int read;
            while ((read = in.read(buffer)) != -1) {
                total += read;
                if (total > 4 * 1024 * 1024) {
                    throw new Exception("ROM too large");
                }
                out.write(buffer, 0, read);
            }
        } catch (Exception e) {
            temp.delete();
            return;
        }
        if (target.exists()) {
            target.delete();
        }
        temp.renameTo(target);
    }
}
