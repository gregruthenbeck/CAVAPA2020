using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace DataViewer {
    public partial class Form1 : Form {
        string settingsFilepath = "settings.txt";
        string inFilepath = "";
        Bitmap bmp;
        float[] mins;
        float[] maxs;
        float[] spans;

        float[][] data;
        public Form1() {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e) {
            if (File.Exists(settingsFilepath)) {
                using (TextReader file = File.OpenText(settingsFilepath)) {
                    string line = file.ReadLine();
                    inFilepath = line;
                    if (File.Exists(inFilepath)) {
                        LoadData();
                        Draw();
                    }
                }
            }
            bmp = new Bitmap(pictureBox1.Width, pictureBox1.Height, PixelFormat.Format24bppRgb);
        }
        void LoadData() {
            data = Utils.LoadCSVData(inFilepath);

            mins = new float[data.Length];
            maxs = new float[data.Length];
            spans = new float[data.Length];

            for (int i = 0; i < data.Length; i++) {
                Utils.GetExtents(out mins[i], out maxs[i], ref data[i]);
                spans[i] = maxs[i] - mins[i];
            }

            trackBar1.Maximum = data[0].Length - 301;
            trackBar1.TickFrequency = trackBar1.Maximum / 20;
            trackBar1.Value = 0;
        }

        private void OpenToolStripMenuItem_Click(object sender, EventArgs e) {
            using (OpenFileDialog dlg = new OpenFileDialog()) {
                if (File.Exists(inFilepath))
                    dlg.InitialDirectory = inFilepath.Substring(0, inFilepath.LastIndexOf("\\"));
                dlg.Filter = "CSV data files (*.csv)|*.csv|All files (*.*)|*.*";
                dlg.FilterIndex = 1;
                dlg.RestoreDirectory = true;

                if (dlg.ShowDialog() == DialogResult.OK) {
                    inFilepath = dlg.FileName;
                    LoadData();
                    Draw();
                }
            }
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e) {
            using (TextWriter file = File.CreateText(settingsFilepath)) {
                file.WriteLine(inFilepath);
                file.Flush();
                file.Close();
            }
        }

        void Draw() {
            if (data == null && data[0] == null)
                return;
            using (Graphics g = Graphics.FromImage(bmp)) {
                g.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.HighQuality;
                g.FillRectangle(Brushes.White, 0, 0, bmp.Width, bmp.Height);
                int offset = trackBar1.Value;
                float thickness = 3.0f;
                int alpha = 128;
                Pen[] pens = new Pen[] {
                    new Pen(Color.FromArgb(alpha,Color.Red), thickness),
                    new Pen(Color.FromArgb(alpha,Color.Green), thickness),
                    new Pen(Color.FromArgb(alpha,Color.Blue), thickness),
                    new Pen(Color.FromArgb(alpha,Color.Pink), thickness),
                    new Pen(Color.FromArgb(alpha,Color.Orange), thickness),
                    new Pen(Color.FromArgb(alpha,Color.Brown), thickness),
                    new Pen(Color.FromArgb(alpha,Color.Gray), thickness),
                    new Pen(Color.FromArgb(alpha,Color.Aqua), thickness),
                };
                List<List<PointF>> lines = new List<List<PointF>>();
                for (int i = 0; i < data.Length - 2; i++) {
                    lines.Add(new List<PointF>()); // ignore 1st two columns
                }
                float xScale = (float)trackBarXScale.Value / 10.0f;
                int samples = (int)((float)bmp.Width / xScale);
                int stride = Math.Max(1, (int)(2.0f / xScale));
                for (int i = 0; i < samples; i+=stride) {
                    if ((i + offset) == data[0].Length)
                        break;
                    for (int k = 2; k < data.Length; k++) {
                        lines[k - 2].Add(new PointF((float)i*xScale, 100.0f + 500.0f*(data[k][i+offset]/spans[k])));
                    }
                }
                int lineId = 0;
                foreach (var line in lines) {
                    g.DrawLines(pens[lineId], line.ToArray());
                    ++lineId;
                }
                { // Draw a legend in the bottom left
                    float margin = 15.0f;
                    float left = 30.0f;
                    float top = (float)bmp.Height - 180.0f;
                    float vSpacing = 20.0f;
                    g.FillRectangle(new SolidBrush(Color.FromArgb(128, Color.White)), left - margin, top - margin, 110.0f, 170.0f);
                    g.DrawRectangle(Pens.Gray, left - margin, top - margin, 110.0f, 190.0f);
                    for (int i = 0; i < pens.Length; i++) {
                        float y = top + (float)i * vSpacing;
                        g.DrawLine(pens[i], left, y, left + 60.0f, y);
                        g.DrawString((i + 1).ToString(), new Font(FontFamily.GenericSansSerif, 12.0f), Brushes.Black, left + 65.0f, y - 10.0f);
                    }
                }
            }
            pictureBox1.Image = bmp;
        }

        private void TrackBar1_ValueChanged(object sender, EventArgs e) {
            Draw();
        }

        private void TrackBarXScale_ValueChanged(object sender, EventArgs e) {
            Draw();
        }

        private void PictureBox1_Resize(object sender, EventArgs e) {
            bmp = new Bitmap(pictureBox1.Width, pictureBox1.Height, PixelFormat.Format24bppRgb);

        }
    }
}