using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;

namespace win_cursor_plus_fallback
{
    class IPC
    {
        public static bool Updated = true;
        public static Dictionary<string, bool> FileNameProcessedMap = new Dictionary<string, bool>();
        public static int SentCount = 0;

        private string selfName = "";
        private Dictionary<string, Func<string, int>> responseMap = new Dictionary<string, Func<string, int>>();
        private Dictionary<string, Func<string, int>> commandMap = new Dictionary<string, Func<string, int>>();
        private UDP udp = new UDP();

        public IPC(string selfNameIn)
        {
            selfName = selfNameIn;

            MapFunction("open udp channel", delegate(string messageBody)
            {
                int port = udp.Assign();
                SendMessage("track_plus", "open udp channel", port.ToString());
                Console.WriteLine("bound to UDP port " + port);
                return 1;
            });
        }

        public void Update()
        {
            if (!IPC.Updated)
                return;

            IPC.Updated = false;

            List<string> fileNameVec = FileSystem.ListFilesInDirectory(Globals.IpcPath);
            foreach (string fileNameCurrent in fileNameVec)
                if (fileNameCurrent.Length > selfName.Length)
                {
                    if (IPC.FileNameProcessedMap.ContainsKey(fileNameCurrent) && IPC.FileNameProcessedMap[fileNameCurrent] == true)
                        continue;
                    else
                        IPC.FileNameProcessedMap[fileNameCurrent] = true;

                    string fileName = fileNameCurrent.Substring(0, selfName.Length);
                    string fileNameID = fileNameCurrent.Substring(selfName.Length, fileNameCurrent.Length - selfName.Length);

                    if (fileName == selfName)
                    {
                        Thread.Sleep(10);

                        List<string> lines = FileSystem.ReadTextFile(Globals.IpcPath + "\\" + fileNameCurrent);
                        // FileSystem.DeleteFile(Globals.IpcPath + "\\" + fileNameCurrent);
                        
                        string[] messageVec = lines[0].Split('!');
                        string messageHead = messageVec[0];
                        string messageBody = messageVec[1];

                        Console.WriteLine("message received " + " " + messageHead + " " + messageBody + " " + fileNameCurrent);

                        if (!responseMap.ContainsKey(messageHead))
                        {
                            if (commandMap.ContainsKey(messageHead))
                                commandMap[messageHead](messageBody);
                        }
                        else
                        {
                            Func<string, int> func = responseMap[messageHead];
                            responseMap.Remove(messageHead);
                            func(messageBody);
                        }
                    }
                }

            IPC.Updated = true;
        }

        public void SendMessage(string recipient, string messageHead, string messageBody)
        {
            List<string> fileNameVec = FileSystem.ListFilesInDirectory(Globals.IpcPath);

            bool found = true;
            int fileCount = 0;

            while (found)
            {
                found = false;
                foreach (string fileNameCurrent in fileNameVec)
                    if (fileNameCurrent == recipient + fileCount.ToString())
                    {
                        found = true;
                        ++fileCount;
                        break;
                    }
            }

            string pathOld = Globals.IpcPath + "\\s" + selfName + IPC.SentCount.ToString();
            string pathNew = Globals.IpcPath + "\\" + recipient + fileCount.ToString();

            FileSystem.WriteStringToFile(pathOld, messageHead + "!" + messageBody);
            FileSystem.RenameFile(pathOld, pathNew);

            ++IPC.SentCount;

            Console.WriteLine("message sent: " + recipient + " " + messageHead + " " + messageBody);
        }

        public void GetResponse(string recipient, string messageHead, string messageBody, Func<string, int> callback)
        {
            SendMessage(recipient, messageHead, messageBody);
            responseMap[messageHead] = callback;
        }

        public void MapFunction(string messageHead, Func<string, int> callback)
        {
            commandMap[messageHead] = callback;
        }

        public void SetUDPCallback(Func<string, int> udpCallbackIn)
        {
            udp.SetUDPCallback(udpCallbackIn);
        }
    }
}
