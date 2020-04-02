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
using SharpMemoryCache;
using System.Runtime.Caching;
using System.Collections.Specialized;

namespace Cavapa_Observation_Score
{
    public partial class MainForm : Form
    {
        string inputFolder;
        string[] inputFramesFilepaths;
        List<Bitmap> frames;
        string[] framesFilenames;
        int obsInterval = 1;
        int loopInterval = 1;
        int loopFrameOffset = 0;
        Timer loopFrameTimer = new Timer();
        TrimmingMemoryCache memoryCache;
        ObjectCache frameCache;
        private CacheItemPolicy cachePolicy = new CacheItemPolicy();

        public MainForm()
        {
            InitializeComponent();
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            cachePolicy.RemovedCallback = new CacheEntryRemovedCallback(CacheItemRemovedCallback);
            NameValueCollection cacheSettings = new NameValueCollection(3);
            cacheSettings.Add("CacheMemoryLimitMegabytes", Convert.ToString(1024)); // 1GB
            cacheSettings.Add("physicalMemoryLimitPercentage", Convert.ToString(49));  // set % here
            cacheSettings.Add("pollingInterval", Convert.ToString("00:00:05"));
            memoryCache = new TrimmingMemoryCache("FrameCache", cacheSettings);
            frameCache = memoryCache;
            loopFrameTimer.Tick += LoopFrameTimer_Tick;
        }
        private static void CacheItemRemovedCallback(CacheEntryRemovedArguments arg)
        {
            //if (arg.RemovedReason != CacheEntryRemovedReason.Removed)
            {
                var item = arg.CacheItem.Value as IDisposable;
                if (item != null)
                    item.Dispose();
            }
        }
        public bool ThumbnailCallback()
        {
            return false;
        }

        private void selectFramesInputFolderToolStripMenuItem_Click(object sender, EventArgs e)
        {
            frames = new List<Bitmap>();

            using (OpenFileDialog ofd = new OpenFileDialog()) {
                ofd.Filter = "Jpg image files *.jpg|*.jp*g|All files *.*|*.*";

                if (ofd.ShowDialog() == DialogResult.OK) {
                    inputFolder = Path.GetDirectoryName(ofd.FileName);
                    this.Text = "VOST - Video Observational Scoring Tool - Greg Ruthenbeck © 2020 - " + inputFolder;
                    inputFramesFilepaths = Directory.GetFiles(inputFolder, "*.jp*g");

                    //statusProgressBar.Maximum = inputFramesFilepaths.Length;
                    //statusProgressBar.Value = 0;
                    //statusProgressBar.Visible = true;
                    List<string> fnames = new List<string>();
                    foreach (string fname in inputFramesFilepaths)
                    {
                        fnames.Add(Path.GetFileName(fname));
                    }
                    framesFilenames = fnames.ToArray();
                    //    Bitmap bmp = new Bitmap(fname);
                    //    Image.GetThumbnailImageAbort myCallback = new Image.GetThumbnailImageAbort(ThumbnailCallback);
                    //    bmp = (Bitmap)bmp.GetThumbnailImage(bmp.Width / 8, bmp.Height / 8, myCallback, IntPtr.Zero);
                    //    frames.Add(bmp);
                    //    ++statusProgressBar.Value;
                    //}
                    //statusProgressBar.Visible = false;
                    //statusLabel.Text = frames.Count.ToString() + " frames loaded";

                    textBoxLoopSpeed_TextChanged(this, null);
                    obsInterval = int.Parse(textBoxObsInterval.Text) * int.Parse(textBoxFps.Text);
                    loopInterval = int.Parse(textBoxLoopInterval.Text) * int.Parse(textBoxFps.Text);

                    trackBarTime.Maximum = framesFilenames.Length - 1;
                    TimeSpan duration = new TimeSpan(10000L * (long)framesFilenames.Length * (1000L / long.Parse(textBoxFps.Text)));
                    labelDuration.Text = duration.ToString("hh':'mm':'ss'.'ff") + "s";

                    trackBar1.Maximum = framesFilenames.Length - 1;
                    trackBar1.Value = 0;
                    trackBar1.TickFrequency = obsInterval;
                    trackBar1.LargeChange = obsInterval;

                    dataGridView1.Rows.Add((int)(0.5f + (float)framesFilenames.Length / (float)obsInterval));
                    foreach (DataGridViewColumn col in dataGridView1.Columns)
                    {
                        col.DefaultCellStyle.Alignment = DataGridViewContentAlignment.BottomRight;
                    }
                    foreach (DataGridViewRow row in dataGridView1.Rows)
                    {
                        int id = (row.Index + 1);
                        row.Cells[0].Value = id;
                        long ticks = 10000L * (long)row.Index * (long)obsInterval * (1000L / long.Parse(textBoxFps.Text));
                        TimeSpan time = new TimeSpan(ticks);
                        row.Cells[1].Value = time;
                    }

                    trackBar1_ValueChanged(this, null);
                    //pictureBox1.Image = frames[0];
                }
            }
        }

        private void LoopFrameTimer_Tick(object sender, EventArgs e)
        {
            ++loopFrameOffset;
            if (loopFrameOffset == loopInterval)
                loopFrameOffset = 0;

            int frameId = trackBar1.Value + loopFrameOffset;
            if (frameId >= 0 && frameId < framesFilenames.Length)
            {
                pictureBox1.Image = GetFrameFromCache(frameId);
                trackBarTime.Value = frameId;
            }
        }

        Bitmap GetFrameFromCache(int frameId) {
            string fname = framesFilenames[frameId];
            if (frameCache[fname] == null)
            {
                frameCache[fname] = new Bitmap(Path.Combine(inputFolder, fname));
                //if (memoryCache.Count() > 1000) { // Cache 2000 frames
                //    memoryCache.Trim(80); // Clear 30% of the cache
                //}
            }
            return (Bitmap)frameCache[fname];
        }

        private void trackBar1_ValueChanged(object sender, EventArgs e)
        {
            //TimeSpan time = new TimeSpan(10000L * (long)trackBar1.Value * (long)obsInterval * (1000L / long.Parse(textBoxFps.Text)));
            TimeSpan time = new TimeSpan(10000L * (long)trackBar1.Value * (1000L / long.Parse(textBoxFps.Text)));
            labelTime.Text = time.ToString("hh':'mm':'ss'.'ff");
            labelFrameCounter.Text = "Frame:  " + trackBar1.Value.ToString() + "/" + trackBar1.Maximum.ToString();

            //pictureBox1.Image = frames[trackBar1.Value];
            pictureBox1.Image = GetFrameFromCache(trackBar1.Value);

            trackBarTime.Value = trackBar1.Value;
            int row = trackBar1.Value / obsInterval;
            //if (row > 0) {
            //    dataGridView1.Rows[row - 1].Selected = false;
            //}
            //if (row < (dataGridView1.Rows.Count-1)) {
            //    dataGridView1.Rows[row + 1].Selected = false;
            //}
            //dataGridView1.Rows[row].Selected = true;
            dataGridView1.CurrentCell = dataGridView1[dataGridView1.CurrentCell.ColumnIndex, row];
        }
        private void dataGridView1_CurrentCellChanged(object sender, EventArgs e)
        {
            trackBar1.Value = dataGridView1.CurrentCell.RowIndex * obsInterval;
        }

        private void buttonPlayLoop_Click(object sender, EventArgs e)
        {
            loopFrameOffset = 0;
            loopFrameTimer.Enabled = !loopFrameTimer.Enabled;
            buttonPlayLoop.BackColor = (loopFrameTimer.Enabled ? Color.LightGreen : Color.LightGray);
        }

        private void textBoxLoopSpeed_TextChanged(object sender, EventArgs e)
        {
            float loopSpeed = 1.0f;
            if (float.TryParse(textBoxLoopSpeed.Text, out loopSpeed) &&
                (loopSpeed > 0.1f && loopSpeed < 20.0f))
            {
                textBoxLoopSpeed.BackColor = Color.White;
                loopFrameTimer.Interval = (int)(1000.0f / ((float)int.Parse(textBoxFps.Text) * loopSpeed));
            }
            else {
                textBoxLoopSpeed.BackColor = Color.Pink;
            }
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
