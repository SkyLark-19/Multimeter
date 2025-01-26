#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Network credentials
const char* ssid = "Multimeters-Assemble-Here";
const char* password = "Assembled19";

ESP8266WebServer server(80);  // Web server instance

String result = "-";   // Multimeter reading variable

const int batteryPin = A0; // Analog pin to read battery voltage

float readBatteryVoltage() {
  int rawValue = analogRead(batteryPin);
  float voltage = (rawValue / 1023.0) * 3.3 * 2; // Voltage divider with two 100k ohm resistors
  return voltage;
}

int getBatteryPercentage(float voltage) {
  int percentage = map(voltage * 100, 300, 420, 0, 100);            //300 = 3V -> 0% , 420 = 4.2V -> 100%
  return constrain(percentage, 0, 100);
}

// Function declarations
void requestReading();
void handleRoot();
void handleToggle1();
void handleToggle2();
void handleToggle3();
void handleGetReading();
void handleStoreValues();
void handleGetStoredValues();
void handleReset();
void setupWiFi();
void setupServerRoutes();

// HTML content for the web page
const char *htmlContent = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Multimeter with STM32</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; }
    body { min-height: 100vh; background: linear-gradient(135deg, #000000, #1a1a00); display: flex; justify-content: center; align-items: center; color: white; }
    .container { background: rgba(26, 26, 0, 0.95); backdrop-filter: blur(10px); padding: 2rem; border-radius: 20px; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3); max-width: 90%; width: 600px; border: 1px solid rgba(255, 255, 0, 0.1); }
    
    .status-bar {margin-bottom: 20px; position: absolute;top: 15px;width: calc(100% - 4rem);display: flex;justify-content: space-between;align-items: center;}

    .reset-button {width: 40px;height: 40px;background: rgba(26,26,0,0.9);border: 2px solid #ffd700;border-radius: 50%;display: flex;align-items: center;justify-content: center;cursor: pointer;transition: all 0.3s ease;padding: 0;position: relative;}
    .reset-button:hover {background: rgba(255,215,0,0.2);}
    .reset-icon {width: 20px;height: 20px;border: 2px solid #ffd700;border-top: 2px solid transparent;border-radius: 50%;position: relative; transition: transform 0.3s ease;}
    .reset-icon::after {content: '';position: absolute;top: -2px;right: 4px;width: 0;height: 0;border-left: 6px solid transparent;border-right: 6px solid transparent;border-bottom: 6px solid #ffd700;transform: rotate(45deg);}
    .reset-icon:hover {transform: rotate(180deg);}
    .reset-text {position: absolute; bottom: -20px; left: 50%;transform: translateX(-50%); color: #ffd700; font-size: 0.8rem;}

    .battery-container {width: 40px;height: 40px;background: rgba(26,26,0,0.9);border: 2px solid #ffd700;border-radius: 50%;display: flex;align-items: center;justify-content: felx-end;position: relative;}
    .battery-indicator {position: relative;width: 20px;height: 10px;border: 2px solid #ffd700;border-radius: 2px;padding: 1px; align-items: center; justify-content: center;display: flex;margin-right: -6px;}
    .battery-indicator::after {content: '';position: absolute;right: -4px;top: 50%;transform: translateY(-50%);width: 2px;height: 6px;background: #ffd700;border-radius: 0 1px 1px 0;}
    .battery-level {height: 100%;background: #ffd700;border-radius: 1px;transition: width 0.3s ease;z-index:1;}
    .battery-percentage {position: absolute;bottom: -20px;left: 50%;transform: translateX(-50%);font-size: 0.8rem;color: #ffd700}

    .view-values-button {background: rgba(26,26,0,0.9);color: #ffd700;padding: 0.8rem 1.5rem;cursor: pointer;font-size: 1rem;margin: 1.5rem auto;transition: all 0.3s ease;width: auto;display: inline-block;clip-path: polygon(0 0, 85% 0, 100% 50%, 85% 100%, 0 100%, 15% 50%);border: 1px solid rgba(255,215,0,0.2);}
    .view-values-button:hover {background: rgba(255,215,0,0.2);transform: translateX(5px);}

    h1 { text-align: center; color: #ffd700; font-size: 2.5rem; margin-bottom: 2rem; margin-top: 4rem; text-shadow: 0 0 10px rgba(255, 215, 0, 0.5); animation: glow 2s ease-in-out infinite; }
    .mode-section { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 1rem; margin-bottom: 2rem; }
    .button { background: rgba(26, 26, 0, 0.9); border: 1px solid rgba(255, 215, 0, 0.2); padding: 1rem 2rem; border-radius: 10px; color: #ffd700; cursor: pointer; font-size: 1.1rem; transition: all 0.3s ease; display: flex; align-items: center; justify-content: center; }
    .button:hover { background: rgba(255, 215, 0, 0.2); border-color: rgba(255, 215, 0, 0.4); transform: translateY(-2px); }
    .button.selected { background: #ffd700; color: #000000; border-color: #ffd700; }
    .reading-section { text-align: center; margin: 2rem 0; }
    .gauge { width: 320px; height: 320px; background: rgba(26, 26, 0, 0.9); border-radius: 50%; margin: 0 auto; position: relative; border: 4px solid #ffd700; animation: pulse 2s infinite; display: flex; align-items: center; justify-content: center; overflow: hidden; }
    .reading { font-size: 4rem; font-weight: bold; color: #ffd700; text-align: center; padding: 0 20px; width: 100%; white-space: nowrap; transform: scale(1); transition: transform 0.3s ease; }
    .reading.scale-down { transform: scale(0.8); }
    .battery-indicator { position: absolute; top: 15px; right: 15px; display: flex; align-items: center; gap: 4px; color: #ffd700; font-size: 0.9rem; margin-bottom: 1rem;}
    
    .mode-buttons { display: flex; justify-content: center; gap: 1rem; margin-bottom: 2rem; }
    .back-button { position: absolute; left: 20px; top: 50%; transform: translateY(-50%); background: none; border: none; color: #ffd700; font-size: 1.5rem; cursor: pointer; padding: 10px; transition: transform 0.3s ease; }
    .back-button:hover { transform: translateY(-50%) scale(1.1); }
    .current-options { position: relative; padding-left: 40px; }
    .store-section { text-align: center; padding-top: 2rem; border-top: 1px solid rgba(255, 215, 0, 0.2); }
    .store-buttons { display: flex; gap: 1rem; justify-content: center; margin-top: 1rem; }
    .store-button { padding: 0.5rem 2rem; border: none; border-radius: 5px; cursor: pointer; font-size: 1rem; transition: all 0.3s ease; }
    .store-button { background: #ffd700; color: #000000; }
    .store-button:hover { transform: scale(1.05); }
    .notification { position: fixed; top: 20px; right: 20px; padding: 1rem; border-radius: 5px; color: #000000; animation: slideIn 0.3s ease-out; }
    .back-arrow::before { content: '←'; font-size: 1.5em; }
    .stored-values-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(200px, 1fr)); gap: 1rem; margin-top: 2rem; }
    .value-card { background: rgba(26, 26, 0, 0.9); border: 1px solid #ffd700; border-radius: 10px; padding: 1rem; text-align: center; }
    .back-to-main { display: block; margin: 2rem auto; background: #ffd700; color: #000; padding: 0.5rem 2rem; border: none; border-radius: 5px; cursor: pointer; }

    @media(max-width:600px){
      .container{padding:1rem;}
      h1{font-size:2rem;}
      .gauge{width:280px;height:280px;}
      .reading{font-size:3rem;}
      .status-bar {width: calc(100% - 2rem);}
    }
    @keyframes glow { 0%, 100% { text-shadow: 0 0 10px rgba(255, 215, 0, 0.5); } 50% { text-shadow: 0 0 20px rgba(255, 215, 0, 0.8); } }
  </style>
</head>
<body>
  <div class="container">
    <div class="status-bar">
      <button class="reset-button">
        <div class="reset-icon"></div>
        <span class="reset-text">Reset</span>
      </button>
      <div class="battery-container">
        <div class="battery-indicator">
          <div class="battery-level" style="width:85%;"></div>
        </div>
        <span class="battery-percentage">85%</span>
      </div>
    </div>
    <h1>MULTIMETER</h1>
    
    <div id="mainScreen">
      <div id="modeButtons" class="mode-buttons">
        <button id="voltageBtn" class="button">Voltage</button>
        <button id="currentBtn" class="button">Current</button>
      </div>
      <div id="currentOptions" class="current-options" style="display: none;">
        <button class="back-button">
          <span class="back-arrow"></span>
        </button>
        <div class="mode-buttons">
          <button id="maBtn" class="button">mA</button>
          <button id="uaBtn" class="button">μA</button>
        </div>
      </div>
      <div class="reading-section">
        <div class="gauge">
          <div id="reading" class="reading">-</div>
        </div>
      </div>
      <div class="store-section">
        <div class="store-buttons">
          <button class="store-button">Store</button>
        </div>
        <button class="view-values-button" id="viewStoredValues">
          Stored Values
          <span style="font-size:1.2em;"></span>
        </button>
      </div>
    </div>
  </div>

  <script>
    document.addEventListener('DOMContentLoaded', function() {
      const resetButton = document.querySelector('.reset-button');
      const viewStoredValues = document.getElementById('viewStoredValues');
      const voltageBtn = document.getElementById('voltageBtn');
      const currentBtn = document.getElementById('currentBtn');
      const maBtn = document.getElementById('maBtn');
      const uaBtn = document.getElementById('uaBtn');
      const modeButtons = document.getElementById('modeButtons');
      const currentOptions = document.getElementById('currentOptions');
      const backButton = document.querySelector('.back-button');
      const storeButton = document.querySelector('.store-button');

      function showNotification(message) {
        const notification = document.createElement('div');
        notification.className = 'notification';
        notification.style.background = '#ff4444'; 
        notification.style.color = '#ffffff'; 
        notification.textContent = message;
        document.body.appendChild(notification);

        setTimeout(() => {
          notification.style.animation = 'slideOut 0.5s ease-out';
          setTimeout(() => notification.remove(), 300);
        }, 5000);
      }

      function createStoredValuesView(data) {
        const values = data.split('\n').filter(v => v.trim());
        return `
          <div class="container stored-values-container">
            <div class="status-bar">
              <button class="reset-button">
                <div class="reset-icon"></div>
              </button>
            </div>
            <h1>Stored Values</h1>
            <div class="stored-values-grid">
              ${values.map(value => `
                <div class="value-card">
                  <p>${value}</p>
                </div>
              `).join('')}
            </div>
            <button class="back-to-main" onclick="location.reload()">Back to Main</button>
          </div>
        `;
      }

      function attachEventListeners() {
        const resetButton = document.querySelector('.reset-button');
        resetButton.addEventListener('click', function() {
          fetch('/reset')
            .then(response => response.text())
            .then(data => {
              showNotification(data);
              location.reload();
            },5000);
        });
      }

      viewStoredValues.addEventListener('click', function() {
        fetch('/getStoredValues')
          .then(response => response.text())
          .then(data => {
            document.body.innerHTML = createStoredValuesView(data);
            attachEventListeners(); // Reattach event listeners after updating the HTML
          });
      });

      resetButton.addEventListener('click', function() {
        fetch('/reset')
          .then(response => response.text())
          .then(data => {
            showNotification(data);
            location.reload();
          },5000);
      });

      voltageBtn.addEventListener('click', function() {
        fetch('/toggle1')
          .then(response => response.text())
          .then(data => {
            document.getElementById('reading').textContent = data;
          });
      });

      currentBtn.addEventListener('click', function() {
        modeButtons.style.display = 'none';
        currentOptions.style.display = 'block';
      });

      maBtn.addEventListener('click', function() {
        fetch('/toggle2')
          .then(response => response.text())
          .then(data => {
            document.getElementById('reading').textContent = data;
          });
      });

      uaBtn.addEventListener('click', function() {
        fetch('/toggle3')
          .then(response => response.text())
          .then(data => {
            document.getElementById('reading').textContent = data;
          });
      });

      backButton.addEventListener('click', function() {
        currentOptions.style.display = 'none';
        modeButtons.style.display = 'flex';
      });

      storeButton.addEventListener('click', function() {
        fetch('/storeValues?store=true')
          .then(response => response.text())
          .then(data => {
            showNotification(data);
          });
      });

      function updateBatteryPercentage() {
        fetch('/getBatteryPercentage')
          .then(response => response.text())
          .then(percentage => {
            const batteryLevel = document.querySelector('.battery-level');
            const batteryPercentage = document.querySelector('.battery-percentage');
            batteryLevel.style.width = `${percentage}%`;
            batteryPercentage.textContent = `${percentage}%`;
          });
      }

      
      updateBatteryPercentage();
      setInterval(updateBatteryPercentage, 60000); 

      attachEventListeners();
    });
  </script>
</body>
</html>
)rawliteral";

// Helper function to send a command to STM32 and get the response
String sendCommandToSTM32(const String& command) {
  Serial1.print(command); // Send command to STM32

  String response = "";
  unsigned long timeout = millis() + 5000; // 5 second timeout
  while (millis() < timeout) {
    if (Serial1.available()) {
      response += Serial1.readStringUntil('\n'); // Read response until newline
      if (response.endsWith("\n")) { // End of transmission
        break;
      }
    }
  }
  return response;
}

// Function to handle the root path
void handleRoot() {
  server.send(200, "text/html", htmlContent);
}

// Function to handle the toggle path for Voltage
void handleToggle1() {
 Serial.println("1");
 Serial1.print("nooV");
  
  // Wait for the STM32 to respond and collect the result
  result = "";
  unsigned long timeout = millis() + 5000; // 5 seconds timeout
  while (millis() < timeout) {
    if (Serial1.available()) {
      result += Serial1.readStringUntil('\n'); // Read result until newline
    }
  }
  
  // Respond with the current voltage value
  server.send(200, "text/plain", result);
}

// Function to handle the toggle path for Milliamphere
void handleToggle2() {
  Serial.println("2");
 Serial1.print("nooM");
  
  // Wait for the STM32 to respond and collect the result
  result = "";
  unsigned long timeout = millis() + 5000; // 5 seconds timeout
  while (millis() < timeout) {
    if (Serial1.available()) {
      result += Serial1.readStringUntil('\n'); // Read result until newline
    }
  }
  
  // Respond with the current voltage value
  server.send(200, "text/plain", result);
}

// Function to handle the toggle path for Microamphere
void handleToggle3() {
  Serial.println("3");
 Serial1.print("nooU");
  
  // Wait for the STM32 to respond and collect the result
  result = "";
  unsigned long timeout = millis() + 5000; // 5 seconds timeout
  while (millis() < timeout) {
    if (Serial1.available()) {
      result += Serial1.readStringUntil('\n'); // Read result until newline
    }
  }
  
  // Respond with the current voltage value
  server.send(200, "text/plain", result);
}

// Function to handle the reading request
void handleGetReading() {
  server.send(200, "text/plain", result);
}


// Function to handle storing values
void handleStoreValues() {
  if (server.arg("store") == "true") {
    Serial.println("Storing values...");
    Serial1.print("Y"); // Send 'Y' command to STM32 to store values
    server.send(200, "text/plain", "Values will be stored.");
  }
}

// Function to handle getting stored values
void handleGetStoredValues() {
  Serial.println("Requesting stored values...");
  String storedValues = sendCommandToSTM32("Y"); // Send 'Y' command to STM32
  server.send(200, "text/plain", storedValues);
}

void handleReset() {
  Serial.println("6");
  Serial1.print("nooR");
  server.send(200, "text/plain", "Device is resetting...");
}

// Function to set up Wi-Fi connection as an access point
void setupWiFi() {
  Serial.begin(115200);
  Serial.println("Setting up WiFi Access Point...");

  // Set up the ESP8266 as an access point
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

// Function to set up server routes
void setupServerRoutes() {
  server.on("/", handleRoot);
  server.on("/toggle1", handleToggle1);
  server.on("/toggle2", handleToggle2);
  server.on("/toggle3", handleToggle3);
  server.on("/getReading", handleGetReading);
  server.on("/storeValues", handleStoreValues);
  server.on("/getStoredValues", handleGetStoredValues);
  server.on("/reset", handleReset); 
}

void setup() {
  setupWiFi();
  setupServerRoutes();
  server.begin();
  Serial.println("HTTP server started.");

  Serial.begin(115200); // Initialize the UART communication
}

void loop() {
  server.handleClient();
  // Check if data is available from STM32
  while (Serial1.available()) {
    // Read and update the global result from STM32
    result += Serial1.readStringUntil('\n'); // Read result until newline
    Serial.print("Received from STM32: ");
    Serial.println(result);
  }
}