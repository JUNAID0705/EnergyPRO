/*
 * ESP32 Energy Monitor with PZEM-004T and Web Dashboard
 * Features: PZEM-004T reading, I2C LCD display, Web dashboard with balance system
 * Author: ESP32 Energy Monitor System
 * Version: 1.0
 */

#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <PZEM004Tv30.h>

// WiFi Configuration
const char* ssid = "realme";
const char* password = "ONLYME@123";

// Hardware Configuration
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define LCD_SDA_PIN 21
#define LCD_SCL_PIN 22
#define LCD_ADDRESS 0x27

// EEPROM Addresses
#define EEPROM_SIZE 512
#define BALANCE_ADDR 0
#define COST_PER_KWH_ADDR 4
#define OV_THRESHOLD_ADDR 8
#define OC_THRESHOLD_ADDR 12
#define THEFT_THRESHOLD_ADDR 16
#define MIN_BALANCE_ADDR 20
#define TOTAL_ENERGY_ADDR 24

// Objects
PZEM004Tv30 pzem(Serial2, PZEM_RX_PIN, PZEM_TX_PIN);
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
WebServer server(80);

// Global Variables
float balance = 100.0;
float costPerKWh = 0.20;
float overVoltageThreshold = 260.0;
float overCurrentThreshold = 10.0;
float theftDetectionThreshold = 0.500;
float minimumBalanceThreshold = 0.00;
float totalEnergy = 0.0;
unsigned long lastEnergyUpdate = 0;
float lastPowerReading = 0.0;

// WiFi Management Variables
unsigned long lastWiFiCheck = 0;
unsigned long wifiReconnectInterval = 30000; // 30 seconds
int wifiReconnectAttempts = 0;
bool wifiConnected = false;

// PZEM Data Structure
struct PZEMData {
  float voltage = 0.0;
  float current = 0.0;
  float power = 0.0;
  float energy = 0.0;
  bool isValid = false;
};

PZEMData currentReading;

// Web Dashboard HTML/CSS/JS
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>EnergyPro - Energy Monitor</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
        }
        
        .sidebar {
            width: 250px;
            background: #2c3e50;
            color: white;
            position: fixed;
            height: 100vh;
            overflow-y: auto;
        }
        
        .logo {
            padding: 20px;
            text-align: center;
            background: #34495e;
            border-bottom: 1px solid #3a526b;
        }
        
        .logo h2 {
            color: #3498db;
            font-size: 24px;
        }
        
        .nav-menu {
            list-style: none;
            padding: 20px 0;
        }
        
        .nav-item {
            padding: 15px 25px;
            cursor: pointer;
            transition: background 0.3s;
            border-left: 3px solid transparent;
        }
        
        .nav-item:hover, .nav-item.active {
            background: #3a526b;
            border-left-color: #3498db;
        }
        
        .nav-item i {
            margin-right: 10px;
            width: 20px;
        }
        
        .main-content {
            flex: 1;
            margin-left: 250px;
            padding: 20px;
        }
        
        .header {
            background: white;
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 20px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        
        .header h1 {
            color: #2c3e50;
            margin-bottom: 5px;
        }
        
        .header p {
            color: #7f8c8d;
        }
        
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        
        .stat-card {
            background: white;
            padding: 25px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            text-align: center;
            transition: transform 0.3s;
        }
        
        .stat-card:hover {
            transform: translateY(-5px);
        }
        
        .stat-icon {
            width: 60px;
            height: 60px;
            margin: 0 auto 15px;
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 24px;
            color: white;
        }
        
        .balance .stat-icon { background: #3498db; }
        .voltage .stat-icon { background: #e74c3c; }
        .current .stat-icon { background: #f39c12; }
        .power .stat-icon { background: #27ae60; }
        
        .stat-value {
            font-size: 28px;
            font-weight: bold;
            color: #2c3e50;
            margin-bottom: 5px;
        }
        
        .stat-label {
            color: #7f8c8d;
            font-size: 14px;
        }
        
        .status-panel {
            background: white;
            padding: 25px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            margin-bottom: 20px;
        }
        
        .status-panel h3 {
            color: #2c3e50;
            margin-bottom: 20px;
        }
        
        .status-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 10px 0;
            border-bottom: 1px solid #ecf0f1;
        }
        
        .status-item:last-child {
            border-bottom: none;
        }
        
        .status-indicator {
            padding: 5px 15px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: bold;
        }
        
        .status-ok {
            background: #d5f4e6;
            color: #27ae60;
        }
        
        .status-warning {
            background: #fef5e7;
            color: #f39c12;
        }
        
        .status-error {
            background: #fadbd8;
            color: #e74c3c;
        }
        
        .page {
            display: none;
        }
        
        .page.active {
            display: block;
        }
        
        .form-group {
            margin-bottom: 20px;
        }
        
        .form-group label {
            display: block;
            margin-bottom: 5px;
            color: #2c3e50;
            font-weight: bold;
        }
        
        .form-group input {
            width: 100%;
            padding: 12px;
            border: 2px solid #ecf0f1;
            border-radius: 5px;
            font-size: 16px;
        }
        
        .form-group input:focus {
            outline: none;
            border-color: #3498db;
        }
        
        .btn {
            background: #3498db;
            color: white;
            padding: 12px 25px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 16px;
            transition: background 0.3s;
        }
        
        .btn:hover {
            background: #2980b9;
        }
        
        .btn-success {
            background: #27ae60;
        }
        
        .btn-success:hover {
            background: #219a52;
        }
        
        .recharge-section {
            background: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            max-width: 500px;
        }
        
        .balance-display {
            text-align: center;
            margin-bottom: 30px;
            padding: 20px;
            background: #f8f9fa;
            border-radius: 10px;
        }
        
        .balance-amount {
            font-size: 36px;
            font-weight: bold;
            color: #3498db;
            margin-bottom: 5px;
        }
        
        .energy-chart {
            height: 300px;
            background: white;
            border-radius: 10px;
            padding: 20px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            display: flex;
            align-items: center;
            justify-content: center;
            color: #7f8c8d;
        }
    </style>
</head>
<body>
    <nav class="sidebar">
        <div class="logo">
            <h2>⚡ EnergyPro</h2>
        </div>
        <ul class="nav-menu">
            <li class="nav-item active" onclick="showPage('dashboard')">
                <i>📊</i> Dashboard
            </li>
            <li class="nav-item" onclick="showPage('account')">
                <i>💰</i> Account
            </li>
            <li class="nav-item" onclick="showPage('settings')">
                <i>⚙</i> Settings
            </li>
            <li class="nav-item" onclick="showPage('reports')">
                <i>📈</i> Reports
            </li>
            <li class="nav-item" onclick="showPage('help')">
                <i>❓</i> Help
            </li>
        </ul>
    </nav>
    
    <main class="main-content">
        <!-- Dashboard Page -->
        <div id="dashboard" class="page active">
            <div class="header">
                <h1>Energy Dashboard</h1>
                <p>Real-time monitoring of your prepaid energy system</p>
                <small>Last updated: <span id="lastUpdate">--:--:-- AM</span></small>
            </div>
            
            <div class="stats-grid">
                <div class="stat-card balance">
                    <div class="stat-icon">₹</div>
                    <div class="stat-value" id="balanceValue">100.00</div>
                    <div class="stat-label">Available Credit</div>
                </div>
                <div class="stat-card voltage">
                    <div class="stat-icon">⚡</div>
                    <div class="stat-value" id="voltageValue">225.3</div>
                    <div class="stat-label">Line Voltage</div>
                </div>
                <div class="stat-card current">
                    <div class="stat-icon">🔌</div>
                    <div class="stat-value" id="currentValue">0.000</div>
                    <div class="stat-label">Line Current</div>
                </div>
                <div class="stat-card power">
                    <div class="stat-icon">⚙</div>
                    <div class="stat-value" id="powerValue">0.0</div>
                    <div class="stat-label">Active Power</div>
                </div>
            </div>
            
            <div class="status-panel">
                <h3>System Status</h3>
                <div class="status-item">
                    <span>Power</span>
                    <span class="status-indicator status-ok" id="powerStatus">ON</span>
                </div>
                <div class="status-item">
                    <span>Fault Status</span>
                    <span class="status-indicator status-ok" id="faultStatus">OK</span>
                </div>
                <div class="status-item">
                    <span>Theft Detection</span>
                    <span class="status-indicator status-ok" id="theftStatus">OK</span>
                </div>
                <div class="status-item">
                    <span>Total Energy</span>
                    <span id="totalEnergyValue">0.000 kWh</span>
                </div>
            </div>
        </div>
        
        <!-- Account Page -->
        <div id="account" class="page">
            <div class="header">
                <h1>Account Management</h1>
                <p>Manage your prepaid account balance and transactions</p>
            </div>
            
            <div class="recharge-section">
                <div class="balance-display">
                    <div class="balance-amount" id="accountBalance">150.00₹</div>
                    <div class="stat-label">Available Credit</div>
                </div>
                
                <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-bottom: 30px;">
                    <div style="text-align: center;">
                        <h4 style="color: #2c3e50; margin-bottom: 10px;">Consumption Rate</h4>
                        <div style="font-size: 20px; color: #3498db;" id="consumptionRate">--₹/day</div>
                        <small style="color: #7f8c8d;">Average daily consumption</small>
                    </div>
                </div>
                
                <h4 style="color: #2c3e50; margin-bottom: 15px;">Recharge Account</h4>
                <div class="form-group">
                    <label for="rechargeAmount">Recharge Amount</label>
                    <input type="number" id="rechargeAmount" value="100" min="1" max="10000">
                </div>
                <button class="btn btn-success" onclick="processRecharge()">Process Recharge</button>
            </div>
        </div>
        
        <!-- Settings Page -->
        <div id="settings" class="page">
            <div class="header">
                <h1>System Settings</h1>
                <p>Configure thresholds and system parameters</p>
            </div>
            
            <div class="status-panel">
                <h3>⚠ Safety Thresholds</h3>
                <div class="form-group">
                    <label for="overVoltage">Over Voltage Threshold</label>
                    <input type="number" id="overVoltage" step="0.1" value="260">
                </div>
                <div class="form-group">
                    <label for="overCurrent">Over Current Threshold</label>
                    <input type="number" id="overCurrent" step="0.01" value="10.00">
                </div>
            </div>
            
            <div class="status-panel">
                <h3>🔒 Security Settings</h3>
                <div class="form-group">
                    <label for="theftThreshold">Theft Detection Threshold</label>
                    <input type="number" id="theftThreshold" step="0.001" value="0.500">
                </div>
            </div>
            
            <div class="status-panel">
                <h3>💳 Billing Settings</h3>
                <div class="form-group">
                    <label for="minBalance">Minimum Balance Threshold</label>
                    <input type="number" id="minBalance" step="0.01" value="0.00">
                </div>
                <div class="form-group">
                    <label for="costKwh">Cost per kWh</label>
                    <input type="number" id="costKwh" step="0.01" value="0.20">
                </div>
            </div>
            
            <button class="btn" onclick="saveSettings()">Save All Settings</button>
        </div>
        
        <!-- Reports Page -->
        <div id="reports" class="page">
            <div class="header">
                <h1>Reports & Analytics</h1>
                <p>View energy consumption reports and analytics</p>
            </div>
            
            <div class="status-panel">
                <h3>Energy Consumption</h3>
                <div class="energy-chart">
                    <div>📊 Consumption reports and charts will be displayed here</div>
                </div>
            </div>
        </div>
        
        <!-- Help Page -->
        <div id="help" class="page">
            <div class="header">
                <h1>Help & Support</h1>
                <p>Get help with your energy monitoring system</p>
            </div>
            
            <div class="status-panel">
                <h3>System Information</h3>
                <div class="status-item">
                    <span>Device Model</span>
                    <span>ESP32 Energy Monitor</span>
                </div>
                <div class="status-item">
                    <span>Firmware Version</span>
                    <span>1.0.0</span>
                </div>
                <div class="status-item">
                    <span>IP Address</span>
                    <span id="deviceIP">192.168.1.100</span>
                </div>
                <div class="status-item">
                    <span>Uptime</span>
                    <span id="systemUptime">--</span>
                </div>
            </div>
        </div>
    </main>
    
    <script>
        let currentPage = 'dashboard';
        
        function showPage(pageId) {
            // Hide all pages
            document.querySelectorAll('.page').forEach(page => {
                page.classList.remove('active');
            });
            
            // Remove active class from all nav items
            document.querySelectorAll('.nav-item').forEach(item => {
                item.classList.remove('active');
            });
            
            // Show selected page
            document.getElementById(pageId).classList.add('active');
            
            // Add active class to clicked nav item
            event.target.classList.add('active');
            
            currentPage = pageId;
        }
        
        function updateDashboard() {
            fetch('/api/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('balanceValue').textContent = data.balance.toFixed(2);
                    document.getElementById('voltageValue').textContent = data.voltage.toFixed(1);
                    document.getElementById('currentValue').textContent = data.current.toFixed(3);
                    document.getElementById('powerValue').textContent = data.power.toFixed(1);
                    document.getElementById('totalEnergyValue').textContent = data.totalEnergy.toFixed(3) + ' kWh';
                    document.getElementById('accountBalance').textContent = data.balance.toFixed(2) + '₹';
                    
                    // Update status indicators
                    document.getElementById('powerStatus').textContent = data.power > 0 ? 'ON' : 'OFF';
                    document.getElementById('powerStatus').className = 'status-indicator ' + (data.power > 0 ? 'status-ok' : 'status-warning');
                    
                    document.getElementById('faultStatus').textContent = (data.voltage > 260 || data.current > 10) ? 'FAULT' : 'OK';
                    document.getElementById('faultStatus').className = 'status-indicator ' + ((data.voltage > 260 || data.current > 10) ? 'status-error' : 'status-ok');
                    
                    document.getElementById('theftStatus').textContent = data.current > 0.5 ? 'DETECTED' : 'OK';
                    document.getElementById('theftStatus').className = 'status-indicator ' + (data.current > 0.5 ? 'status-warning' : 'status-ok');
                    
                    // Update timestamp
                    const now = new Date();
                    document.getElementById('lastUpdate').textContent = now.toLocaleTimeString();
                })
                .catch(error => console.error('Error fetching data:', error));
        }
        
        function loadSettings() {
            fetch('/api/settings')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('overVoltage').value = data.overVoltageThreshold;
                    document.getElementById('overCurrent').value = data.overCurrentThreshold;
                    document.getElementById('theftThreshold').value = data.theftDetectionThreshold;
                    document.getElementById('minBalance').value = data.minimumBalanceThreshold;
                    document.getElementById('costKwh').value = data.costPerKWh;
                });
        }
        
        function saveSettings() {
            const settings = {
                overVoltageThreshold: parseFloat(document.getElementById('overVoltage').value),
                overCurrentThreshold: parseFloat(document.getElementById('overCurrent').value),
                theftDetectionThreshold: parseFloat(document.getElementById('theftThreshold').value),
                minimumBalanceThreshold: parseFloat(document.getElementById('minBalance').value),
                costPerKWh: parseFloat(document.getElementById('costKwh').value)
            };
            
            fetch('/api/settings', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(settings)
            })
            .then(response => response.json())
            .then(data => {
                alert('Settings saved successfully!');
            });
        }
        
        function processRecharge() {
            const amount = parseFloat(document.getElementById('rechargeAmount').value);
            
            fetch('/api/recharge', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({amount: amount})
            })
            .then(response => response.json())
            .then(data => {
                alert('Recharge processed successfully!');
                updateDashboard();
            });
        }
        
        // Initialize
        document.addEventListener('DOMContentLoaded', function() {
            updateDashboard();
            loadSettings();
            setInterval(updateDashboard, 2000); // Update every 2 seconds
        });
        
        // Set device IP
        document.getElementById('deviceIP').textContent = window.location.hostname;
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  loadFromEEPROM();
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("EnergyPro v1.0");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
  
  // Configure WiFi for stability
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  
  // Disable power saving mode for better stability
  WiFi.setSleep(false);
  
  // Initialize WiFi with improved connection handling
  connectToWiFi();
  
  // Setup web server routes
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlPage);
  });
  
  server.on("/api/data", HTTP_GET, handleAPIData);
  server.on("/api/settings", HTTP_GET, handleAPISettingsGet);
  server.on("/api/settings", HTTP_POST, handleAPISettingsPost);
  server.on("/api/recharge", HTTP_POST, handleAPIRecharge);
  
  server.begin();
  Serial.println("HTTP server started");
  
  lastEnergyUpdate = millis();
  lastWiFiCheck = millis();
}

void loop() {
  // Check and maintain WiFi connection
  checkWiFiConnection();
  
  // Handle web server only if WiFi is connected
  if (wifiConnected) {
    server.handleClient();
  }
  
  // Read PZEM data
  readPZEMData();
  
  // Update LCD display
  updateLCDDisplay();
  
  // Update energy consumption and balance
  updateEnergyAndBalance();
  
  // Save to EEPROM every 10 seconds
  static unsigned long lastSave = 0;
  if (millis() - lastSave > 10000) {
    saveToEEPROM();
    lastSave = millis();
  }
  
  delay(1000);
}

void readPZEMData() {
  currentReading.voltage = pzem.voltage();
  currentReading.current = pzem.current();
  currentReading.power = pzem.power();
  currentReading.energy = pzem.energy();
  
  // Check if readings are valid
  currentReading.isValid = !isnan(currentReading.voltage) && 
                          !isnan(currentReading.current) && 
                          !isnan(currentReading.power);
  
  if (!currentReading.isValid) {
    currentReading.voltage = 0.0;
    currentReading.current = 0.0;
    currentReading.power = 0.0;
    currentReading.energy = 0.0;
  }
}

void updateLCDDisplay() {
  static unsigned long lastLCDUpdate = 0;
  static int displayMode = 0;
  
  if (millis() - lastLCDUpdate > 3000) { // Change display every 3 seconds
    lcd.clear();
    
    switch (displayMode) {
      case 0: // Voltage and Current
        lcd.setCursor(0, 0);
        lcd.print("V:");
        lcd.print(currentReading.voltage, 1);
        lcd.print("V");
        lcd.setCursor(8, 0);
        lcd.print("I:");
        lcd.print(currentReading.current, 2);
        lcd.print("A");
        lcd.setCursor(0, 1);
        lcd.print("P:");
        lcd.print(currentReading.power, 1);
        lcd.print("W");
        break;
        
      case 1: // Balance and Energy
        lcd.setCursor(0, 0);
        lcd.print("Balance:");
        lcd.print(balance, 2);
        lcd.setCursor(0, 1);
        lcd.print("Energy:");
        lcd.print(totalEnergy, 3);
        lcd.print("kWh");
        break;
        
      case 2: // Status
        lcd.setCursor(0, 0);
        lcd.print("Status: ");
        if (balance > 0 && currentReading.isValid) {
          lcd.print("ACTIVE");
        } else if (balance <= 0) {
          lcd.print("NO BAL");
        } else {
          lcd.print("ERROR");
        }
        lcd.setCursor(0, 1);
        lcd.print("IP:");
        lcd.print(WiFi.localIP());
        break;
    }
    
    displayMode = (displayMode + 1) % 3;
    lastLCDUpdate = millis();
  }
}

void updateEnergyAndBalance() {
  if (balance > 0 && currentReading.isValid && currentReading.power > 0) {
    unsigned long currentTime = millis();
    float timeDiffHours = (currentTime - lastEnergyUpdate) / 3600000.0; // Convert to hours
    
    if (timeDiffHours > 0) {
      float energyConsumed = (currentReading.power / 1000.0) * timeDiffHours; // kWh
      totalEnergy += energyConsumed;
      
      float cost = energyConsumed * costPerKWh;
      balance -= cost;
      
      if (balance < 0) balance = 0;
      
      lastEnergyUpdate = currentTime;
    }
  }
}

void handleAPIData() {
  StaticJsonDocument<500> doc;
  
  doc["balance"] = balance;
  doc["voltage"] = currentReading.voltage;
  doc["current"] = currentReading.current;
  doc["power"] = currentReading.power;
  doc["totalEnergy"] = totalEnergy;
  doc["timestamp"] = millis();
  doc["isValid"] = currentReading.isValid;
  
  String response;
  serializeJson(doc, response);
  
  server.send(200, "application/json", response);
}

void handleAPISettingsGet() {
  StaticJsonDocument<300> doc;
  
  doc["overVoltageThreshold"] = overVoltageThreshold;
  doc["overCurrentThreshold"] = overCurrentThreshold;
  doc["theftDetectionThreshold"] = theftDetectionThreshold;
  doc["minimumBalanceThreshold"] = minimumBalanceThreshold;
  doc["costPerKWh"] = costPerKWh;
  
  String response;
  serializeJson(doc, response);
  
  server.send(200, "application/json", response);
}

void handleAPISettingsPost() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    StaticJsonDocument<300> doc;
    
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    
    // Update settings
    if (doc.containsKey("overVoltageThreshold")) {
      overVoltageThreshold = doc["overVoltageThreshold"];
    }
    if (doc.containsKey("overCurrentThreshold")) {
      overCurrentThreshold = doc["overCurrentThreshold"];
    }
    if (doc.containsKey("theftDetectionThreshold")) {
      theftDetectionThreshold = doc["theftDetectionThreshold"];
    }
    if (doc.containsKey("minimumBalanceThreshold")) {
      minimumBalanceThreshold = doc["minimumBalanceThreshold"];
    }
    if (doc.containsKey("costPerKWh")) {
      costPerKWh = doc["costPerKWh"];
    }
    
    // Save to EEPROM
    saveToEEPROM();
    
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"No data received\"}");
  }
}

void handleAPIRecharge() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    StaticJsonDocument<100> doc;
    
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    
    if (doc.containsKey("amount")) {
      float amount = doc["amount"];
      if (amount > 0 && amount <= 10000) {
        balance += amount;
        saveToEEPROM();
        
        StaticJsonDocument<100> response;
        response["success"] = true;
        response["newBalance"] = balance;
        
        String responseStr;
        serializeJson(response, responseStr);
        server.send(200, "application/json", responseStr);
        
        // Update LCD to show recharge
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("RECHARGED!");
        lcd.setCursor(0, 1);
        lcd.print("+" + String(amount, 2));
        delay(2000);
      } else {
        server.send(400, "application/json", "{\"error\":\"Invalid amount\"}");
      }
    } else {
      server.send(400, "application/json", "{\"error\":\"Amount not specified\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"No data received\"}");
  }
}

void saveToEEPROM() {
  EEPROM.writeFloat(BALANCE_ADDR, balance);
  EEPROM.writeFloat(COST_PER_KWH_ADDR, costPerKWh);
  EEPROM.writeFloat(OV_THRESHOLD_ADDR, overVoltageThreshold);
  EEPROM.writeFloat(OC_THRESHOLD_ADDR, overCurrentThreshold);
  EEPROM.writeFloat(THEFT_THRESHOLD_ADDR, theftDetectionThreshold);
  EEPROM.writeFloat(MIN_BALANCE_ADDR, minimumBalanceThreshold);
  EEPROM.writeFloat(TOTAL_ENERGY_ADDR, totalEnergy);
  EEPROM.commit();
  
  Serial.println("Data saved to EEPROM");
}

void loadFromEEPROM() {
  // Check if EEPROM has been initialized
  float testValue = EEPROM.readFloat(BALANCE_ADDR);
  if (isnan(testValue) || testValue < 0 || testValue > 100000) {
    // First time setup - initialize with default values
    Serial.println("Initializing EEPROM with default values");
    saveToEEPROM();
  } else {
    // Load existing values
    balance = EEPROM.readFloat(BALANCE_ADDR);
    costPerKWh = EEPROM.readFloat(COST_PER_KWH_ADDR);
    overVoltageThreshold = EEPROM.readFloat(OV_THRESHOLD_ADDR);
    overCurrentThreshold = EEPROM.readFloat(OC_THRESHOLD_ADDR);
    theftDetectionThreshold = EEPROM.readFloat(THEFT_THRESHOLD_ADDR);
    minimumBalanceThreshold = EEPROM.readFloat(MIN_BALANCE_ADDR);
    totalEnergy = EEPROM.readFloat(TOTAL_ENERGY_ADDR);
    
    Serial.println("Data loaded from EEPROM");
    Serial.println("Balance: " + String(balance));
    Serial.println("Cost per kWh: " + String(costPerKWh));
  }
}

// Additional utility functions
void resetSystem() {
  // Reset all values to defaults
  balance = 100.0;
  costPerKWh = 0.20;
  overVoltageThreshold = 260.0;
  overCurrentThreshold = 10.0;
  theftDetectionThreshold = 0.500;
  minimumBalanceThreshold = 0.00;
  totalEnergy = 0.0;
  
  saveToEEPROM();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SYSTEM RESET");
  lcd.setCursor(0, 1);
  lcd.print("COMPLETE");
  delay(2000);
}

void checkSystemAlerts() {
  static unsigned long lastAlertCheck = 0;
  
  if (millis() - lastAlertCheck > 5000) { // Check every 5 seconds
    bool alertActive = false;
    
    // Check over voltage
    if (currentReading.voltage > overVoltageThreshold) {
      Serial.println("ALERT: Over Voltage - " + String(currentReading.voltage) + "V");
      alertActive = true;
    }
    
    // Check over current
    if (currentReading.current > overCurrentThreshold) {
      Serial.println("ALERT: Over Current - " + String(currentReading.current) + "A");
      alertActive = true;
    }
    
    // Check theft detection
    if (currentReading.current > theftDetectionThreshold && currentReading.power < 10) {
      Serial.println("ALERT: Possible Theft Detected");
      alertActive = true;
    }
    
    // Check low balance
    if (balance <= minimumBalanceThreshold) {
      Serial.println("ALERT: Low Balance - ₹" + String(balance));
      alertActive = true;
    }
    
    // If any alert is active, briefly show on LCD
    if (alertActive) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("!!! ALERT !!!");
      lcd.setCursor(0, 1);
      if (balance <= minimumBalanceThreshold) {
        lcd.print("LOW BALANCE");
      } else if (currentReading.voltage > overVoltageThreshold) {
        lcd.print("HIGH VOLTAGE");
      } else if (currentReading.current > overCurrentThreshold) {
        lcd.print("HIGH CURRENT");
      } else {
        lcd.print("CHECK SYSTEM");
      }
      delay(1000);
    }
    
    lastAlertCheck = millis();
  }
}

// Function to handle PZEM communication errors
void handlePZEMErrors() {
  static int errorCount = 0;
  static unsigned long lastErrorReset = 0;
  
  if (!currentReading.isValid) {
    errorCount++;
    
    if (errorCount > 5) { // If 5 consecutive errors
      Serial.println("PZEM Communication Error - Attempting Reset");
      
      // Try to reset PZEM communication
      Serial2.end();
      delay(100);
      Serial2.begin(9600, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);
      delay(100);
      
      errorCount = 0;
      lastErrorReset = millis();
    }
  } else {
    errorCount = 0;
  }
  
  // Show communication status on LCD occasionally
  if (millis() - lastErrorReset < 3000 && errorCount == 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PZEM RECONNECTED");
    lcd.setCursor(0, 1);
    lcd.print("System Normal");
    delay(1000);
  }
}

// WiFi Connection Functions
void connectToWiFi() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  
  Serial.println("Connecting to WiFi...");
  Serial.println("SSID: " + String(ssid));
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) { // 30 second timeout
    delay(1000);
    attempts++;
    
    Serial.print(".");
    lcd.setCursor(0, 1);
    lcd.print("Attempt: " + String(attempts));
    
    // Show connecting animation
    static int animFrame = 0;
    lcd.setCursor(10 + (animFrame % 6), 1);
    lcd.print(".");
    animFrame++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    wifiReconnectAttempts = 0;
    
    Serial.println("\nWiFi Connected Successfully!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength (RSSI): ");
    Serial.println(WiFi.RSSI());
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(3000);
  } else {
    wifiConnected = false;
    wifiReconnectAttempts++;
    
    Serial.println("\nFailed to connect to WiFi!");
    Serial.println("Status: " + String(WiFi.status()));
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed");
    lcd.setCursor(0, 1);
    lcd.print("Check Settings");
    delay(3000);
  }
}

void checkWiFiConnection() {
  if (millis() - lastWiFiCheck > 5000) { // Check every 5 seconds
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiConnected) {
        Serial.println("WiFi connection lost! Attempting reconnection...");
        wifiConnected = false;
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("WiFi Disconnected");
        lcd.setCursor(0, 1);
        lcd.print("Reconnecting...");
      }
      
      // Attempt reconnection every 30 seconds
      if (millis() - lastWiFiCheck > wifiReconnectInterval) {
        Serial.println("Attempting WiFi reconnection...");
        WiFi.disconnect();
        delay(1000);
        WiFi.begin(ssid, password);
        
        // Wait up to 15 seconds for connection
        int timeout = 0;
        while (WiFi.status() != WL_CONNECTED && timeout < 15) {
          delay(1000);
          timeout++;
          Serial.print(".");
        }
        
        if (WiFi.status() == WL_CONNECTED) {
          wifiConnected = true;
          wifiReconnectAttempts = 0;
          Serial.println("\nWiFi Reconnected!");
          Serial.println("IP: " + WiFi.localIP().toString());
          
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("WiFi Reconnected");
          lcd.setCursor(0, 1);
          lcd.print(WiFi.localIP());
          delay(2000);
        } else {
          wifiReconnectAttempts++;
          Serial.println("\nReconnection failed. Attempt: " + String(wifiReconnectAttempts));
          
          // Increase retry interval after multiple failures
          if (wifiReconnectAttempts > 5) {
            wifiReconnectInterval = 60000; // 1 minute
          }
        }
      }
    } else {
      if (!wifiConnected) {
        wifiConnected = true;
        wifiReconnectAttempts = 0;
        wifiReconnectInterval = 30000; // Reset to 30 seconds
        Serial.println("WiFi connection restored!");
      }
    }
    lastWiFiCheck = millis();
  }
}

// Enhanced loop function with better error handling
void enhancedLoop() {
  // Check and maintain WiFi connection
  checkWiFiConnection();
  
  // Handle web server only if WiFi is connected
  if (wifiConnected) {
    server.handleClient();
  }
  
  // Read PZEM data with error handling
  readPZEMData();
  handlePZEMErrors();
  
  // Check for system alerts
  checkSystemAlerts();
  
  // Update LCD display
  updateLCDDisplay();
  
  // Update energy consumption and balance
  updateEnergyAndBalance();
  
  // Save to EEPROM periodically
  static unsigned long lastSave = 0;
  if (millis() - lastSave > 30000) { // Save every 30 seconds
    saveToEEPROM();
    lastSave = millis();
  }
  
  // Watchdog timer simulation
  static unsigned long lastWatchdog = 0;
  if (millis() - lastWatchdog > 60000) { // Reset every minute
    Serial.println("System Status: OK - Uptime: " + String(millis()/1000) + "s");
    Serial.println("WiFi Status: " + String(wifiConnected ? "Connected" : "Disconnected"));
    Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    lastWatchdog = millis();
  }
  
  delay(1000);
}