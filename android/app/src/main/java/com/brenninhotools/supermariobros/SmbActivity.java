package com.brenninhotools.supermariobros;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;

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
