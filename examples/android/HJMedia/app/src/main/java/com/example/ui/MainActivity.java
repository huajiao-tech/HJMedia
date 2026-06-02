package com.example.ui;

import android.content.Intent;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_main);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        // Start the selective copy process
        copyAssetsToInternalStorage();

        // Set up button listeners
        setupButtonListeners();
    }

    private void setupButtonListeners() {
        setButtonClickListener(findViewById(R.id.btnSRActivity), SRActivity.class);
        setButtonClickListener(findViewById(R.id.btnSRFaceAcitivity), SRFaceActivity.class);
        setButtonClickListener(findViewById(R.id.btnFaceuActivity), FaceuActivity.class);
        setButtonClickListener(findViewById(R.id.btnMusicPlayerActivity), MusicPlayerActivity.class);
        setButtonClickListener(findViewById(R.id.btnAudioMixerActivity), AudioMixerActivity.class);
        setButtonClickListener(findViewById(R.id.btnAudioCompositeActivity), AudioCompositeActivity.class);
    }

    private void setButtonClickListener(Button button, Class<?> targetActivity) {
        button.setOnClickListener(v -> {
            try {
                Intent intent = new Intent(MainActivity.this, targetActivity);
                startActivity(intent);
            } catch (Exception e) {
                Log.e(TAG, "Error starting activity", e);
            }
        });
    }

    private void copyAssetsToInternalStorage() {
        File resourceDestDir = new File(getFilesDir(), "HJResource");

        if (resourceDestDir.exists()) {
            deleteRecursive(resourceDestDir);
        }

        String[] foldersToCopy = {"faceu", "image", "imgseq", "model", "audio"};

        Log.d(TAG, "Starting selective asset copy to: " + resourceDestDir.getAbsolutePath());

        AssetManager assetManager = getAssets();
        for (String folder : foldersToCopy) {
            copyAsset(assetManager, folder, new File(resourceDestDir, folder).getAbsolutePath());
        }

        Log.d(TAG, "Asset copy finished.");
    }

    private void deleteRecursive(File fileOrDirectory) {
        if (fileOrDirectory.isDirectory()) {
            File[] children = fileOrDirectory.listFiles();
            if (children != null) {
                for (File child : children) {
                    deleteRecursive(child);
                }
            }
        }
        fileOrDirectory.delete();
    }

    private void copyAsset(AssetManager assetManager, String assetPath, String destinationPath) {
        try {
            String[] assets = assetManager.list(assetPath);
            if (assets == null || assets.length == 0) {
                // This is a file, or an empty directory.
                // AssetManager.open() will fail for an empty directory.
                copyFile(assetManager, assetPath, destinationPath);
            } else {
                // This is a directory with content.
                File destDir = new File(destinationPath);
                destDir.mkdirs();
                for (String asset : assets) {
                    String newAssetPath = assetPath.isEmpty() ? asset : assetPath + "/" + asset;
                    String newDestinationPath = destinationPath + "/" + asset;
                    copyAsset(assetManager, newAssetPath, newDestinationPath);
                }
            }
        } catch (IOException e) {
            // This means assetManager.list() failed, so it's almost certainly a file.
            copyFile(assetManager, assetPath, destinationPath);
        }
    }

    private void copyFile(AssetManager assetManager, String assetPath, String destinationPath) {
        try (InputStream in = assetManager.open(assetPath);
             OutputStream out = new FileOutputStream(destinationPath)) {
            Log.d(TAG, "Copying file: '" + assetPath + "' to '" + destinationPath + "'");
            byte[] buffer = new byte[1024];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
        } catch (IOException e) {
            // This might happen if it's an empty directory, which is fine to ignore.
            // We only care about copying files.
            Log.w(TAG, "Could not copy asset (might be an empty directory, which is ignored): " + assetPath);
        }
    }
}
