# HookTools
Plugin for Process Hacker 2 (http://processhacker.sourceforge.net), displays system hooks and able to unhook (right click menu).

Grab the Process Hacker source and compile it, then add HookTools.vcxproj to Plugins.sln. VS 2013 was used. Set your library path in VC++ directories.

This project is based on https://github.com/jay/gethooks.git

Windows 7 x64 required, will not work on Windows 8/10, would be great if someone finds the solution, GetProcAddress(LoadLibraryA("user32"), "gSharedInfo") doesn't work in those OS's.
