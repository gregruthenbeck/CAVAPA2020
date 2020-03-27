using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using Microsoft.Office.Interop;
using Excel = Microsoft.Office.Interop.Excel;

namespace Cavapa_Observation_Score
{
    public partial class MainForm : Form
    {
        string inputFolder;
        string[] inputFramesFilepaths;
        List<Bitmap> frames;
        int obsInterval = 1;
        int loopInterval = 1;
        int loopFrameOffset = 0;
        Timer loopFrameTimer = new Timer();
        public MainForm()
        {
            InitializeComponent();
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            loopFrameTimer.Tick += LoopFrameTimer_Tick;
        }

        private void selectFramesInputFolderToolStripMenuItem_Click(object sender, EventArgs e)
        {
            frames = new List<Bitmap>();

            using (OpenFileDialog ofd = new OpenFileDialog()) {
                ofd.Filter = "Jpg image files *.jpg|*.jp*g|All files *.*|*.*";

                if (ofd.ShowDialog() == DialogResult.OK) {
                    inputFolder = Path.GetDirectoryName(ofd.FileName);
                    inputFramesFilepaths = Directory.GetFiles(inputFolder, "*.jp*g");

                    statusProgressBar.Maximum = inputFramesFilepaths.Length;
                    statusProgressBar.Value = 0;
                    statusProgressBar.Visible = true;

                    foreach (string fname in inputFramesFilepaths)
                    {
                        frames.Add(new Bitmap(fname));
                        ++statusProgressBar.Value;
                    }

                    statusProgressBar.Visible = false;
                    statusLabel.Text = frames.Count.ToString() + " frames loaded";

                    textBoxLoopSpeed_TextChanged(this, null);
                    obsInterval = int.Parse(textBoxObsInterval.Text) * int.Parse(textBoxFps.Text);
                    loopInterval = int.Parse(textBoxLoopInterval.Text) * int.Parse(textBoxFps.Text);

                    trackBarTime.Maximum = frames.Count - 1;

                    trackBar1.Maximum = frames.Count - 1;
                    trackBar1.Value = 0;
                    trackBar1.TickFrequency = obsInterval;
                    trackBar1.LargeChange = obsInterval;

                    dataGridView1.Rows.Add((int)(0.5f + (float)frames.Count / (float)obsInterval) - 1); // already have 1 row
                    foreach (DataGridViewColumn col in dataGridView1.Columns)
                    {
                        col.DefaultCellStyle.Alignment = DataGridViewContentAlignment.BottomRight;
                    }
                    foreach (DataGridViewRow row in dataGridView1.Rows)
                    {
                        int id = (row.Index + 1);
                        row.Cells[0].Value = id;
                        TimeSpan time = new TimeSpan((long)(10000 * id * obsInterval * (1000 / int.Parse(textBoxFps.Text))));
                        row.Cells[1].Value = time;
                    }

                    pictureBox1.Image = frames[0];
                }
            }
        }

        private void LoopFrameTimer_Tick(object sender, EventArgs e)
        {
            ++loopFrameOffset;
            if (loopFrameOffset == loopInterval)
                loopFrameOffset = 0;

            int frame = trackBar1.Value + loopFrameOffset;
            if (frame >= 0 && frame < frames.Count)
            {
                pictureBox1.Image = frames[frame];
                trackBarTime.Value = frame;
            }
        }

        private void trackBar1_ValueChanged(object sender, EventArgs e)
        {
            pictureBox1.Image = frames[trackBar1.Value];
            trackBarTime.Value = trackBar1.Value;
            int row = trackBar1.Value / obsInterval;
            if (row > 0) {
                dataGridView1.Rows[row - 1].Selected = false;
            }
            if (row < (dataGridView1.Rows.Count-1)) {
                dataGridView1.Rows[row + 1].Selected = false;
            }
            dataGridView1.Rows[row].Selected = true;
        }

        private void buttonPlayLoop_Click(object sender, EventArgs e)
        {
            loopFrameOffset = 0;
            loopFrameTimer.Enabled = !loopFrameTimer.Enabled;
            buttonPlayLoop.BackColor = (loopFrameTimer.Enabled ? Color.LightGreen : Color.LightGray);
        }

        private void textBoxLoopSpeed_TextChanged(object sender, EventArgs e)
        {
            loopFrameTimer.Interval = (int)(1000.0f / ((float)int.Parse(textBoxFps.Text) * float.Parse(textBoxLoopSpeed.Text)));
        }

        private void textBoxLoopInterval_TextChanged(object sender, EventArgs e)
        {
            textBoxLoopSpeed_TextChanged(this, null);
        }

        private void buttonOpenInExcel_Click(object sender, EventArgs e)
        {
            string csvFilepath = "cavapa_obs.csv";
            using (TextWriter file = File.CreateText(csvFilepath))
            {
                string line = "";
                foreach (DataGridViewColumn col in dataGridView1.Columns)
                {
                    line += "\"" + col.HeaderText + "\",";
                }
                file.WriteLine(line);
                line = "";
                foreach (DataGridViewRow row in dataGridView1.Rows)
                {
                    foreach (DataGridViewCell cell in row.Cells)
                    {
                        if (cell.Value == null)
                        {
                            line += ",";
                            continue;
                        }
                        line += cell.Value.ToString() + ",";
                    }
                    file.WriteLine(line);
                    line = "";
                }
                file.Flush();
                file.Close();
            }

            Process proc = new Process();
            proc.StartInfo = new ProcessStartInfo("excel.exe", csvFilepath);
            proc.Start();

            //var excelApp = new Excel.Application();
            //// Make the object visible.
            //excelApp.Visible = true;

            //// Create a new, empty workbook and add it to the collection returned 
            //// by property Workbooks. The new workbook becomes the active workbook.
            //// Add has an optional parameter for specifying a praticular template. 
            //// Because no argument is sent in this example, Add creates a new workbook. 
            //excelApp.Workbooks.Add();

            //// This example uses a single workSheet. The explicit type casting is
            //// removed in a later procedure.
            //Excel._Worksheet workSheet = (Excel.Worksheet)excelApp.ActiveSheet;

            //foreach (DataGridViewColumn col in dataGridView1.Columns)
            //{
            //    workSheet.Cells[1, col.Index] = col.HeaderText;
            //}

            ////var row = 1;
            ////foreach (var acct in accounts)
            ////{
            ////    row++;
            ////    workSheet.Cells[row, "A"] = acct.ID;
            ////    workSheet.Cells[row, "B"] = acct.Balance;
            ////}

            //workSheet.Columns[1].AutoFit();
            //workSheet.Columns[2].AutoFit();
        }
    }
}
