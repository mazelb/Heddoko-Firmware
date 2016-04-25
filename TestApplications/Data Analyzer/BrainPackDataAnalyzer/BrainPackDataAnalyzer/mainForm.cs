using System;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO.Ports;
using System.IO;
using System.Threading;
using System.Collections.Concurrent;

namespace BrainPackDataAnalyzer
{
    public partial class mainForm : Form
    {
        public Thread readThread;

        public FileStream dataFile;        
        public DataTable analysisData;
        public DataTable sensorStats;
        public ConcurrentQueue<string> incomingDataQueue; 
        public imu[] imuArray = new imu[9];
        private bool openSerialPort = false;
        private bool processDataTheadEnabled = true; 
        public mainForm()
        {
            InitializeComponent();
        }

        public void ReadThread()
        {
            while (true)
            {
                if (serialPort.IsOpen)
                {
                    try
                    {
                        string line = serialPort.ReadLine();
                        lock (serialPortPassThrough)
                        {
                            if (serialPortPassThrough.IsOpen)
                            {
                                serialPortPassThrough.WriteLine(line);
                            }
                        }
                        if (line.Length == 176)
                        {
                            processEntry(line);
                            if (dataFile != null)
                            {
                                lock (dataFile)
                                {
                                    if (dataFile.CanWrite)
                                    {
                                        line += "\r\n";
                                        dataFile.Write(System.Text.Encoding.ASCII.GetBytes(line), 0, line.Length);
                                    }
                                }
                            }                            
                        }
                        else
                        {
                            this.BeginInvoke((MethodInvoker)(() => tb_Console.AppendText(line + "\r\n")));    
                                                        
                        }

                    }
                    catch
                    {
                        //do nothing, this is alright
                    }
                    if(!openSerialPort)
                    {
                        this.BeginInvoke((MethodInvoker)(() => tb_Console.AppendText("Serial Port Closed\r\n")));
                        serialPort.Close();
                        return;
                    }

                }
            }
        }
        public void processDataThread()
        {
            while(processDataTheadEnabled)
            {
                string line;
                if (incomingDataQueue.Count > 0)
                {
                    if (incomingDataQueue.TryDequeue(out line))
                    {
                        if (line.Length == 176)
                        {
                            processEntry(line);
                            if (dataFile != null)
                            {
                                lock (dataFile)
                                {
                                    if (dataFile.CanWrite)
                                    {
                                        line += "\r\n";
                                        dataFile.Write(System.Text.Encoding.ASCII.GetBytes(line), 0, line.Length);
                                    }
                                }
                            }
                        }
                        else
                        {
                            this.BeginInvoke((MethodInvoker)(() => tb_Console.AppendText(line + "\r\n")));
                        }
                    }
                }                 
                
            }
        }
        public string convertToArrow(double value)
        {
            string retVal = "";
            if(value >= (-Math.PI/8) && value < (Math.PI / 8))
            {
                //right arrow
                retVal += " \u2192";
            }
            else if (value >= (-(3*Math.PI) / 8) && value < (-Math.PI / 8))
            {
                //right down
                retVal += " \u2198";
            }
            else if (value >= (-(5*Math.PI) / 8) && value < (-(3*Math.PI) / 8))
            {
                //down
                retVal += " \u2193";
            }
            else if (value >= (-(7 * Math.PI) / 8) && value < (-(5 * Math.PI) / 8))
            {
                //left down
                retVal += " \u2199";
            }
            else if ((value >= (7 * Math.PI)/8) && value <= Math.PI)
            {
                //left
                retVal += " \u2190";
            }
            else if ((value >= -Math.PI) && (value <= -(7 * Math.PI) / 8))
            {
                //left
                retVal += " \u2190";
            }
            else if (value >= ((5 * Math.PI) / 8) && value < ((7 * Math.PI) / 8))
            {
                //left up
                retVal += " \u2196";
            }
            else if (value >= ((3 * Math.PI) / 8) && value < ((5 * Math.PI) / 8))
            {
                //up
                retVal += " \u2191";
            }
            else if (value >= ((Math.PI) / 8) && value < ((3 * Math.PI) / 8))
            {
                //up right
                retVal += " \u2197";
            }
            else
            {
                retVal += "X"; 
            }
            return retVal; 
        } 
        public void processEntry(string entry)
        {            
            string[] entrySplit = entry.Split(',');
            //there should be 12 columns of data
            if (entrySplit.Length == 13)
            {
                try
                {
                    UInt32 timeStamp = UInt32.Parse(entrySplit[0]);
                    UInt16 sensorMask = UInt16.Parse(entrySplit[1], System.Globalization.NumberStyles.HexNumber);
                    for(int i = 0; i < 9; i++)
                    {
                        //if the frame is valid for this sensor then process it. 
                        if ((sensorMask & (1<<i)) > 0)
                        {
                            imuArray[i].ProcessEntry(timeStamp, entrySplit[i + 2]);
                            imuArray[i].EntryUpdated = true;
                        }
                        else
                        {
                            imuArray[i].EntryUpdated = false;  
                        }
                        //DataRow row = sensorStats.NewRow();
                        sensorStats.Rows[i]["Sensor ID"] = i.ToString();
                        sensorStats.Rows[i]["Roll"] = imuArray[i].GetCurrentEntry().Roll.ToString("F3") + convertToArrow(imuArray[i].GetCurrentEntry().Roll);
                        sensorStats.Rows[i]["Pitch"] = imuArray[i].GetCurrentEntry().Pitch.ToString("F3") + convertToArrow(imuArray[i].GetCurrentEntry().Pitch);
                        sensorStats.Rows[i]["Yaw"] = imuArray[i].GetCurrentEntry().Yaw.ToString("F3") + convertToArrow(imuArray[i].GetCurrentEntry().Yaw);
                        sensorStats.Rows[i]["Frame Count"] = imuArray[i].GetTotalEntryCount().ToString();
                        sensorStats.Rows[i]["Interval"] = imuArray[i].GetLastInterval().ToString();
                        sensorStats.Rows[i]["Max Interval"] = imuArray[i].GetMaxInterval().ToString();
                        sensorStats.Rows[i]["Average Interval"] = imuArray[i].GetAverageInterval().ToString();
                    }
                    this.BeginInvoke((MethodInvoker)(() => tb_stretchData.Text = entrySplit[11]));
                }
                catch
                {
                    //error
                }
            }
        }
        public void analyzeEntry(string entry)
        {
            string[] entrySplit = entry.Split(',');
            string result = "";
            //there should be 12 columns of data
            if (entrySplit.Length == 13)
            {
                try
                {
                    UInt32 timeStamp = UInt32.Parse(entrySplit[0]);
                    UInt16 sensorMask = UInt16.Parse(entrySplit[1], System.Globalization.NumberStyles.HexNumber);
                    for (int i = 0; i < 9; i++)
                    {
                        //if the frame is valid for this sensor then process it. 
                        if ((sensorMask & (1 << i)) > 0)
                        {
                            imuArray[i].ProcessEntry(timeStamp, entrySplit[i + 2]);
                            imuArray[i].EntryUpdated = true;
                        }
                        else
                        {
                            imuArray[i].EntryUpdated = false;
                        }
                        //DataRow row = sensorStats.NewRow();

                        //save the data to the row

                        //sensorStats.Rows[i]["Sensor ID"] = i.ToString();
                        //sensorStats.Rows[i]["Roll"] = imuArray[i].GetCurrentEntry().Roll.ToString("F3") + convertToArrow(imuArray[i].GetCurrentEntry().Roll);
                        //sensorStats.Rows[i]["Pitch"] = imuArray[i].GetCurrentEntry().Pitch.ToString("F3") + convertToArrow(imuArray[i].GetCurrentEntry().Pitch);
                        //sensorStats.Rows[i]["Yaw"] = imuArray[i].GetCurrentEntry().Yaw.ToString("F3") + convertToArrow(imuArray[i].GetCurrentEntry().Yaw);
                        sensorStats.Rows[i]["Frame Count"] = imuArray[i].GetTotalEntryCount().ToString();
                        sensorStats.Rows[i]["Interval"] = imuArray[i].GetLastInterval().ToString();
                        sensorStats.Rows[i]["Max Interval"] = imuArray[i].GetMaxInterval().ToString();
                        //sensorStats.Rows[i]["Average Interval"] = imuArray[i].GetAverageInterval().ToString();
                    }
                    //this.BeginInvoke((MethodInvoker)(() => tb_stretchData.Text = entrySplit[11]));
                }
                catch
                {
                    //error
                }
            }
        }

        private void mainForm_Load(object sender, EventArgs e)
        {
            cb_serialPorts.Items.AddRange(SerialPort.GetPortNames());
            cb_serialPassT.Items.AddRange(SerialPort.GetPortNames()); 
            serialPort.NewLine = "\r\n";
            serialPortPassThrough.NewLine = "\r\n"; 
            //start read thread --> automatically put things in list box            
            tb_Console.AppendText("Brain Data Analyzer\r\n");
            sensorStats = new DataTable("Sensor Statistics");
            sensorStats.Columns.Add("Sensor ID", typeof(string));
            sensorStats.Columns.Add("Roll", typeof(string));
            sensorStats.Columns.Add("Pitch", typeof(string));
            sensorStats.Columns.Add("Yaw", typeof(string));
            sensorStats.Columns.Add("Frame Count", typeof(string));
            sensorStats.Columns.Add("Interval", typeof(string));
            sensorStats.Columns.Add("Max Interval", typeof(string));
            sensorStats.Columns.Add("Average Interval", typeof(string));
            for (int i = 0; i < 9; i++)
            {
                DataRow row = sensorStats.NewRow();
                row["Sensor ID"] = i.ToString();
                row["Roll"] = "0";
                row["Pitch"] = "0";
                row["Yaw"] = "0";
                row["Frame Count"] = "0";
                row["Interval"] = "0";
                row["Max Interval"] = "0";
                row["Average Interval"] = "0";
                sensorStats.Rows.Add(row);
            }
            for(int i = 0; i < imuArray.Length; i++)
            {
                imuArray[i] = new imu(); 
            }
            bindingSource1.DataSource = sensorStats;
            dgv_SensorStats.DataSource = bindingSource1; 
            dgv_SensorStats.Update();

            //initialize queue
            incomingDataQueue = new ConcurrentQueue<string>();
            //start processor thread
            processDataTheadEnabled = true; 
            Thread dataProcessorThread = new Thread(processDataThread);
            dataProcessorThread.Start();

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
            try
            {
                openSerialPort = true;
                serialPort.Open();
                //if (serialPort.IsOpen)
                //{
                //    serialThread.Start();
                //}
                tb_Console.AppendText("Port: " + serialPort.PortName + " Open\r\n");
            }
            catch(Exception ex)
            {
                tb_Console.AppendText("Failed to open Port: " + serialPort.PortName + " \r\n");
                tb_Console.AppendText("Exception " + ex.Message + " \r\n");
                openSerialPort = false;
            }
        }
        private void btn_disconnect_Click(object sender, EventArgs e)
        {
            if(serialPort.IsOpen)
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
        private void sendCommmand(string command)
        {
            try
            {
                serialPort.Write(command);
            }
            catch
            {
                tb_Console.AppendText("Send command failed \r\n");
            }
        }
        private void btn_SendCmd_Click(object sender, EventArgs e)
        {
            if (serialPort.IsOpen)
            {
                if (tb_cmd.Text.Length > 0)
                {
                    sendCommmand(tb_cmd.Text + "\r\n");
                }
            }
        }
        private void btn_getState_Click(object sender, EventArgs e)
        {
            if (serialPort.IsOpen)
            {
                sendCommmand("GetState\r\n");                
            }
        }
        private void btn_record_Click(object sender, EventArgs e)
        {
            if (serialPort.IsOpen)
            {
                sendCommmand("Record\r\n");
            }
        }
        private void btn_reset_Click(object sender, EventArgs e)
        {
            if (serialPort.IsOpen)
            {
                sendCommmand("Reset\r\n");
            }
        }
        private void btn_setTime_Click(object sender, EventArgs e)
        {
            if (serialPort.IsOpen)
            {
                DateTime time = DateTime.Now;
                int dayOfWeekNumber = (int)time.DayOfWeek;
                string timeCommand = "setTime" + time.Year.ToString() + "-" + time.Month.ToString() + "-" + time.Day.ToString() + "-" +
                 dayOfWeekNumber.ToString() + "-" + time.Hour.ToString() + ":" + time.Minute.ToString() + ":" + time.Second.ToString() + "\r\n";
                sendCommmand(timeCommand);
            }
        }
        private DataTable convertDatFile(string filename)
        {
            string tempFilename = Application.StartupPath + "\\temp.csv";
            try
            {
                if(File.Exists(tempFilename))
                {
                    File.Delete(tempFilename);
                }
                FileStream tempFile = new FileStream(tempFilename, FileMode.Create);
                FileStream rawFile = new FileStream(filename, FileMode.Open);
                rawFile.Seek(0, SeekOrigin.Begin);
                Int32 temp = 0x00;  
                for(long i = 0; i < rawFile.Length; i++)
                {
                    temp = rawFile.ReadByte();
                    //check if the end of file has been reached. 
                    if (temp == -1)
                    {
                        break; 
                    }
                    tempFile.WriteByte((byte)(temp - 0x80)); 
                }
                tempFile.Close(); 
            }
            catch
            {
                return null; 
            }

            return CSVDataAdapter.Fill(tempFilename, false); 
        }
        private void btn_Analyze_Click(object sender, EventArgs e)
        {
            ofd_AnalyzeFile.Filter = "comma seperated values(*.csv)|*.csv|Brain pack data(*.dat)|*.dat|All files (*.*)|*.*";
            ofd_AnalyzeFile.FilterIndex = 3;
            ofd_AnalyzeFile.RestoreDirectory = true;
            

            if (ofd_AnalyzeFile.ShowDialog() == DialogResult.OK)
            {
                if(ofd_AnalyzeFile.FileName.Contains(".dat"))
                {
                    analysisData = convertDatFile(ofd_AnalyzeFile.FileName); 
                }
                else
                {
                    analysisData = CSVDataAdapter.Fill(ofd_AnalyzeFile.FileName, false);
                }
                if(analysisData == null)
                {
                    tb_Console.AppendText("Failed to load the file\r\n");
                }
                //StreamReader rawDataFile = new StreamReader(ofd_AnalyzeFile.FileName);
                //StreamWriter analysisDataStream = new StreamWriter(ofd_AnalyzeFile.FileName + ".dat",false); 
                tb_Console.AppendText("Loaded " + analysisData.Rows.Count.ToString() + " Rows \r\n");
                tb_Console.AppendText("From File: " + ofd_AnalyzeFile.FileName + " \r\n");
                Int32 maxTime = 0;
                DataTable convertedData = new DataTable("Converted_Data");
                convertedData.Columns.Add("TimeStamp", typeof(float));
                ImuEntry defaultValue = new ImuEntry(0.0, 0.0, 0.0);
                //create the columns for the IMUs
                for (int i = 0; i < 10; i++)
                {
                    //Add column with the IMU index. 
                    convertedData.Columns.Add(i.ToString(), typeof(string));
                    convertedData.Columns[i.ToString()].DefaultValue = (i+1).ToString();
                    string columnName = "IMU" + i.ToString();
                    convertedData.Columns.Add(columnName, typeof(ImuEntry));
                    convertedData.Columns[columnName].DefaultValue = defaultValue; //set default to all zeros 
                }
                //create the column for the stretch sense, should be 5 columns
                convertedData.Columns.Add("SS1", typeof(string));
                convertedData.Columns.Add("SS2", typeof(string));
                convertedData.Columns.Add("SS3", typeof(string));
                convertedData.Columns.Add("SS4", typeof(string));
                convertedData.Columns.Add("SS5", typeof(string));
                Int32 startTime = Int32.Parse(analysisData.Rows[0][0].ToString());
                for (int i = 1; i < analysisData.Rows.Count; i++)
                {
                    //try
                    //{
                    //    string rowString = rawDataFile.ReadLine();
                    //    analyzeEntry(rowString);
                    //    string entryString = "";
                    //    for (int j = 0; j < 9; j++)
                    //    {
                    //        entryString += imuArray[j].GetLastInterval().ToString() + ",";
                    //        //if (imuArray[j].EntryUpdated)
                    //        //{
                    //        //    entryString += imuArray[j].GetLastInterval().ToString() + ",";
                    //        //}
                    //        //else
                    //        //{
                    //        //    entryString += "0,";
                    //        //}
                    //    }
                    //    entryString += "\r\n";
                    //    analysisDataStream.Write(entryString);
                    //}
                    //catch
                    //{

                    //}
                    Int32 val1 = Int32.Parse(analysisData.Rows[i][0].ToString());
                    Int32 val2 = Int32.Parse(analysisData.Rows[i - 1][0].ToString());
                    Int32 interval = val1 - val2;
                    if (interval > maxTime)
                    {
                        maxTime = interval;
                    }
                    DataRow row = convertedData.NewRow();
                    row[0] = (float)(val1 - startTime) / 1000; //convert to float
                    
                    for (int j = 1, k =2; j < 18; j += 2,k++)
                    {
                        row[j+1] = new ImuEntry(analysisData.Rows[i][k].ToString());
                    }
                    string[] fabSense = analysisData.Rows[i][11].ToString().Split(';'); 
                    if(fabSense.Length == 5)
                    {
                        row["SS1"] = fabSense[0];
                        row["SS2"] = fabSense[1];
                        row["SS3"] = fabSense[2];
                        row["SS4"] = fabSense[3];
                        row["SS5"] = fabSense[4];
                    }
                    convertedData.Rows.Add(row);

                }
                //analysisDataStream.Close();
                tb_Console.AppendText("Max Interval" + maxTime.ToString() + "ms \r\n");
                sfd_ConvertedFile.DefaultExt = ".csv";
                sfd_ConvertedFile.AddExtension = true;
                if (sfd_ConvertedFile.ShowDialog() == DialogResult.OK)
                {
                    //have to create header for the file before writting it in. 
                    string line1 = Guid.NewGuid().ToString() + "\r\n";
                    string line2 = Guid.NewGuid().ToString() + "\r\n";
                    string line3 = Guid.NewGuid().ToString() + "\r\n";
                    StreamWriter writer = File.CreateText(sfd_ConvertedFile.FileName);
                    writer.Write(line1);
                    writer.Write(line2);
                    writer.Write(line3);
                    writer.Close(); 
                    CSVDataAdapter.Write(convertedData, false, sfd_ConvertedFile.FileName, true);
                }


            }
            //FileStream dataFile = new FileStream(ofd_AnalyzeFile.FileName, FileMode.Open);
        }
        //experimental encryption functions
        private byte rotlByte(byte value, int count)
        {
            const int CHAR_BIT = 8;
            const int mask = (CHAR_BIT * sizeof(byte) - 1);
            count &= mask;

            return (byte)((value << count) | (value >> ((-count) & mask)));
        }

        private byte rotrByte(byte value, int count)
        {
            const int CHAR_BIT = 8;
            const int mask = (CHAR_BIT * sizeof(byte) - 1);
            count &= mask;
            return (byte)((value >> count) | (value << ((-count) & mask)));
        }
        private void encrypt(ref byte[] data, int size)
        {
            int shift = 0;
            for (int i=0; i < size; i++)
            {
                shift = i % 7;
                if (shift == 0)
                {
                    shift = 3;
                }
                data[i] = rotlByte(data[i], shift);
            }
        }

        private void decrypt(ref byte[] data, int size)
        {
            int shift = 0;
            for (int i = 0; i < size; i++)
            {
                shift = i % 7;
                if (shift == 0)
                {
                    shift = 3;
                }
                data[i] = rotrByte(data[i], shift);
            }
        }

        private void btn_EncryptSettings_Click(object sender, EventArgs e)
        {
            if (ofd_AnalyzeFile.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    FileStream settingsFs = File.Open(ofd_AnalyzeFile.FileName, FileMode.Open);
                    byte[] data = new byte[500];
                    int totalBytesRead = settingsFs.Read(data, 0, 500);
                    settingsFs.Close(); 
                    encrypt(ref data, totalBytesRead);

                    //decrypt(ref data, totalBytesRead);  

                    if(sfd_ConvertedFile.ShowDialog() == DialogResult.OK)
                    {
                        FileStream encryptedSettings = File.Open(sfd_ConvertedFile.FileName, FileMode.Create);
                        byte[] header = new byte[2];
                        header[0] = (byte)'e';
                        header[1] = (byte)'e';
                        encryptedSettings.Write(header, 0, 2);
                        encryptedSettings.Write(data, 0, totalBytesRead);
                        encryptedSettings.Close(); 
                    }
                }
                catch
                {

                }
            }
        }
        private byte reverse(byte c)
        {
            int shift;
            int result = 0;
            for (shift = 0; shift < 8; shift++)
            {
                if ((c & (0x01 << shift)) > 0)
                    result |= ((byte)0x80 >> shift);
            }
            return (byte)result;
        }
        private byte[] bitReverseAllBytes(byte[] data)
        {
            byte[] reversedBytes = new byte[data.Length];

            for (int i = 0, j=0;j < reversedBytes.Length; i++,j++)
            {
                reversedBytes[j] = reverse(data[i]); 
            }
            return reversedBytes; 

        }
        private void btn_CreateFwBin_Click(object sender, EventArgs e)
        {
            if (ofd_AnalyzeFile.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    FileStream fwStream = File.Open(ofd_AnalyzeFile.FileName, FileMode.Open);
                    byte[] data = new byte[fwStream.Length];
                    int totalBytesRead = fwStream.Read(data, 0, data.Length);
                    fwStream.Close();

                    CRC_Calculator crcCal = new CRC_Calculator(InitialCrcValue.NonZero1);
                    CRCTool crcTool = new CRCTool();
                    crcTool.Init(CRCTool.CRCCode.CRC32);
                    ulong crcValue1 = crcTool.crcbitbybitfast(data); 
                    ushort crcValue2 = crcCal.ComputeChecksum(data);
                    ulong crcValue3 = crcTool.crcbitbybitfast(bitReverseAllBytes(data)); 
                    tb_Console.AppendText("CRC method 1 Calculated: " + crcValue1.ToString() + " \r\n");
                    tb_Console.AppendText("CRC method 2 Calculated: " + crcValue2.ToString() + " \r\n");
                    tb_Console.AppendText("CRC method 3 Calculated: " + crcValue3.ToString() + " \r\n");
                    //header 0x55AA55AA CRC(16bit) CRC(16bit), Length(32bit)
                    byte[] header = { 0x55, 0xAA, 0x55, 0xAA, 0, 0, 0, 0, 0, 0, 0, 0};

                    header[4] = (byte)(crcValue1 & 0x00FF);
                    header[5] = (byte)((crcValue1 >> 8) & 0x00FF);
                    header[6] = (byte)((crcValue1 >> 16) & 0x00FF);
                    header[7] = (byte)((crcValue1 >> 24) & 0x00FF);


                    //decrypt(ref data, totalBytesRead);  

                    if (sfd_ConvertedFile.ShowDialog() == DialogResult.OK)
                    {
                        FileStream outputFw = File.Open(sfd_ConvertedFile.FileName, FileMode.Create);
                        outputFw.Write(header, 0, header.Length);
                        outputFw.Write(data, 0, totalBytesRead);
                        outputFw.Close();
                    }
                }
                catch
                {

                }
            }
        }

        private void dgv_SensorStats_DataError(object sender, DataGridViewDataErrorEventArgs e)
        {
            Console.WriteLine("data Error"); 
        }

        private void btn_clearStats_Click(object sender, EventArgs e)
        {
            for(int i = 0; i < imuArray.Length; i++)
            {
                imuArray[i].clearStats(); 
            }
        }

        private void btn_setSaveLocation_Click(object sender, EventArgs e)
        {
            if(sfd_ConvertedFile.ShowDialog() == DialogResult.OK)
            {
                tb_saveLocation.Text = sfd_ConvertedFile.FileName;
            }
        }

        private void cb_saveRecordEntries_CheckedChanged(object sender, EventArgs e)
        {
            if(cb_saveRecordEntries.Checked)
            {
                if(tb_saveLocation.Text.Length > 0)
                {
                    try
                    {
                        dataFile = File.Open(tb_saveLocation.Text, FileMode.Append); 
                    }
                    catch
                    {

                    }
                }
            }
            else
            {
                try
                {
                    dataFile.Lock(0, dataFile.Length);
                    dataFile.Close();
                }
                catch
                {

                }
            }
        }

        private void mainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            processDataTheadEnabled = false; 
        }

        private void cb_serialPassEn_CheckedChanged(object sender, EventArgs e)
        {
            if (cb_serialPassEn.Checked)
            {
                lock(serialPortPassThrough)
                {
                    serialPortPassThrough.PortName = cb_serialPassT.Items[cb_serialPassT.SelectedIndex].ToString();
                    try
                    {
                        serialPortPassThrough.Open();
                        if (serialPortPassThrough.IsOpen)
                        {
                            tb_Console.AppendText("Pass through Port: " + serialPortPassThrough.PortName + "Open\r\n");
                        }

                    }
                    catch
                    {
                        tb_Console.AppendText("Failed to open pass through Port: " + serialPortPassThrough.PortName + "Open\r\n");
                        openSerialPort = false;
                    }
                }

            }
            else
            {
                lock (serialPortPassThrough)
                {
                    serialPortPassThrough.Close();
                }
            }
        }

        private void serialPortPassThrough_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                
                string line = serialPortPassThrough.ReadLine();
                if(serialPort.IsOpen)
                {
                    serialPort.WriteLine(line); 
                }
            }
            catch
            {

            }
        }

        private void serialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            if (serialPort.IsOpen)
            {
                try
                {

                    while (serialPort.BytesToRead > 0)
                    {
                        string line = serialPort.ReadLine();
                        lock (serialPortPassThrough)
                        {
                            if (serialPortPassThrough.IsOpen)
                            {
                                serialPortPassThrough.WriteLine(line);
                            }
                        }
                        incomingDataQueue.Enqueue(line); 
                    }

                }
                catch
                {
                    //do nothing, this is alright
                }

            }
        }

        private void cb_serialPorts_SelectedIndexChanged(object sender, EventArgs e)
        {

        }

        private void mainForm_DoubleClick(object sender, EventArgs e)
        {
            if (!serialPort.IsOpen)
            {
                cb_serialPorts.Items.Clear();
                cb_serialPorts.Items.AddRange(SerialPort.GetPortNames());
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog fBrowser = new FolderBrowserDialog();
            fBrowser.Description = "Select Input Folder";
            DialogResult result = fBrowser.ShowDialog();
            string folderResult = "";
            if (result == DialogResult.OK)
            {
                folderResult = fBrowser.SelectedPath;

                FolderBrowserDialog fOutputBrowser = new FolderBrowserDialog();
                fOutputBrowser.Description = "Select Output Folder";
                DialogResult foutputResult = fOutputBrowser.ShowDialog();
                if (foutputResult == DialogResult.OK)
                {
                    string vOutput = fOutputBrowser.SelectedPath;
                    string[] vRawCsvs = Directory.GetFiles(folderResult);
                    int vCount = 3;
                    foreach (var vRawCsv in vRawCsvs)
                    {
                        AnalyzeAndWriteToCSV(vRawCsv, vOutput + "\\Recording_" + vCount + ".csv");
                        vCount++;
                    }
                }
            }
        }
        private static void AnalyzeAndWriteToCSV(string vInputPath, string vOutputPath)
        {
            DataTable analysisData = CSVDataAdapter.Fill(vInputPath, false);

            Int32 maxTime = 0;

            DataTable convertedData = new DataTable("Converted_Data");
            convertedData.Columns.Add("TimeStamp", typeof(float));
            ImuEntry defaultValue = new ImuEntry(0.0, 0.0, 0.0);
            //create the columns for the IMUs
            for (int i = 0; i < 10; i++)
            {
                //Add column with the IMU index. 
                convertedData.Columns.Add(i.ToString(), typeof(string));
                convertedData.Columns[i.ToString()].DefaultValue = (i + 1).ToString();
                string columnName = "IMU" + i.ToString();
                convertedData.Columns.Add(columnName, typeof(ImuEntry));
                convertedData.Columns[columnName].DefaultValue = defaultValue; //set default to all zeros 
            }
            //create the column for the stretch sense, should be 5 columns
            convertedData.Columns.Add("SS1", typeof(string));
            convertedData.Columns.Add("SS2", typeof(string));
            convertedData.Columns.Add("SS3", typeof(string));
            convertedData.Columns.Add("SS4", typeof(string));
            convertedData.Columns.Add("SS5", typeof(string));
            Int32 startTime = Int32.Parse(analysisData.Rows[0][0].ToString());
            for (int i = 1; i < analysisData.Rows.Count; i++)
            {
                Int32 val1 = Int32.Parse(analysisData.Rows[i][0].ToString());
                Int32 val2 = Int32.Parse(analysisData.Rows[i - 1][0].ToString());
                Int32 interval = val1 - val2;
                if (interval > maxTime)
                {
                    maxTime = interval;
                }
                DataRow row = convertedData.NewRow();
                row[0] = (float)(val1 - startTime) / 1000; //convert to float

                for (int j = 1, k = 2; j < 18; j += 2, k++)
                {
                    row[j + 1] = new ImuEntry(analysisData.Rows[i][k].ToString());
                }
                string[] fabSense = analysisData.Rows[i][11].ToString().Split(';');
                if (fabSense.Length == 5)
                {
                    row["SS1"] = fabSense[0];
                    row["SS2"] = fabSense[1];
                    row["SS3"] = fabSense[2];
                    row["SS4"] = fabSense[3];
                    row["SS5"] = fabSense[4];
                }
                convertedData.Rows.Add(row);

            }



            //have to create header for the file before writting it in. 
            string line1 = Guid.NewGuid().ToString() + "\r\n";
            string line2 = Guid.NewGuid().ToString() + "\r\n";
            string line3 = Guid.NewGuid().ToString() + "\r\n";
            StreamWriter writer = File.CreateText(vOutputPath);
            writer.Write(line1);
            writer.Write(line2);
            writer.Write(line3);
            writer.Close();
            CSVDataAdapter.Write(convertedData, false, vOutputPath, true);



        }
    }
}



