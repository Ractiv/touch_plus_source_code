#Build instructions:
1. Install Visual Studio Community 2015

2. Quit Visual Studio if it is already running, then add the following variables to "Windows Environment Variables" (replace "root_dir" with the path of the cloned repository, e.g. "C:\touch_plus_source_code")
<pre>
DIRECTSHOW_DIR      root_dir\dependencies\Windows\DirectShow
ETRON_DIR           root_dir\dependencies\Windows\Etron
OPENCV_DIR          root_dir\dependencies\Windows\OpenCV\build\x86\vc12
SFML_DIR      	    root_dir\dependencies\Windows\SFML
LIBJPEG-TURBO_DIR   root_dir\dependencies\Windows\libjpeg-turbo
</pre>

3. Start Visual Studio with administrator permissions, then open "root_dir\track_plus_visual_studio\track_plus.sln"

4. Press CTRL+SHIFT+B to build solution. If solution builds without a hiccup, you're all set. If an error is thrown, double click on the error in "Error List", and it should open up "winnt.h". Locate the following lines:
<pre>
typedef void *PVOID;
typedef void * POINTER_64 PVOID64;
</pre>
Add one line, so that they become:
<pre>
#define POINTER_64 __ptr64
typedef void *PVOID;
typedef void * POINTER_64 PVOID64;
</pre>
If you couldn't save "winnt.h", restart Visual Studio as administrator and try again. You should be able to build the solution now.