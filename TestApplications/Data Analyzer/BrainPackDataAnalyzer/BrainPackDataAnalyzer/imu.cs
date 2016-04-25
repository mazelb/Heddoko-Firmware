using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BrainPackDataAnalyzer
{
    public class imu
    {
        private UInt32 maxInterval;
        private UInt32 averageInterval;
        private UInt32 interval; 
        private UInt32 lastEntryTime;
        private UInt32 totalEntryCount;
        private UInt32 intervalSum;
        private bool entryUpdated = false; 
        private ImuEntry entry;
        private Queue<UInt32> intervalAverage;
        private const int numAverages = 50;

        public bool EntryUpdated
        {
            get
            {
                return entryUpdated;
            }

            set
            {
                entryUpdated = value;
            }
        }
        public imu()
        {
            this.maxInterval = 0;
            this.averageInterval = 0;
            this.lastEntryTime = 0;
            this.totalEntryCount = 0;
            this.entry = new ImuEntry(0.0, 0.0, 0.0);
            this.intervalAverage = new Queue<uint>(numAverages);
            this.intervalSum = 0;
        }

        public void ProcessEntry(UInt32 timeStamp, string rawEntry)
        {
            if(this.lastEntryTime == 0)
            {
                this.lastEntryTime = timeStamp; 
            }
            else
            {
                //calculate the new interval

                this.interval = timeStamp - this.lastEntryTime;
                if (this.intervalAverage.Count == numAverages)
                {
                    //dequeue the oldest value, and subtract it from
                    //the running average sum
                    this.intervalSum -= this.intervalAverage.Dequeue();
                    //add the new value to the running average sum
                    this.intervalSum += this.interval;
                    this.averageInterval = intervalSum / numAverages;
                    this.intervalAverage.Enqueue(this.interval);
                }
                else
                {
                    this.intervalSum += this.interval;
                    this.intervalAverage.Enqueue(this.interval);
                }
                              
                //update the last entry time. 
                this.lastEntryTime = timeStamp;
                if (this.interval > this.maxInterval)
                {
                    this.maxInterval = this.interval; 
                }
                
            }
            this.entry.updateImuEntry(rawEntry);
            this.totalEntryCount++;
        }
        
        public UInt32 GetMaxInterval()
        {
            return this.maxInterval;
        }
        public UInt32 GetLastInterval()
        {
            return this.interval;
        }
        public UInt32 GetTotalEntryCount()
        {
            return this.totalEntryCount;
        }
        public ImuEntry GetCurrentEntry()
        {
            return this.entry;
        }
        public UInt32 GetAverageInterval()
        {
            return this.averageInterval;
        }
        public void clearStats()
        {
            this.averageInterval = 0;
            this.interval = 0;
            this.lastEntryTime = 0;
            this.maxInterval = 0;
            this.totalEntryCount = 0;
            this.intervalSum = 0;
            this.intervalAverage.Clear(); 
        }
    }
}
