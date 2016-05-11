namespace ProtobufDecoder
{
    partial class form_decoder
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.btn_disconnect = new System.Windows.Forms.Button();
            this.tb_Console = new System.Windows.Forms.TextBox();
            this.bnt_Connect = new System.Windows.Forms.Button();
            this.cb_serialPorts = new System.Windows.Forms.ComboBox();
            this.btn_decode = new System.Windows.Forms.Button();
            this.sfd_SaveFile = new System.Windows.Forms.SaveFileDialog();
            this.cb_forwardPorts = new System.Windows.Forms.ComboBox();
            this.cb_BaudRate = new System.Windows.Forms.ComboBox();
            this.ofd_OpenFile = new System.Windows.Forms.OpenFileDialog();
            this.lb_forward = new System.Windows.Forms.Label();
            this.sp_recieve = new System.IO.Ports.SerialPort(this.components);
            this.sp_foward = new System.IO.Ports.SerialPort(this.components);
            this.cp_openForwardPort = new System.Windows.Forms.CheckBox();
            this.pb_progressBar = new System.Windows.Forms.ProgressBar();
            this.cb_decodeForApp = new System.Windows.Forms.CheckBox();
            this.SuspendLayout();
            // 
            // btn_disconnect
            // 
            this.btn_disconnect.Location = new System.Drawing.Point(191, 335);
            this.btn_disconnect.Name = "btn_disconnect";
            this.btn_disconnect.Size = new System.Drawing.Size(73, 23);
            this.btn_disconnect.TabIndex = 25;
            this.btn_disconnect.Text = "Disconnect";
            this.btn_disconnect.UseVisualStyleBackColor = true;
            this.btn_disconnect.Click += new System.EventHandler(this.btn_disconnect_Click);
            // 
            // tb_Console
            // 
            this.tb_Console.Location = new System.Drawing.Point(40, 23);
            this.tb_Console.Multiline = true;
            this.tb_Console.Name = "tb_Console";
            this.tb_Console.ReadOnly = true;
            this.tb_Console.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.tb_Console.Size = new System.Drawing.Size(455, 218);
            this.tb_Console.TabIndex = 24;
            // 
            // bnt_Connect
            // 
            this.bnt_Connect.Location = new System.Drawing.Point(191, 306);
            this.bnt_Connect.Name = "bnt_Connect";
            this.bnt_Connect.Size = new System.Drawing.Size(73, 23);
            this.bnt_Connect.TabIndex = 23;
            this.bnt_Connect.Text = "Connect";
            this.bnt_Connect.UseVisualStyleBackColor = true;
            this.bnt_Connect.Click += new System.EventHandler(this.bnt_Connect_Click);
            // 
            // cb_serialPorts
            // 
            this.cb_serialPorts.FormattingEnabled = true;
            this.cb_serialPorts.Location = new System.Drawing.Point(42, 306);
            this.cb_serialPorts.Name = "cb_serialPorts";
            this.cb_serialPorts.Size = new System.Drawing.Size(126, 21);
            this.cb_serialPorts.TabIndex = 22;
            this.cb_serialPorts.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.cb_serialPorts_MouseDoubleClick);
            // 
            // btn_decode
            // 
            this.btn_decode.Location = new System.Drawing.Point(191, 422);
            this.btn_decode.Name = "btn_decode";
            this.btn_decode.Size = new System.Drawing.Size(75, 23);
            this.btn_decode.TabIndex = 26;
            this.btn_decode.Text = "Decode File";
            this.btn_decode.UseVisualStyleBackColor = true;
            this.btn_decode.Click += new System.EventHandler(this.btn_decode_Click);
            // 
            // sfd_SaveFile
            // 
            this.sfd_SaveFile.Filter = "Comma Separated Files (*.csv)|*.csv|All Files (*.*)|*.*";
            // 
            // cb_forwardPorts
            // 
            this.cb_forwardPorts.FormattingEnabled = true;
            this.cb_forwardPorts.Location = new System.Drawing.Point(152, 364);
            this.cb_forwardPorts.Name = "cb_forwardPorts";
            this.cb_forwardPorts.Size = new System.Drawing.Size(112, 21);
            this.cb_forwardPorts.TabIndex = 38;
            // 
            // cb_BaudRate
            // 
            this.cb_BaudRate.FormattingEnabled = true;
            this.cb_BaudRate.Location = new System.Drawing.Point(43, 333);
            this.cb_BaudRate.Name = "cb_BaudRate";
            this.cb_BaudRate.Size = new System.Drawing.Size(125, 21);
            this.cb_BaudRate.TabIndex = 39;
            // 
            // ofd_OpenFile
            // 
            this.ofd_OpenFile.FileName = "Recording.dat";
            this.ofd_OpenFile.Filter = "Dat Files (*.dat)|*.dat|All Files (*.*)|*.*";
            // 
            // lb_forward
            // 
            this.lb_forward.AutoSize = true;
            this.lb_forward.Location = new System.Drawing.Point(43, 371);
            this.lb_forward.Name = "lb_forward";
            this.lb_forward.Size = new System.Drawing.Size(81, 13);
            this.lb_forward.TabIndex = 40;
            this.lb_forward.Text = "Forwarding Port";
            // 
            // sp_recieve
            // 
            this.sp_recieve.BaudRate = 115200;
            this.sp_recieve.DataReceived += new System.IO.Ports.SerialDataReceivedEventHandler(this.sp_recieve_DataReceived);
            // 
            // sp_foward
            // 
            this.sp_foward.BaudRate = 115200;
            this.sp_foward.DataReceived += new System.IO.Ports.SerialDataReceivedEventHandler(this.sp_foward_DataReceived);
            // 
            // cp_openForwardPort
            // 
            this.cp_openForwardPort.AutoSize = true;
            this.cp_openForwardPort.Location = new System.Drawing.Point(152, 391);
            this.cp_openForwardPort.Name = "cp_openForwardPort";
            this.cp_openForwardPort.Size = new System.Drawing.Size(115, 17);
            this.cp_openForwardPort.TabIndex = 41;
            this.cp_openForwardPort.Text = "Open Forward Port";
            this.cp_openForwardPort.UseVisualStyleBackColor = true;
            this.cp_openForwardPort.CheckedChanged += new System.EventHandler(this.cp_openForwardPort_CheckedChanged);
            // 
            // pb_progressBar
            // 
            this.pb_progressBar.Location = new System.Drawing.Point(40, 258);
            this.pb_progressBar.Name = "pb_progressBar";
            this.pb_progressBar.Size = new System.Drawing.Size(455, 23);
            this.pb_progressBar.Step = 1;
            this.pb_progressBar.Style = System.Windows.Forms.ProgressBarStyle.Continuous;
            this.pb_progressBar.TabIndex = 42;
            // 
            // cb_decodeForApp
            // 
            this.cb_decodeForApp.AutoSize = true;
            this.cb_decodeForApp.Checked = true;
            this.cb_decodeForApp.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cb_decodeForApp.Location = new System.Drawing.Point(34, 428);
            this.cb_decodeForApp.Name = "cb_decodeForApp";
            this.cb_decodeForApp.Size = new System.Drawing.Size(151, 17);
            this.cb_decodeForApp.TabIndex = 43;
            this.cb_decodeForApp.Text = "Decode For Heddoko App";
            this.cb_decodeForApp.UseVisualStyleBackColor = true;
            // 
            // form_decoder
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(516, 542);
            this.Controls.Add(this.cb_decodeForApp);
            this.Controls.Add(this.pb_progressBar);
            this.Controls.Add(this.cp_openForwardPort);
            this.Controls.Add(this.lb_forward);
            this.Controls.Add(this.cb_BaudRate);
            this.Controls.Add(this.cb_forwardPorts);
            this.Controls.Add(this.btn_decode);
            this.Controls.Add(this.btn_disconnect);
            this.Controls.Add(this.tb_Console);
            this.Controls.Add(this.bnt_Connect);
            this.Controls.Add(this.cb_serialPorts);
            this.Name = "form_decoder";
            this.Text = "Protocol Buffer Decoder";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.form_decoder_FormClosing);
            this.Load += new System.EventHandler(this.form_decoder_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btn_disconnect;
        private System.Windows.Forms.TextBox tb_Console;
        private System.Windows.Forms.Button bnt_Connect;
        private System.Windows.Forms.ComboBox cb_serialPorts;
        private System.Windows.Forms.Button btn_decode;
        private System.Windows.Forms.SaveFileDialog sfd_SaveFile;
        private System.Windows.Forms.ComboBox cb_forwardPorts;
        private System.Windows.Forms.ComboBox cb_BaudRate;
        private System.Windows.Forms.OpenFileDialog ofd_OpenFile;
        private System.Windows.Forms.Label lb_forward;
        private System.IO.Ports.SerialPort sp_recieve;
        private System.IO.Ports.SerialPort sp_foward;
        private System.Windows.Forms.CheckBox cp_openForwardPort;
        private System.Windows.Forms.ProgressBar pb_progressBar;
        private System.Windows.Forms.CheckBox cb_decodeForApp;
    }
}

