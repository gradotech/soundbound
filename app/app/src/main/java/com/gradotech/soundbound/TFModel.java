package com.gradotech.soundbound;

import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.util.Log;

import org.json.JSONObject;
import org.tensorflow.lite.Interpreter;

import java.io.FileInputStream;
import java.io.InputStream;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;

public class TFModel {
    private static class Shape {
        private final String name;
        private final String type;
        private final int min;
        private final int max;

        public Shape(String name, String type, int min, int max) {
            this.name = name;
            this.type = type;
            this.min = min;
            this.max = max;
        }

        public String getName() {
            return name;
        }

        public String getType() {
            return type;
        }

        public int getMin() {
            return min;
        }

        public int getMax() {
            return max;
        }
    }

    private static class Speaker {
        String id;
        int[] coordinates;

        public Speaker(String id, int[] coordinates) {
            this.id = id;
            this.coordinates = coordinates;
        }

        public String getId() {
            return id;
        }

        public int[] getCoordinates() {
            return coordinates;
        }
    }

    private static class Beacon {
        String id1;
        String measures;
        float distance;

        public Beacon(String id1, String measures) {
            this.id1 = id1;
            this.measures = measures;
            this.distance = 0;
        }

        public String getId1() {
            return id1;
        }

        public String getMeasures() {
            return measures;
        }

        public void setDistance(float distance) {
            this.distance = distance;
        }

        public float getDistance() {
            return distance;
        }
    }

    private Interpreter tflite;
    private JSONObject config;
    private HashMap<String, Shape> inputs;
    private HashMap<String, Shape> outputs;
    private HashMap<String, Speaker> speakers;
    private HashMap<String, Beacon> beacons;

    public TFModel(AssetManager assets, String modelName, String configName) {
        try {
            int objCount;
            int i;

            AssetFileDescriptor modelFD = assets.openFd(modelName);
            InputStream configStream = assets.open(configName);

            tflite = new Interpreter(loadModelFile(modelFD));
            config = new JSONObject(loadJSONConfig(configStream));

            objCount = config.getJSONArray("inputs").length();
            inputs = new HashMap<String, Shape>();
            for (i = 0; i < objCount; i++) {
                JSONObject obj = config.getJSONArray("inputs").getJSONObject(i);

                inputs.put(obj.getString("name"),
                        new Shape(obj.getString("name"),
                        obj.getString("type"),
                        obj.getInt("min"),
                        obj.getInt("max")));
            }

            objCount = config.getJSONArray("outputs").length();
            outputs = new HashMap<String, Shape>();
            for (i = 0; i < objCount; i++) {
                JSONObject obj = config.getJSONArray("outputs").getJSONObject(i);

                outputs.put(obj.getString("name"),
                        new Shape(obj.getString("name"),
                        obj.getString("type"),
                        obj.getInt("min"),
                        obj.getInt("max")));
            }

            objCount = config.getJSONArray("speakers").length();
            speakers = new HashMap<String, Speaker>();
            for (i = 0; i < objCount; i++) {
                JSONObject obj = config.getJSONArray("speakers").getJSONObject(i);
                String id = obj.getString("id");
                int[] coordinates = new int[2];

                coordinates[0] = obj.getJSONArray("coordinates").getInt(0);
                coordinates[1] = obj.getJSONArray("coordinates").getInt(1);

                speakers.put(id, new Speaker(id, coordinates));
            }

            objCount = config.getJSONArray("beacons").length();
            beacons = new HashMap<String, Beacon>();
            for (i = 0; i < objCount; i++) {
                JSONObject obj = config.getJSONArray("beacons").getJSONObject(i);
                String id1 = obj.getString("id1");

                beacons.put(id1, new Beacon(id1, obj.getString("measures")));
            }
        } catch (Exception e) {
            Log.e(this.getClass().getName(), "Failed to parse model config!");
            e.printStackTrace();
        }
    }

    public HashMap<String, Integer> predict() {
        int inputMax = inputs.size();
        int outputMax = outputs.size();

        float[][] tfInput = new float[1][inputMax];
        float[][] tfOutput = new float[1][outputMax];
        HashMap<String, Integer> result = new HashMap<>();

        int i = 0;
        for (Map.Entry<String, Beacon> beacon : beacons.entrySet()) {
            float distance = beacon.getValue().getDistance() * 100;
            int clampMax = inputs.get(beacon.getValue().getMeasures()).getMax();
            int clampMin = inputs.get(beacon.getValue().getMeasures()).getMin();

            tfInput[0][i++] = Math.max(clampMin, Math.min(clampMax, distance));
        }

        tflite.run(tfInput, tfOutput);

        i = 0;
        for (Map.Entry<String, Shape> output : outputs.entrySet()) {
            /* Outputs index in the config should always match those of the model! */
            int volume = Math.round(tfOutput[0][i++]);
            int clampMax = output.getValue().getMax();
            int clampMin = output.getValue().getMin();

            result.put(output.getValue().getName(),
                       Math.max(clampMin, Math.min(clampMax, volume)));
        }

        return result;
    }

    public void updateInput(String id1, float distance) {
        Beacon beacon = beacons.get(id1);

        if (beacon != null) {
            beacon.setDistance(distance);
        }
        else {
            Log.w(this.getClass().getName(), "Unsupported beacon id1 " + id1);
        }
    }

    private MappedByteBuffer loadModelFile(AssetFileDescriptor modelFD) throws Exception {
        FileInputStream inputStream = new FileInputStream(modelFD.getFileDescriptor());
        FileChannel fileChannel = inputStream.getChannel();
        long startOffset = modelFD.getStartOffset();
        long declaredLength = modelFD.getDeclaredLength();
        return fileChannel.map(FileChannel.MapMode.READ_ONLY, startOffset, declaredLength);
    }

    private String loadJSONConfig(InputStream configStream) throws Exception {
        int size = configStream.available();
        byte[] buffer = new byte[size];
        configStream.read(buffer);
        configStream.close();

        return new String(buffer, StandardCharsets.UTF_8);
    }
}
