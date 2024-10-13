package com.gradotech.soundbound;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;

import fi.iki.elonen.NanoHTTPD;

public class AudioServer extends NanoHTTPD {

    private String selectedFilePath;
    private boolean isPlaying;

    public AudioServer(int port) {
        super(port);
        isPlaying = false;
    }

    public boolean isPlaying() {
        return isPlaying;
    }

    public void setPlayingStatus(boolean isPlaying) {
        this.isPlaying = isPlaying;
    }

    @Override
    public Response serve(IHTTPSession session) {
        Method method = session.getMethod();
        String uri = session.getUri();

        if (Method.GET.equals(method) && "/stream".equals(uri)) {
            return handleStreamRequest(session);
        }

        return newFixedLengthResponse("Invalid URL");
    }

    private Response handleStreamRequest(IHTTPSession session) {
        File file = new File(selectedFilePath);
        if (file.exists()) {
            try {
                FileInputStream fileInputStream = new FileInputStream(file);
                return newChunkedResponse(Response.Status.OK, "audio/mp3", fileInputStream);
            } catch (IOException e) {
                e.printStackTrace();
                return newFixedLengthResponse("Error serving the file");
            }
        } else {
            return newFixedLengthResponse("File not found");
        }
    }

    public void startServer() {
        try {
            start(NanoHTTPD.SOCKET_READ_TIMEOUT, false);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void setFilePath(String selectedFilePath) {
        this.selectedFilePath = selectedFilePath;
    }
}

