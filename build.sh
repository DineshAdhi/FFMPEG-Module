JAVA_HOME=/Library/Java/JavaVirtualMachines/jdk1.8.0_77.jdk/Contents/Home

gcc -I"$JAVA_HOME/include" -I"$JAVA_HOME/include/darwin" -dynamiclib -o libffmpeg.dylib FFMPEGTest.c native/log.c native/ffmpeg.c -lavformat -lavcodec

javac FFMPEGTest.java

java -Djava.library.path=. FFMPEGTest