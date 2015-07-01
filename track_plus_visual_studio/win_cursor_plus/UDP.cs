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
using System.Threading.Tasks;
using System.Net;
using System.Net.Sockets;
using System.Windows.Forms;

namespace win_cursor_plus
{
    class UDP
    {
        private Socket udpSock;
        private byte[] buffer;
        private EndPoint endPoint;

        private bool callbackSet = false;
        private Func<string, int> udpCallback;

        public int Assign()
        {
            //Setup the socket and message buffer
            udpSock = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
            endPoint = new IPEndPoint(IPAddress.Parse("127.0.0.1"), 0);
            udpSock.Bind(endPoint);
            buffer = new byte[1024];
         
            //Start listening for a new message.
            udpSock.BeginReceiveFrom(buffer, 0, buffer.Length, SocketFlags.None, ref endPoint, DoReceiveFrom, udpSock);

            int portCurrent = int.Parse(udpSock.LocalEndPoint.ToString().Split(':')[1]);
            return portCurrent;
        }
         
        private void DoReceiveFrom(IAsyncResult iar)
        {
            try
            {
                //Get the received message.
                Socket recvSock = (Socket)iar.AsyncState;
                int msgLen = recvSock.EndReceiveFrom(iar, ref endPoint);
                char[] localMsg = new char[msgLen];
                Array.Copy(buffer, localMsg, msgLen);
         
                //Start listening for a new message.
                udpSock.BeginReceiveFrom(buffer, 0, buffer.Length, SocketFlags.None, ref endPoint, DoReceiveFrom, udpSock);

                string message = new string(localMsg);
                if (callbackSet)
                    udpCallback(message);
            }
            catch (ObjectDisposedException)
            {
                //expected termination exception on a closed socket.
                // ...I'm open to suggestions on a better way of doing this.
            }
        }

        public void SetUDPCallback(Func<string, int> udpCallbackIn)
        {
            udpCallback = udpCallbackIn;
            callbackSet = true;
        }
    }
}
