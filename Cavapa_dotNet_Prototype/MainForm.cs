using System;
using System.CodeDom;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Cavapa_dotNet_Prototype
{
    public partial class MainForm : Form
    {
        string settingsFilepath = "cavapa.txt";
        string inputFolder = "";
        bool processingPaused = false;
        List<Bitmap> aveFrames = new List<Bitmap>();
        public MainForm()
        {
            InitializeComponent();
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            if (File.Exists(settingsFilepath))
            {
                using (TextReader file = File.OpenText(settingsFilepath))
                {
                    inputFolder = file.ReadLine();
                }
            }

            bool showFolderSelectDlg = false;
            string[] files;
            if (inputFolder == "" || !Directory.Exists(inputFolder))
            {
                showFolderSelectDlg = true;
            }
            else
            {
                files = Directory.GetFiles(inputFolder);
                if (files.Length < 4 || Path.GetExtension(files[0]) != ".jpg")
                {
                    showFolderSelectDlg = true;
                }
            }

            if (showFolderSelectDlg)
            {
                using (OpenFileDialog dlg = new OpenFileDialog())
                {
                    dlg.InitialDirectory = inputFolder;
                    dlg.Filter = "jpg files (*.jpg)|*.jpg|All files (*.*)|*.*";
                    dlg.FilterIndex = 2;
                    dlg.RestoreDirectory = true;

                    if (dlg.ShowDialog() == DialogResult.OK)
                    {
                        inputFolder = Path.GetDirectoryName(dlg.FileName) + "\\";
                    }
                }
            }

            files = Directory.GetFiles(inputFolder, "*.jpg");

            int startFrame = 5000;
            int endFrame = Math.Min(7000, files.Length);
            Debug.Assert(startFrame < files.Length);
            int framesWindowLength = 100;
            List<Bitmap> framesWindow = new List<Bitmap>();
            for (int i = 0; i < framesWindowLength; i++)
            {
                framesWindow.Add(new Bitmap(files[startFrame + i * 2]));
            }
            for (int i = startFrame; i < (endFrame - framesWindowLength); i++)
            {
                framesWindow.Add(new Bitmap(files[i + framesWindowLength]));
                Bitmap bg = ComputeAverage(ref framesWindow, framesWindow[0].Width, framesWindow[0].Height);
                aveFrames.Add(bg);
                framesWindow.RemoveAt(0);
            }

            trackBar1.Minimum = 0;
            trackBar1.Maximum = aveFrames.Count - 1;
            trackBar1.Value = (trackBar1.Maximum + trackBar1.Minimum) / 2;
            trackBar1_Scroll(this, null);
            statusLabel.Text = "Processing: Done!";
        }

        /// <summary>
        /// Displays a Violet image when successful (red + blue)
        /// </summary>
        private void TestComputeAverage() {
            Bitmap a = new Bitmap(1080, 720, PixelFormat.Format24bppRgb);
            Bitmap b = new Bitmap(1080, 720, PixelFormat.Format24bppRgb);
            Rectangle rect = new Rectangle(0, 0, a.Width, a.Height);
            using (Graphics g = Graphics.FromImage(a))
            {
                g.FillRectangle(Brushes.Red, rect);
            }
            using (Graphics g = Graphics.FromImage(b))
            {
                g.FillRectangle(Brushes.Blue, rect);
            }
            List<Bitmap> l = new List<Bitmap>();
            l.Add(a); l.Add(b);
            Bitmap c = ComputeAverage(ref l, a.Width, a.Height);
            pictureBox1.Image = c;
        }

        private unsafe Bitmap ComputeAverage(ref List<Bitmap> bitmaps, int width, int height) {
            Bitmap[] bmps = bitmaps.ToArray();
            Rectangle fullRect = new Rectangle(0, 0, width, height);
            RGB[] rgbTotals = new RGB[width * height];
            for (int i = 0; i < rgbTotals.Length; i++)
            {
                rgbTotals[i] = new RGB();
            }
            BitmapData[] bitmapDatas = new BitmapData[bitmaps.Count];
            for (int i = 0; i < bitmapDatas.Length; i++)
            {
                bitmapDatas[i] = bmps[i].LockBits(fullRect, ImageLockMode.ReadOnly, bmps[i].PixelFormat);
            }
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    for (int i = 0; i < bitmapDatas.Length; i++) {
                        byte* p = (byte*)(bitmapDatas[i].Scan0.ToPointer()) + bitmapDatas[i].Stride * y + 3 * x;
                        rgbTotals[y * width + x].Add((float)*(p + 0),
                                                     (float)*(p + 1),
                                                     (float)*(p + 2));
                    }
                }
            }
            for (int i = 0; i < bitmapDatas.Length; i++)
            {
                bmps[i].UnlockBits(bitmapDatas[i]);
            }
            Bitmap ave = new Bitmap(width, height, bitmaps[0].PixelFormat);
            BitmapData bmpData = ave.LockBits(fullRect, ImageLockMode.WriteOnly, bitmaps[0].PixelFormat);
            float oneOnCount = 1.0f / (float)bitmaps.Count;
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    byte* p = (byte*)(bmpData.Scan0.ToPointer()) + bmpData.Stride * y + 3 * x;
                    RGB pix = rgbTotals[y * width + x];
                    *(p + 0) = (byte)(pix.r * oneOnCount);
                    *(p + 1) = (byte)(pix.g * oneOnCount);
                    *(p + 2) = (byte)(pix.b * oneOnCount);
                }
            }
            ave.UnlockBits(bmpData);
            return ave;
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (File.Exists(settingsFilepath))
            {
                File.Delete(settingsFilepath);
            }

            using (TextWriter file = File.CreateText(settingsFilepath)) {
                file.WriteLine(inputFolder);
                file.Flush();
                file.Close();
            }
        }

        private void buttonPause_Click(object sender, EventArgs e)
        {
            processingPaused = !processingPaused;
            statusLabel.Text = (processingPaused ? "Paused" : "Processing");
        }

        private void trackBar1_Scroll(object sender, EventArgs e)
        {
            pictureBox1.Image = aveFrames[trackBar1.Value];
        }
    }

    public class RGB { 
        public float r, g, b;
        public RGB() {
            r = 0.0f;
            g = 0.0f;
            b = 0.0f;
        }
        public void Add(RGB v)
        {
            r += v.r;
            g += v.g;
            b += v.b;
        }
        public void Add(float x, float y, float z)
        {
            r += x;
            g += y;
            b += z;
        }
    }
}
