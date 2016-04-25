using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace PacketTester
{
    public enum PacketStatus { PacketComplete, PacketError, Processing, newPacketDetected};
    public class RawPacket
    {
        const byte startByte = 0xDE;
        const byte escapeByte = 0xDF;
        const byte escapedByteOffset = 0x10;
        

        const UInt32 maxPacketSize = 255; 
        byte[] payload;
        UInt16 payloadSize;
        bool packetComplete;
        bool escapeFlag;
        UInt16 bytesReceived;
        byte[] rawPacketBytes;
        UInt16 rawPacketBytesIndex; 

        public byte[] Payload
        {
            get
            {
                return payload;
            }
        }
        public ushort PayloadSize
        {
            get
            {
                return payloadSize;
            }
        }
        public ushort BytesReceived
        {
            get
            {
                return bytesReceived;
            }
        }
        public RawPacket()
        {
            payload = new byte[maxPacketSize];
            payloadSize = 0;
            packetComplete = false;
            escapeFlag = false;
            bytesReceived = 0;
            rawPacketBytes = new byte[maxPacketSize]; 
        }
        public RawPacket(byte[] payloadBytes, UInt16 payloadBytesSize)
        {
            payload = payloadBytes;
            payloadSize = payloadBytesSize;
            packetComplete = false;
            bytesReceived = 0;
            rawPacketBytes = new byte[maxPacketSize];
            rawPacketBytesIndex = 0;
        }
        public RawPacket(RawPacket packet)
        {
            payload = new byte[maxPacketSize];
            Buffer.BlockCopy(packet.payload, 0, this.payload, 0, packet.payloadSize);
            payloadSize = packet.payloadSize;
            packetComplete = false;
            bytesReceived = 0;
            rawPacketBytes = new byte[maxPacketSize];
            rawPacketBytesIndex = 0;
        }
        public void resetPacket()
        {
            payloadSize = 0;
            packetComplete = false;
            escapeFlag = false;
            bytesReceived = 0;
        }
        public RawPacket DeepCopy()
        {
            RawPacket other = (RawPacket)this.MemberwiseClone();
            other.payload = this.payload;
            Buffer.BlockCopy(other.payload, 0, this.payload, 0, this.payloadSize);
            other.payloadSize = this.payloadSize;
            return other;
        }
        public PacketStatus processByte(byte incoming)
        {
            PacketStatus status = PacketStatus.Processing; 
            //if byte is start byte
            if (incoming == startByte)
            {
                if (bytesReceived > 0)
                {
                    //this means there was an error receiving a packet
                    status = PacketStatus.PacketError; 
                }
                else
                {
                    status = PacketStatus.newPacketDetected; 
                }
                //reset the counts and everything for reception of the packet
                bytesReceived = 0;
                escapeFlag = false;
                payloadSize = 0;
                return status; 
            }
            //if byte is escape byte
            if (incoming == escapeByte)
            {
                //set escape flag, so the next byte is properly offset. 
                escapeFlag = true;
                return status;
            }

            //if escape byte flag is set
            if (escapeFlag == true)
            {
                //un-escape the byte and process it as any other byte.
                incoming = (byte)((int)incoming - (int)escapedByteOffset);
                //unset the flag
                escapeFlag = false;
            }

            //if receive count is  0 
            if (bytesReceived == 0)
            {
                //this is the first byte of the payload size		
                //copy byte to LSB of the payload size
                payloadSize |= (UInt16)incoming;
                //increment received count
                bytesReceived++;
            }
            else if (bytesReceived == 1)
            {
                //this is the second byte of the payload size
                //copy byte to MSB of the payload size
                payloadSize |= (UInt16)(incoming << 8);
                //increment received count
                bytesReceived++;
            }
            else
            {   //copy byte to payload at point receivedBytes - 2
                if ((bytesReceived - 2) >= payload.Length)
                {
                    status = PacketStatus.PacketError;
                }
                else
                {
                    payload[bytesReceived++ - 2] = incoming;
                    //check if we received the whole packet.
                    if (bytesReceived == (payloadSize + 2))
                    {
                        //process the packet
                        status = PacketStatus.PacketComplete;
                    }
                }
            }
            return status;
        }

        void addByteToRawPacket(byte rawByte)
        {
            if(rawByte == escapeByte || rawByte == startByte)
            {
                rawPacketBytes[rawPacketBytesIndex++] = escapeByte;
                rawPacketBytes[rawPacketBytesIndex++] = (byte)((int)rawByte + (int)escapedByteOffset);
            }
            else
            {
                rawPacketBytes[rawPacketBytesIndex++] = rawByte; 
            }
        }
        public byte[] createRawPacket(ref UInt16 rawSize)
        {
            rawSize = 0;
            rawPacketBytes[rawPacketBytesIndex++] = startByte;
            addByteToRawPacket((byte)(payloadSize & 0x00ff));
            addByteToRawPacket((byte)((payloadSize >> 8) & 0x00ff));
                     
            for (int i = 0; i < payloadSize; i++)
            {
                addByteToRawPacket(payload[i]); 
            }
            //return the size of the raw bytes. 
            rawSize = rawPacketBytesIndex; 
            return rawPacketBytes; 

        }
        
    }
}
