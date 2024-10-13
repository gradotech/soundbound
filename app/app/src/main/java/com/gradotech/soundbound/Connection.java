package com.gradotech.soundbound;

import android.os.Handler;
import android.util.Log;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingDeque;

public class Connection extends Thread {
    static class CommandQueue {
        static class Command {
            private final byte[] buff;

            public Command(byte[] buff) {
                this.buff = buff;
            }

            public byte[] getBuff() {
                return buff;
            }
        }

        private final BlockingQueue<Command> cmdQeueue;

        public CommandQueue() {
            cmdQeueue = new LinkedBlockingDeque<>();
        }

        public void enqueueCmd(byte[] cmd) {
            cmdQeueue.add(new Command(cmd));
        }

        public byte[] dequeueCmd() throws InterruptedException {
            byte[] buff;

            buff = cmdQeueue.take().getBuff();

            return buff;
        }
    }

    MainActivity activity;
    private final int startIpSuffix;
    private final int endIpSuffix;
    private final String ipAddress;
    private final int targetPort;
    private Socket socket;
    private CommandQueue commandQueue;
    private boolean isInitDone;

    public Connection(MainActivity activity, int startIpSuffix, int endIpSuffix, String ipAddress,
                      int targetPort) {
        this.activity = activity;

        this.startIpSuffix = startIpSuffix;
        this.endIpSuffix = endIpSuffix;
        this.targetPort = targetPort;
        this.ipAddress = ipAddress;

        this.commandQueue = new CommandQueue();
        this.isInitDone = false;
    }

    private byte[] doHandshake()
    {
        for (int i = startIpSuffix; i <= endIpSuffix; i++) {
            String ip = ipAddress + i;

            try {
                socket = new Socket();
                socket.connect(new InetSocketAddress(ip, targetPort), 100);

                while(true) {
                    int length = activity.getQDataPacketSize();
                    byte[] buff = new byte[length];
                    int n = socket.getInputStream().read(buff);
                    if (n > 1)
                        return buff;
                }
            } catch (IOException ignored)
            { }
        }

        return null;
    }

    public void emitCommand(byte[] cmd) {
        commandQueue.enqueueCmd(cmd);
    }

    @Override
    public void run() {
        while (this.isAlive()) {
            if (isInitDone) {
                try {
                    /* Block and wait for cmd to be available */
                    byte[] cmd = commandQueue.dequeueCmd();

                    socket.getOutputStream().write(cmd);
                } catch (IOException e) {
                    Log.e(this.getClass().getName(), "Failed to write command");
                } catch (InterruptedException ignored) {}

            } else {
                Handler mainHandler = new Handler(activity.getMainLooper());
                Runnable runnable;
                byte[] packet = doHandshake();

                isInitDone = (packet != null);

                if (isInitDone)
                    runnable = () -> activity.handshakeDone(packet);
                else
                    runnable = activity::handshakeFailed;

                mainHandler.post(runnable);
            }
        }
    }

    public void close()
    {
        try {
            this.socket.close();
        } catch (IOException e) {
            Log.e(this.getClass().getName(), "Failed to close socket: " + e.getMessage());
        }
    }
}