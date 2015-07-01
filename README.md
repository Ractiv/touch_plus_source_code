#Build instructions:
1. Install Visual Studio 2013 (do not install any updates, Visual Studio 2013 Update 4 has been proven to produce bad builds. Other versions of Visual Studio have not yet been tested).

2. Quit Visual Studio if it is already running, then add the following variables to "Windows Environment Variables" (replace "root_dir" with the path of the cloned repository, e.g. "C:\touch_plus_source_code")
<pre>
DIRECTSHOW_DIR  root_dir\dependencies\DirectShow
ETRON_DIR       root_dir\dependencies\Etron
OPENCV_DIR      root_dir\dependencies\Opencv\build\x86\vc12
SFML_DIR        root_dir\dependencies\SFML
</pre>

3. Start Visual Studio with administrator permissions, then open "root_dir\track_plus_visual_studio\track_plus.sln"

4. Press F7 to build solution. If solution builds without a hiccup, you're all set. If an error is thrown, double click on the error in "Error List", and it should open up "winnt.h". Locate the following lines:
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
