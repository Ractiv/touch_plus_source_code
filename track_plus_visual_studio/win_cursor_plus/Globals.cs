using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace win_cursor_plus
{
    class Globals
    {
        public static string ExecutablePath = Directory.GetCurrentDirectory().Replace("\\win_cursor_plus", "");
        public static string IpcPath = ExecutablePath + "\\ipc";
    }
}
