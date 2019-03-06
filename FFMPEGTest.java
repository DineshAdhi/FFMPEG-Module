import java.util.*;

public class FFMPEGTest
{
    static {
        System.loadLibrary("ffmpeg"); 
    }

    public static void main(String args[])
    {
        String main_file = "/Users/dinesh-6810/Desktop/SonyTest.mp4";
        String merge_file = "/Users/dinesh-6810/Desktop/4KTest.mp4";
        String out_file = "/Users/dinesh-6810/Desktop/Output/JNIFile.mp4";

        FFMPEGTest t = new FFMPEGTest();

        long context = t.getContext(out_file);
        t.cutvideo(context, main_file, 60, 120);
        t.executemux(context);
    }

    private native long getContext(String out_file);
    private native void mergevideo(long context, String videofile);
    private native void cutvideo(long context, String videofile, int start_time, int end_time);
    private native void insertvideo(long context, String main_video, String insert_video, int timestamp);
    private native void executemux(long context);
}