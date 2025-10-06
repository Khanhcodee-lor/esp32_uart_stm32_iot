#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

Preferences preferences;
HardwareSerial MySerial(2); // UART2: RX=16, TX=17
AsyncWebServer server(80);


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Cấu hình WiFi - ESP32</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600&display=swap" rel="stylesheet">
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css">
    <style>
        :root {
            --primary: #6366f1;
            --primary-dark: #4f46e5;
            --success: #10b981;
            --error: #ef4444;
            --dark: #0f172a;
            --darker: #020617;
            --light: #f8fafc;
            --gray: #94a3b8;
        }
        
        body {
            font-family: 'Inter', sans-serif;
            background: var(--darker);
            color: var(--light);
            min-height: 100vh;
            display: grid;
            place-items: center;
            margin: 0;
            padding: 1rem;
        }
        
        .card {
            background: var(--dark);
            border-radius: 1rem;
            padding: 2.5rem;
            width: 100%;
            max-width: 400px;
            box-shadow: 0 10px 25px rgba(0, 0, 0, 0.3);
        }
        
        .header {
            text-align: center;
            margin-bottom: 2rem;
        }
        
        .header i {
            font-size: 3rem;
            color: var(--primary);
            margin-bottom: 1rem;
        }
        
        .header h1 {
            font-size: 1.5rem;
            font-weight: 600;
            margin: 0.5rem 0;
        }
        
        .header p {
            color: var(--gray);
            margin: 0;
            font-size: 0.9rem;
        }
        
        .form-group {
            margin-bottom: 1.5rem;
        }
        
        label {
            display: block;
            margin-bottom: 0.5rem;
            font-weight: 500;
            font-size: 0.95rem;
        }
        
        .input-container {
            position: relative;
            display: flex;
            align-items: center;
        }
        
        input {
            width: 100%;
            padding: 0.75rem 1rem 0.75rem 3rem;
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 0.5rem;
            color: var(--light);
            font-size: 1rem;
            transition: all 0.2s;
        }
        
        input:focus {
            outline: none;
            border-color: var(--primary);
            box-shadow: 0 0 0 2px rgba(99, 102, 241, 0.2);
        }
        
        .input-icon {
            position: absolute;
            left: 1rem;
            color: var(--gray);
            font-size: 1rem;
        }
        
        .password-toggle {
            position: absolute;
            right: 1rem;
            color: var(--gray);
            cursor: pointer;
            font-size: 1rem;
        }
        
        button {
            width: 100%;
            padding: 0.85rem;
            background: var(--primary);
            color: white;
            border: none;
            border-radius: 0.5rem;
            font-weight: 500;
            font-size: 1rem;
            cursor: pointer;
            transition: all 0.2s;
            margin-top: 0.5rem;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 0.5rem;
        }
        
        button:hover {
            background: var(--primary-dark);
        }
        
        .status {
            text-align: center;
            margin: 1.5rem 0 0;
            padding: 1rem;
            border-radius: 0.5rem;
            display: none;
            font-size: 0.95rem;
        }
        
        .success {
            background: rgba(16, 185, 129, 0.1);
            color: var(--success);
            border-left: 3px solid var(--success);
        }
        
        .error {
            background: rgba(239, 68, 68, 0.1);
            color: var(--error);
            border-left: 3px solid var(--error);
        }
        
        .loading {
            display: none;
            text-align: center;
            margin: 1.5rem 0;
        }
        
        .spinner {
            width: 2rem;
            height: 2rem;
            border: 3px solid rgba(255, 255, 255, 0.1);
            border-top-color: var(--primary);
            border-radius: 50%;
            animation: spin 1s linear infinite;
            margin: 0 auto 1rem;
        }
        
        @keyframes spin {
            to { transform: rotate(360deg); }
        }
    </style>
</head>
<body>
    <div class="card">
        <div class="header">
            <i class="fas fa-wifi"></i>
            <h1>ESP32 WiFi Config</h1>
            <p>Kết nối thiết bị với mạng WiFi của bạn</p>
        </div>
        
        <form id="wifiForm">
            <div class="form-group">
                <label for="ssid">Tên mạng WiFi</label>
                <div class="input-container">
                    <i class="fas fa-wifi input-icon"></i>
                    <input type="text" id="ssid" placeholder="Nhập tên WiFi" required>
                </div>
            </div>
            
            <div class="form-group">
                <label for="pass">Mật khẩu WiFi</label>
                <div class="input-container">
                    <i class="fas fa-lock input-icon"></i>
                    <input type="password" id="pass" placeholder="Nhập mật khẩu" required>
                    <i class="fas fa-eye password-toggle" id="togglePassword"></i>
                </div>
            </div>
            
            <button type="submit">
                <i class="fas fa-plug"></i>
                <span>Kết nối</span>
            </button>
        </form>
        
        <div class="loading" id="loading">
            <div class="spinner"></div>
            <p>Đang kết nối...</p>
        </div>
        
        <div class="status" id="status"></div>
    </div>

    <script>
        const form = document.getElementById('wifiForm');
        const loading = document.getElementById('loading');
        const status = document.getElementById('status');
        
        form.addEventListener('submit', async (e) => {
            e.preventDefault();
            
            loading.style.display = 'block';
            status.style.display = 'none';
            
            try {
                const response = await fetch('/save', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: `ssid=${encodeURIComponent(form.ssid.value)}&pass=${encodeURIComponent(form.pass.value)}`
                });
                
                if (!response.ok) throw new Error('Kết nối thất bại');
                
                const data = await response.text();
                showStatus(data, 'success');
            } catch (err) {
                showStatus(err.message || 'Lỗi kết nối với thiết bị', 'error');
            } finally {
                loading.style.display = 'none';
            }
        });
        
        function showStatus(message, type) {
            status.innerHTML = `<i class="fas fa-${type === 'success' ? 'check-circle' : 'exclamation-circle'}"></i> ${message}`;
            status.className = `status ${type}`;
            status.style.display = 'block';
        }
        
        document.getElementById('togglePassword').addEventListener('click', function() {
            const pass = document.getElementById('pass');
            const isPassword = pass.type === 'password';
            pass.type = isPassword ? 'text' : 'password';
            this.classList.toggle('fa-eye-slash', isPassword);
            this.classList.toggle('fa-eye', !isPassword);
        });
    </script>
</body>
</html>
  )rawliteral";

const char *apSSID = "ESP32_Setup";
const char *apPassword = "12345678";

String wifiSSID = "";
String wifiPass = "";

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Biến để lưu trữ dữ liệu
int rainDigital = 0;
int rainAnalog = 0;
float rainVolt = 0.0;
int rainPercent = 0;
bool isRaining = false;
float humidity = 0.0;
float temperature = 0.0;
float distance = 0.0;

unsigned long dataMillis = 0;

void startAPMode(){
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPassword);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
}

void initFirebase() {
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;
  
  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096, 1024);
  
  Serial.println("Initializing Firebase...");
  Firebase.begin(&config, &auth);
  Serial.println("Firebase initialized");
}

bool loadWiFiCredentials(String& ssid, String& password) {
  preferences.begin("wifi-config", true);
  bool configured = preferences.getBool("configured", false);
  if (configured) {
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
  }
  preferences.end();
  
  if (configured && ssid != "") {
    Serial.println("✓ Đã tải thông tin WiFi từ bộ nhớ");
    Serial.println("SSID: " + ssid);
    return true;
  }
  return false;
}

void initWiFi() {
  // Tải thông tin WiFi đã lưu
  if (loadWiFiCredentials(wifiSSID, wifiPass)) {
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    
    Serial.println("Đang kết nối WiFi với thông tin đã lưu...");
    Serial.println("SSID: " + wifiSSID);
    
    WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());
    
    int timeout = 20;
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
      Serial.print(".");
      delay(500);
      timeout--;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✓ Kết nối WiFi thành công!");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      
      // Khởi tạo Firebase
      initFirebase();
      return; // Thoát hàm nếu kết nối thành công
    } else {
      Serial.println("\n✗ Kết nối WiFi thất bại!");
    }
  }
  
  // Nếu không có thông tin đã lưu hoặc kết nối thất bại
  Serial.println("Khởi động chế độ AP để cấu hình...");
  startAPMode();
}

void sendToFirebase() {
  if (Firebase.ready()) {

    
    // DEBUG: In giá trị trước khi gửi
    Serial.println("=== DEBUG VALUES ===");
    Serial.print("rainDigital: "); Serial.println(rainDigital);
    Serial.print("rainAnalog: "); Serial.println(rainAnalog);
    Serial.print("rainVolt: "); Serial.println(rainVolt);
    Serial.print("rainPercent: "); Serial.println(rainPercent);
    Serial.print("isRaining: "); Serial.println(isRaining);
    Serial.println("====================");
    
    json.set("rain/digital", rainDigital);
    json.set("rain/analog", rainAnalog);
    json.set("rain/volt", rainVolt);
    json.set("rain/percent", rainPercent);
    json.set("rain/raining", isRaining ? 1 : 0); // QUAN TRỌNG: Chuyển bool thành int
    json.set("dht/humidity", humidity);
    json.set("dht/temperature", temperature);
    json.set("distance/cm", distance);
    json.set("timestamp", millis());

    Serial.println("Sending data to Firebase...");
    
    if (Firebase.RTDB.setJSON(&fbdo, "/sensor_data", &json)) {
      Serial.println("✓ Data sent successfully");
    } else {
      Serial.println("✗ Failed to send data");
      Serial.println("Reason: " + fbdo.errorReason());
    }
  }
}

void processSTM32Data(String data) {
  data.trim();
  
  if (data.length() > 0) {
    Serial.print("Nhan du lieu: ");
    Serial.println(data);

    if (data.startsWith("{")) {
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, data);
      
      if (error) {
        Serial.print("Loi JSON: ");
        Serial.println(error.c_str());
        return;
      }

      // DEBUG: In toàn bộ JSON nhận được
      Serial.println("=== RAW JSON ===");
      serializeJsonPretty(doc, Serial);
      Serial.println();
      Serial.println("================");

      if (doc.containsKey("rain")) {
        JsonObject rain = doc["rain"];
        
        // Đọc giá trị với kiểm tra tồn tại
        if (rain.containsKey("digital")) rainDigital = rain["digital"];
        if (rain.containsKey("analog")) rainAnalog = rain["analog"];
        if (rain.containsKey("volt")) rainVolt = rain["volt"];
        if (rain.containsKey("percent")) rainPercent = rain["percent"];
        
        // QUAN TRỌNG: Xử lý trường 'raining' đúng cách
        if (rain.containsKey("raining")) {
          int rainingValue = rain["raining"]; // Đọc dưới dạng số
          isRaining = (rainingValue == 1);   // Chuyển sang boolean
          Serial.print("Raw raining value: "); Serial.println(rainingValue);
          Serial.print("Converted isRaining: "); Serial.println(isRaining);
        }

        Serial.println("===== Du lieu cam bien mua =====");
        Serial.print("Digital: "); Serial.println(rainDigital);
        Serial.print("Analog: "); Serial.println(rainAnalog);
        Serial.print("Voltage: "); Serial.print(rainVolt); Serial.println("V");
        Serial.print("Percent: "); Serial.print(rainPercent); Serial.println("%");
        Serial.print("Raining? "); Serial.println(isRaining ? "YES" : "NO");
        Serial.println("================================");
      }
    } 
    else if (data.startsWith("Humidity:")) {
      sscanf(data.c_str(), "Humidity: %f%% Temperature: %fC", &humidity, &temperature);
      Serial.print("Humidity: "); Serial.print(humidity); Serial.println("%");
      Serial.print("Temperature: "); Serial.print(temperature); Serial.println("C");
    } 
    else if (data.startsWith("Distance:")) {
      sscanf(data.c_str(), "Distance: %f cm", &distance);
      Serial.print("Distance: "); Serial.print(distance); Serial.println(" cm");
    }
  }
}

void saveWiFiCredentials(const String& ssid, const String& password) {
  preferences.begin("wifi-config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putBool("configured", true);
  preferences.end();
  Serial.println("✓ Đã lưu thông tin WiFi vào bộ nhớ");
}



void clearWiFiCredentials() {
  preferences.begin("wifi-config", false);
  preferences.clear();
  preferences.end();
  Serial.println("✓ Đã xóa thông tin WiFi");
}
void setup() {
  Serial.begin(115200);
  MySerial.begin(115200, SERIAL_8N1, 16, 17);
  Serial.println("ESP32 ready, waiting for STM32 data...");
  
  initWiFi();
   // Khởi động Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", index_html);
  });

    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {
        wifiSSID = request->getParam("ssid", true)->value();
        wifiPass = request->getParam("pass", true)->value();
        
        Serial.println("Thông tin WiFi mới:");
        Serial.println("SSID: " + wifiSSID);
        Serial.println("Pass: " + wifiPass);
        
        // LƯU VÀO BỘ NHỚ
        saveWiFiCredentials(wifiSSID, wifiPass);
        
        request->send(200, "text/plain", "Đã lưu WiFi! ESP32 sẽ kết nối...");
        
        delay(2000);
        ESP.restart(); // Khởi động lại để kết nối WiFi
    } else {
        request->send(400, "text/plain", "Thiếu thông tin!");
    }
    });
    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
        clearWiFiCredentials();
        request->send(200, "text/plain", "Đã xóa cấu hình WiFi. ESP32 sẽ khởi động lại...");
        delay(2000);
        ESP.restart();
    });
  server.begin();
}

void loop() {
  // Kiểm tra kết nối WiFi mỗi 30 giây
  static unsigned long lastWifiCheck = 0;
  if (millis() - lastWifiCheck > 30000) {
    lastWifiCheck = millis();
    
    if (WiFi.status() != WL_CONNECTED && WiFi.getMode() == WIFI_STA) {
      Serial.println("Mất kết nối WiFi, đang kết nối lại...");
      WiFi.reconnect();
      
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 10) {
        delay(500);
        Serial.print(".");
        attempts++;
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✓ Kết nối lại WiFi thành công!");
      } else {
        Serial.println("\n✗ Không thể kết nối lại WiFi!");
      }
    }
  }
  
  // Code hiện tại của bạn
  if (MySerial.available()) {
    String data = MySerial.readStringUntil('\n');
    processSTM32Data(data);
  }
  
  if (millis() - dataMillis > 5000 || dataMillis == 0) {
    dataMillis = millis();
    sendToFirebase();
  }
  
  delay(100);

}
