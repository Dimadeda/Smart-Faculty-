#include <ETH.h>
#include <ESPAsyncWebServer.h>
#include <PN5180.h>
#include <PN5180ISO14443.h>
#include <Crypto.h>
#include <SHA256.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

//убрать
const char* ssidW = "tanker";
const char* passwordW = "Haeil91572JqaY";

// Настройки точки доступа (начальные значения)
char ap_ssid[32] = "ESP32-Access-Point";
char ap_password[64] = "12345678";
IPAddress ap_local_ip(192, 168, 4, 1);  // Статический IP-адрес для точки доступа
IPAddress ap_gateway(192, 168, 4, 1);   // Шлюз для точки доступа
IPAddress ap_subnet(255, 255, 255, 0);  // Маска подсети для точки доступа

// Настройки Ethernet (начальные значения)
IPAddress eth_ip(192, 168, 1, 200);
IPAddress eth_gateway(192, 168, 1, 1);
IPAddress eth_subnet(255, 255, 255, 0);

// Настройки сервера (начальные значения)
char web_host[100] = "smart.dev.rfct.info";
char web_path[100] = "/api/TvHab";
int web_port = 443;
char rootCACertificate[2048] =
"-----BEGIN CERTIFICATE-----\n"
"MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw\n"
"CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg\n"
"R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00\n"
"MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT\n"
"ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw\n"
"EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW\n"
"+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9\n"
"ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T\n"
"AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI\n"
"zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW\n"
"tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1\n"
"/q4AaOeMSQ+2b1tbFfLn\n"
"-----END CERTIFICATE-----";

WiFiClientSecure client;

// Новая переменная для reader_id
int reader_id = 0;

// Экземпляр веб-сервера
AsyncWebServer server(80);

// Экземпляр Preferences для работы с NVS
Preferences preferences;
//
Preferences preferences_save_uid; 

static bool eth_connected = false;

// Обработка событий Ethernet
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      ETH.setHostname("esp32-ethernet");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("Got an IP Address for ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.println(ETH.localIP());
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

void loadPreferences() {
  preferences.begin("eth-config", true);

  if (preferences.isKey("ap_ssid")) {
    preferences.getString("ap_ssid", ap_ssid, sizeof(ap_ssid));
  }
  if (preferences.isKey("ap_password")) {
    preferences.getString("ap_password", ap_password, sizeof(ap_password));
  }
  if (preferences.isKey("web_host")) {
    preferences.getString("web_host", web_host, sizeof(web_host));
  }
  if (preferences.isKey("web_path")) {
    preferences.getString("web_path", web_path, sizeof(web_path));
  }
  if (preferences.isKey("web_port")) {
    web_port = preferences.getInt("web_port", web_port);
  }
  if (preferences.isKey("rootCACertificate")) {
    preferences.getString("rootCACertificate", rootCACertificate, sizeof(rootCACertificate));
  }
  if (preferences.isKey("eth_ip")) {
    String eth_ip_str = preferences.getString("eth_ip");
    eth_ip.fromString(eth_ip_str);
  }
  if (preferences.isKey("eth_gateway")) {
    String eth_gateway_str = preferences.getString("eth_gateway");
    eth_gateway.fromString(eth_gateway_str);
  }
  if (preferences.isKey("eth_subnet")) {
    String eth_subnet_str = preferences.getString("eth_subnet");
    eth_subnet.fromString(eth_subnet_str);
  }
  if (preferences.isKey("reader_id")) {
    reader_id = preferences.getInt("reader_id", reader_id);
  }

  preferences.end();
}

void savePreferences() {
  preferences.begin("eth-config", false);

  preferences.putString("ap_ssid", ap_ssid);
  preferences.putString("ap_password", ap_password);
  preferences.putString("web_host", web_host);
  preferences.putString("web_path", web_path);
  preferences.putInt("web_port", web_port);
  preferences.putString("rootCACertificate", rootCACertificate);

  preferences.putString("eth_ip", eth_ip.toString());
  preferences.putString("eth_gateway", eth_gateway.toString());
  preferences.putString("eth_subnet", eth_subnet.toString());

  preferences.putInt("reader_id", reader_id);

  preferences.end();
}

#define PN5180_NSS  2
#define PN5180_BUSY 5
#define PN5180_RST  3
#define PN5180_SCK  14
#define PN5180_MOSI 4
#define PN5180_MISO 13

PN5180ISO14443 nfc(PN5180_NSS, PN5180_BUSY, PN5180_RST);

void setup() {
  // Запуск последовательного порта
  Serial.begin(115200);

  // Загрузка сохраненных настроек
  loadPreferences();


  // убрать
  WiFi.begin(ssidW, passwordW);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Настройка статического IP для точки доступа
  WiFi.softAPConfig(ap_local_ip, ap_gateway, ap_subnet);

  // Настройка точки доступа
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("Access Point Started");
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Регистрация обработчика событий
  WiFi.onEvent(WiFiEvent);

  // Запуск Ethernet с сохраненными параметрами
  ETH.config(eth_ip, eth_gateway, eth_subnet);
  ETH.begin();

  // Устанавливаем корневой сертификат (если используется)
  client.setCACert(rootCACertificate);
  client.setInsecure(); // Убедитесь, что это отключено, если вы используете сертификат

  Serial.println(F("PN5180 ISO14443 Demo"));

  // Set up SPI pins
  SPI.begin(PN5180_SCK, PN5180_MISO, PN5180_MOSI);

  nfc.begin();
  nfc.reset();

  preferences_save_uid.begin("uid", false); // 

  Serial.println(F("Reading last UID hash..."));
  String lastUIDHash = preferences_save_uid.getString("hash", ""); // 
  Serial.println("Last UID hash: " + lastUIDHash);

  sendPostRequest(String(reader_id),  lastUIDHash); //

  Serial.println(F("Reading product version..."));
  uint8_t productVersion[2];
  nfc.readEEprom(PRODUCT_VERSION, productVersion, sizeof(productVersion));
  Serial.print(F("Product version="));
  Serial.print(productVersion[1]);
  Serial.print(F("."));
  Serial.println(productVersion[0]);

  if (productVersion[1] == 0xFF) { // Check if initialization failed
    Serial.println(F("Initialization failed!"));
    while (1);
  }

  Serial.println(F("Reading firmware version..."));
  uint8_t firmwareVersion[2];
  nfc.readEEprom(FIRMWARE_VERSION, firmwareVersion, sizeof(firmwareVersion));
  Serial.print(F("Firmware version="));
  Serial.print(firmwareVersion[1]);
  Serial.print(F("."));
  Serial.println(firmwareVersion[0]);

  Serial.println(F("Reading EEPROM version..."));
  uint8_t eepromVersion[2];
  nfc.readEEprom(EEPROM_VERSION, eepromVersion, sizeof(eepromVersion));
  Serial.print(F("EEPROM version="));
  Serial.print(eepromVersion[1]);
  Serial.print(F("."));
  Serial.println(eepromVersion[0]);

  Serial.println(F("Enable RF field..."));
  nfc.setupRF();
  //

  // Сервисы веб-сервера
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html",
        "<!DOCTYPE html>"
        "<html lang='en'>"
        "<head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        "<title>Smart Reader Settings</title>"
        "<style>"
        "body { font-family: Arial, sans-serif; background-color: #f4f4f9; color: #333; }"
        "h1 { text-align: center; color: #4CAF50; }"
        "form { max-width: 600px; margin: 0 auto; background: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }"
        "fieldset { border: 1px solid #ccc; padding: 10px 15px; margin-bottom: 20px; border-radius: 4px; }"
        "legend { font-weight: bold; color: #4CAF50; }"
        "label { display: block; margin-bottom: 8px; font-weight: bold; }"
        "input[type='text'], input[type='number'], textarea { width: 100%; padding: 8px; margin-bottom: 20px; border: 1px solid #ccc; border-radius: 4px; }"
        "input[type='submit'] { background-color: #4CAF50; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; }"
        "input[type='submit']:hover { background-color: #45a049; }"
        "</style>"
        "</head>"
        "<body>"
        "<h1>Smart Reader Settings</h1>"
        "<form action='/update' method='post'>"
        "<fieldset>"
        "<legend>Ap Server Settings</legend>"
        "<label for='ap_ssid'>AP SSID</label>"
        "<input type='text' id='ap_ssid' name='ap_ssid' value='" + String(ap_ssid) + "' required>"
        "<label for='ap_password'>AP Password</label>"
        "<input type='text' id='ap_password' name='ap_password' value='" + String(ap_password) + "' required>"
        "<label for='ap_ip'>AP IP</label>"
        "<input type='text' id='ap_ip' name='ap_ip' value='" + ap_local_ip.toString() + "' required>"
        "<label for='ap_gateway'>AP Gateway</label>"
        "<input type='text' id='ap_gateway' name='ap_gateway' value='" + ap_gateway.toString() + "' required>"
        "<label for='ap_subnet'>AP Subnet</label>"
        "<input type='text' id='ap_subnet' name='ap_subnet' value='" + ap_subnet.toString() + "' required>"
        "</fieldset>"
        "<fieldset>"
        "<legend>Ethernet Settings</legend>"
        "<label for='eth_ip'>ETH IP</label>"
        "<input type='text' id='eth_ip' name='eth_ip' value='" + eth_ip.toString() + "' required>"
        "<label for='eth_gateway'>ETH Gateway</label>"
        "<input type='text' id='eth_gateway' name='eth_gateway' value='" + eth_gateway.toString() + "' required>"
        "<label for='eth_subnet'>ETH Subnet</label>"
        "<input type='text' id='eth_subnet' name='eth_subnet' value='" + eth_subnet.toString() + "' required>"
        "</fieldset>"
        "<fieldset>"
        "<legend>Web Server Settings</legend>"
        "<label for='web_host'>Web Host</label>"
        "<input type='text' id='web_host' name='web_host' value='" + String(web_host) + "' required>"
        "<label for='web_path'>Web Path</label>"
        "<input type='text' id='web_path' name='web_path' value='" + String(web_path) + "' required>"
        "<label for='web_port'>Web Port</label>"
        "<input type='number' id='web_port' name='web_port' value='" + String(web_port) + "' required>"
        "<label for='rootCACertificate'>Root CA Certificate</label>"
        "<textarea id='rootCACertificate' name='rootCACertificate' rows='10' required>" + String(rootCACertificate) + "</textarea>"
        "</fieldset>"

        "<fieldset>"
        "<legend>Reader Settings</legend>"
        "<label for='reader_id'>Reader ID</label>"
        "<input type='number' id='reader_id' name='reader_id' value='" + String(reader_id) + "' required>"
        "</fieldset>"

        "<input type='submit' value='Update'>"
        "</form>"
        "</body>"
        "</html>"

                  );
  });

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Получение параметров из запроса
    if (request->hasParam("ap_ssid", true)) {
      String new_ssid = request->getParam("ap_ssid", true)->value();
      strncpy(ap_ssid, new_ssid.c_str(), sizeof(ap_ssid));
    }
    if (request->hasParam("ap_password", true)) {
      String new_password = request->getParam("ap_password", true)->value();
      strncpy(ap_password, new_password.c_str(), sizeof(ap_password));
    }
    if (request->hasParam("ap_ip", true)) {
      String new_ap_ip = request->getParam("ap_ip", true)->value();
      ap_local_ip.fromString(new_ap_ip);
    }
    if (request->hasParam("ap_gateway", true)) {
      String new_ap_gateway = request->getParam("ap_gateway", true)->value();
      ap_gateway.fromString(new_ap_gateway);
    }
    if (request->hasParam("ap_subnet", true)) {
      String new_ap_subnet = request->getParam("ap_subnet", true)->value();
      ap_subnet.fromString(new_ap_subnet);
    }
    if (request->hasParam("eth_ip", true)) {
      String new_eth_ip = request->getParam("eth_ip", true)->value();
      eth_ip.fromString(new_eth_ip);
    }
    if (request->hasParam("eth_gateway", true)) {
      String new_eth_gateway = request->getParam("eth_gateway", true)->value();
      eth_gateway.fromString(new_eth_gateway);
    }
    if (request->hasParam("eth_subnet", true)) {
      String new_eth_subnet = request->getParam("eth_subnet", true)->value();
      eth_subnet.fromString(new_eth_subnet);
    }
    if (request->hasParam("web_host", true)) {
      String new_web_host = request->getParam("web_host", true)->value();
      strncpy(web_host, new_web_host.c_str(), sizeof(web_host));
    }
    if (request->hasParam("web_path", true)) {
      String new_web_path = request->getParam("web_path", true)->value();
      strncpy(web_path, new_web_path.c_str(), sizeof(web_path));
    }
    if (request->hasParam("web_port", true)) {
      web_port = request->getParam("web_port", true)->value().toInt();
    }
    if (request->hasParam("rootCACertificate", true)) {
      String new_rootCACertificate = request->getParam("rootCACertificate", true)->value();
      strncpy(rootCACertificate, new_rootCACertificate.c_str(), sizeof(rootCACertificate));
    }
    if (request->hasParam("reader_id", true)) {
      reader_id = request->getParam("reader_id", true)->value().toInt();
    }

    // Сохранение новых настроек
    savePreferences();

    // Перезагрузка устройства для применения изменений
    request->send(200, "text/html", "Settings updated! Rebooting...");
    delay(2000);
    ESP.restart();
  });

  server.begin();
}

void sendPostRequest(String readerId, String uidHash) {
  // Создаем объект клиента и подключаемся к серверу
  Serial.println("Trying to connect");
  if (!client.connect(web_host, web_port)) {
    Serial.println("Connection failed");
    delay(5000);
    return;
  }
  Serial.println("Connected");

  // Данные для POST запроса в формате query parameters
  String url =  String(web_path) + "?readerId=" + readerId + "&UidHash=" + uidHash;

  // Создаем POST запрос
  String request = String("POST ") + url + " HTTP/1.1\r\n" + 
                   "Host: " + web_host + "\r\n" +
                   "Content-Type: application/x-www-form-urlencoded\r\n" + // Можно оставить этот тип контента, хотя тело запроса пустое
                   "Connection: close\r\n" + // Закрываем соединение после запроса
                   "\r\n";

  // Отладочная информация
  Serial.println("Request:");
  Serial.println(request);

  // Отправка запроса
  client.print(request);
  Serial.println("Request sent");

  // Читаем ответ от сервера
  while (client.connected() || client.available()) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
    }
  }

  // Закрываем соединение
  client.stop();
  Serial.println("Connection closed");
  delay(3000);//
}

void loop() {
  uint8_t uid[8] = {0};
  int8_t uid_len = nfc.readCardSerial(uid);

  if (uid_len > 0) {
    String uidString = "";
    for (int i = 0; i < uid_len; i++) {
      if (uid[i] < 0x10) {
        uidString += "0";  // Add leading zero for single digit hex numbers
      }
      uidString += String(uid[i], HEX);
    }
    uidString.toUpperCase();
    int length = uidString.length();
    char charArray[length + 2]; // Дополнительное место для \0
    uidString.toCharArray(charArray, length + 1);
    charArray[length] = '\0';
    Serial.print(F("UID="));
    Serial.println(uidString);
    uidString = String(charArray);
    // Hash the UID with SHA256
    SHA256 sha256;
    byte hash[32];  // Buffer to hold the hash
    sha256.reset();
    sha256.update((const byte*)uidString.c_str(), uidString.length());
    sha256.finalize(hash, sizeof(hash));

    // Convert hash to string
    String hashString = "";
    for (int i = 0; i < sizeof(hash); i++) {  // SHA256 produces a 32-byte hash
      if (hash[i] < 16) {
        hashString += "0";  // Add leading zero for single digit hex numbers
      }
      hashString += String(hash[i], HEX);
    }

    Serial.print(F("SHA256 hash of UID="));
    Serial.println(hashString);

    // Сохранение хеша UID в памяти
    preferences_save_uid.putString("hash", hashString); 
    Serial.println("Hash saved to memory.");

    sendPostRequest(String(reader_id),  hashString);

  }

  delay(1000);
}