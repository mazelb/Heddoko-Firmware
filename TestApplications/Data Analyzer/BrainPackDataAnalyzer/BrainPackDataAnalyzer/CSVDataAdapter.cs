using System;
using System.Collections.Generic;
using System.Data;
using System.IO;
using System.Text;

namespace BrainPackDataAnalyzer
{
    /// <summary>
    /// Version 2.5 (2010-03-23)
    /// </summary>
    public static class CSVDataAdapter
    {
        #region PUBLIC MEMBERS
        #region Fill
        //This method returns an empty datatable with the schema of the csv file.
        public static DataTable GetHeader(string path, bool hasHeader)
        {
            Encoding encoding = Encoding.GetEncoding(1252);
            DataTable table = new DataTable();

                        // Assert that the file exists
            if (!File.Exists(path))
                throw new ArgumentException("The file cannot be found or does not exist.", "path");

            // Parse the file
            string[] rowbuffer;
            using (StreamReader sr = new StreamReader(path, encoding))
            {
                // Set the DataTable columns
                if (hasHeader)
                {
                    // Read the header row and construct the table's schema
                    string[] headers = SplitRow(sr.ReadLine());

                    // Add a column in the DataTable for each header column
                    foreach (string columnName in headers)
                    {
                        DataColumn column = table.Columns.Add(columnName, typeof(string));
                        column.DefaultValue = "";
                    }
                }
                else
                {
                    // Create default columns
                    rowbuffer = SplitRow(sr.ReadLine());
                    for (int i = 0; i < rowbuffer.Length; i++)
                    {
                        // Add a column for each string in the row buffer
                        DataColumn dc = table.Columns.Add(string.Format("Column {0}", i), typeof(string));
                        dc.DefaultValue = "";
                    }


                }
            }
            return table;
        }
        public static DataTable Fill(string path, bool hasHeader)
        {
            return Fill(path, hasHeader, Encoding.GetEncoding(1252));
        }
        public static DataTable Fill(string path, bool hasHeader, Encoding encoding)
        {
            // Create a new DataTable
            DataTable table = new DataTable();

            // Assert that the file exists
            if (!File.Exists(path))
                throw new ArgumentException("The file cannot be found or does not exist.", "path");

            // Parse the file
            string[] rowbuffer;
            using (StreamReader sr = new StreamReader(path, encoding))
            {
                // Set the DataTable columns
                if (hasHeader)
                {
                    // Read the header row and construct the table's schema
                    string[] headers = SplitRow(sr.ReadLine());

                    // Add a column in the DataTable for each header column
                    foreach (string columnName in headers)
                    {
                        DataColumn column = table.Columns.Add(columnName, typeof(string));
                        column.DefaultValue = "";
                    }
                }
                else
                {
                    // Create default columns
                    rowbuffer = SplitRow(sr.ReadLine());
                    for (int i = 0; i < rowbuffer.Length; i++)
                    {
                        // Add a column for each string in the row buffer
                        DataColumn dc = table.Columns.Add(string.Format("Column {0}", i), typeof(string));
                        dc.DefaultValue = "";
                    }

                    // Add the row to the table
                    table.Rows.Add(rowbuffer);
                }

                // Get the data from the csv file
                while (!sr.EndOfStream)
                {
                    try
                    {
                        // Read a line of data from the csv file and split it into an array of string
                        rowbuffer = SplitRow(sr.ReadLine());

                        // Add the row to the table
                        table.Rows.Add(rowbuffer);
                    }
                    catch { break; }
                }
            }

            // Accept all changes to the DataTable
            table.AcceptChanges();

            // Return the DataTable
            return table;
        }
        #endregion

        #region Write
        public static void Write(DataTable table, bool hasHeader, string path, bool append)
        {
            Write(table, hasHeader, path, append, Encoding.GetEncoding(1252));
        }
        public static void Write(DataTable table, bool hasHeader, string path, bool append, Encoding encoding)
        {
            using (StreamWriter sw = new StreamWriter(path, append, encoding))
            {
                // Write the Header Row
                if (hasHeader)
                {
                    // Get the column names
                    string[] columnNames = new string[table.Columns.Count];
                    for (int i = 0; i < table.Columns.Count; ++i)
                        columnNames[i] = table.Columns[i].ColumnName;

                    // Write the assembled column names
                    sw.WriteLine(BuildRow(columnNames));
                }

                // Write each row to the csv file (unless they were deleted)
                foreach (DataRow row in table.Rows)
                {
                    if (row.RowState != DataRowState.Deleted)
                        sw.WriteLine(BuildRow(row.ItemArray));
                }
            }
        }
        public static void Write(DataRow row, string path)
        {
            Write(row, path, true, Encoding.UTF8);
        }
        public static void Write(DataRow row, string path, bool append, Encoding encoding)
        {
            // Write the row to the output file
            if (append)
                File.AppendAllText(path, BuildRow(row.ItemArray)+ "\r\n", encoding);
            else
                File.WriteAllText(path, BuildRow(row.ItemArray), encoding);
        }
        public static void Write(DataRow row, FileStream Append_FS)
        {
            // Write the row to the output file
            byte[] Row_Data = Encoding.ASCII.GetBytes(BuildRow(row.ItemArray) + "\r\n");
            Append_FS.Seek(0, SeekOrigin.End);        //Set the pointer to the end of the file
            Append_FS.Write(Row_Data,0,Row_Data.Length);
            Append_FS.Flush();
        }
        #endregion
        #endregion


        #region STATIC HELPER METHODS
        #region Quote String
        /// <summary>
        /// Formats a an object so that it can be output to a CSV file.
        /// </summary>
        /// <param name="value">The object to construct a quoted string from.</param>
        /// <returns>The CSV-formatted string.</returns>
        private static string QuoteString(object value)
        {
            // Check if the value is null
            if (value == null)
                return "";

            // Convert the object to a string
            string quotedString = value.ToString();

            //if (quotedString.StartsWith(" ") || quotedString.EndsWith(" ") ||
            //    quotedString.Contains("\n") ||
            //    quotedString.Contains(",") || quotedString.Contains("\""))
            //{
            //    return "\"" + quotedString.Replace("\"", "\"\"") + "\"";
            //}

            return quotedString;
        }
        #endregion
        #region Build Row
        /// <summary>
        /// Builds a row into a single string with delimiting characters
        /// </summary>
        /// <returns>The built string.</returns>
        public static string BuildRow(object[] values)
        {
            string[] stringValues = new string[values.Length];

            // Parse each object
            for (int i = 0; i < values.Length; ++i)
                stringValues[i] = QuoteString(values[i]);

            // Join all the string values and return the result
            return string.Join(",", stringValues);
        }
        #endregion
        #region Split Row
        /// <summary>
        /// Splits a delimited row into an array of strings.
        /// </summary>
        /// <param name="row">The row of data to split.</param>
        /// <returns>The split data.</returns>
        public static string[] SplitRow(string row)
        {
            // Split the row of data (basic splitting - no special cases)
            string[] segments = row.Split(',');

            List<string> parsedsegments = new List<string>();
            bool iscontinuation = false;
            string buffer = "";

            // Further split the segments of data
            foreach (string segment in segments)
            {
                if (iscontinuation)
                {
                    if (CountOccurrences(segment, '\"') % 2 == 1)
                    {
                        // Add the buffer, a comma, and the last segment
                        parsedsegments.Add(
                            buffer + "," +
                            segment.Substring(0, segment.Length - 1).Replace("\"\"", "\""));

                        // This is the end of the continuation
                        iscontinuation = false;
                    }
                    else
                    {
                        // Buffer the segment and continue
                        buffer += "," + segment.Replace("\"\"", "\"");
                    }
                }
                else
                {
                    // if segment begins with a quote
                    if (segment.StartsWith("\""))
                    {
                        // if segment also ends with a quote
                        if (CountOccurrences(segment, '\"') % 2 == 0)
                        {
                            // Remove start and end quotes, and remove all pairs of double quotes
                            parsedsegments.Add(segment.Substring(1, segment.Length - 2).Replace("\"\"", "\""));
                        }
                        else
                        {
                            // Add this segment to the buffer and continue to the next segment
                            buffer = segment.Substring(1).Replace("\"\"", "\"");

                            // This segment does not end here
                            iscontinuation = true;
                        }
                    }
                    else
                    {
                        // Item is an unquoted value -> Add straight in
                        parsedsegments.Add(segment);
                    }
                }
            }

            return parsedsegments.ToArray();
        }
        #endregion
        #region Count Occurrences
        /// <summary>
        /// Counts the occurrences of a character in a string.
        /// </summary>
        /// <param name="instance">The string to search in.</param>
        /// <param name="c">The character to search for.</param>
        /// <returns>The number of occurrences of the character found in the string.</returns>
        private static int CountOccurrences(string instance, char c)
        {
            int occurrences = 0;
            for (int i = 0; i < instance.Length; ++i)
            {
                if (instance[i] == c)
                    ++occurrences;
            }

            return occurrences;
        }
        #endregion
        #endregion
    }
}
