using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Cavapa_Observation_Score
{
    public partial class MainForm : Form
    {
        string inputFolder;
        string[] inputFramesFilepaths;
        List<Bitmap> frames;
        int obsInterval = 1;
        public MainForm()
        {
            InitializeComponent();
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
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

                    obsInterval = int.Parse(textBoxObsInterval.Text) * int.Parse(textBoxFps.Text);

                    trackBar1.Maximum = frames.Count - 1;
                    trackBar1.Value = 0;
                    trackBar1.TickFrequency = obsInterval;
                    trackBar1.LargeChange = obsInterval;

                    dataGridView1.Rows.Add((int)(0.5f + (float)frames.Count / (float)obsInterval) - 1); // already have 1 row

                    pictureBox1.Image = frames[0];
                }
            }
        }

        private void trackBar1_ValueChanged(object sender, EventArgs e)
        {
            pictureBox1.Image = frames[trackBar1.Value];
            int row = trackBar1.Value / obsInterval;
            if (row > 0) {
                dataGridView1.Rows[row - 1].Selected = false;
            }
            if (row < (dataGridView1.Rows.Count-1)) {
                dataGridView1.Rows[row + 1].Selected = false;
            }
            dataGridView1.Rows[row].Selected = true;
        }
    }
}
