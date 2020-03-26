using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Cavapa_dotNet_Batch_Processor
{
    public partial class MainForm : Form
    {
        string settingsFilepath = "cavapa_batch.txt";

        public MainForm()
        {
            InitializeComponent();

            if (File.Exists(settingsFilepath))
            {
                try
                {
                    using (TextReader file = File.OpenText(settingsFilepath))
                    {
                        textBoxInputFolder.Text = file.ReadLine();
                        textBoxOutputFolder.Text = file.ReadLine();
                    }
                    textBoxInputFolder_TextChanged(this, null);
                    textBoxOutputFolder_TextChanged(this, null);
                }
                catch (Exception ex) {
                    MessageBox.Show(ex.Message, ex.Source);
                }
            }

            if (textBoxInputFolder.Text == "" || !Directory.Exists(textBoxInputFolder.Text))
            {
                buttonBrowseInput_Click(this, null);
            }
            else
            {
                string[] files = Directory.GetFiles(textBoxInputFolder.Text);
                if (files.Length < 4 || Path.GetExtension(files[0]) != ".jpg")
                {
                    buttonBrowseInput_Click(this, null);
                }
            }
        }

        private void buttonBrowseInput_Click(object sender, EventArgs e)
        {
            using (OpenFileDialog dlg = new OpenFileDialog())
            {
                dlg.InitialDirectory = textBoxInputFolder.Text;
                dlg.Filter = "jpg files (*.jpg)|*.jpg|All files (*.*)|*.*";
                dlg.FilterIndex = 2;
                dlg.RestoreDirectory = true;

                if (dlg.ShowDialog() == DialogResult.OK)
                {
                    textBoxInputFolder.Text = Path.GetDirectoryName(dlg.FileName) + "\\";
                }
            }
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (File.Exists(settingsFilepath))
            {
                File.Delete(settingsFilepath);
            }

            using (TextWriter file = File.CreateText(settingsFilepath))
            {
                file.WriteLine(textBoxInputFolder.Text);
                file.WriteLine(textBoxOutputFolder.Text);
                file.Flush();
                file.Close();
            }
        }

        private void buttonSelectOutputFolder_Click(object sender, EventArgs e)
        {
            using (FolderBrowserDialog dlg = new FolderBrowserDialog()) {
                if (dlg.ShowDialog() == DialogResult.OK) {
                    textBoxOutputFolder.Text = dlg.SelectedPath;
                }
            }
        }
        private void textBoxInputFolder_TextChanged(object sender, EventArgs e)
        {
            if (Directory.Exists(textBoxInputFolder.Text))
            {
                string[] files = Directory.GetFiles(textBoxInputFolder.Text);
                if (files.Length < 4 || Path.GetExtension(files[0]) != ".jpg")
                {
                    textBoxInputFolder.BackColor = Color.Pink;
                    return;
                }
                textBoxInputFolder.BackColor = Color.LightGreen;
                buttonStartProcessing.Enabled = checkSettings();
                if (buttonStartProcessing.Enabled)
                    buttonStartProcessing.Focus();
                return;
            }
            else
            {
                textBoxInputFolder.BackColor = Color.Pink;
                return;
            }
        }

        bool checkSettings() {
            return textBoxInputFolder.BackColor == Color.LightGreen &&
                   textBoxOutputFolder.BackColor == Color.LightGreen &&
                   !textBoxInputFolder.Text.Contains(textBoxOutputFolder.Text);
        }

        private void textBoxOutputFolder_TextChanged(object sender, EventArgs e)
        {
            textBoxOutputFolder.BackColor = (Directory.Exists(textBoxOutputFolder.Text) && !textBoxInputFolder.Text.Contains(textBoxOutputFolder.Text) ? Color.LightGreen : Color.Pink);
            buttonStartProcessing.Enabled = checkSettings();
            if (buttonStartProcessing.Enabled)
                buttonStartProcessing.Focus();
        }

        private void buttonProcess_Click(object sender, EventArgs e)
        {
            List<Bitmap> bmps = new List<Bitmap>();
            Task.Factory.StartNew(() =>
            {
                string[] files = Directory.GetFiles(textBoxInputFolder.Text, "*.jpg");
                foreach (string file in files)
                {
                    Bitmap bmp = new Bitmap(file);
                    _importWorker.ReportProgress(1, image);

                    this.BeginInvoke(new Action(() =>
                    {
                        bmps.Add(bmp);
                    }));
                }
            });
        }
    }
}
