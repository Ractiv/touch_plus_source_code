using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace win_cursor_plus
{
    class FileSystem
    {
        public static List<string> ListFilesInDirectory(string path)
        {
            List<string> result = new List<string>();
            DirectoryInfo apple = new DirectoryInfo(@path);
            foreach (FileInfo file in apple.GetFiles("*"))
                result.Add(file.Name);

            return result;
        }

        public static void WriteStringToFile(string path, string str)
        {
            File.WriteAllText(path, str);
        }

        public static List<string> ReadTextFile(string path)
        {
            List<string> result = new List<string>();
            string line;

            StreamReader file = new System.IO.StreamReader(path);
            while((line = file.ReadLine()) != null)
                result.Add(line);

            file.Close();
            return result;
        }

        public static void DeleteFile(string path)
        {
            File.Delete(path);
        }

        public static void RenameFile(string pathOld, string pathNew)
        {
            string fileNameNew = Path.GetFileName(pathNew);
            Microsoft.VisualBasic.FileIO.FileSystem.RenameFile(pathOld, fileNameNew);
        }
    }
}
