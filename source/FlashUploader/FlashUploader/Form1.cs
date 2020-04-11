using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace FlashUploader
{
    public partial class Form1 : Form
    {
        private SerialPort _serialPort;
        private long _cnt1;
        public Form1()
        {
            InitializeComponent();
        }

        private void buttonOpenPort_Click(object sender, EventArgs e)
        {
            _serialPort = new SerialPort(textBoxComPort.Text, 57600, Parity.None, 8, StopBits.One);
            _serialPort.Handshake = Handshake.None;

            // "sp_DataReceived" is a custom method that I have created  
            _serialPort.DataReceived += new SerialDataReceivedEventHandler(sp_DataReceived);
            //_serialPort.d
            _serialPort.ReceivedBytesThreshold = 1;
            _serialPort.WriteBufferSize = 32;
            _serialPort.Open();
            buttonOpenPort.Enabled = false;
            buttonClosePort.Enabled = true;
        }

        private void buttonClosePort_Click(object sender, EventArgs e)
        {
            _serialPort.Close();
            buttonOpenPort.Enabled = true;
            buttonClosePort.Enabled = false;

        }



        private void uploadfile(string filename)
        {
            int psize = int.Parse(textBoxSize.Text);
            if (checkBoxUseFileSize.Checked)
            {
                System.IO.FileInfo fi = new FileInfo(filename);
                psize = (int) fi.Length;
            }

            byte[] bytes = new byte[psize];
           
            _cnt1 = 0;
           
            var filedata = File.ReadAllBytes(filename);

            filedata.Take(psize).ToArray().CopyTo(bytes, 0);


            for (int i = 0; i < bytes.Length; i += 8)
            {
                _replysize = 8;
                if ((i + 8) > bytes.Length)
                {
                    _replysize = 4;
                    _serialPort.Write(bytes, i, 4);
                }
                else
                    _serialPort.Write(bytes, i, 8);
                Thread.Sleep(50);
                Application.DoEvents();
                WaitReply();
                Application.DoEvents();
                //Thread.Sleep(100);
            }
        }
        int _replysize = 0;
        private void WaitReply()
        {

           
            while (_replysize > 0)
            {
              
                Application.DoEvents();
            }
        }
        private delegate void SetTextDeleg(string c);

        void sp_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
           
            string text = "";
            
            while (_serialPort.BytesToRead > 0)
            {
                int data = _serialPort.ReadChar();
               
                text += System.Char.ConvertFromUtf32(data);

            }
            if (text.Contains("."))
            {

                int cnt_r = text.Count(c => c == '.');
                _replysize -= cnt_r;
                _cnt1 += cnt_r;
                this.BeginInvoke(new SetTextDeleg(si_TextBox2), new object[] { _cnt1.ToString() });
                

            }
            this.BeginInvoke(new SetTextDeleg(si_DataReceived), new object[] { text });
        }

        private void si_DataReceived(string text)
        {

            textBox1.AppendText(text);
        }


        private void si_TextBox2(string text)
        {

            textBox2.Text = text;
        }

        private void buttonSelectFolder_Click(object sender, EventArgs e)
        {
          var dialogresult=  folderBrowserDialog1.ShowDialog();
            if(dialogresult == DialogResult.OK)
            {
                textBoxFolder.Text = folderBrowserDialog1.SelectedPath;
            }
        }

        private void buttonStart_Click(object sender, EventArgs e)
        {

            var files = System.IO.Directory.GetFiles(textBoxFolder.Text, "*.bin", SearchOption.TopDirectoryOnly);

            foreach (var file in files)
            {
                uploadfile(file);
            }
                
           
        }

        private void textBoxSize_TextChanged(object sender, EventArgs e)
        {

        }
    }
}
