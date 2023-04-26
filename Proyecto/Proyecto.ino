//Añade las librerias
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <PubSubClient.h>

//Inicializacion de valores para la Red.
const char* ssid = "Galaxy A04e2887";  //Añade tu SSID WiFi
const char* password = "wlqf9683";  //Agrega tu contraseña WiFi
const char* mqtt_server = "broker.emqx.io";  

String apiKey = "8927583";              // Añade tu número simbólico que Bot le ha enviado en WhatsApp
String phone_number = "+524181183344";      // Agregue su número de teléfono registrado en la aplicación WhatsApp (el mismo número que el Bot le envía en la URL)

String url;     // URL en cadena de caracteres, se utilizara para almacenar la URL final generada

const int DHT_PIN = 15;

WiFiClient espClient;
PubSubClient client(espClient);
DHTesp dhtSensor;
Servo servo;

TinyGPS gps;
SoftwareSerial ss(4, 2);

unsigned long lastMsg = 0;

#define LED 13
#define MSG_BUFFER_SIZE	(50)

char msg[MSG_BUFFER_SIZE];

int value = 0;
int pinServo = 12;
int pinBoton = 14;
bool boton = LOW;

String numTemperatura;
String numHumedad;
String msgTempAndHumidity;
String numFlat;
String numFlon;
String msgFlatAndFlon;
String motor;

void setup_wifi() {
  delay(10);
    
  //Empezamos conectándonos a una red WiFi.
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  
  Serial.println();

  //Encender el LED si se recibió un E como primer carácter
  if ((char)payload[0] == 'E') {
    digitalWrite(LED, HIGH);   //Se enciende el LED.
    Serial.println(msgTempAndHumidity);   //Envia el mensaje
    message_to_whatsapp(msgFlatAndFlon);
    message_to_whatsapp(msgTempAndHumidity); 

    //Mandar mensaje dependiendo de la temperatura y mover el servomotor
    if(data.temperature >= 30 && data.temperature <= 45) {
      message_to_whatsapp("En el lugar que se encuentra esta: Caliente");
    } 
  
    if(data.temperature > 15 && data.temperature < 30) {
      message_to_whatsapp("En el lugar que se encuentra esta: Normal");
    } 

    if(data.temperature >= 0 && data.temperature <= 15) {
      message_to_whatsapp("En el lugar que se encuentra esta: Frio");
    }
    
    delay(6000);
    digitalWrite(LED, LOW);    //Se apaga el LED.    
  }
}

void reconnect() {
  //Bucle hasta que estemos reconectados.
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    //Crear una identificación de cliente aleatoria.
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    //intento de conexión.
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      
      //Una vez conectado, publicar un anuncio...
      client.publish("tic/iot", "IOT");
      
      // ...y se subscribe
      client.subscribe("tic/iot");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      
      //Espere 5 segundos antes de volver a intentarlo.
      delay(5000);
    }
  }
}

void setup() {
  //Define propiedades del sensor
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  pinMode(LED, OUTPUT);
  pinMode(pinBoton, INPUT);
  servo.attach(pinServo, 500, 2500);

  ss.begin(9600);
  
  Serial.print("Simple TinyGPS "); Serial.println(TinyGPS::library_version());
  Serial.println("Ricardo Gloria y Sandra Karina");
  Serial.println();

  //Monitoriar puerto serial
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Use la función mensaje_a_whatsapp para enviar su propio mensaje
  message_to_whatsapp("Conectado a la API de mensajeria de WhatsApp.");  // Usted envía su propio mensaje simplemente cambie "Hola de TechTOnions" a su mensaje.  
}

void loop() {
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;

  // Por un segundo, analizamos los datos del GPS y reportamos algunos valores clave
  for (unsigned long start = millis(); millis() - start < 1000;) {
    while (ss.available()) {
      char c = ss.read();
      // Serial.write(c); // Descomente esta línea si desea ver el flujo de datos GPS.
      if (gps.encode(c)) // ¿Entró una nueva oración válida?
        newData = true;
    }
  }

  if (newData) {
    float flat, flon;
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);
    numFlat = String(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
    numFlon = String(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);     
    Serial.print("LAT = ");
    Serial.print(numFlat);
    Serial.print(" LON = ");
    Serial.print(numFlon);
    Serial.print(" SAT = ");
    Serial.print(gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites());
    Serial.print(" PREC = ");
    Serial.println(gps.hdop() == TinyGPS::GPS_INVALID_HDOP ? 0 : gps.hdop());
  }
  
  gps.stats(&chars, &sentences, &failed);
  Serial.print("CHARS = ");
  Serial.print(chars);
  Serial.print(" SENTENCES = ");
  Serial.print(sentences);
  Serial.print(" CSUM ERR = ");
  Serial.println(failed);
  
  if (chars == 0) {
    Serial.println("** No characters received from GPS: check wiring **");
  }
  
  //Declaración de variables de datos que obtiene temperatura y humedad
  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  double temp = dhtSensor.getTemperature();
  double humi = dhtSensor.getHumidity();

  numTemperatura = String(data.temperature, 2);
  numHumedad = String(data.humidity, 1);

  msgFlatAndFlon = ("Tu ubicación es: Latitud: " + numFlat + " y Longitud: " + numFlon);
  msgTempAndHumidity = ("Temperatura: " + numTemperatura + "°C" + " y " + "Humedad: " + numHumedad + "%");

  //motor = String(servo.write(90));

  //Conversión a char
  char tempChar[50] = "";
  char humidChar[50] = "";
  char flatChar[50] = "";
  char flonChar[50] = "";
  //char motorChar[50] = "";
  numTemperatura.toCharArray(tempChar, 50);
  numHumedad.toCharArray(humidChar, 50);
  numFlat.toCharArray(flatChar, 50);
  numFlon.toCharArray(flonChar, 50);
  //motor.toCharArray(motorChar, 50);
  
  char msgChar[50] = "";
  msgTempAndHumidity.toCharArray(msgChar, 50);  

  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  unsigned long now = millis();

  if (now - lastMsg > 2000) {
    lastMsg = now;
    Serial.println(WiFi.localIP());
    Serial.print("Publish message: ");
    Serial.println("");
    snprintf(msg, MSG_BUFFER_SIZE, "%s,%s,%s,%s", tempChar, humidChar, flatChar, flonChar);
    
    //Se publican mensajes al topico.
    client.publish("tic/iot", msg);
  }    
  
  Serial.println("Ricardo Gloria y Sandra Alvarez");
  //Supervición de datos de sensores en serie
  Serial.println("Temperatura: " + numTemperatura + "°C");
  Serial.println("Humedad: " + numHumedad + "%");

  Serial.println("");

  //Mandar mensaje dependiendo de la temperatura y mover el servomotor
  if(data.temperature >= 30 && data.temperature <= 45) {
    Serial.println("Caliente");
    servo.write(0);
    servo.write(90);
  } 
  
  if(data.temperature > 15 && data.temperature < 30) {
    Serial.println("Normal");
    servo.write(0);
  } 

  if(data.temperature >= 0 && data.temperature <= 15) {
    Serial.println("Frio");
    servo.write(0);
    servo.write(-90);
  }
  
  //Enviar mesaje al display mediante el botón si este lee en alto.
  boton = digitalRead(pinBoton);
  /*if (boton == HIGH) {
    digitalWrite(LED, HIGH);   //Se 0enciende el LED.
    Serial.println(msgTempAndHumidity);   //Envia el mensaje
    message_to_whatsapp(msgFlatAndFlon);
    message_to_whatsapp(msgTempAndHumidity); 

    //Mandar mensaje dependiendo de la temperatura y mover el servomotor
    if(data.temperature >= 30 && data.temperature <= 45) {
      message_to_whatsapp("En el lugar que se encuentra esta: Caliente");
    } 
  
    if(data.temperature > 15 && data.temperature < 30) {
      message_to_whatsapp("En el lugar que se encuentra esta: Normal");
    } 

    if(data.temperature >= 0 && data.temperature <= 15) {
      message_to_whatsapp("En el lugar que se encuentra esta: Frio");
    }
    
    delay(6000);
    digitalWrite(LED, LOW);    //Se apaga el LED.    
  } */

  //Demora un segundo para leer la proxima vez
  delay(10000);
}

void  message_to_whatsapp(String message) {  // Función definida por el usuario para enviar mensajes a la aplicación WhatsApp
  // Agregando todos los números, su clave API, su mensaje en una URL completa
  url = "https://api.callmebot.com/whatsapp.php?phone=" + phone_number + "&apikey=" + apiKey + "&text=" + urlencode(message);

  postData(); // Llamando a postData() para ejecutar la URL generada anteriormente una vez para que reciba un mensaje
}

void postData() { // Función de definición de usuario utilizada para llamar la API (datos POST).
  int httpCode; // Variable utilizada para obtener el código HTTP de respuesta después de llamar la API. 
  HTTPClient http; // Declarar objeto de la clase HTTPClient
  http.begin(url); // Comience el objeto HTTPClient con la URL generada 
  httpCode = http.POST(url); // Finalmente, publique la URL con esta función y almacenará el código HTTP
  
  if (httpCode == 200) { // Compruebe si el código de respuesta es de 200.
    Serial.println("Sent ok."); // Imprimir mensaje: Sent ok
  } else { // Si el código HTTP de respuesta no es 200, significa que hay algún error.
    Serial.println("Error."); // Imprimir mensaje: Error.
  }

  http.end();     // Después de llamar a la API, finalice el objeto de cliente HTTP.
}

String urlencode(String str) {      // Función utilizada para codificar la URL
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
    
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);

    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0=c - 10 + 'A';
      }
      code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
      //encodedString += code2;
    }
    yield();
  }
  return encodedString;
}
