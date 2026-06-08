// BIBLIOTECAS
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// WIFI
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// SERVIDOR WEB
WebServer server(80);

// DHT22
#define DHT_PIN 4
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

// MQ2
#define MQ2_PIN 33

// LEDS
#define LED_VERDE 25
#define LED_AMARELO 26
#define LED_VERMELHO 27

// BOTÕES
#define BTN_SENSORES 16
#define BTN_TERRA 17
#define BTN_LUA 5
#define BTN_MARTE 18

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ENUM TELAS
enum TelaAtual
{
  TELA_HABITAT = 0,
  TELA_TERRA,
  TELA_LUA,
  TELA_MARTE
};

int telaAtual = TELA_HABITAT;
int ultimaTela = -1;

// CONTROLE DE TEMPO
// Debounce dos botões
unsigned long ultimoClique = 0;
const unsigned long DEBOUNCE = 250;

// Atualização do LCD
unsigned long ultimoUpdateLCD = 0;
const unsigned long INTERVALO_LCD = 1000;

// DADOS DO HABITAT - Puxa dos sensores
float temperatura = 0;
float umidade = 0;
int gas = 0;
String statusHabitat = "SEGURO ";

// DADOS DA TERRA - Puxa da API
float earthTemp = 0;
float earthHumidity = 0;
float earthWind = 0;
String statusTerra = "SEGURO ";

// DADOS DA LUA - SIMULADO
float moonTemp = -53;
float moonRadiation = 8.7;
float moonGravity = 1.62;
String statusLua = "ATENCAO";

// DADOS DE MARTE - SIMULADO
float marsTemp = -50;
float marsPressure = 400;
float marsWind = 30;
String statusMarte = "ATENCAO ";

// PROTÓTIPOS DAS FUNÇÕES
// Sensores
void lerSensores();

// APIs
void buscarDadosTerra();

// Status
void calcularStatusHabitat();
void calcularStatusTerra();
void calcularStatusLua();
void calcularStatusMarte();

// Interface
void atualizarLeds(String statusAtual);
void verificarBotoes();
void atualizarLCD();

// Wifi
void conectarWiFi();

// Dashboard
void handleRoot();
void handleStatus();
void handleLuaStatus();
void handleEarthStatus();
void handleMarsStatus();

// LEITURA DOS SENSORES
void lerSensores(){
  temperatura = dht.readTemperature();
  umidade = dht.readHumidity();
  gas = analogRead(MQ2_PIN);
  Serial.println(gas);

  // Evita valores inválidos do DHT
  // isnan ele verifica se tem valor
  if (isnan(temperatura))
  {
    temperatura = 0;
  }

  if (isnan(umidade))
  {
    umidade = 0;
  }
}

// STATUS DO HABITAT
void calcularStatusHabitat(){
  if (gas > 700 || temperatura > 40)
  {
    statusHabitat = "CRITICO";
  }
  else if (gas > 300 || temperatura > 30)
  {
    statusHabitat = "ATENCAO";
  }
  else
  {
    statusHabitat = "SEGURO ";
  }
}

// STATUS DA TERRA
void calcularStatusTerra(){
  if (earthTemp >= 40)
  {
    statusTerra = "CRITICO";
  }
  else if (earthTemp >= 30)
  {
    statusTerra = "ATENCAO";
  }
  else
  {
    statusTerra = "SEGURO ";
  }
}

// STATUS DA LUA
void calcularStatusLua(){
  if (moonRadiation >= 9)
  {
    statusLua = "CRITICO";
  }
  else if (moonRadiation >= 6)
  {
    statusLua = "ATENCAO";
  }
  else
  {
    statusLua = "SEGURO ";
  }
}

// STATUS DE MARTE
void calcularStatusMarte(){
  if (marsWind >= 30)
  {
    statusMarte = "CRITICO";
  }
  else if (marsWind >= 15)
  {
    statusMarte = "ATENCAO";
  }
  else
  {
    statusMarte = "SEGURO ";
  }
}

// CALCULA TODOS OS STATUS
void calcularTodosStatus(){
  calcularStatusHabitat();
  calcularStatusTerra();
  calcularStatusLua();
  calcularStatusMarte();
}

// CONTROLE DOS LEDS
void atualizarLeds(String statusAtual){
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERMELHO, LOW);

  if (statusAtual == "SEGURO ")
  {
    digitalWrite(LED_VERDE, HIGH);
  }
  if (statusAtual == "ATENCAO")
  {
    digitalWrite(LED_AMARELO, HIGH);
  }
  if (statusAtual == "CRITICO")
  {
    digitalWrite(LED_VERMELHO, HIGH);
  }
}

// LEITURA DOS BOTÕES COM UM TEMPO EM MILLIS
void verificarBotoes(){
  if (millis() - ultimoClique < DEBOUNCE)
  {
    return;
  }

  if (digitalRead(BTN_SENSORES) == LOW)
  {
    telaAtual = TELA_HABITAT;
    ultimoClique = millis();
  }
  else if (digitalRead(BTN_TERRA) == LOW)
  {
    buscarDadosTerra();
    telaAtual = TELA_TERRA;
    ultimoClique = millis();
  }
  else if (digitalRead(BTN_LUA) == LOW)
  {
    telaAtual = TELA_LUA;
    ultimoClique = millis();
  }
  else if (digitalRead(BTN_MARTE) == LOW)
  {
    telaAtual = TELA_MARTE;
    ultimoClique = millis();
  }
}

// BUSCA DE DADOS DA TERRA (API OPEN-METEO)
void buscarDadosTerra(){
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi desconectado.");
    return;
  }

  HTTPClient http;

  String url =
    "https://api.open-meteo.com/v1/forecast?latitude=-23.55&longitude=-46.63&current=temperature_2m,relative_humidity_2m,wind_speed_10m";

  http.begin(url);

  int httpCode = http.GET();

  if (httpCode > 0)
  {
    String payload = http.getString();

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);
    if (!error)
    {
      JsonVariant temperatureNode = doc["current"]["temperature_2m"];
      JsonVariant earthHumidityNode = doc["current"]["relative_humidity_2m"];
      JsonVariant earthWindNode = doc["current"]["wind_speed_10m"];

      earthTemp = temperatureNode;
      earthHumidity = earthHumidityNode;
      earthWind = earthWindNode;

      Serial.println("Dados Terra atualizados.");
    }
    else
    {
      Serial.println("Erro ao ler JSON.");
    }
  }
  else
  {
    Serial.print("Erro HTTP: ");
    Serial.println(httpCode);
  }

  http.end();
}

// CONEXÃO WIFI
void conectarWiFi(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando...");

  Serial.println("Conectando WiFi...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado.");

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("WiFi Conectado");
  lcd.setCursor(0, 1);
  lcd.print("IP: ");
  lcd.print(WiFi.localIP());

  delay(2000);
}

// TELA HABITAT
void mostrarTelaHabitat(){
  atualizarLeds(statusHabitat);

  lcd.setCursor(0, 0);
  lcd.print("Habitat- ");
  lcd.print(statusHabitat);
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print((int)temperatura);
  lcd.print("C");
}

// TELA TERRA
void mostrarTelaTerra(){
  atualizarLeds(statusTerra);

  lcd.setCursor(0, 0);
  lcd.print("Terra- ");
  lcd.print(statusTerra);
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print((int)earthTemp);
  lcd.print("C");
}

// TELA LUA
void mostrarTelaLua(){
  atualizarLeds(statusLua);

  lcd.setCursor(0, 0);
  lcd.print("Lua- ");
  lcd.print(statusLua);
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print((int)moonTemp);
  lcd.print("C");
}

// TELA MARTE
void mostrarTelaMarte(){
  atualizarLeds(statusMarte);

  lcd.setCursor(0, 0);
  lcd.print("Marte- ");
  lcd.print(statusMarte);
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print((int)marsTemp);
  lcd.print("C");
}

// LCD
void atualizarLCD(){
  if (millis() - ultimoUpdateLCD < INTERVALO_LCD)
  {
    return;
  }

  ultimoUpdateLCD = millis();

  if (telaAtual != ultimaTela)
  {
    lcd.clear();
    ultimaTela = telaAtual;
  }

  switch (telaAtual)
  {
    case TELA_HABITAT:
      mostrarTelaHabitat();
      break;

    case TELA_TERRA:
      mostrarTelaTerra();
      break;

    case TELA_LUA:
      mostrarTelaLua();
      break;

    case TELA_MARTE:
      mostrarTelaMarte();
      break;
  }
}

// DASHBOARD WEB
void handleRoot(){
  String html = R"rawliteral(

  <!DOCTYPE html>
  <html>

  <head>

  <meta charset="UTF-8">

  <meta http-equiv="refresh" content="5">

  <title>Habitat Espacial</title>

  <style>

  body{
    background:#0f172a;
    color:white;
    font-family:Arial;
    text-align:center;
    padding:20px;
  }

  .card{
    background:#1e293b;
    width:320px;
    margin:auto;
    margin-bottom:20px;
    padding:20px;
    border-radius:12px;
  }

  h1{
    color:#38bdf8;
  }

  </style>

  </head>

  <body>

  <h1>Habitat Espacial</h1>

  <div class="card">
  <h2>Habitat</h2>
  <p>Status: )rawliteral";

    html += statusHabitat;

    html += R"rawliteral(</p>
  <p>Temperatura: )rawliteral";

    html += String(temperatura, 1);

    html += R"rawliteral( °C</p>
  <p>Umidade: )rawliteral";

    html += String(umidade, 1);

    html += R"rawliteral( %</p>
  <p>Umidade: )rawliteral";

    html += String(gas, 1);

    html += R"rawliteral( %</p>
  </div>

  <div class="card">
  <h2>Terra</h2>
  <p>Status: )rawliteral";

    html += statusTerra;

    html += R"rawliteral(</p>
  <p>Temperatura: )rawliteral";

    html += String(earthTemp, 1);

    html += R"rawliteral( °C</p>
  <p>Umidade: )rawliteral";

    html += String(earthHumidity, 1);

    html += R"rawliteral( %</p>
  <p>Vento: )rawliteral";

    html += String(earthWind, 1);

    html += R"rawliteral( km/h</p>
  </div>

  <div class="card">
  <h2>Lua</h2>
  <p>Status: )rawliteral";

    html += statusLua;

    html += R"rawliteral(</p>
  <p>Radiacao: )rawliteral";

    html += String(moonRadiation, 1);

    html += R"rawliteral(</p>
  </div>

  <div class="card">
  <h2>Marte</h2>
  <p>Status: )rawliteral";

    html += statusMarte;

    html += R"rawliteral(</p>
  <p>Temperatura: )rawliteral";

    html += String(marsTemp, 1);

    html += R"rawliteral( °C</p>
  <p>Pressao: )rawliteral";

    html += String(marsPressure, 1);

    html += R"rawliteral( Pa</p>
  <p>Vento: )rawliteral";

    html += String(marsWind, 1);

    html += R"rawliteral( km/h</p>
  </div>

  </body>
  </html>

  )rawliteral";

    server.send(200, "text/html", html);
}

// JSON HABITAT
void handleStatus(){
  DynamicJsonDocument doc(512);

  doc["temperatura"] = temperatura;
  doc["umidade"] = umidade;
  doc["gas"] = gas;
  doc["status"] = statusHabitat;

  String json;

  serializeJson(doc, json);

  server.send(200, "application/json", json);
}

// JSON TERRA
void handleEarthStatus(){
  DynamicJsonDocument doc(512);

  doc["temperatura"] = earthTemp;
  doc["umidade"] = earthHumidity;
  doc["vento"] = earthWind;
  doc["status"] = statusTerra;

  String json;

  serializeJson(doc, json);

  server.send(200, "application/json", json);
}

// JSON LUA
void handleLuaStatus(){
  DynamicJsonDocument doc(512);

  doc["temperatura"] = moonTemp;
  doc["Radiação"] = moonRadiation;
  doc["Gravidade"] = moonGravity;
  doc["status"] = statusLua;

  String json;

  serializeJson(doc, json);

  server.send(200, "application/json", json);
}

// JSON MARTE
void handleMarsStatus(){
  DynamicJsonDocument doc(512);

  doc["temperatura"] = marsTemp;
  doc["pressao"] = marsPressure;
  doc["vento"] = marsWind;
  doc["status"] = statusMarte;

  String json;

  serializeJson(doc, json);

  server.send(200, "application/json", json);
}

// CONFIGURAÇÃO DOS PINOS
void configurarPinos(){
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);

  pinMode(BTN_SENSORES, INPUT_PULLUP);
  pinMode(BTN_TERRA, INPUT_PULLUP);
  pinMode(BTN_LUA, INPUT_PULLUP);
  pinMode(BTN_MARTE, INPUT_PULLUP);
}

// CONFIGURAÇÃO DO LCD
void configurarLCD(){
  lcd.init();
  lcd.backlight();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("HELIOS IOT");
  lcd.setCursor(0, 1);
  lcd.print("Inicializando");

  delay(1500);
}

// CONFIGURAÇÃO DO SERVIDOR WEB
void configurarServidor(){
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status-sensores", HTTP_GET, handleStatus);
  server.on("/status-lua", HTTP_GET, handleLuaStatus);
  server.on("/status-terra", HTTP_GET, handleEarthStatus);
  server.on("/status-marte", HTTP_GET, handleMarsStatus);

  server.begin();
  Serial.println("Servidor Web iniciado.");
}

// DADOS INICIAIS
void carregarDadosIniciais(){
  buscarDadosTerra();
  calcularTodosStatus();
}

// SETUP
void setup(){
  Serial.begin(115200);

  randomSeed(millis());

  Serial.println();
  Serial.println("============");
  Serial.println(" HELIOS IOT");
  Serial.println("============");

  // Inicializa sensores
  dht.begin();

  // LCD
  configurarLCD();

  // LEDs e botões
  configurarPinos();

  // WiFi
  conectarWiFi();

  // Dados iniciais
  carregarDadosIniciais();

  // Servidor Web
  configurarServidor();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Sistema OK");
  lcd.setCursor(0, 1);
  lcd.print("Pronto");

  Serial.println("Sistema iniciado com sucesso.");
}

// LOOP PRINCIPAL
void loop(){
  // LEITURA DOS SENSORES
  lerSensores();

  // CÁLCULO DOS STATUS
  calcularTodosStatus();

  // LEITURA DOS BOTÕES
  verificarBotoes();

  // LCD
  atualizarLCD();

  // WEB SERVER
  server.handleClient();
}