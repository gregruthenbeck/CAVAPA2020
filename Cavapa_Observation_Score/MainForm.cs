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
                }
            }
        }
    }
}
