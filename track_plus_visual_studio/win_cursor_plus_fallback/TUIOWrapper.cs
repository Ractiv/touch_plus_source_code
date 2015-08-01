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
using System.Collections;
using System.Collections.Generic;
using System.Text;
using OSC.NET;

namespace win_cursor_plus
{
    public class TUIO
    {

        public static OSCMessage TUIO2Dcur(int session, float x, float y, float motion, float rotation)
        {
            return TUIOParams("set", session, x, y, motion, rotation);
        }

        //touchlib extended format: d.ID << d.X << d.Y << d.dX << d.dY << m << d.width << d.height
        public static OSCMessage TUIO2DcurExt(int session, float x, float y, float dX, float dY, float motion, float height, float width)
        {
            return TUIOParams("set", session, x, y, dX, dY, motion, height, width);
        }

        public static OSCMessage TUIOFseq(int fseq)
        {
            OSCMessage oscm = new OSCMessage("/tuio/2Dcur");
            oscm.Append("fseq");
            oscm.Append(fseq);
            return oscm;
        }

        public static OSCMessage TUIOAlive(ArrayList args)
        {
            return TUIOAlive(args.ToArray());
        }

        public static OSCMessage TUIOAlive(params object[] args)
        {
            OSCMessage oscm = new OSCMessage("/tuio/2Dcur");
            oscm.Append("alive");

            for (int I = 0; I < args.Length; I++)
            {
                oscm.Append(args[I]);
            }
            return oscm;
        }

        public static OSCMessage TUIOParams(params object[] args)
        {
            OSCMessage oscm = new OSCMessage("/tuio/2Dcur");

            for (int I = 0; I < args.Length; I++)
            {
                oscm.Append(args[I]);
            }
            return oscm;
        }
    }
}