using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.IO.Ports;
using System.Linq;
using System.Management;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace AutoDoorFlasher
{
    public class MainForm : Form
    {
        // UI Controls
        private Label lblHeaderTitle = null!;
        private Label lblHeaderSub = null!;
        private Label lblStatusPill = null!;
        private GroupBox grpConnection = null!;
        private ComboBox cboPorts = null!;
        private Button btnRefreshPorts = null!;
        private Button btnTestChip = null!;
        private Button btnDeviceManager = null!;
        private ComboBox cboBaud = null!;
        private GroupBox grpFirmware = null!;
        private Label lblFwStatus = null!;
        private Label lblFwDetails = null!;
        private CheckBox chkEraseFlash = null!;
        private ProgressBar progressBar = null!;
        private Label lblProgressText = null!;
        private Label lblProgressPct = null!;
        private Button btnOneClickFlash = null!;
        private RichTextBox txtLog = null!;
        private Button btnClearLog = null!;
        private Label lblTip = null!;

        // Background Port Monitor Timer
        private System.Windows.Forms.Timer portMonitorTimer = null!;
        private List<string> knownPorts = new List<string>();

        // Working directories for embedded extraction
        private string appDataDir = "";
        private string esptoolDir = "";
        private string esptoolExe = "";

        // Partition Offsets
        private const string OFFSET_BOOTLOADER = "0x0000";
        private const string OFFSET_PARTITIONS = "0x8000";
        private const string OFFSET_BOOT_APP0  = "0x29000";
        private const string OFFSET_FIRMWARE   = "0x30000";

        public MainForm()
        {
            InitializeComponentModern();
            InitEngine();
        }

        private void InitializeComponentModern()
        {
            // Modern Dark Theme Colors
            Color colBgMain     = Color.FromArgb(24, 24, 37);      // #181825
            Color colBgCard     = Color.FromArgb(30, 30, 46);      // #1e1e2e
            Color colBgCardAlt  = Color.FromArgb(37, 37, 56);      // #252538
            Color colTextMain   = Color.FromArgb(205, 214, 244);   // #cdd6f4
            Color colTextMuted  = Color.FromArgb(147, 153, 178);   // #9399b2
            Color colAccentCyan = Color.FromArgb(0, 210, 255);     // #00d2ff
            Color colAccentBlue = Color.FromArgb(0, 119, 182);     // #0077b6
            Color colSuccess    = Color.FromArgb(16, 185, 129);    // #10b981
            Color colError      = Color.FromArgb(239, 68, 68);     // #ef4444
            Color colWarning    = Color.FromArgb(245, 158, 11);    // #f59e0b

            this.Text = "⚡ AutoDoor ESP32-C3 - One Click Flasher (Standalone Edition)";
            this.Size = new Size(820, 780);
            this.MinimumSize = new Size(760, 720);
            this.StartPosition = FormStartPosition.CenterScreen;
            this.BackColor = colBgMain;
            this.ForeColor = colTextMain;
            this.Font = new Font("Segoe UI", 9F, FontStyle.Regular);

            // 1. Header Panel
            Panel pnlHeader = new Panel
            {
                Dock = DockStyle.Top,
                Height = 70,
                BackColor = Color.FromArgb(17, 17, 27),
                Padding = new Padding(16, 10, 16, 10)
            };
            this.Controls.Add(pnlHeader);

            lblHeaderTitle = new Label
            {
                Text = "⚡ AutoDoor ESP32-C3 One-Click Flasher",
                Font = new Font("Segoe UI", 15F, FontStyle.Bold),
                ForeColor = colAccentCyan,
                AutoSize = true,
                Location = new Point(16, 10)
            };
            pnlHeader.Controls.Add(lblHeaderTitle);

            lblHeaderSub = new Label
            {
                Text = "โปรแกรมแฟลชเฟิร์มแวร์ ESP32-C3 ผ่านสาย USB รวมทุกอย่างในไฟล์เดียว (Offset 0x30000 | 2x OTA + NVS 128KB)",
                Font = new Font("Segoe UI", 8.5F),
                ForeColor = colTextMuted,
                AutoSize = true,
                Location = new Point(18, 38)
            };
            pnlHeader.Controls.Add(lblHeaderSub);

            lblStatusPill = new Label
            {
                Text = "🔴 รอเสียบสาย USB",
                Font = new Font("Segoe UI", 9F, FontStyle.Bold),
                ForeColor = colError,
                BackColor = colBgCardAlt,
                AutoSize = false,
                Size = new Size(160, 32),
                TextAlign = ContentAlignment.MiddleCenter,
                Location = new Point(pnlHeader.Width - 180, 18),
                Anchor = AnchorStyles.Top | AnchorStyles.Right
            };
            pnlHeader.Controls.Add(lblStatusPill);

            // 2. Main Container Panel
            Panel pnlMain = new Panel
            {
                Dock = DockStyle.Fill,
                BackColor = colBgMain,
                Padding = new Padding(16, 8, 16, 8),
                AutoScroll = true
            };
            this.Controls.Add(pnlMain);
            pnlMain.BringToFront();

            int curY = 8;

            // -------------------------------------------------------------
            // GroupBox: Connection
            // -------------------------------------------------------------
            grpConnection = new GroupBox
            {
                Text = " 🔌 การเชื่อมต่อพอร์ต Serial (USB COM Port) ",
                Location = new Point(16, curY),
                Size = new Size(770, 115),
                Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right,
                ForeColor = Color.FromArgb(137, 220, 235),
                BackColor = colBgCard,
                Font = new Font("Segoe UI", 9.5F, FontStyle.Bold)
            };
            pnlMain.Controls.Add(grpConnection);

            Label lblPort = new Label
            {
                Text = "COM Port:",
                Location = new Point(14, 28),
                Size = new Size(80, 24),
                ForeColor = colTextMain,
                Font = new Font("Segoe UI", 9F, FontStyle.Bold)
            };
            grpConnection.Controls.Add(lblPort);

            cboPorts = new ComboBox
            {
                Location = new Point(95, 25),
                Size = new Size(370, 26),
                DropDownStyle = ComboBoxStyle.DropDownList,
                Font = new Font("Segoe UI", 9F),
                BackColor = Color.FromArgb(17, 17, 27),
                ForeColor = Color.White
            };
            grpConnection.Controls.Add(cboPorts);

            btnRefreshPorts = new Button
            {
                Text = "🔄 สแกนพอร์ต",
                Location = new Point(475, 24),
                Size = new Size(110, 28),
                FlatStyle = FlatStyle.Flat,
                BackColor = Color.FromArgb(49, 50, 68),
                ForeColor = colTextMain,
                Font = new Font("Segoe UI", 8.5F, FontStyle.Bold),
                Cursor = Cursors.Hand
            };
            btnRefreshPorts.FlatAppearance.BorderSize = 1;
            btnRefreshPorts.FlatAppearance.BorderColor = Color.FromArgb(69, 71, 90);
            btnRefreshPorts.Click += (s, e) => ScanPorts(true);
            grpConnection.Controls.Add(btnRefreshPorts);

            btnDeviceManager = new Button
            {
                Text = "🛠️ Device Manager",
                Location = new Point(595, 24),
                Size = new Size(150, 28),
                FlatStyle = FlatStyle.Flat,
                BackColor = Color.FromArgb(49, 50, 68),
                ForeColor = colTextMain,
                Font = new Font("Segoe UI", 8.5F),
                Cursor = Cursors.Hand
            };
            btnDeviceManager.FlatAppearance.BorderSize = 1;
            btnDeviceManager.FlatAppearance.BorderColor = Color.FromArgb(69, 71, 90);
            btnDeviceManager.Click += (s, e) => {
                try { Process.Start(new ProcessStartInfo("devmgmt.msc") { UseShellExecute = true }); }
                catch (Exception ex) { MessageBox.Show("ไม่สามารถเปิด Device Manager ได้: " + ex.Message); }
            };
            grpConnection.Controls.Add(btnDeviceManager);

            Label lblBaud = new Label
            {
                Text = "ความเร็ว (Baud):",
                Location = new Point(14, 68),
                Size = new Size(100, 24),
                ForeColor = colTextMuted,
                Font = new Font("Segoe UI", 8.5F)
            };
            grpConnection.Controls.Add(lblBaud);

            cboBaud = new ComboBox
            {
                Location = new Point(115, 65),
                Size = new Size(140, 26),
                DropDownStyle = ComboBoxStyle.DropDownList,
                Font = new Font("Segoe UI", 8.5F),
                BackColor = Color.FromArgb(17, 17, 27),
                ForeColor = Color.White
            };
            cboBaud.Items.AddRange(new object[] { "460800", "921600", "115200", "230400" });
            cboBaud.SelectedIndex = 0;
            grpConnection.Controls.Add(cboBaud);

            Label lblBaudNote = new Label
            {
                Text = "(460800 แนะนำ: เร็วและเสถียร | 115200 สำหรับสายยาว)",
                Location = new Point(265, 68),
                AutoSize = true,
                ForeColor = colTextMuted,
                Font = new Font("Segoe UI", 8F)
            };
            grpConnection.Controls.Add(lblBaudNote);

            btnTestChip = new Button
            {
                Text = "🔍 เช็คชิป ESP32",
                Location = new Point(595, 64),
                Size = new Size(150, 28),
                FlatStyle = FlatStyle.Flat,
                BackColor = Color.FromArgb(49, 50, 68),
                ForeColor = Color.FromArgb(166, 227, 161),
                Font = new Font("Segoe UI", 8.5F, FontStyle.Bold),
                Cursor = Cursors.Hand
            };
            btnTestChip.FlatAppearance.BorderSize = 1;
            btnTestChip.FlatAppearance.BorderColor = Color.FromArgb(69, 71, 90);
            btnTestChip.Click += async (s, e) => await TestChipAsync();
            grpConnection.Controls.Add(btnTestChip);

            curY += 122;

            // -------------------------------------------------------------
            // GroupBox: Firmware Status
            // -------------------------------------------------------------
            grpFirmware = new GroupBox
            {
                Text = " 📦 เฟิร์มแวร์และโครงสร้างพาร์ติชัน (Embedded Firmware Binaries) ",
                Location = new Point(16, curY),
                Size = new Size(770, 95),
                Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right,
                ForeColor = Color.FromArgb(137, 220, 235),
                BackColor = colBgCard,
                Font = new Font("Segoe UI", 9.5F, FontStyle.Bold)
            };
            pnlMain.Controls.Add(grpFirmware);

            lblFwStatus = new Label
            {
                Text = "✅ ฝังไบนารีครบ 4 ไฟล์ในตัว: Bootloader (0x0000) | Partitions (0x8000) | Boot_App0 (0x29000) | Firmware (0x30000)",
                Location = new Point(14, 25),
                AutoSize = true,
                ForeColor = colSuccess,
                Font = new Font("Segoe UI", 9F, FontStyle.Bold)
            };
            grpFirmware.Controls.Add(lblFwStatus);

            lblFwDetails = new Label
            {
                Text = "📦 firmware.bin: ~1.55 MB (พาร์ติชัน 2x OTA รองรับ NVS 128KB แท็กไม่หาย) พร้อมทำงานได้ทุกเครื่องทันที",
                Location = new Point(14, 48),
                AutoSize = true,
                ForeColor = colTextMuted,
                Font = new Font("Segoe UI", 8F)
            };
            grpFirmware.Controls.Add(lblFwDetails);

            chkEraseFlash = new CheckBox
            {
                Text = "ล้างข้อมูล Flash ทั้งหมดก่อนอัพโหลด (Erase Flash)",
                Location = new Point(14, 68),
                AutoSize = true,
                ForeColor = Color.FromArgb(243, 139, 168),
                Font = new Font("Segoe UI", 8F),
                BackColor = Color.Transparent
            };
            grpFirmware.Controls.Add(chkEraseFlash);

            curY += 102;

            // -------------------------------------------------------------
            // Action Area: Progress & One Click Flash Button
            // -------------------------------------------------------------
            Panel pnlAction = new Panel
            {
                Location = new Point(16, curY),
                Size = new Size(770, 115),
                Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right,
                BackColor = Color.Transparent
            };
            pnlMain.Controls.Add(pnlAction);

            progressBar = new ProgressBar
            {
                Location = new Point(0, 0),
                Size = new Size(770, 20),
                Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right,
                Minimum = 0,
                Maximum = 100,
                Value = 0
            };
            pnlAction.Controls.Add(progressBar);

            Panel pnlProgText = new Panel
            {
                Location = new Point(0, 23),
                Size = new Size(770, 20),
                Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right,
                BackColor = Color.Transparent
            };
            pnlAction.Controls.Add(pnlProgText);

            lblProgressText = new Label
            {
                Text = "พร้อมทำงาน - เสียบสาย USB แล้วกดปุ่ม ONE CLICK FLASH ด้านล่าง",
                Dock = DockStyle.Left,
                AutoSize = true,
                ForeColor = colTextMuted,
                Font = new Font("Segoe UI", 8.5F)
            };
            pnlProgText.Controls.Add(lblProgressText);

            lblProgressPct = new Label
            {
                Text = "0%",
                Dock = DockStyle.Right,
                AutoSize = true,
                ForeColor = colAccentCyan,
                Font = new Font("Segoe UI", 8.5F, FontStyle.Bold)
            };
            pnlProgText.Controls.Add(lblProgressPct);

            btnOneClickFlash = new Button
            {
                Text = "⚡ ONE CLICK FLASH (อัพโหลดลงบอร์ด)",
                Location = new Point(0, 48),
                Size = new Size(770, 58),
                Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right,
                FlatStyle = FlatStyle.Flat,
                BackColor = Color.FromArgb(0, 180, 216),
                ForeColor = Color.Black,
                Font = new Font("Segoe UI", 13F, FontStyle.Bold),
                Cursor = Cursors.Hand
            };
            btnOneClickFlash.FlatAppearance.BorderSize = 0;
            btnOneClickFlash.Click += async (s, e) => await StartOneClickFlashAsync();
            pnlAction.Controls.Add(btnOneClickFlash);

            curY += 120;

            // -------------------------------------------------------------
            // Log Console
            // -------------------------------------------------------------
            GroupBox grpLog = new GroupBox
            {
                Text = " 💻 บันทึกการทำงานสด (Live Console Output) ",
                Location = new Point(16, curY),
                Size = new Size(770, 250),
                Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right,
                ForeColor = Color.FromArgb(137, 220, 235),
                BackColor = colBgCard,
                Font = new Font("Segoe UI", 9.5F, FontStyle.Bold)
            };
            pnlMain.Controls.Add(grpLog);

            txtLog = new RichTextBox
            {
                Location = new Point(12, 24),
                Size = new Size(745, 185),
                Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right,
                BackColor = Color.FromArgb(17, 17, 27),
                ForeColor = Color.FromArgb(205, 214, 244),
                Font = new Font("Consolas", 9F),
                ReadOnly = true,
                BorderStyle = BorderStyle.None
            };
            grpLog.Controls.Add(txtLog);

            lblTip = new Label
            {
                Text = "💡 หากโปรแกรมค้างที่ Connecting... ให้กดปุ่ม BOOT บนบอร์ดค้างไว้ แล้วกดปุ่ม RESET (EN) 1 ครั้ง",
                Location = new Point(12, 218),
                AutoSize = true,
                Anchor = AnchorStyles.Bottom | AnchorStyles.Left,
                ForeColor = Color.FromArgb(249, 226, 175),
                Font = new Font("Segoe UI", 8F)
            };
            grpLog.Controls.Add(lblTip);

            btnClearLog = new Button
            {
                Text = "🧹 ล้าง Log",
                Location = new Point(660, 214),
                Size = new Size(95, 26),
                Anchor = AnchorStyles.Bottom | AnchorStyles.Right,
                FlatStyle = FlatStyle.Flat,
                BackColor = Color.FromArgb(49, 50, 68),
                ForeColor = colTextMuted,
                Font = new Font("Segoe UI", 8F),
                Cursor = Cursors.Hand
            };
            btnClearLog.FlatAppearance.BorderSize = 1;
            btnClearLog.FlatAppearance.BorderColor = Color.FromArgb(69, 71, 90);
            btnClearLog.Click += (s, e) => txtLog.Clear();
            grpLog.Controls.Add(btnClearLog);
        }

        private void InitEngine()
        {
            try
            {
                appDataDir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "AutoDoorFlasher");
                Directory.CreateDirectory(appDataDir);
                esptoolDir = Path.Combine(appDataDir, "engine");
                Directory.CreateDirectory(esptoolDir);

                ExtractEmbeddedFiles();

                AppendLog("✨ ยินดีต้อนรับสู่ AutoDoor ESP32-C3 One-Click Flasher (Standalone Edition)");
                AppendLog("📦 ระบบ Single-File EXE พร้อมทำงานทันที ไม่ต้องติดตั้งโปรแกรมหรือ Python เพิ่มเติม");
                AppendLog("🔌 เสียบสาย USB เข้ากับคอมพิวเตอร์ ระบบจะสแกนพอร์ตให้อัตโนมัติ");

                // Start Port Monitor Timer
                portMonitorTimer = new System.Windows.Forms.Timer { Interval = 1500 };
                portMonitorTimer.Tick += (s, e) => CheckPortsPeriodic();
                portMonitorTimer.Start();

                ScanPorts(true);
            }
            catch (Exception ex)
            {
                AppendLog($"❌ Init error: {ex.Message}");
            }
        }

        private void ExtractEmbeddedFiles()
        {
            var assembly = Assembly.GetExecutingAssembly();

            // 1. Extract firmware binaries
            string[] binFiles = new[] { "bootloader.bin", "partitions.bin", "boot_app0.bin", "firmware.bin" };
            foreach (var bin in binFiles)
            {
                string targetPath = Path.Combine(appDataDir, bin);
                ExtractResourceIfNewer(assembly, bin, targetPath);
            }

            // 2. Extract embedded standalone esptool engine
            string zipPath = Path.Combine(appDataDir, "esptool.zip");
            ExtractResourceIfNewer(assembly, "esptool.zip", zipPath);

            esptoolExe = Path.Combine(esptoolDir, "esptool-standalone.exe");
            if (!File.Exists(esptoolExe))
            {
                AppendLog("⚙️ กำลังเตรียมระบบ Flasher Engine ในเครื่องครั้งแรก...");
                if (File.Exists(zipPath))
                {
                    ZipFile.ExtractToDirectory(zipPath, esptoolDir, true);
                }
            }
        }

        private void ExtractResourceIfNewer(Assembly assembly, string resourceNameEnd, string targetPath)
        {
            string? fullResourceName = assembly.GetManifestResourceNames()
                .FirstOrDefault(r => r.EndsWith(resourceNameEnd, StringComparison.OrdinalIgnoreCase));

            if (fullResourceName == null) return;

            using (Stream? resStream = assembly.GetManifestResourceStream(fullResourceName))
            {
                if (resStream == null) return;

                if (!File.Exists(targetPath) || new FileInfo(targetPath).Length != resStream.Length)
                {
                    using (FileStream fs = new FileStream(targetPath, FileMode.Create, FileAccess.Write))
                    {
                        resStream.CopyTo(fs);
                    }
                }
            }
        }

        private void AppendLog(string message)
        {
            if (txtLog.InvokeRequired)
            {
                txtLog.Invoke(new Action(() => AppendLog(message)));
                return;
            }
            txtLog.AppendText(message + Environment.NewLine);
            txtLog.SelectionStart = txtLog.Text.Length;
            txtLog.ScrollToCaret();
        }

        private void ScanPorts(bool autoSelect)
        {
            var ports = GetDetailedSerialPorts();
            var nonBt = ports.Where(p => !p.IsBluetooth).ToList();

            string detected = AutoDetectEspPort(nonBt);
            string previousSelection = cboPorts.SelectedItem?.ToString()?.Split(' ')[0] ?? "";

            cboPorts.Items.Clear();
            int selectedIdx = -1;

            for (int i = 0; i < nonBt.Count; i++)
            {
                var p = nonBt[i];
                string tag = (p.PortName == detected) ? " 🌟 [ESP32-C3 แนะนำ]" : "";
                string item = $"{p.PortName} : {p.Description}{tag}";
                cboPorts.Items.Add(item);

                if (p.PortName == previousSelection)
                    selectedIdx = i;
                else if (selectedIdx == -1 && p.PortName == detected)
                    selectedIdx = i;
            }

            knownPorts = nonBt.Select(p => p.PortName).ToList();

            if (cboPorts.Items.Count > 0)
            {
                cboPorts.SelectedIndex = (selectedIdx >= 0) ? selectedIdx : 0;
                string currentPort = cboPorts.SelectedItem.ToString()!.Split(' ')[0];
                lblStatusPill.Text = $"🟢 เชื่อมต่อแล้ว ({currentPort})";
                lblStatusPill.ForeColor = Color.FromArgb(16, 185, 129);
                btnOneClickFlash.Enabled = true;
                btnTestChip.Enabled = true;
            }
            else
            {
                cboPorts.Items.Add("ไม่พบพอร์ต Serial (กรุณาเสียบสาย USB)");
                cboPorts.SelectedIndex = 0;
                lblStatusPill.Text = "🔴 รอเสียบสาย USB";
                lblStatusPill.ForeColor = Color.FromArgb(239, 68, 68);
                btnOneClickFlash.Enabled = false;
                btnTestChip.Enabled = false;
            }
        }

        private void CheckPortsPeriodic()
        {
            var currentNonBt = GetDetailedSerialPorts().Where(p => !p.IsBluetooth).Select(p => p.PortName).ToList();
            if (!currentNonBt.SequenceEqual(knownPorts))
            {
                ScanPorts(true);
            }
        }

        private string AutoDetectEspPort(List<PortInfo> ports)
        {
            var espCandidates = ports.Where(p => p.IsEspLikely).ToList();
            if (espCandidates.Count == 1) return espCandidates[0].PortName;
            if (espCandidates.Count > 1) return espCandidates[0].PortName;
            if (ports.Count == 1 && ports[0].PortName != "COM1") return ports[0].PortName;
            return "";
        }

        private async Task TestChipAsync()
        {
            string port = GetSelectedPort();
            if (string.IsNullOrEmpty(port)) return;

            btnTestChip.Enabled = false;
            AppendLog(new string('=', 60));
            AppendLog($"🔍 ทดสอบเชื่อมต่อและอ่านข้อมูลชิปบนพอร์ต {port}...");

            await Task.Run(() =>
            {
                RunEngine(new[] { "--chip", "esp32c3", "--port", port, "chip-id" }, line =>
                {
                    if (!line.StartsWith("esptool v", StringComparison.OrdinalIgnoreCase))
                        AppendLog("   " + line);
                });
            });

            btnTestChip.Enabled = true;
        }

        private async Task StartOneClickFlashAsync()
        {
            string port = GetSelectedPort();
            if (string.IsNullOrEmpty(port))
            {
                MessageBox.Show("กรุณาเชื่อมต่อสาย USB และเลือก COM Port ให้เรียบร้อย", "แจ้งเตือน", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            int baud = 460800;
            int.TryParse(cboBaud.SelectedItem?.ToString(), out baud);
            bool erase = chkEraseFlash.Checked;

            if (erase)
            {
                var dr = MessageBox.Show(
                    "คำเตือน: การล้างข้อมูล Flash ทั้งหมดจะทำให้ข้อมูล Tags และค่า WiFi หายไป\nต้องการดำเนินการต่อหรือไม่?",
                    "ยืนยันการ Erase Flash",
                    MessageBoxButtons.YesNo,
                    MessageBoxIcon.Warning
                );
                if (dr != DialogResult.Yes) return;
            }

            // Set UI to busy
            btnOneClickFlash.Enabled = false;
            btnOneClickFlash.Text = "⏳ กำลังแฟลชเฟิร์มแวร์ โปรดรอสักครู่...";
            btnOneClickFlash.BackColor = Color.FromArgb(88, 91, 112);
            btnTestChip.Enabled = false;
            btnRefreshPorts.Enabled = false;
            progressBar.Value = 0;
            lblProgressPct.Text = "0%";
            lblProgressText.Text = $"กำลังเชื่อมต่อกับ ESP32-C3 บน {port}...";
            lblProgressText.ForeColor = Color.FromArgb(0, 210, 255);

            AppendLog(new string('=', 65));
            AppendLog($"🚀 เริ่มต้น One Click Flash บนพอร์ต {port} (Baud: {baud})...");
            AppendLog("📌 พิกัด Flash Offsets (ตรงตาม partitions_two_ota.csv):");
            AppendLog($"   - {OFFSET_BOOTLOADER} ➔ bootloader.bin");
            AppendLog($"   - {OFFSET_PARTITIONS} ➔ partitions.bin (2x OTA + NVS 128KB)");
            AppendLog($"   - {OFFSET_BOOT_APP0} ➔ boot_app0.bin");
            AppendLog($"   - {OFFSET_FIRMWARE} ➔ firmware.bin");
            if (erase) AppendLog("⚠️ โหมด Erase All Flash: เปิดใช้งาน");
            AppendLog(new string('=', 65));

            var stopwatch = Stopwatch.StartNew();
            bool success = false;

            await Task.Run(() =>
            {
                string bootloaderFile = Path.Combine(appDataDir, "bootloader.bin");
                string partitionsFile = Path.Combine(appDataDir, "partitions.bin");
                string bootApp0File   = Path.Combine(appDataDir, "boot_app0.bin");
                string firmwareFile   = Path.Combine(appDataDir, "firmware.bin");

                var args = new List<string>
                {
                    "--chip", "esp32c3",
                    "--port", port,
                    "--baud", baud.ToString(),
                    "--before", "default-reset",
                    "--after", "hard-reset",
                    "write-flash", "-z",
                    "--flash-mode", "dio",
                    "--flash-freq", "80m",
                    "--flash-size", "4MB"
                };

                if (erase) args.Add("--erase-all");

                args.AddRange(new[]
                {
                    OFFSET_BOOTLOADER, bootloaderFile,
                    OFFSET_PARTITIONS, partitionsFile,
                    OFFSET_BOOT_APP0,  bootApp0File,
                    OFFSET_FIRMWARE,   firmwareFile
                });

                var pctRegex = new Regex(@"\((\d+)\s*%\)");
                int lastReportedPct = -1;

                success = RunEngine(args, line =>
                {
                    var match = pctRegex.Match(line);
                    if (match.Success)
                    {
                        if (int.TryParse(match.Groups[1].Value, out int pct))
                        {
                            UpdateProgress(pct);
                            if (pct != lastReportedPct && pct % 20 == 0)
                            {
                                AppendLog($"   ⏳ ความคืบหน้า: {pct}%");
                                lastReportedPct = pct;
                            }
                        }
                    }
                    else
                    {
                        AppendLog("   " + line);
                    }
                });
            });

            stopwatch.Stop();

            // Restore UI
            btnOneClickFlash.Enabled = true;
            btnOneClickFlash.Text = "⚡ ONE CLICK FLASH (อัพโหลดลงบอร์ด)";
            btnOneClickFlash.BackColor = Color.FromArgb(0, 180, 216);
            btnTestChip.Enabled = true;
            btnRefreshPorts.Enabled = true;

            if (success)
            {
                UpdateProgress(100);
                lblProgressText.Text = "🎉 แฟลชสำเร็จเรียบร้อย! บอร์ดรีบูตพร้อมทำงาน";
                lblProgressText.ForeColor = Color.FromArgb(16, 185, 129);

                AppendLog(new string('=', 65));
                AppendLog($"🎉 แฟลชเฟิร์มแวร์สำเร็จเรียบร้อย 100%! (ใช้เวลา {stopwatch.Elapsed.TotalSeconds:F1} วินาที)");
                AppendLog("💡 บอร์ด ESP32-C3 รีเซ็ตอัตโนมัติและพร้อมใช้งานทันที");
                AppendLog(new string('=', 65));

                MessageBox.Show(
                    $"🎉 แฟลชเฟิร์มแวร์ลงบอร์ด ESP32-C3 สำเร็จเรียบร้อย 100%!\n\nใช้เวลา: {stopwatch.Elapsed.TotalSeconds:F1} วินาที\nบอร์ดรีเซ็ตและพร้อมทำงานทันที",
                    "อัพโหลดสำเร็จ",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information
                );
            }
            else
            {
                lblProgressText.Text = "❌ การแฟลชล้มเหลว กรุณาตรวจสอบการเชื่อมต่อ";
                lblProgressText.ForeColor = Color.FromArgb(239, 68, 68);

                AppendLog(new string('=', 65));
                AppendLog("❌ การแฟลชล้มเหลว!");
                AppendLog("💡 วิธีแก้ไข:");
                AppendLog("   1. กดปุ่ม BOOT บนบอร์ดค้างไว้ แล้วกดปุ่ม RESET (EN) 1 ครั้ง");
                AppendLog("   2. ตรวจสอบว่าสาย USB เป็นสายส่งข้อมูล ไม่ใช่สายชาร์จอย่างเดียว");
                AppendLog("   3. ลองปรับ Baud Rate ลงเป็น 115200 bps หากสายยาว");
                AppendLog(new string('=', 65));

                MessageBox.Show(
                    "❌ การแฟลชล้มเหลว!\n\nคำแนะนำแก้ไข:\n1. กดปุ่ม BOOT บนบอร์ดค้างไว้แล้วกด RESET 1 ครั้ง เพื่อเข้า Bootloader Mode\n2. ตรวจสอบสาย USB หรือลองเสียบช่อง USB อื่น\n3. ลองสแกนพอร์ตใหม่อีกครั้ง",
                    "เกิดข้อผิดพลาด",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error
                );
            }
        }

        private void UpdateProgress(int pct)
        {
            if (this.InvokeRequired)
            {
                this.Invoke(new Action(() => UpdateProgress(pct)));
                return;
            }
            progressBar.Value = Math.Clamp(pct, 0, 100);
            lblProgressPct.Text = $"{pct}%";
            if (pct < 100)
            {
                lblProgressText.Text = $"กำลังเขียน Flash... ({pct}%)";
            }
        }

        private string GetSelectedPort()
        {
            string item = cboPorts.SelectedItem?.ToString() ?? "";
            if (string.IsNullOrEmpty(item) || item.Contains("ไม่พบพอร์ต")) return "";
            return item.Split(' ')[0];
        }

        private bool RunEngine(IEnumerable<string> arguments, Action<string> onLineOutput)
        {
            if (!File.Exists(esptoolExe))
            {
                AppendLog($"❌ Error: ไม่พบไฟล์เครื่องมือ {esptoolExe}");
                return false;
            }

            try
            {
                var psi = new ProcessStartInfo
                {
                    FileName = esptoolExe,
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    CreateNoWindow = true,
                    StandardOutputEncoding = Encoding.UTF8,
                    StandardErrorEncoding = Encoding.UTF8
                };

                psi.EnvironmentVariables["PYTHONIOENCODING"] = "utf-8";
                psi.EnvironmentVariables["PYTHONUTF8"] = "1";

                foreach (var arg in arguments) psi.ArgumentList.Add(arg);

                using (var proc = new Process { StartInfo = psi })
                {
                    proc.Start();

                    // Read character-by-character to parse \r in-place progress updates
                    var stream = proc.StandardOutput.BaseStream;
                    var buffer = new List<byte>();

                    int b;
                    while ((b = stream.ReadByte()) != -1)
                    {
                        if (b == '\r' || b == '\n')
                        {
                            if (buffer.Count > 0)
                            {
                                string line = Encoding.UTF8.GetString(buffer.ToArray()).Trim();
                                buffer.Clear();
                                if (!string.IsNullOrEmpty(line))
                                {
                                    onLineOutput(line);
                                }
                            }
                        }
                        else
                        {
                            buffer.Add((byte)b);
                        }
                    }

                    if (buffer.Count > 0)
                    {
                        string line = Encoding.UTF8.GetString(buffer.ToArray()).Trim();
                        if (!string.IsNullOrEmpty(line)) onLineOutput(line);
                    }

                    proc.WaitForExit();
                    return proc.ExitCode == 0;
                }
            }
            catch (Exception ex)
            {
                AppendLog($"❌ Exception running engine: {ex.Message}");
                return false;
            }
        }

        private List<PortInfo> GetDetailedSerialPorts()
        {
            var result = new List<PortInfo>();
            try
            {
                using (var searcher = new ManagementObjectSearcher("SELECT DeviceID, Description, PNPDeviceID FROM Win32_PnPEntity WHERE Caption LIKE '%(COM%'"))
                {
                    foreach (var obj in searcher.Get())
                    {
                        string desc = obj["Description"]?.ToString() ?? "";
                        string caption = obj["Caption"]?.ToString() ?? "";
                        string pnpId = obj["PNPDeviceID"]?.ToString() ?? "";

                        var match = Regex.Match(caption, @"\((COM\d+)\)");
                        if (match.Success)
                        {
                            string port = match.Groups[1].Value;
                            bool isBt = desc.Contains("Bluetooth", StringComparison.OrdinalIgnoreCase) || pnpId.Contains("BTHENUM", StringComparison.OrdinalIgnoreCase);
                            bool isEspVid = pnpId.Contains("VID_303A", StringComparison.OrdinalIgnoreCase) ||
                                            pnpId.Contains("VID_1A86", StringComparison.OrdinalIgnoreCase) ||
                                            pnpId.Contains("VID_10C4", StringComparison.OrdinalIgnoreCase) ||
                                            pnpId.Contains("VID_0403", StringComparison.OrdinalIgnoreCase);
                            bool isEspDesc = desc.Contains("USB Serial", StringComparison.OrdinalIgnoreCase) ||
                                             desc.Contains("CH340", StringComparison.OrdinalIgnoreCase) ||
                                             desc.Contains("CP210", StringComparison.OrdinalIgnoreCase) ||
                                             desc.Contains("FTDI", StringComparison.OrdinalIgnoreCase) ||
                                             desc.Contains("ESP32", StringComparison.OrdinalIgnoreCase);

                            result.Add(new PortInfo
                            {
                                PortName = port,
                                Description = desc,
                                PnpId = pnpId,
                                IsBluetooth = isBt,
                                IsEspLikely = (isEspVid || isEspDesc) && !isBt
                            });
                        }
                    }
                }
            }
            catch
            {
                // Fallback to standard SerialPort.GetPortNames
                foreach (var port in SerialPort.GetPortNames())
                {
                    result.Add(new PortInfo
                    {
                        PortName = port,
                        Description = "Serial Port",
                        PnpId = "",
                        IsBluetooth = false,
                        IsEspLikely = port != "COM1"
                    });
                }
            }
            return result;
        }

        public class PortInfo
        {
            public string PortName { get; set; } = "";
            public string Description { get; set; } = "";
            public string PnpId { get; set; } = "";
            public bool IsBluetooth { get; set; }
            public bool IsEspLikely { get; set; }
        }
    }
}
