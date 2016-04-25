using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace PacketTester
{
    class packetHandler
    {
        const byte startByte = 0xDE;
        const byte escapeByte = 0xDF;
        byte[] payload;
        UInt16 payloadSize;
        bool packetComplete;
        bool escapeNextByte;
        UInt16 receivedBytes;

        public packetHandler()
        {

        }

    }
}
