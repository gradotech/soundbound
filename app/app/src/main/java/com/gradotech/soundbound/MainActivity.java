package com.gradotech.soundbound;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Environment;
import android.os.RemoteException;
import android.view.View;
import android.widget.AbsListView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.gradotech.soundbound.databinding.ActivityMainBinding;

import org.altbeacon.beacon.Beacon;
import org.altbeacon.beacon.BeaconManager;
import org.altbeacon.beacon.Region;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("soundbound");
    }

    private ActivityMainBinding binding;
    private Connection hwConnection;
    private String deviceName;
    private HashMap<String, Integer> hwVersion;
    private ListView songListView;
    private List<String> fileList;
    private final String songsDir = "/Music/";
    private String selectedFilePath;
    private ArrayAdapter<String> adapter;
    private AudioServer audioServer;
    private BeaconManager beaconManager;
    private Region region;
    private static final int permRequestCode = 1;
    private static final long beaconScanPeriod = 200L;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        TFModel tfModel = new TFModel(getAssets(),
                "room_to_volume.tflite",
                "room_to_volume.json");

        region = new Region("Soundbound", null, null, null);

        beaconManager = BeaconManager.getInstanceForApplication(this);
        beaconManager.addRangeNotifier((beacons, region) -> {
            for (Beacon beacon : beacons) {
                String id1 = beacon.getId1().toString();
                float distance = (float)beacon.getDistance();

                tfModel.updateInput(id1, distance);
            }

            HashMap<String, Integer> modelOutput = tfModel.predict();
            for (Map.Entry<String, Integer> output : modelOutput.entrySet()) {
                byte id = (byte) output.getKey().charAt(0);
                byte volume = output.getValue().byteValue();
                byte[] cmd = getVolumeCmd(id, volume);

                hwConnection.emitCommand(cmd);
            }
        });

        try {
            beaconManager.setForegroundScanPeriod(beaconScanPeriod);
            beaconManager.updateScanPeriods();
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }

        int serverPort = getServerPort();
        String ipAddress = getIpAddress();
        int streamPort = getStreamPort();
        audioServer = new AudioServer(streamPort);

        hwConnection = new Connection(this,100, 120, ipAddress,
                                      serverPort);
        hwConnection.start();

        songListView = findViewById(R.id.songList);
        fileList = new ArrayList<>();

        checkPermission();
        listFiles(Environment.getExternalStorageDirectory() + songsDir);

        adapter = new ListViewAdapter(this, android.R.layout.simple_list_item_activated_1,
                                    fileList);
        songListView.setAdapter(adapter);
        songListView.setChoiceMode(AbsListView.CHOICE_MODE_SINGLE);

        songListView.setOnItemClickListener((parent, view, position, id) -> {
            String selectedFileName = fileList.get(position);
            selectedFilePath = Environment.getExternalStorageDirectory() + songsDir +
                                selectedFileName;

            songListView.setItemChecked(position, true);
        });

        Button playBtn = findViewById(R.id.playBtn);
        Button pauseBtn = findViewById(R.id.pauseBtn);

        playBtn.setOnClickListener(view -> {
            byte[] cmd = getStartCmd();

            if (selectedFilePath != null) {
                audioServer.setFilePath(selectedFilePath);

                if (!audioServer.wasStarted())
                    audioServer.startServer();
            } else {
                Toast.makeText(this, "Please select a song", Toast.LENGTH_SHORT).show();
                return;
            }

            hwConnection.emitCommand(cmd);

            playBtn.setVisibility(View.GONE);
            pauseBtn.setVisibility(View.VISIBLE);

            audioServer.setPlayingStatus(true);

            beaconManager.startRangingBeacons(region);
        });

        pauseBtn.setOnClickListener(view -> {
            playBtn.setVisibility(View.VISIBLE);
            pauseBtn.setVisibility(View.GONE);
            byte[] cmd = getStopCmd();

            hwConnection.emitCommand(cmd);

            audioServer.setPlayingStatus(false);

            beaconManager.stopRangingBeacons(region);
        });
    }

    private void checkPermission() {
        if (ContextCompat.checkSelfPermission(this,
                Manifest.permission.READ_MEDIA_AUDIO)
                != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this,
                        Manifest.permission.ACCESS_FINE_LOCATION)
                        != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this,
                        Manifest.permission.BLUETOOTH_CONNECT)
                        != PackageManager.PERMISSION_GRANTED
        ) {
            ActivityCompat.requestPermissions(this,
                    new String[] {
                            Manifest.permission.READ_MEDIA_AUDIO,
                            Manifest.permission.ACCESS_COARSE_LOCATION,
                            Manifest.permission.ACCESS_FINE_LOCATION,
                            Manifest.permission.BLUETOOTH_CONNECT,
                            Manifest.permission.BLUETOOTH_SCAN
                    },
                    permRequestCode);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode == permRequestCode) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                listFiles(Environment.getExternalStorageDirectory() + songsDir);
            } else {
                Toast.makeText(this, "Permission denied", Toast.LENGTH_SHORT).show();
            }
        }
    }

    private void listFiles(String directoryPath) {
        File directory = new File(directoryPath);

        if (directory.exists() && directory.isDirectory()) {
            File[] files = directory.listFiles();
            if (files != null) {
                for (File file : files) {
                    String name = file.getName();

                    if (name.endsWith(".mp3"))
                        fileList.add(name);
                }

                ArrayAdapter<String> adapter = new ArrayAdapter<>(this,
                        android.R.layout.simple_list_item_1, fileList);
                songListView.setAdapter(adapter);
            }
        } else {
            Toast.makeText(this, "Music directory not found",
                            Toast.LENGTH_SHORT).show();
        }
    }

    /**
     * Callback called from the Connection class
     */

    @SuppressLint("DefaultLocale")
    public void handshakeDone(byte[] packet) {
        TextView statusTextView = findViewById(R.id.statusTextView);
        TextView hwVer = findViewById(R.id.hwVer);

        deviceName = getDeviceName(packet);
        hwVersion = getHWVersion(packet);

        statusTextView.setText(deviceName);
        statusTextView.setTextColor(Color.GREEN);

        hwVer.setText(String.format("V%s.%s", hwVersion.get("major"), hwVersion.get("minor")));
    }

    public void handshakeFailed() {
        TextView statusTextView = findViewById(R.id.statusTextView);

        statusTextView.setText("ERROR");
        statusTextView.setTextColor(Color.RED);
    }

    private String getDeviceName(byte[] packet) {
        return parseDeviceName(packet);
    }

    private HashMap<String, Integer> getHWVersion(byte[] packet) {
        int[] versions = parseHWVersion(packet);
        HashMap<String, Integer> mVersions = new HashMap<>();

        mVersions.put("major", versions[0]);
        mVersions.put("minor", versions[1]);

        return mVersions;
    }

    private String getIpAddress() {
        WifiManager wifiMan = (WifiManager) this.getSystemService(Context.WIFI_SERVICE);
        WifiInfo wifiInf = wifiMan.getConnectionInfo();
        int ipAddress = wifiInf.getIpAddress();

        @SuppressLint("DefaultLocale")
        String ip = String.format("%d.%d.%d.",
                (ipAddress & 0xff),
                (ipAddress >> 8 & 0xff),
                (ipAddress >> 16 & 0xff));

        return ip;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        audioServer.stop();
        hwConnection.close();
    }

    /**
     * A native method that is implemented by the 'soundbound' native library,
     * which is packaged with this application.
     */
    public native int getServerPort();
    public native int getStreamPort();
    public native int getQDataPacketSize();
    public native char getNextSpkId(byte[] packet, int index);
    public native byte[] getStartCmd();
    public native byte[] getStopCmd();
    public native byte[] getVolumeCmd(byte id, byte volume);
    public native String parseDeviceName(byte[] packet);
    public native int[] parseHWVersion(byte[] packet);
}
