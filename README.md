#Build instructions:
1. Install Visual Studio Community 2015

2. Start Visual Studio with administrator permissions, then open "root_dir\track_plus_visual_studio\track_plus.sln"

3. Press CTRL+SHIFT+B to build solution. If solution builds without a hiccup, you're all set. If an error is thrown, double click on the error in "Error List", and it should open up "winnt.h". Locate the following lines:
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