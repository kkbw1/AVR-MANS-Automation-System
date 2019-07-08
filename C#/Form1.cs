using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO.Ports;

namespace miro_gui
{
    public partial class Form1 : Form
    {
        IAsyncResult AR;

        Point Pos = new Point();

        int receive_count = 0;
        int count_data = 0;
        byte[] va = new byte[48];

        // Block Information
        int bRow, bCol;
        int bW, bH;
        int bGap;

        // Background Panel Information
        int fW, fH;
        int fX, fY;

        // Background Panel
        Panel AP;
        // Start Panel
        Panel[] SP;
        // Block Panel
        Panel[,] BP;
        // End Panel
        Panel EP;

        byte[] Map = new byte[36];

        byte[,] Map_fin = new byte[6, 8];
        public Form1()
        {
            InitializeComponent();

            bRow = 6;
            bCol = 6;
            bW = 50;
            bH = 50;
            bGap = 5;

            fW = bGap + ((bW + bGap) * bRow);
            fH = bGap + ((bH + bGap) * (bCol + 2));
            fX = 10;
            fY = 10;

            AP = new Panel();
            AP.BackColor = Color.White;
            AP.Size = new Size(fW, fH);
            AP.Location = new Point(10, 10);
            this.Controls.Add(AP);

            SP = new Panel[bRow];
            for (int i = 0; i < bRow; i++)
            {
                SP[i] = new Panel();
                SP[i].BackColor = Color.Black;
                SP[i].Size = new Size(bW, bH);
                SP[i].Location = new Point((i * (bW + bGap)) + bGap, bGap);
                SP[i].Click += new EventHandler(SP_Click);
                AP.Controls.Add(SP[i]);
            }

            BP = new Panel[bRow, bCol];
            for (int j = 0; j < bCol; j++)
            {
                for (int i = 0; i < bRow; i++)
                {
                    BP[i, j] = new Panel();
                    BP[i, j].BackColor = Color.OrangeRed;
                    BP[i, j].Size = new Size(bW, bH);
                    BP[i, j].Location = new Point((i * (bW + bGap)) + bGap, (j * (bH + bGap)) + (bH + (2 * bGap)));
                    BP[i, j].Click += new EventHandler(BP_Click);
                    AP.Controls.Add(BP[i, j]);
                }
            }

            EP = new Panel();
            EP.BackColor = Color.Black;
            EP.Size = new Size(fW - (bGap * 2), bH);
            EP.Location = new Point(bGap, bGap + (bH + bGap) * (bCol + 1));
            AP.Controls.Add(EP);
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            cbPort.BeginUpdate();
            if (SerialPort.GetPortNames().Length == 0)
            {
                cbPort.Text = "No Ports";
            }
            else
            {
                foreach (string comport in SerialPort.GetPortNames())
                {
                    cbPort.Items.Add(comport);
                }
                cbPort.Text = SerialPort.GetPortNames().GetValue(0).ToString();
            }
            cbPort.EndUpdate();
        }

        private void SP_Click(object sender, EventArgs e)
        {
            Panel CSP = (Panel)sender;      // Current Start Point

            for (int i = 0; i < bRow; i++)
            {
                SP[i].BackColor = Color.Black;
            }

            CSP.BackColor = Color.Yellow;
        }
        private void BP_Click(object sender, EventArgs e)
        {
            Panel BP = (Panel)sender;
            if (BP.BackColor == Color.OrangeRed)
            {
                BP.BackColor = Color.Blue;
            }
            else
            {
                BP.BackColor = Color.OrangeRed;
            }
        }

        private void btnConn_Click(object sender, EventArgs e)
        {
            try
            {
                sp1.PortName = cbPort.Text;
                sp1.BaudRate = 57600;
                sp1.DataBits = 8;
                sp1.Parity = System.IO.Ports.Parity.None;
                sp1.StopBits = System.IO.Ports.StopBits.One;

                sp1.Open();

                btnConn.Enabled = false;
                btnDisconn.Enabled = true;

                cbPort.Enabled = false;

                textBox1.AppendText("\r\nConnected Serial Port.\r\n");
            }
            catch (System.Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private void btnDisconn_Click(object sender, EventArgs e)
        {
            if (sp1.IsOpen)
            {
                if (AR != null)
                {
                    this.EndInvoke(AR);
                }
                sp1.DiscardInBuffer();
                sp1.DiscardOutBuffer();
                sp1.Close();

                textBox1.AppendText("\r\nDisconnected Serial Port.\r\n");

                btnConn.Enabled = true;
                btnDisconn.Enabled = false;
            }
        }

        private void btnSend_Click(object sender, EventArgs e)
        {
            sp1.Write(Encoding.ASCII.GetBytes("@"), 0, 1);
            for (int i = 0; i < bRow; i++)
            {
                if (SP[i].BackColor == Color.Yellow)
                {
                    sp1.Write(new byte[] { (byte)i }, 0, 1);
                    break;
                }
            }

            sp1.Write(Encoding.ASCII.GetBytes("@"), 0, 1);
            for (int j = 0; j < 6; j++)
            {
                for (int i = 0; i < 6; i++)
                {
                    if (BP[i, j].BackColor == Color.OrangeRed)
                    {
                        sp1.Write(new byte[] { 0 }, 0, 1);
                    }
                    else if (BP[i, j].BackColor == Color.Blue)
                    {
                        sp1.Write(new byte[] { 99 }, 0, 1);
                    }
                }
            }
            sp1.Write(Encoding.ASCII.GetBytes("#"), 0, 1);
        }

        private void btnClr_Click(object sender, EventArgs e)
        {
            textBox1.Text = "";
        }

        private void button1_Click(object sender, EventArgs e)
        {
            for (int i = 0; i < bRow; i++)
            {
                SP[i].BackColor = Color.Black;
            }

            for (int j = 0; j < bCol; j++)
            {
                for (int i = 0; i < bRow; i++)
                {
                    BP[i, j].BackColor = Color.OrangeRed;
                }
            }

            EP.BackColor = Color.Black;
        }

        private void sp1_DataReceived(object sender, System.IO.Ports.SerialDataReceivedEventArgs e)
        {
            try
            {
                // dataReceive라는 메서드를 따로 스레드 돌림
                AR = this.BeginInvoke(new EventHandler(SerialReceived));
            }
            catch (System.Exception)
            {

            }
        }

        private void SerialReceived(object s, EventArgs e)
        {
            if (sp1.IsOpen)
            {
                int rec_size = sp1.BytesToRead;

                if (rec_size > 0)
                {
                    byte[] buff = new byte[rec_size];
                    sp1.Read(buff, 0, rec_size);

                    for (int i = 0; i < buff.Length; i++)
                    {
                        if ((buff[i] == '@') && (receive_count == 0))
                        {
                            receive_count++;
                            count_data = 0;
                        }
                        else if ((buff[i] != '#') && (receive_count == 1))
                        {
                            va[count_data] = buff[i];

                            count_data++;
                        }
                        else if ((buff[i] == '#') && (receive_count == 1))
                        {
                            receive_count = 0;

                            if (count_data == 42)
                            {
                                count_data = 0;
                                for (int a = 0; a < 6; a++)
                                {
                                    if (va[a] == 1)
                                    {
                                        Pos.X = a;
                                        Pos.Y = 0;
                                        SP[a].BackColor = Color.Yellow;
                                        break;
                                    }
                                }

                                for (int b = 6; b < 42; b++)
                                {
                                    if (va[b] == 0)
                                    {
                                        BP[(b - 6) % 6, (b - 6) / 6].BackColor = Color.OrangeRed;
                                    }
                                    else if (va[b] == 99)
                                    {
                                        BP[(b - 6) % 6, (b - 6) / 6].BackColor = Color.Blue;
                                    }
                                }
                            }
                            else if (count_data == 48)
                            {
                                count_data = 0;
                                for (int c = 0; c < 48; c++)
                                {
                                    Map_fin[c % 6, c / 6] = va[c];
                                }

                                for (int c = 0; c < 48; c++)
                                {
                                    if ((Map_fin[c % 6, c / 6] >= 1) && (Map_fin[c % 6, c / 6] <= 25))
                                    {
                                        if (c / 6 == 0)
                                        {
                                            SP[c % 6].BackColor = Color.Yellow;
                                        }
                                        else if (c / 6 == 7)
                                        {
                                            EP.BackColor = Color.Yellow;
                                        }
                                        else
                                        {
                                            BP[c % 6, (c / 6) - 1].BackColor = Color.Yellow;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    string strBuffAsci = Encoding.ASCII.GetString(buff);
                    textBox1.AppendText(strBuffAsci);
                }
            }
        }
    }
}
