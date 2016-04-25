using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Threading;

namespace PacketTester
{
    public partial class mainForm : Form
    {
        private bool openSerialPort = false;
        private bool processDebugThreadEnabled = false;
        public ConcurrentQueue<string> debugMessageQueue;
        private bool processPacketQueueEnabled = false; 
        public ConcurrentQueue<RawPacket> packetQueue;

        private bool streamDataEnabled = false;
        private long startTime = 0;
        private UInt16 graphIndex = 0;
        private const UInt16 graphMaxSize = 100; 

        public mainForm()
        {
            InitializeComponent();
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
            }
        }
        
        public string createAsciiFrame(ImuFrame[] frameArray, bool[] receivedFlags)
        {
            StringBuilder strBuilder = new StringBuilder();
            //so we need to create a frame compatible with the old system. 
            //0000246665,03ff,A2D1;9707; C11B,311B; 5D14; C6C9,7713; B2E2; 73FF,9D3F; AD1A; 07A5,3B2A; E7D7; 125E,7331; 4AFA; 8B42,2132; 97F8; 4EA1,1E3C; 30FD; C8D3,B337; 28FD; BCAA,1234; BBBB; CCCC; DDDD; EEEE, 
            long timeStamp = (DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond) - startTime;
            UInt16 receivedMask = 0;
            for (int i = 0; i < receivedFlags.Length; i++)
            {
                if(receivedFlags[i])
                {
                    receivedMask += (UInt16)(1 << i);
                }
            }

            strBuilder.Append(timeStamp.ToString("D10") + ",");
            strBuilder.Append(receivedMask.ToString("x4") + ",");
            for (int i = 0; i< frameArray.Length; i++)
            {
                strBuilder.Append(frameArray[i].getAsciiString() + ",");
            }
            for(int i = 0;i < 9 - frameArray.Length; i++)
            {
                strBuilder.Append("0000;0000;0000,");
            }
            strBuilder.Append("1234;BBBB;CCCC;DDDD;EEEE,\r\n");

            return strBuilder.ToString();
        }
        public void streamDataThread()
        {
            startTime = (DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond);
            this.BeginInvoke((MethodInvoker)(() => chrt_dataChart.Series["Qx"].Points.Clear()));
            this.BeginInvoke((MethodInvoker)(() => chrt_dataChart.Series["Qy"].Points.Clear()));
            this.BeginInvoke((MethodInvoker)(() => chrt_dataChart.Series["Qz"].Points.Clear()));
            this.BeginInvoke((MethodInvoker)(() => chrt_dataChart.Series["Qw"].Points.Clear()));
            const UInt16 numberOfSensors = 9;
            graphIndex = 0;
            //close the other listenning thread for the queue
            processPacketQueueEnabled = false;
            ImuFrame[] frameArray = new ImuFrame[numberOfSensors];
            for(int i = 0; i < frameArray.Length; i++)
            {
                frameArray[i] = new ImuFrame(); 
            }
            long maxTime = 0;
            long minTime = 100;
            long interval = 0;
            bool[] frameReceived = new bool[numberOfSensors];
            RawPacket framePacket = new RawPacket(); 
            while (streamDataEnabled)
            {
                DateTime startGetFrame = DateTime.Now;
                sendUpdateCommand();
                Thread.Sleep(3);
                for(int i = 0; i < numberOfSensors; i++)
                {
                    sendGetFrameCommand((byte)i);
                    frameReceived[i] = false; 
                    DateTime start = DateTime.Now;
                    while ((DateTime.Now - start).Milliseconds < 5)
                    {
                        if (packetQueue.TryDequeue(out framePacket))
                        {
                            if(frameArray[i].ParseImuFrame(framePacket, (byte)i))
                            {
                                //we got the packet.
                                frameReceived[i] = true;
                                break;
                            }
                        }
                        Thread.Yield(); 
                    }
                }
                interval = (DateTime.Now - startGetFrame).Milliseconds;
                if(interval < minTime)
                {
                    debugMessageQueue.Enqueue(String.Format("{0}:New min interval found:{1} ms\r\n", (DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond), interval));
                    minTime = interval;
                }
                if(interval > maxTime)
                {
                    debugMessageQueue.Enqueue(String.Format("{0}:New max interval found:{1} ms\r\n", (DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond), interval));
                    maxTime = interval; 
                }
                //create a frame from all the received data
                if (forwardSerialPort.IsOpen)
                {
                    forwardSerialPort.Write(createAsciiFrame(frameArray, frameReceived));
                }
                //Thread.Sleep(1); 
            }
            //start up other listenning thread again
            processPacketQueueEnabled = true;
            Thread packetProcessorThread = new Thread(processPacketThread);
            packetProcessorThread.Start();
        }
        
        enum CommandIds {update=0x11,getFrame,getFrameResp,setupMode,buttonPress,setImuId,setImuIdResp,getStatus,getStatusResp};
        string processSensorSerialNumber(RawPacket packet)
        {
            StringBuilder strBuilder = new StringBuilder();
            if(packet.PayloadSize != 18)
            {
                return "Failed to parse serial Number";
            }
            strBuilder.Append("0x");
            for(int i = 0; i < 16; i++)
            {
                strBuilder.Append(string.Format("{0}", packet.Payload[i + 2]));
            }
            return strBuilder.ToString();
        }
        string processGetStatusResponse(RawPacket packet)
        {
            StringBuilder strBuilder = new StringBuilder();
            UInt32 imuStatus = BitConverter.ToUInt32(packet.Payload, 3);
            UInt32 receivedPacketError = BitConverter.ToUInt32(packet.Payload,7);
            UInt32 quatError = BitConverter.ToUInt32(packet.Payload, 11);
            UInt32 magError = BitConverter.ToUInt32(packet.Payload, 15);
            UInt32 accelError = BitConverter.ToUInt32(packet.Payload, 19);
            UInt32 gyroError = BitConverter.ToUInt32(packet.Payload, 23);
            strBuilder.Append(string.Format("IMU Status:{0:x}\r\n", imuStatus));
            strBuilder.Append(string.Format("Received Error Count :{0}\r\n", receivedPacketError));
            strBuilder.Append(string.Format("Quat Error:{0}\r\n", quatError));
            strBuilder.Append(string.Format("Mag Error:{0}\r\n", magError));
            strBuilder.Append(string.Format("Accel Error:{0}\r\n", accelError));
            strBuilder.Append(string.Format("Gyro Error:{0}\r\n", gyroError));
            return strBuilder.ToString();

        }
        private void updateChart(ImuFrame frame)
        {
            this.BeginInvoke((MethodInvoker)(() => chrt_dataChart.Series["Qx"].Points.AddY((double)frame.Quaternion_x)));
            this.BeginInvoke((MethodInvoker)(() => chrt_dataChart.Series["Qy"].Points.AddY((double)frame.Quaternion_y)));
            this.BeginInvoke((MethodInvoker)(() => chrt_dataChart.Series["Qz"].Points.AddY((double)frame.Quaternion_z)));
            this.BeginInvoke((MethodInvoker)(() => chrt_dataChart.Series["Qw"].Points.AddY((double)frame.Quaternion_w)));
            if (chrt_dataChart.Series["Qx"].Points.Count > graphMaxSize)
            {
                this.BeginInvoke((MethodInvoker)(() => chrt_dataChart.Series["Qx"].Points.RemoveAt(0)));
                this.BeginInvoke((MethodInvoker)(() => chrt_dataChart.Series["Qy"].Points.RemoveAt(0)));
                this.BeginInvoke((MethodInvoker)(() => chrt_dataChart.Series["Qz"].Points.RemoveAt(0)));
                this.BeginInvoke((MethodInvoker)(() => chrt_dataChart.Series["Qw"].Points.RemoveAt(0)));

            }
        }
        private void processPacket(RawPacket packet)
        {
            //check that the packet comes from an IMU sensor
            if (packet.Payload[0] == 0x03)
            {
                CommandIds commandId = (CommandIds)packet.Payload[1];
                switch(commandId)
                {
                    case CommandIds.buttonPress:
                        debugMessageQueue.Enqueue(String.Format("{0}:Received Button Press for serial#{1}\r\n", (DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond), processSensorSerialNumber(packet)));
                        break;
                    case CommandIds.getFrameResp:
                        //debugMessageQueue.Enqueue(String.Format("{0}:Received Frame\r\n", (DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond)));

                        ImuFrame frame = new ImuFrame(packet);
                        updateChart(frame);
                        //debugMessageQueue.Enqueue(frame.ToString());
                        break;
                    case CommandIds.setImuIdResp:
                        debugMessageQueue.Enqueue(String.Format("{0}:Received set Imu Resp\r\n", (DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond)));
                        break;
                    case CommandIds.getStatusResp:
                        debugMessageQueue.Enqueue(processGetStatusResponse(packet));
                        break;
                    default:
                        break;
                }
            }
            
        }

        public void processPacketThread()
        {
            while(processPacketQueueEnabled)
            {
                RawPacket packet;
                if(packetQueue.Count > 0)
                {
                    if(packetQueue.TryDequeue(out packet))
                    {
                        processPacket(packet);
                    }
                }
            }
        }
        private void mainForm_Load(object sender, EventArgs e)
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

        private void bnt_Connect_Click(object sender, EventArgs e)
        {
            //set the serial port to the selected item. 
            //Thread serialThread = new Thread(ReadThread);
            if (serialPort.IsOpen)
            {
                //do nothing
                return;
            }

            serialPort.PortName = cb_serialPorts.Items[cb_serialPorts.SelectedIndex].ToString();
            serialPort.BaudRate = int.Parse(cb_BaudRate.Items[cb_BaudRate.SelectedIndex].ToString()); 
            try
            {
                openSerialPort = true;
                serialPort.Open();

                tb_Console.AppendText("Port: " + serialPort.PortName + " Open\r\n");
            }
            catch (Exception ex)
            {
                tb_Console.AppendText("Failed to open Port: " + serialPort.PortName + " \r\n");
                tb_Console.AppendText("Exception " + ex.Message + " \r\n");
                openSerialPort = false;
            }
        }

        private void btn_disconnect_Click(object sender, EventArgs e)
        {
            if (serialPort.IsOpen)
            {
                try
                {                    
                    serialPort.Close();
                    openSerialPort = false;
                    tb_Console.AppendText("Port: " + serialPort.PortName + " Closed\r\n");
                }
                catch
                {
                    tb_Console.AppendText("Failed to close Port: " + serialPort.PortName + "\r\n");
                }
            }
        }

        RawPacket packet = new RawPacket();
        private void serialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            
            while(serialPort.BytesToRead > 0)
            {
                int receivedByte = serialPort.ReadByte();
                if (receivedByte != -1)
                {                    
                    //process the byte
                    byte newByte = (byte)receivedByte;
                    int bytesReceived = packet.BytesReceived + 1;
                    PacketStatus status = packet.processByte((byte)receivedByte);
                    switch(status)
                    {
                        case PacketStatus.PacketComplete:                        
                            //debugMessageQueue.Enqueue(String.Format("{0} Packet Received {1} bytes\r\n", (DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond), packet.PayloadSize));
                            RawPacket packetCopy = new RawPacket(packet);  
                            packetQueue.Enqueue(packetCopy); 
                            packet.resetPacket(); 
                            break;
                        case PacketStatus.PacketError:
                            if (cb_logErrors.Checked)
                            {
                                debugMessageQueue.Enqueue(String.Format("{0} Packet ERROR! {1} bytes received\r\n", (DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond), bytesReceived));
                            }
                            packet.resetPacket(); 
                            break;
                        case PacketStatus.Processing:
                        case PacketStatus.newPacketDetected:
                            break;
                    }
                }
            }

        }
        private void sendPacket(byte[] payload, UInt16 size)
        {
            RawPacket packetToSend = new RawPacket(payload, size);
            UInt16 rawPacketSize = 0;
            byte[] rawPacketBytes = packetToSend.createRawPacket(ref rawPacketSize);
            try
            {
                serialPort.Write(rawPacketBytes, 0, rawPacketSize);
            }
            catch
            {
                debugMessageQueue.Enqueue(String.Format("{0} Failed to send packet\r\n", (DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond)));
            }
        }

        private void btn_SendCmd_Click(object sender, EventArgs e)
        {
            if (serialPort.IsOpen)
            {
                if (tb_cmd.Text.Length > 0)
                {
                    byte[] payload = Encoding.ASCII.GetBytes(tb_cmd.Text);
                    sendPacket(payload, (UInt16)payload.Length); 
                }
            }
        }

        private void mainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            //stop debug thread
            processDebugThreadEnabled = false;
            processPacketQueueEnabled = false; 
            //close the serial port
            if (serialPort.IsOpen)
            {
                serialPort.Close(); 
            }
        }
        private void sendGetFrameCommand(byte sensorId)
        {
            if (serialPort.IsOpen)
            {
                byte[] getFrameBytes = new byte[3];
                getFrameBytes[0] = 0x01;
                getFrameBytes[1] = 0x12;
                getFrameBytes[2] = sensorId;
                sendPacket(getFrameBytes, 3);
            }
        }
        private void sendGetStatusCommand(byte sensorId)
        {
            if (serialPort.IsOpen)
            {
                byte[] getFrameBytes = new byte[3];
                getFrameBytes[0] = 0x01;
                getFrameBytes[1] = 0x18;
                getFrameBytes[2] = sensorId;
                sendPacket(getFrameBytes, 3);
            }
        }
        private void btn_getFrame_Click(object sender, EventArgs e)
        {
            sendGetFrameCommand((byte)nud_SelectedImu.Value);
        }

        private void btn_SetupMode_Click(object sender, EventArgs e)
        {
            byte[] setupModeBytes = new byte[3];
            setupModeBytes[0] = 0x01;
            setupModeBytes[1] = 0x14;
            if (cb_SetupModeEn.Checked)
            {
                setupModeBytes[2] = 0x01;
            }
            else
            {
                setupModeBytes[2] = 0x00;
            }
            if (serialPort.IsOpen)
            {
                sendPacket(setupModeBytes, 3);
            }

        }
        private void sendUpdateCommand()
        {
            if (serialPort.IsOpen)
            {
                byte[] getFrameBytes = new byte[3];
                getFrameBytes[0] = 0x01;
                getFrameBytes[1] = 0x11;
                sendPacket(getFrameBytes, 2);
            }
        }
        private void btn_sendUpdateCmd_Click(object sender, EventArgs e)
        {
            sendUpdateCommand();
        }

        private void cb_enableStream_CheckedChanged(object sender, EventArgs e)
        {
            if(cb_enableStream.Checked)
            {
                Thread streamThread = new Thread(streamDataThread);
                streamDataEnabled = true;
                //open the serial port
                forwardSerialPort.PortName = cb_forwardPorts.Items[cb_forwardPorts.SelectedIndex].ToString();
                try
                {
                    forwardSerialPort.Open();
                    tb_Console.AppendText("Forward Port: " + forwardSerialPort.PortName + " Open\r\n");
                }
                catch (Exception ex)
                {
                    tb_Console.AppendText("Failed to forward open Port: " + forwardSerialPort.PortName + " \r\n");
                    tb_Console.AppendText("Exception " + ex.Message + " \r\n");
                }
                streamThread.Start(); 
            }
            else
            {
                streamDataEnabled = false;
                try
                {
                    if (forwardSerialPort.IsOpen)
                    {
                        forwardSerialPort.Close();
                        tb_Console.AppendText("Forward Port: " + forwardSerialPort.PortName + " Closed\r\n");
                    }
                }
                catch
                {
                    tb_Console.AppendText("Failed to close Port: " + forwardSerialPort.PortName + "\r\n");
                }
            }
        }

        private void tb_y_max_KeyPress(object sender, KeyPressEventArgs e)
        {
            e.Handled = !(char.IsDigit(e.KeyChar) || (e.KeyChar == '.') ||(e.KeyChar == '-' ) || (e.KeyChar == '\b'));
        }

        private void tb_y_min_KeyPress(object sender, KeyPressEventArgs e)
        {
            e.Handled = !(char.IsDigit(e.KeyChar) || (e.KeyChar == '.') || (e.KeyChar == '-') || (e.KeyChar == '\b'));
        }

        private void btn_setAxis_Click(object sender, EventArgs e)
        {
            try
            {
                float yaxis_max = 0.0F, yaxis_min = 0.0F;
                if(float.TryParse(tb_y_max.Text,out yaxis_max))
                {
                    chrt_dataChart.ChartAreas[0].AxisY.Maximum = yaxis_max;
                }
                if (float.TryParse(tb_y_min.Text, out yaxis_min))
                {
                    chrt_dataChart.ChartAreas[0].AxisY.Minimum = yaxis_min;
                }
                chrt_dataChart.ChartAreas[0].AxisY.RoundAxisValues();
            }
            catch
            {
                debugMessageQueue.Enqueue(String.Format("{0} Failed to parse Axis values (must be decimal number)\r\n", (DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond)));
            }
        }

        private void chrt_dataChart_Click(object sender, EventArgs e)
        {

        }

        private void btn_getStatus_Click(object sender, EventArgs e)
        {
            sendGetStatusCommand((byte)nud_SelectedImu.Value);
        }

        private void btn_clearScreen_Click(object sender, EventArgs e)
        {
            tb_Console.Clear();
        }
    }
}
