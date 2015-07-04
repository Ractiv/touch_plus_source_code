/*
 * Touch+ Software
 * Copyright (C) 2015
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Aladdin Free Public License as
 * published by the Aladdin Enterprises, either version 9 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Aladdin Free Public License for more details.
 *
 * You should have received a copy of the Aladdin Free Public License
 * along with this program.  If not, see <http://ghostscript.com/doc/8.54/Public.htm>.
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace Assets
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
            while ((line = file.ReadLine()) != null)
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
            System.IO.File.Move(pathOld, pathNew);
        }
    }
}