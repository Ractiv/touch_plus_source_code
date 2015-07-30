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
using TCD.System.TouchInjection;
using System.Runtime.InteropServices;
using System.Windows.Interop;
using OSC.NET;
using TUIO;

namespace win_cursor_plus
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

        private static bool useTUIO = true;

        private static int tuioFSeq = 0;
        private static OSCTransmitter transmitter = new OSCTransmitter("127.0.0.1", 3333);

        private static int screenWidth = Screen.PrimaryScreen.Bounds.Width;
        private static int screenHeight = Screen.PrimaryScreen.Bounds.Height;
        private static int windowWidth = screenWidth;
        private static int windowHeight = screenHeight;

        private static int updateNumNew = 0;
        private static int updateNumOld = 0;

        public MainWindow()
        {
            this.Loaded += MainWindow_Loaded;

            InitializeComponent();

            TouchInjector.InitializeTouchInjection();

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
                ipc.Clear();
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

                        bool b0 = float.TryParse(xyStr[0], System.Globalization.NumberStyles.Float, null, out x);
                        bool b1 = float.TryParse(xyStr[1], System.Globalization.NumberStyles.Float, null, out y);
                        bool b2 = float.TryParse(xyStr[2], System.Globalization.NumberStyles.Float, null, out z);
                        bool b3 = int.TryParse(xyStr[3], System.Globalization.NumberStyles.Float, null, out down);

                        if (b0 && b1 && b2)
                            if (xyStr[4] == "index")
                            {
                                xCursorIndex = x * screenWidth / 1000;
                                if (xCursorIndex < 0)
                                    xCursorIndex = 0;
                                else if (xCursorIndex > screenWidth)
                                    xCursorIndex = screenWidth;

                                yCursorIndex = y * screenHeight / 1000;
                                if (yCursorIndex < 0)
                                    yCursorIndex = 0;
                                else if (yCursorIndex > screenHeight)
                                    yCursorIndex = screenHeight;

                                zCursorIndex = z;

                                showCursorIndex = true;
                                cursorIndexDown = down == 1;
                            }
                            else if (xyStr[4] == "thumb")
                            {
                                xCursorThumb = x * screenWidth / 1000;
                                if (xCursorThumb < 0)
                                    xCursorThumb = 0;
                                else if (xCursorThumb > screenWidth)
                                    xCursorThumb = screenWidth;

                                yCursorThumb = y * screenHeight / 1000;
                                if (yCursorThumb < 0)
                                    yCursorThumb = 0;
                                else if (yCursorThumb > screenHeight)
                                    yCursorThumb = screenHeight;

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

            Timer ipcTimer = new Timer();
            ipcTimer.Interval = 100;
            ipcTimer.Tick += delegate(object o, EventArgs e)
            {
                ipc.Update();
            };
            ipcTimer.Start();

            Timer timer = new Timer();
            timer.Interval = 20;
            timer.Tick += delegate(object o, EventArgs e)
            {
                if (updateNumNew == updateNumOld && showCursorIndex)
                    return;

                updateNumOld = updateNumNew;

                if (!useTUIO && showCursorIndex)
                {
                    List<PointerTouchInfo> contacts = new List<PointerTouchInfo>();

                    if (cursorIndexDown && !cursorIndexDownOld)
                    {
                        PointerTouchInfo contact = MakePointerTouchInfo((int)xCursorIndex, (int)yCursorIndex, 2, 1);
                        contact.PointerInfo.PointerFlags = PointerFlags.DOWN | PointerFlags.INRANGE | PointerFlags.INCONTACT;
                        contacts.Add(contact);
                    }
                    else if (cursorIndexDown && cursorIndexDownOld)
                    {
                        PointerTouchInfo contact = MakePointerTouchInfo((int)xCursorIndex, (int)yCursorIndex, 2, 1);
                        contact.PointerInfo.PointerFlags = PointerFlags.UPDATE | PointerFlags.INRANGE | PointerFlags.INCONTACT;
                        contacts.Add(contact);
                    }
                    else if (!cursorIndexDown && cursorIndexDownOld)
                    {
                        PointerTouchInfo contact = MakePointerTouchInfo((int)xCursorIndexOld, (int)yCursorIndexOld, 2, 1);
                        contact.PointerInfo.PointerFlags = PointerFlags.UP;
                        contacts.Add(contact);
                    }

                    if (cursorThumbDown && !cursorThumbDownOld)
                    {
                        PointerTouchInfo contact = MakePointerTouchInfo((int)xCursorThumb, (int)yCursorThumb, 2, 2);
                        contact.PointerInfo.PointerFlags = PointerFlags.DOWN | PointerFlags.INRANGE | PointerFlags.INCONTACT;
                        contacts.Add(contact);
                    }
                    else if (cursorThumbDown && cursorThumbDownOld)
                    {
                        PointerTouchInfo contact = MakePointerTouchInfo((int)xCursorThumb, (int)yCursorThumb, 2, 2);
                        contact.PointerInfo.PointerFlags = PointerFlags.UPDATE | PointerFlags.INRANGE | PointerFlags.INCONTACT;
                        contacts.Add(contact);
                    }
                    else if (!cursorThumbDown && cursorThumbDownOld)
                    {
                        PointerTouchInfo contact = MakePointerTouchInfo((int)xCursorThumbOld, (int)yCursorThumbOld, 2, 2);
                        contact.PointerInfo.PointerFlags = PointerFlags.UP;
                        contacts.Add(contact);
                    }

                    bool success = TouchInjector.InjectTouchInput(contacts.Count, contacts.ToArray());
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
                    if (cursorSize > 150)
                        cursorSize = 150;
                    else if (cursorSize < 50)
                        cursorSize = 50;

                    cursorImageIndex.Width = cursorImageIndex.Height = cursorSize;

                    int xPosRemapped = (int)map_val(xCursorIndex, 0, screenWidth, 0, windowWidth);
                    int yPosRemapped = (int)map_val(yCursorIndex, 0, screenHeight, 0, windowHeight);

                    if (xPosRemapped < 0)
                        xPosRemapped = 0;
                    else if (xPosRemapped > screenWidth)
                        xPosRemapped = screenWidth;
                    if (yPosRemapped < 0)
                        yPosRemapped = 0;
                    else if (yPosRemapped > screenHeight)
                        yPosRemapped = screenHeight;

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
                    if (cursorSize > 150)
                        cursorSize = 150;
                    else if (cursorSize < 50)
                        cursorSize = 50;

                    cursorImageThumb.Width = cursorImageThumb.Height = cursorSize;

                    int xPosRemapped = (int)map_val(xCursorThumb, 0, screenWidth, 0, windowWidth);
                    int yPosRemapped = (int)map_val(yCursorThumb, 0, screenHeight, 0, windowHeight);

                    if (xPosRemapped < 0)
                        xPosRemapped = 0;
                    else if (xPosRemapped > screenWidth)
                        xPosRemapped = screenWidth;
                    if (yPosRemapped < 0)
                        yPosRemapped = 0;
                    else if (yPosRemapped > screenHeight)
                        yPosRemapped = screenHeight;

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

        private PointerTouchInfo MakePointerTouchInfo(int x, int y, int radius, uint id, uint orientation = 90, uint pressure = 32000)
        {
            PointerTouchInfo contact = new PointerTouchInfo();
            contact.PointerInfo.pointerType = PointerInputType.TOUCH;
            contact.TouchFlags = TouchFlags.NONE;
            contact.Orientation = orientation;
            contact.Pressure = pressure;
            contact.PointerInfo.PointerFlags = PointerFlags.DOWN | PointerFlags.INRANGE | PointerFlags.INCONTACT;
            contact.TouchMasks = TouchMask.CONTACTAREA | TouchMask.ORIENTATION | TouchMask.PRESSURE;
            contact.PointerInfo.PtPixelLocation.X = x;
            contact.PointerInfo.PtPixelLocation.Y = y;
            contact.PointerInfo.PointerId = id;
            contact.ContactArea.left = x - radius;
            contact.ContactArea.right = x + radius;
            contact.ContactArea.top = y - radius;
            contact.ContactArea.bottom = y + radius;
            return contact;
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
