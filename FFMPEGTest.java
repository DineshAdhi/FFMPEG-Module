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

        new FFMPEGTest().mergevideo(main_file, merge_file, out_file);
    }

    private native void mergevideo(String a, String b, String out);
}