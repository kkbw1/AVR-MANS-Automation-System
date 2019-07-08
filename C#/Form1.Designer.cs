namespace miro_gui
{
    partial class Form1
    {
        /// <summary>
        /// 필수 디자이너 변수입니다.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 사용 중인 모든 리소스를 정리합니다.
        /// </summary>
        /// <param name="disposing">관리되는 리소스를 삭제해야 하면 true이고, 그렇지 않으면 false입니다.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form 디자이너에서 생성한 코드

        /// <summary>
        /// 디자이너 지원에 필요한 메서드입니다.
        /// 이 메서드의 내용을 코드 편집기로 수정하지 마십시오.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.sp1 = new System.IO.Ports.SerialPort(this.components);
            this.btnConn = new System.Windows.Forms.Button();
            this.btnDisconn = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.cbPort = new System.Windows.Forms.ComboBox();
            this.btnSend = new System.Windows.Forms.Button();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.btnClr = new System.Windows.Forms.Button();
            this.button1 = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // sp1
            // 
            this.sp1.DataReceived += new System.IO.Ports.SerialDataReceivedEventHandler(this.sp1_DataReceived);
            // 
            // btnConn
            // 
            this.btnConn.Location = new System.Drawing.Point(504, 16);
            this.btnConn.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.btnConn.Name = "btnConn";
            this.btnConn.Size = new System.Drawing.Size(86, 31);
            this.btnConn.TabIndex = 1;
            this.btnConn.Text = "연결";
            this.btnConn.UseVisualStyleBackColor = true;
            this.btnConn.Click += new System.EventHandler(this.btnConn_Click);
            // 
            // btnDisconn
            // 
            this.btnDisconn.Enabled = false;
            this.btnDisconn.Location = new System.Drawing.Point(504, 55);
            this.btnDisconn.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.btnDisconn.Name = "btnDisconn";
            this.btnDisconn.Size = new System.Drawing.Size(86, 31);
            this.btnDisconn.TabIndex = 2;
            this.btnDisconn.Text = "해제";
            this.btnDisconn.UseVisualStyleBackColor = true;
            this.btnDisconn.Click += new System.EventHandler(this.btnDisconn_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(505, 103);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(38, 17);
            this.label1.TabIndex = 3;
            this.label1.Text = "Port:";
            // 
            // cbPort
            // 
            this.cbPort.FormattingEnabled = true;
            this.cbPort.Location = new System.Drawing.Point(546, 99);
            this.cbPort.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.cbPort.Name = "cbPort";
            this.cbPort.Size = new System.Drawing.Size(138, 24);
            this.cbPort.TabIndex = 4;
            // 
            // btnSend
            // 
            this.btnSend.Location = new System.Drawing.Point(504, 141);
            this.btnSend.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.btnSend.Name = "btnSend";
            this.btnSend.Size = new System.Drawing.Size(86, 31);
            this.btnSend.TabIndex = 5;
            this.btnSend.Text = "맵 보내기";
            this.btnSend.UseVisualStyleBackColor = true;
            this.btnSend.Click += new System.EventHandler(this.btnSend_Click);
            // 
            // textBox1
            // 
            this.textBox1.BackColor = System.Drawing.Color.White;
            this.textBox1.Font = new System.Drawing.Font("BatangChe", 12F);
            this.textBox1.Location = new System.Drawing.Point(504, 182);
            this.textBox1.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.textBox1.Multiline = true;
            this.textBox1.Name = "textBox1";
            this.textBox1.ReadOnly = true;
            this.textBox1.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.textBox1.Size = new System.Drawing.Size(354, 392);
            this.textBox1.TabIndex = 6;
            // 
            // btnClr
            // 
            this.btnClr.Location = new System.Drawing.Point(742, 141);
            this.btnClr.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.btnClr.Name = "btnClr";
            this.btnClr.Size = new System.Drawing.Size(117, 31);
            this.btnClr.TabIndex = 7;
            this.btnClr.Text = "LOG창 클리어";
            this.btnClr.UseVisualStyleBackColor = true;
            this.btnClr.Click += new System.EventHandler(this.btnClr_Click);
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(770, 86);
            this.button1.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(88, 44);
            this.button1.TabIndex = 8;
            this.button1.Text = "맵 클리어";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(883, 599);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.btnClr);
            this.Controls.Add(this.textBox1);
            this.Controls.Add(this.btnSend);
            this.Controls.Add(this.cbPort);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.btnDisconn);
            this.Controls.Add(this.btnConn);
            this.Margin = new System.Windows.Forms.Padding(3, 4, 3, 4);
            this.Name = "Form1";
            this.Text = "Maze Monitor";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.IO.Ports.SerialPort sp1;
        private System.Windows.Forms.Button btnConn;
        private System.Windows.Forms.Button btnDisconn;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ComboBox cbPort;
        private System.Windows.Forms.Button btnSend;
        private System.Windows.Forms.TextBox textBox1;
        private System.Windows.Forms.Button btnClr;
        private System.Windows.Forms.Button button1;
    }
}

