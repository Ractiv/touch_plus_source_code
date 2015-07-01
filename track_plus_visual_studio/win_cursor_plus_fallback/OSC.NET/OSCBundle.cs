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

namespace OSC.NET
{
	/// <summary>
	/// OSCBundle
	/// </summary>
	public class OSCBundle : OSCPacket
	{
		protected const string BUNDLE = "#bundle";
		private long timestamp = 0;
		
		public OSCBundle(long ts)
		{
			this.address = BUNDLE;
			this.timestamp = ts;
		}

		public OSCBundle()
		{
			this.address = BUNDLE;
			this.timestamp = 0;
		}

		override protected void pack()
		{
			ArrayList data = new ArrayList();

			addBytes(data, packString(this.Address));
			padNull(data);
			addBytes(data, packLong(0)); // TODO
			
			foreach(object value in this.Values)
			{
				if(value is OSCPacket)
				{
					byte[] bs = ((OSCPacket)value).BinaryData;
					addBytes(data, packInt(bs.Length));
					addBytes(data, bs);
				}
				else 
				{
					// TODO
				}
			}
			
			this.binaryData = (byte[])data.ToArray(typeof(byte));
		}

		public static new OSCBundle Unpack(byte[] bytes, ref int start, int end)
		{

			string address = unpackString(bytes, ref start);
			//Console.WriteLine("bundle: " + address);
			if(!address.Equals(BUNDLE)) return null; // TODO

			long timestamp = unpackLong(bytes, ref start);
			OSCBundle bundle = new OSCBundle(timestamp);
			
			while(start < end)
			{
				int length = unpackInt(bytes, ref start);
				int sub_end = start + length;
				//Console.WriteLine(bytes.Length +" "+ start+" "+length+" "+sub_end);
				bundle.Append(OSCPacket.Unpack(bytes, ref start, sub_end));

			}

			return bundle;
		}

		public long getTimeStamp() {
			return timestamp;
		}

		override public void Append(object value)
		{
			if( value is OSCPacket) 
			{
				values.Add(value);
			}
			else 
			{
				// TODO: exception
			}
		}

		override public bool IsBundle() { return true; }
	}
}

