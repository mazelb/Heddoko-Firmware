using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BrainPackDataAnalyzer
{
    public class ImuEntry
    {
        private double yaw;
        private double pitch;
        private double roll;

        public ImuEntry(string rawEntry)
        {
            //char separator[] = ';';
            string[] separatedEntry = rawEntry.Split(';'); 
            if(separatedEntry.Length < 3)
            {
                //this is an error
                Console.WriteLine("error failed to parse data");
                this.yaw = 0.0;
                this.pitch = 0.0;
                this.roll = 0.0; 
                return;
            }            
            this.roll = convertRawDataToFloat(separatedEntry[0]);
            this.pitch = convertRawDataToFloat(separatedEntry[1]);
            this.yaw = convertRawDataToFloat(separatedEntry[2]);
        }

        public ImuEntry(double y, double p, double r)
        {
            this.yaw = y;
            this.pitch = p;
            this.roll = r; 
        }
        public void updateImuEntry(string rawEntry)
        {
            string[] separatedEntry = rawEntry.Split(';');
            if (separatedEntry.Length < 3)
            {
                //this is an error
                Console.WriteLine("error failed to parse data");
                this.yaw = 0.0;
                this.pitch = 0.0;
                this.roll = 0.0;
                return;
            }
            this.roll = convertRawDataToFloat(separatedEntry[0]);
            this.pitch = convertRawDataToFloat(separatedEntry[1]);
            this.yaw = convertRawDataToFloat(separatedEntry[2]);
        }
        public double Yaw
        {
            get
            {
                return yaw;
            }

            set
            {
                yaw = value;
            }
        }

        public double Pitch
        {
            get
            {
                return pitch;
            }

            set
            {
                pitch = value;
            }
        }

        public double Roll
        {
            get
            {
                return roll;
            }

            set
            {
                roll = value;
            }
        }

        override public string ToString()
        {
            //return this.roll.ToString("F6") + ";0.0;0.0";
            return this.pitch.ToString("F6") + ";" + this.roll.ToString("F6") + ";" + this.yaw.ToString("F6");
        }


        static public double convertRawDataToFloat(string val)
        {
            //try swaping the bytes
            //string swapped = "0000";
            if (val.Length >= 4)
            {
                string byte1 = val[0].ToString() + val[1].ToString();
                string byte2 = val[2].ToString() + val[3].ToString();
                Byte byte_1 = Byte.Parse(byte1, System.Globalization.NumberStyles.HexNumber);
                Byte byte_2 = Byte.Parse(byte2, System.Globalization.NumberStyles.HexNumber);

                //swapped = val[2].ToString() + val[3].ToString() + val[0].ToString() + val[1].ToString();
                //swapped = val[0].ToString() + val[1].ToString() + val[2].ToString() + val[3].ToString();
                //Int16 data = Int16.Parse(swapped, System.Globalization.NumberStyles.HexNumber);

                int data = ((int)byte_1) | (((int)byte_2) << 8);
                float fVal = (float)(data << 16);
                fVal = fVal / (1 << 29);

                return (double)fVal;

                /*
                //Pose 6d
		        short int x = bytes[0] | (bytes[1] << 8);
		        short int y = bytes[2] | (bytes[3] << 8);
		        short int z = bytes[4] | (bytes[5] << 8);
		
		        int roll = ((int)bytes[6]) | (((int)bytes[7]) << 8);
		        float rollf = (float)(roll << 16);
		        int pitch = ((int)bytes[8]) | (((int)bytes[9]) << 8);
		        float pitchf = (float)(pitch << 16);
		        int yaw = ((int)bytes[10]) | (((int)bytes[11]) << 8);
		        float yawf = (float)(yaw << 16);		
		
		        short id = bytes[12] | bytes[13] << 8;

		        rollf = rollf / (1 << 29);
		        pitchf = pitchf / (1 << 29);
		        yawf = yawf / (1 << 29);

		        Pose6DEvent pEvent;
		        pEvent.x = x;
		        pEvent.y = y;
		        pEvent.z = z;
		        pEvent.roll = rollf;
		        pEvent.pitch = pitchf;
		        pEvent.yaw = yawf;
		        pEvent.sender = id;
                */
            }
            return 0.0; 
        }

    }
}
