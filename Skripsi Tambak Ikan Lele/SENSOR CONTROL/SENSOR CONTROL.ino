#include <EEPROM.h>             // Library EEPROM Arduino
#include "GravityTDS.h"         // Library Sensor TDS
#include <Wire.h>               // Library komunikasi arduino
#include <LiquidCrystal_I2C.h>  // Library LCD Arduino
#include "LowPower.h"           // Library konsumsi daya arduino
#include <DallasTemperature.h>  // 
#include <OneWire.h>            // 

#define TdsSensorPin A0     // sensor tds pin analog nomor 0 arduino uno
#define TRIGGER_PIN 3       // pin trigger ultrasonik pin digital nomor 3 arduino uno
#define ECHO_PIN 2          // pin echo ultrasonik pin digital nomor 2 arduino uno
#define indikator_bahaya 4  // pin led merah  (indikator bahaya) pin digital nomor 4 arduino uno
#define indikator_sedang 5  // pin led kuning (indikator sedang) pin digital nomor 5 arduino uno
#define indikator_aman 6    // pin led hijau  (indikator aman) pin digital nomor 6 arduino uno
#define buzzer 7            // pin buzzer pin digital nomor 7 arduino uno
#define ONE_WIRE_BUS 8

GravityTDS gravityTds;                 // inisialisasi variabel untuk sensor TDS (total disolved sensor)
LiquidCrystal_I2C lcd(0x27, 16, 2);    // inisialisasi variabel untuk lcd 16 x 2 I2C
OneWire oneWire(ONE_WIRE_BUS);         // 
DallasTemperature sensors(&oneWire);   // 

float tdsValue = 0;                    // buat variabel dengan tipe data float untuk set nilai suhu dan nutrisi
const int measureLimit = 200;          // jarak maksimum sensor ultrasonik 200 cm
float tempC = 0;

int durasi  = 0;           // buat variabel integer untuk mengatur durasi pada sensor ultrasonik
int jarak   = 0;           // buat variabel integer untuk mengatur jarak pada sensor ultrasonik
int nutrisi_rendah = 300;  // setpoint untuk kadar nutrisi rendah
int nutrisi_tinggi = 500;  // setpoint untuk kadar nutrisi tinggi

// fungsi untuk mereset otomatis
void (*resetFunc)(void) = 0;

// bagian awal program untuk inisialisasi input dan output sistem
void setup() {
  Serial.begin(9600);  // serial komunikasi dengan kecepatan 9600
  Serial.println("Elektronik catfish...");

  // konfigurasi modul LCD
  lcd.init();
  lcd.backlight();

  // tampilan awal lcd saat pertama kali menyala
  lcd.setCursor(0, 0);  // baris lcd bagian pertama
  lcd.print("E-CATFISH");
  lcd.setCursor(0, 1);  // baris lcd bagian kedua
  lcd.print("Tian Triyatna");

  // setting input dan output sistem
  // pin trigger sebagai output sistem
  pinMode(TRIGGER_PIN, OUTPUT);
  // pin echo sebagai input sistem
  pinMode(ECHO_PIN, INPUT);
  // pin led dan buzzer diatur sebagai keluran sistem
  pinMode(indikator_aman, OUTPUT);
  pinMode(indikator_sedang, OUTPUT);
  pinMode(indikator_bahaya, OUTPUT);
  pinMode(buzzer, OUTPUT);

  // setting inout dan output sensor tds
  sensors.begin();
  gravityTds.setPin(TdsSensorPin);
  // nilai voltage atau tegangan arduino yang dipakai pada sensor
  gravityTds.setAref(5.0);
  // nilai analog to digital conventer (adc) yang digunakan 10 bit dengan nilai range 0 - 1023
  gravityTds.setAdcRange(1024);
  gravityTds.begin();
  sensors.begin();

  // ketika sistem dinyalakan buzzer berbunyi selama 800 detik dan mati selama 800 detik
  digitalWrite(buzzer, HIGH);
  delay(800);
  digitalWrite(buzzer, LOW);
  delay(800);

  // bersihkan tampilan lcd
  lcd.clear();
}

// bagian utama sistem yang akan diproses secara berulang
void loop() {
  // buat variabl untuk meminta data ke microcontroller esp8226
  String minta = "";
  // jika data yang diterima lebih dari 0
  while (Serial.available() > 0) {
    // maka data akan ditambah dan dibaca oleh sistem
    minta += char(Serial.read());
  }

  // hapus spasi pada data
  minta.trim();
  // jika microcontroler arduino menerima data ya
  // maka arduino uno akan mengirim data sensor ke esp8226
  if (minta == "Ya") {
    // kirim datanya
    kirimdata(); //  fungsi untuk mengirimkan data sensor 
    AutoReset(); //  fungsi reset sistem otomatis 
  }
  // kosongkan variabel minta saat data nya sudah dikirimkan 
  minta = "";
  // fungsi untuk pengambilan keputusan pada saat nilai nutrisi diukur
  decision();
  // delay selama 1 detik 
  delay(1000);
}

// fungsi untuk reset otomatis (softreset)
void AutoReset() {
  // jika clock pada arduino mencapai nilai 3600000 atau sama dengan 1 jam 
  // maka sistem akan direset dan dinyalakan kembali 
  if (millis() >= 3600000) {
    // fungsi untuk softreset
    resetFunc();

    // pada saat reset sistem akan memasuki mode sleep selama 8 seconds
    // dan aan dinyalakan kembali saat proses soft reset selesai  
    LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF,
                  SPI_OFF, USART0_OFF, TWI_OFF);
  }
}

// bagian untuk fungsi pengambilan keputusan 
void decision() {
  // set nilai kalibrasi sensor tds kemudian abaca nilai tds
  sensors.requestTemperatures(); 
  gravityTds.setTemperature(sensors.getTempCByIndex(0));
  gravityTds.update();
  
  tdsValue = gravityTds.getTdsValue();
  tempC    = sensors.getTempCByIndex(0);
 
  // kalibrasi untuk sensor ultrasonik
  // pin trigger dibuat low selama 2 ms dan buat menyala selama 10 ms
  // kemudian kembali di low kan
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  // perhitungan durasi menggunakan pulse in 
  // pada saat pin trig mengenai objek benda 
  // kemudian  pin echo menerima hasil pemantulan gelombang tersebut 
  // dengan rumus durasi : 2 kali pemantulan kemudian : dengan nilai 29.1
  // nilai jarak yang sudah diketahui dibulatkan dengan nilai jarak maksimum
  durasi = pulseIn(ECHO_PIN, HIGH);
  jarak = (durasi / 2) / 29.1;
  jarak = constrain(jarak, 0, measureLimit);
  
  // tampilan lcd untuk menampilkan nilai sensor
  lcd.setCursor(0, 0); // baris pertama kolom 0
  lcd.print("N :" + String(tdsValue, 0));
  lcd.setCursor(7, 0); // baris pertama kolom 7
  lcd.print("D :" + String(jarak));
  lcd.setCursor(0, 1); // baris kedua kolom 1
  lcd.print("T :" + String(tempC));

  // Role Decision
  // jika nilai tds lebih besar dari setpoint nilai nutrisi tinggi
  // maka led merah menyala 10 x dan buzzer berbunyi
  if (tdsValue > nutrisi_tinggi) {
    digitalWrite(buzzer, HIGH);
    digitalWrite(indikator_aman, LOW);   // led hijau mati
    digitalWrite(indikator_sedang, LOW); // led kuning mati

    for (int i = 0; i < 10; i++) {
      digitalWrite(indikator_bahaya, HIGH);
      delay(500);
      digitalWrite(indikator_bahaya, LOW);
      delay(500);
    }
  } 
  // jika tidak nilai tds lebih kecil dari nilai nutrisi rendah maka
  // buzzer berhenti berbunyi led merah dan kuning mati
  else if (tdsValue < nutrisi_rendah) {
    digitalWrite(buzzer, LOW);
    digitalWrite(indikator_bahaya, LOW);
    digitalWrite(indikator_sedang, LOW);
    digitalWrite(indikator_aman, HIGH);
  } 
  // jika nilai tds lebih besar dari nutrisi rendah dan tds lebih kecil dari nutrisi tinggi
  // maka kedua nilai tersebut akan dibandingkan 
  // buzzer berhenti berbunyi dan led merah serta hijau mati 
  // led kuning menyala 
  else if (tdsValue > nutrisi_rendah || tdsValue < nutrisi_tinggi) {
    digitalWrite(buzzer, LOW);
    digitalWrite(indikator_bahaya, LOW);
    digitalWrite(indikator_sedang, HIGH);
    digitalWrite(indikator_aman, LOW);
  }
}

// fungsi untuk mengirim data sensor ke esp8226
void kirimdata() {
  String datakirim = String(tdsValue, 0) + "#" + String(jarak) + "#" + String(tempC);
  Serial.println(datakirim);
}