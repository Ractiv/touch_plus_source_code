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
using System.Collections;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Forms;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Interop;
using OSC.NET;
using TUIO;

namespace win_cursor_plus_fallback
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow:Window
    {
        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);
            var hwnd = new WindowInteropHelper(this).Handle;
            WindowsServices.SetWindowExTransparent(hwnd);
        }
        
        private static float xCursorIndex = 9999;
        private static float yCursorIndex = 9999;
        private static float zCursorIndex = 9999;

        private static float xCursorThumb = 9999;
        private static float yCursorThumb = 9999;
        private static float zCursorThumb = 9999;

        private static float xCursorIndexOld = 9999;
        private static float yCursorIndexOld = 9999;
        private static float zCursorIndexOld = 9999;

        private static float xCursorThumbOld = 9999;
        private static float yCursorThumbOld = 9999;
        private static float zCursorThumbOld = 9999;

        private static bool showCursorIndex = false;
        private static bool showCursorThumb = false;

        private static bool cursorIndexDown = false;
        private static bool cursorIndexDownOld = false;
        private static bool cursorThumbDown = false;
        private static bool cursorThumbDownOld = false;

        private static bool useTUIO = false;

        private static int tuioFSeq = 0;
        private static OSCTransmitter transmitter = new OSCTransmitter("127.0.0.1", 3333);

        private static int screenWidth = Screen.PrimaryScreen.Bounds.Width;
        private static int screenHeight = Screen.PrimaryScreen.Bounds.Height;
        private static int windowWidth = screenWidth;
        private static int windowHeight = screenHeight;

        private static int updateNumNew = 0;
        private static int updateNumOld = 0;

        [DllImport("user32.dll",CharSet=CharSet.Auto, CallingConvention=CallingConvention.StdCall)]
        public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint cButtons, uint dwExtraInfo);

        private const int MOUSEEVENTF_LEFTDOWN = 0x02;
        private const int MOUSEEVENTF_LEFTUP = 0x04;
        private const int MOUSEEVENTF_RIGHTDOWN = 0x08;
        private const int MOUSEEVENTF_RIGHTUP = 0x10;

        private bool mouseIsDown = false;

        public void mouseDown()
        {
            if (!mouseIsDown)
            {
                mouse_event(MOUSEEVENTF_LEFTDOWN, (uint)xCursorIndex, (uint)yCursorIndex, 0, 0);
                mouseIsDown = true;
            }
        }

        public void mouseUp()
        {
            if (mouseIsDown)
            {
                mouse_event(MOUSEEVENTF_LEFTUP, (uint)xCursorIndex, (uint)yCursorIndex, 0, 0);
                mouseIsDown = false;
            }
        }

        public MainWindow()
        {
            this.Loaded += MainWindow_Loaded;

            InitializeComponent();

            mainCanvas.Width = Screen.PrimaryScreen.Bounds.Width;
            mainCanvas.Height = Screen.PrimaryScreen.Bounds.Height;

            BitmapImage bi0 = new BitmapImage();
            bi0.BeginInit();
            bi0.UriSource = new Uri(Globals.ExecutablePath + "\\win_cursor_plus\\cursor0.png");
            bi0.EndInit();

            BitmapImage bi1 = new BitmapImage();
            bi1.BeginInit();
            bi1.UriSource = new Uri(Globals.ExecutablePath + "\\win_cursor_plus\\cursor1.png");
            bi1.EndInit();

            cursorImageIndex.Source = bi0;
            cursorImageIndex.Width = cursorImageIndex.Height = 100;

            cursorImageThumb.Source = bi0;
            cursorImageThumb.Width = cursorImageThumb.Height = 100;

            cursorImageIndex.Opacity = 0;
            cursorImageThumb.Opacity = 0;

            IPC ipc = new IPC("win_cursor_plus");

            ipc.MapFunction("exit", delegate(string messageBody)
            {
                Environment.Exit(0);
                return 1;
            });

            ipc.SetUDPCallback(delegate(string message)
            {
                if (message != "hide_cursor_index" || message != "hide_cursor_thumb")
                {
                    string[] xyStr = message.Split('!');
                    
                    if (xyStr.Length == 5)
                    {
                        cursorIndexDown = false;
                        cursorThumbDown = false;

                        float x;
                        float y;
                        float z;
                        int down;

                        bool b0 = float.TryParse(xyStr[0], out x);
                        bool b1 = float.TryParse(xyStr[1], out y);
                        bool b2 = float.TryParse(xyStr[2], out z);
                        bool b3 = int.TryParse(xyStr[3], out down);

                        if (b0 && b1 && b2)
                            if (xyStr[4] == "index")
                            {
                                xCursorIndex = x * screenWidth / 1000;
                                yCursorIndex = y * screenHeight / 1000;
                                zCursorIndex = z;
                                showCursorIndex = true;
                                cursorIndexDown = down == 1;
                            }
                            else if (xyStr[4] == "thumb")
                            {
                                xCursorThumb = x * screenWidth / 1000;
                                yCursorThumb = y * screenHeight / 1000;
                                zCursorThumb = z;
                                showCursorThumb = true;
                                cursorIndexDown = true;
                                cursorThumbDown = true;
                            }
                    }
                    else if (message == "hide_cursor_index")
                        showCursorIndex = false;
                    else if (message == "hide_cursor_thumb")
                        showCursorThumb = false;
                    else if (xyStr.Length == 2 && xyStr[0] == "update")
                        updateNumNew = int.Parse(xyStr[1]);
                }

                return 1;
            });

            Timer timer = new Timer();
            timer.Interval = 20;
            timer.Tick += delegate(object o, EventArgs e)
            {
                ipc.Update();

                if (updateNumNew == updateNumOld)
                    return;

                updateNumOld = updateNumNew;

                if (!useTUIO && showCursorIndex)
                {
                    System.Windows.Forms.Cursor.Position = new System.Drawing.Point((int)xCursorIndex, (int)yCursorIndex);

                    if (cursorIndexDown)
                        mouseDown();
                    else
                        mouseUp();
                }
                else if (showCursorIndex)
                {
                    Object[] tCur0 = new Object[7];
                    Object[] tCur1 = new Object[7];
                    OSCBundle bundle = new OSCBundle();
                    bundle.Append(TUIO.TUIOFseq(tuioFSeq));
                    ArrayList TUIOSessions = new ArrayList();

                    if (cursorIndexDown)
                    {
                        bundle.Append(TUIO.TUIO2DcurExt(0, xCursorIndex / screenWidth, yCursorIndex / screenHeight, 0, 0, 0, 10, 10));
                        TUIOSessions.Add(0);
                    }
                    if (cursorThumbDown) {
                        bundle.Append(TUIO.TUIO2DcurExt(1, xCursorThumb / screenWidth, yCursorThumb / screenHeight, 0, 0, 0, 10, 10));
                        TUIOSessions.Add(1);
                    }
                    bundle.Append(TUIO.TUIOAlive(TUIOSessions));
                    transmitter.Send(bundle);
                    tuioFSeq++;
                }

                if (showCursorIndex)
                {
                    if (cursorIndexDown)
                        cursorImageIndex.Source = bi1;
                    else
                        cursorImageIndex.Source = bi0;

                    if (cursorImageIndex.Opacity < 1)
                        cursorImageIndex.Opacity += 0.2;

                    int cursorSize = (int)(zCursorIndex * 5 * (float)windowWidth / (float)screenWidth);
                    if (cursorSize > 200)
                        cursorSize = 200;
                    else if (cursorSize < 50)
                        cursorSize = 50;

                    cursorImageIndex.Width = cursorImageIndex.Height = cursorSize;

                    int xPosRemapped = (int)map_val(xCursorIndex, 0, screenWidth, 0, windowWidth);
                    int yPosRemapped = (int)map_val(yCursorIndex, 0, screenHeight, 0, windowHeight);

                    Canvas.SetLeft(cursorImageIndex, xPosRemapped - (cursorSize / 2));
                    Canvas.SetTop(cursorImageIndex, yPosRemapped - (cursorSize / 2));
                }
                else if (cursorImageIndex.Opacity > 0)
                    cursorImageIndex.Opacity -= 0.2;

                if (showCursorThumb)
                {
                    if (cursorThumbDown)
                        cursorImageThumb.Source = bi1;
                    else
                        cursorImageThumb.Source = bi0;

                    if (cursorImageThumb.Opacity < 1)
                        cursorImageThumb.Opacity += 0.2;

                    int cursorSize = (int)(zCursorThumb * 5 * (float)windowWidth / (float)screenWidth);
                    if (cursorSize > 200)
                        cursorSize = 200;
                    else if (cursorSize < 50)
                        cursorSize = 50;

                    cursorImageThumb.Width = cursorImageThumb.Height = cursorSize;

                    int xPosRemapped = (int)map_val(xCursorThumb, 0, screenWidth, 0, windowWidth);
                    int yPosRemapped = (int)map_val(yCursorThumb, 0, screenHeight, 0, windowHeight);

                    Canvas.SetLeft(cursorImageThumb, xPosRemapped - (cursorSize / 2));
                    Canvas.SetTop(cursorImageThumb, yPosRemapped - (cursorSize / 2));
                }
                else if (cursorImageThumb.Opacity > 0)
                    cursorImageThumb.Opacity -= 0.2;

                cursorIndexDownOld = cursorIndexDown;
                cursorThumbDownOld = cursorThumbDown;

                xCursorThumbOld = xCursorThumb;
                yCursorThumbOld = yCursorThumb;
                zCursorThumbOld = zCursorThumb;

                xCursorIndexOld = xCursorIndex;
                yCursorIndexOld = yCursorIndex;
                zCursorIndexOld = zCursorIndex;
            };

            timer.Start();
        }

        void MainWindow_Loaded(object sender, RoutedEventArgs e)
        {
            windowHeight = (int)((System.Windows.Controls.Panel)System.Windows.Application.Current.MainWindow.Content).ActualHeight;
            windowWidth = (int)((System.Windows.Controls.Panel)System.Windows.Application.Current.MainWindow.Content).ActualWidth;
        }

        float map_val(float value, float left_min, float left_max, float right_min, float right_max)
        {
            float left_span = left_max - left_min;
            float right_span = right_max - right_min;
            float value_scaled = (value - left_min) / left_span;
            return right_min + (value_scaled * right_span);
        }
    }

    public static class WindowsServices
    {
        const int WS_EX_TRANSPARENT = 0x00000020;
        const int GWL_EXSTYLE = (-20);

        [DllImport("user32.dll")]
        static extern int GetWindowLong(IntPtr hwnd, int index);

        [DllImport("user32.dll")]
        static extern int SetWindowLong(IntPtr hwnd, int index, int newStyle);

        public static void SetWindowExTransparent(IntPtr hwnd)
        {
            var extendedStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
            SetWindowLong(hwnd, GWL_EXSTYLE, extendedStyle | WS_EX_TRANSPARENT);

            IntPtr hWndHiddenOwner = GetWindow(hwnd, GetWindowCmd.GW_OWNER);

            if ( hWndHiddenOwner != IntPtr.Zero )
            {
                IntPtr HWND_TOPMOST = new IntPtr(-1);
                SetWindowPos( hWndHiddenOwner, HWND_TOPMOST, 0, 0, 0, 0,
                SetWindowPosFlags.SWP_NOMOVE |
                SetWindowPosFlags.SWP_NOSIZE |
                SetWindowPosFlags.SWP_NOACTIVATE );
            }
        }

        public enum GetWindowCmd : uint
        {
            GW_HWNDFIRST = 0,
            GW_HWNDLAST = 1,
            GW_HWNDNEXT = 2,
            GW_HWNDPREV = 3,
            GW_OWNER = 4,
            GW_CHILD = 5,
            GW_ENABLEDPOPUP = 6
        }

        [DllImport( "user32.dll" )]
        public static extern IntPtr GetWindow( IntPtr hWnd, GetWindowCmd uCmd );

        [Flags]
        public enum SetWindowPosFlags
        {
            SWP_NOSIZE = 0x0001,
            SWP_NOMOVE = 0x0002,
            SWP_NOZORDER = 0x0004,
            SWP_NOREDRAW = 0x0008,
            SWP_NOACTIVATE = 0x0010,
            SWP_FRAMECHANGED = 0x0020,
            SWP_SHOWWINDOW = 0x0040,
            SWP_HIDEWINDOW = 0x0080,
            SWP_NOCOPYBITS = 0x0100,
            SWP_NOOWNERZORDER = 0x0200,
            SWP_NOSENDCHANGING = 0x0400
        }

        [DllImport( "user32.dll" )]
        public static extern int SetWindowPos( IntPtr hWnd, IntPtr hWndInsertAfter, int x, int y, int cx, int cy, SetWindowPosFlags uFlags );
    }
}
