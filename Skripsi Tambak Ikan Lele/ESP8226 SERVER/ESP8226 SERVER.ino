// import Library yang digunakan
#include <SoftwareSerial.h>  // Library Untuk Komunikasi Serial 
#include <ESP8266WiFi.h>     // Library Untuk Terhubung  dengan WiFI
#include <ThingerESP8266.h>  // Library untuk Terhubung  dengan Platform Thinger Io

// buat variabel untuk software serial (Rx, Tx)
// D6 (RX:12)
// D7 (TX:13)
SoftwareSerial DataSerial(12, 13);

// Konfigurasi Thinger Io
#define USERNAME          "TIAN"              // username akun thingger io
#define DEVICE_ID         "tambak_lele"       // alamat perangkat keras yang terdafar pada aplikasi
#define DEVICE_CREDENTIAL "51Cm&M%WFH7zecaj"  // API untuk menghubungkan perangkat keras dan lunak

// Variabel untuk indikator sistem yang terhubung
 // pin D2 ESP8226
#define LED_PIN 4 

// variabel untuk thingerIo
ThingerESP8266 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

// Konfigurasi WiFi ESP8226
#define SSID "sobat misqueen masuk sini hiks.." // username wifi handphone atau access point  
#define SSID_PASSWORD "EMPATKATAHURUFKECIL"     // password wifi handphone atau access point 

// sediakan variabel untuk menampung nilai dari node Sensor
// dikirim ke ThingerIO
float tds, jarak, suhu;

// millis sebagai pegganti delay
unsigned long previousMillis = 0;
const long interval = 3000;

// variabel array untuk data parsing
String arrData[3];

void setup() {
  Serial.begin(9600);       // Baudrate HardSerial 9600
  DataSerial.begin(9600);   // Baudrate SoftSerial 9600
  pinMode(LED_PIN, OUTPUT); // Led pin sebagai Output

  // Koneksi ke WiFi dan Cek koneksi
  WiFi.begin(SSID, SSID_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    digitalWrite(LED_PIN, LOW);
  }

  // apabila terkoneksi
  digitalWrite(LED_PIN,  HIGH);
  delay(500);

  // hubungkan nodeMCU ke ThingerIO
  thing.add_wifi(SSID, SSID_PASSWORD);

  // data yang akan dikirim
  thing["dashboard_lele"] >> [] (pson & out)
  {
    out["tds"]    = tds;
    out["jarak"]  = jarak;
    out["suhu"]   = suhu;
  };
}

void loop() {
  // konfigurasi millis
  unsigned long currentMillis = millis(); // baca waktu saat ini
  if (currentMillis - previousMillis >= interval)
  {
    // update previousMillis
    previousMillis = currentMillis;

    // prioritaskan pembacaan data dari arduino uno
    // baca data serial
    String data = "";
    while (DataSerial.available() > 0)
    {
      data += char(DataSerial.read());
    }
    // buang spasi datanya
    data.trim();

    // uji data
    if (data != "")
    {
      // parsing data (pecah data)
      int index = 0;
      for (int i = 0; i <= data.length(); i++)
      {
        char delimiter = '#';
        if (data[i] != delimiter)
          arrData[index] += data[i];
        else
          index++; // variabel index bertambah 1
      }

      // pastikan bahwa data yang dikirim lengkap
      if (index == 2)
      {
        // tampilkan nilai sensor ke serial monitor
        Serial.println("TDS  : " + arrData[0]); // TDS
        Serial.println("DIST : " + arrData[1]); // DIST
        Serial.println("TEMP : " + arrData[2]); // TEMP
        Serial.println();

        // isi variabel data yang akan dikrim
        tds   = arrData[0].toInt();
        jarak = arrData[1].toInt();
        suhu  = arrData[2].toFloat();

        // thing.write_bucket("Hasil_penelitian", "Node Gateway1");

        // push data ke platform Thinger Io
        thing.handle();
      }

      arrData[0] = "";
      arrData[1] = "";
      arrData[2] = "";
    }
    // minta data ke arduino
    DataSerial.println("Ya");
  }
}
