using System;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using PacketTester;
using System.Diagnostics;
using System.IO.Ports;
using System.Threading;
using heddoko;
using ProtoBuf;
using System.Collections.Concurrent;

namespace ProtobufDecoder
{
    public partial class form_decoder : Form
    {
        private bool processDebugThreadEnabled = false;
        public ConcurrentQueue<string> debugMessageQueue;
        private bool processPacketQueueEnabled = false;
        public ConcurrentQueue<RawPacket> packetQueue;
        private Stopwatch mStopwatch =  new Stopwatch();
         enum CommandIds { update = 0x11, getFrame, getFrameResp, setupMode, buttonPress, setImuId, setImuIdResp, getStatus, getStatusResp };
        public form_decoder()
        {
            InitializeComponent();
        }

        private void form_decoder_Load(object sender, EventArgs e)
        {
            cb_serialPorts.Items.AddRange(SerialPort.GetPortNames());
            cb_forwardPorts.Items.AddRange(SerialPort.GetPortNames());
            string[] baudrates = { "110", "150", "300", "1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200", "230400"
                    , "460800","500000", "921600","1000000"};
            cb_BaudRate.Items.AddRange(baudrates);
            cb_BaudRate.SelectedIndex = 12;
            //initialize the message queue
            debugMessageQueue = new ConcurrentQueue<string>();
            //start the debug message thread
            processDebugThreadEnabled = true;
            Thread debugMessageThread = new Thread(processDebugMessagesThread);
            debugMessageThread.Start();

            packetQueue = new ConcurrentQueue<RawPacket>();
            processPacketQueueEnabled = true;
            Thread packetProcessorThread = new Thread(processPacketThread);
            packetProcessorThread.Start();
        }
        private void processProtoPacket(Packet packet)
        {
            switch (packet.type)
            {
                case PacketType.BrainPackVersionResponse:
                    if (packet.brainPackVersionSpecified)
                    {
                        debugMessageQueue.Enqueue("Received Version Response:" +
                            packet.brainPackVersion + "\r\n");
                    }
                    else
                    {
                        debugMessageQueue.Enqueue("Error Version not found\r\n");
                    }
                    break;
                case PacketType.BatteryChargeResponse:
                    debugMessageQueue.Enqueue("Received Battery Charge Response:" +
                        packet.batteryCharge.ToString() + "\r\n");
                    break;
                case PacketType.StateResponse:
                    break;
                case PacketType.DataFrame:
                    if (packet.fullDataFrame != null)
                    {
                        string convertedFrame = convertProtoBufPacket(packet); 
                        if(sp_foward.IsOpen && convertedFrame.Length > 0)
                        {
                            sp_foward.Write(convertedFrame);
                        }
                    }
                    break;
            }

        }

        private void processPacket(RawPacket packet)
        {
            //check that the packet comes from an IMU sensor
            if (packet.Payload[0] == 0x03)
            {
                CommandIds commandId = (CommandIds)packet.Payload[1];
                switch (commandId)
                {
                    case CommandIds.buttonPress:
                        debugMessageQueue.Enqueue(String.Format("{0}:Received Button Press for serial#\r\n", (DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond)));
                        break;
                    case CommandIds.getFrameResp:
                        //debugMessageQueue.Enqueue(String.Format("{0}:Received Frame\r\n", (DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond)));

                        ImuFrame frame = new ImuFrame(packet);
                        //debugMessageQueue.Enqueue(frame.ToString());
                        break;
                    case CommandIds.setImuIdResp:
                        debugMessageQueue.Enqueue(String.Format("{0}:Received set Imu Resp\r\n", (DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond)));
                        break;
                    case CommandIds.getStatusResp:
                        debugMessageQueue.Enqueue("Recieved Get status\r\n");
                        break;
                    default:
                        break;
                }
            }
            else if (packet.Payload[0] == 0x04) //this is a protocol buffer file. 
            {
                Stream stream = new MemoryStream(packet.Payload, 1, packet.PayloadSize - 1);
                try
                {
                    Packet protoPacket = Serializer.Deserialize<Packet>(stream);
                    processProtoPacket(protoPacket);
                }
                catch
                {
                    debugMessageQueue.Enqueue("Failed to deserialize packet\r\n");
                }
            }

        }

        public void processPacketThread()
        {
            while (processPacketQueueEnabled)
            {
                RawPacket packet;
                if (packetQueue.Count > 0)
                {
                    if (packetQueue.TryDequeue(out packet))
                    {
                        processPacket(packet);
                    }
                }
                //Play nicely with other threads
                Thread.Sleep(1);
            }
        }


        private string swapHexBytes(string hexString)
        {
            return hexString[2].ToString() + hexString[3].ToString() + hexString[0].ToString() + hexString[1].ToString();
        }
        private string ConvertFloatToHex(float value)
        {
            Int16 convertedValue = (Int16)(value * 8192);
            string convertedString = convertedValue.ToString("X4");
            return swapHexBytes(convertedString);
        }


        private string convertProtoBufPacket(Packet heddokoPacket)
        {
            StringBuilder retString = new StringBuilder();
            if (heddokoPacket.type == PacketType.DataFrame)
            {
                //hard coding mask for now
                retString.Append( heddokoPacket.fullDataFrame.timeStamp.ToString().PadLeft(10,'0') + ",3ff,");
                for (int i = 0; i < heddokoPacket.fullDataFrame.imuDataFrame.Count; i++)
                {                    
                    //do stuff for each frame received.
                    retString.Append(ConvertFloatToHex(heddokoPacket.fullDataFrame.imuDataFrame.ElementAt(i).quat_z_roll)+";");
                    retString.Append(ConvertFloatToHex(heddokoPacket.fullDataFrame.imuDataFrame.ElementAt(i).quat_y_pitch) + ";");
                    retString.Append(ConvertFloatToHex(heddokoPacket.fullDataFrame.imuDataFrame.ElementAt(i).quat_x_yaw) + ",");
                }
                for (int i = 0; i < 9 - heddokoPacket.fullDataFrame.imuDataFrame.Count; i++)
                {
                    retString.Append("0000;0000;0000,");
                }
                retString.Append("1234;BBBB;CCCC;DDDD;EEEE,\r\n");
                return retString.ToString(); 
            }
            return "";
        }
        private uint startTime = 0;
        private string convertProtoPacketToAppFormat(Packet heddokoPacket)
        {
            StringBuilder retString = new StringBuilder();
             if (heddokoPacket.type == PacketType.DataFrame)
            {
                //hard coding mask for now
                if(startTime == 0)
                {
                    startTime = heddokoPacket.fullDataFrame.timeStamp;
                }
                double timeStamp = (double)(heddokoPacket.fullDataFrame.timeStamp - startTime)/1000; 
                retString.Append(timeStamp.ToString() + ",");
                for (int i = 0; i < heddokoPacket.fullDataFrame.imuDataFrame.Count; i++)
                {
                    //do stuff for each frame received.
                    retString.Append(heddokoPacket.fullDataFrame.imuDataFrame.ElementAt(i).imuId + ",");
                    retString.Append(heddokoPacket.fullDataFrame.imuDataFrame.ElementAt(i).quat_z_roll.ToString("F3") + ";");
                    retString.Append(heddokoPacket.fullDataFrame.imuDataFrame.ElementAt(i).quat_y_pitch.ToString("F3") + ";");
                    retString.Append(heddokoPacket.fullDataFrame.imuDataFrame.ElementAt(i).quat_x_yaw.ToString("F3") + ",");
                }
                for (int i = 0; i < 10 - heddokoPacket.fullDataFrame.imuDataFrame.Count; i++)
                {
                    retString.Append("9,0.0;0.0;0.0,");
                }
                retString.Append("1234,BBBB,CCCC,DDDD,EEEE\r\n");
                return retString.ToString();
            }
            return "";
        }


        private string processRawPacket(RawPacket packet)
        {
            if (packet.Payload[0] == 0x04) //this is a protocol buffer file. 
            {
                Stream stream = new MemoryStream(packet.Payload, 1, packet.PayloadSize - 1);
                try
                {
                    Packet protoPacket = Serializer.Deserialize<Packet>(stream);
                    if(protoPacket.type == PacketType.DataFrame)
                    {
                        if (cb_decodeForApp.Checked)
                        {
                            return convertProtoPacketToAppFormat(protoPacket); 
                        }
                        else
                        {
                            return convertProtoBufPacket(protoPacket);
                        }
                    }
                }
                catch
                {
                    debugMessageQueue.Enqueue("Failed to deserialize packet\r\n");
                }

            }
            return ""; 
        }
        private void btn_decode_Click(object sender, EventArgs e)
        {
            RawPacket packet = new RawPacket();
            if (ofd_OpenFile.ShowDialog() != DialogResult.OK)
            {
                return; 
            }
            if(sfd_SaveFile.ShowDialog() != DialogResult.OK)
            {
                return; 
            }

            UInt32 packetCount = 0, packetErrors = 0; ;
            try
            {
        //        FileStream dataFile = File.Open(ofd_OpenFile.FileName, FileMode.Open);
                FileStream outputFile = File.Open(sfd_SaveFile.FileName, FileMode.Create);
                debugMessageQueue.Enqueue(String.Format("Processing File: {0}\r\n", ofd_OpenFile.FileName));
                float percent = 0.0F;
                //initialize the start time of the file. 
                startTime = 0;
                if(cb_decodeForApp.Checked)
                {
                    //have to create header for the file before writting it in. 
                    string line1 = Guid.NewGuid().ToString() + "\r\n";
                    string line2 = Guid.NewGuid().ToString() + "\r\n";
                    string line3 = Guid.NewGuid().ToString() + "\r\n";                    
                    outputFile.Write(ASCIIEncoding.ASCII.GetBytes(line1), 0, line1.Length);
                    outputFile.Write(ASCIIEncoding.ASCII.GetBytes(line2), 0, line2.Length);
                    outputFile.Write(ASCIIEncoding.ASCII.GetBytes(line3), 0, line3.Length);
                }
                byte[] vByteArray = File.ReadAllBytes(ofd_OpenFile.FileName);
                for (int i = 0; i < vByteArray.Length; i++)
                {
                    float newPercent = ((i ) / vByteArray.Length) * 100;
                    if (newPercent != percent)
                    {
                        pb_progressBar.Value = (int)newPercent;
                        percent = newPercent; 
                    }
                    
                    byte newByte = vByteArray[i];
                    int bytesReceived = packet.BytesReceived + 1;
                    PacketStatus status = packet.processByte(newByte);
                    switch (status)
                    {
                        case PacketStatus.PacketComplete:
                            //debugMessageQueue.Enqueue(String.Format("{0} Packet Received {1} bytes\r\n", (DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond), packet.PayloadSize));
                            RawPacket packetCopy = new RawPacket(packet);
                            string frameString = processRawPacket(packetCopy);                            
                            if (frameString.Length > 0)
                            {
                                outputFile.Write(ASCIIEncoding.ASCII.GetBytes(frameString), 0, frameString.Length);
                                packetCount++;
                            }
                            packet.resetPacket();
                            break;
                        case PacketStatus.PacketError:
                            debugMessageQueue.Enqueue(String.Format("Packet ERROR! {0} bytes received\r\n", bytesReceived));
                            packetErrors++;
                            packet.resetPacket();
                            break;
                        case PacketStatus.Processing:
                        case PacketStatus.newPacketDetected:
                            break;
                    }
                }          
              
              debugMessageQueue.Enqueue(String.Format("Processed File Size:{0} Bytes, {1} frames extracted ,{2} Errors \r\n", vByteArray.Length, packetCount, packetErrors));
                 outputFile.Close(); 
            }
            catch(Exception vE)
            {
                debugMessageQueue.Enqueue(String.Format("Error Processing file, {0} errors found\r\n", packetErrors));
                debugMessageQueue.Enqueue("Exception found "+ vE);

            }
            pb_progressBar.Value = 0;

        }
        public void processDebugMessagesThread()
        {
            while (processDebugThreadEnabled)
            {
                string line;
                if (debugMessageQueue.Count > 0)
                {
                    if (debugMessageQueue.TryDequeue(out line))
                    {
                        this.BeginInvoke((MethodInvoker)(() => tb_Console.AppendText(line)));
                    }
                }
                Thread.Sleep(1);
            }
        }
        private void form_decoder_FormClosing(object sender, FormClosingEventArgs e)
        {
            processDebugThreadEnabled = false;
            processPacketQueueEnabled = false; 
        }
        private void bnt_Connect_Click(object sender, EventArgs e)
        {
            //set the serial port to the selected item. 
            //Thread serialThread = new Thread(ReadThread);
            if(cb_serialPorts.SelectedIndex < 0)
            {
                return; 
            }
            if (sp_recieve.IsOpen)
            {
                //do nothing
                return;
            }
            sp_recieve.PortName = cb_serialPorts.Items[cb_serialPorts.SelectedIndex].ToString();
            sp_recieve.BaudRate = int.Parse(cb_BaudRate.Items[cb_BaudRate.SelectedIndex].ToString());
            try
            {
                sp_recieve.Open();
                tb_Console.AppendText("Port: " + sp_recieve.PortName + " Open\r\n");
            }
            catch (Exception ex)
            {
                tb_Console.AppendText("Failed to open Port: " + sp_recieve.PortName + " \r\n");
                tb_Console.AppendText("Exception " + ex.Message + " \r\n");
            }
        }

        RawPacket rawPacket = new RawPacket();
        private void sp_foward_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {

        }

        private void sp_recieve_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                while (sp_recieve.BytesToRead > 0)
                {
                    int receivedByte = sp_recieve.ReadByte();
                    if (receivedByte != -1)
                    {
                        //process the byte
                        byte newByte = (byte)receivedByte;
                        int bytesReceived = rawPacket.BytesReceived + 1;
                        PacketStatus status = rawPacket.processByte((byte)receivedByte);
                        switch (status)
                        {
                            case PacketStatus.PacketComplete:
                                debugMessageQueue.Enqueue(String.Format("Packet Received {0} bytes\r\n", rawPacket.PayloadSize));
                                RawPacket packetCopy = new RawPacket(rawPacket);
                                packetQueue.Enqueue(packetCopy);
                                rawPacket.resetPacket();
                                break;
                            case PacketStatus.PacketError:
                                //if (cb_logErrors.Checked)
                                //{
                                debugMessageQueue.Enqueue(String.Format("Packet ERROR! {1} bytes received\r\n", bytesReceived));
                                //}
                                rawPacket.resetPacket();
                                break;
                            case PacketStatus.Processing:
                            case PacketStatus.newPacketDetected:
                                break;
                        }
                    }
                }
            }
            catch
            {

            }
        }

        private void btn_disconnect_Click(object sender, EventArgs e)
        {
            if(sp_recieve.IsOpen)
            {
                sp_recieve.Close();
            }
        }

        private void cp_openForwardPort_CheckedChanged(object sender, EventArgs e)
        {
            if(cp_openForwardPort.Checked)
            {
                sp_foward.PortName = cb_forwardPorts.Items[cb_forwardPorts.SelectedIndex].ToString();
                try
                {
                    sp_foward.Open();
                    tb_Console.AppendText("Forward Port: " + sp_foward.PortName + " Open\r\n");
                }
                catch (Exception ex)
                {
                    tb_Console.AppendText("Failed to forward open Port: " + sp_foward.PortName + " \r\n");
                    tb_Console.AppendText("Exception " + ex.Message + " \r\n");
                }
            }
            else
            {
                sp_foward.Close();

            }
        }

        private void cb_serialPorts_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            if (!sp_recieve.IsOpen)
            {
                cb_serialPorts.Items.Clear();
                cb_serialPorts.Items.AddRange(SerialPort.GetPortNames());
            }
        }

      
    }
}
