// Java JNI wrapper to load allegro
// and execute program
// Author: Daniel Giritzer
package org.microwindows._skeleton;
import org.liballeg.android.AllegroActivity;
public class MainActivity extends AllegroActivity {
    // load monolithic allegro so
    static {
	System.loadLibrary("allegro");
    }
   // execute _skeleton
   public MainActivity() {
      super("_skeleton.so");
   }
}
