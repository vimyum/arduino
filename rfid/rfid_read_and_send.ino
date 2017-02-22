/**
 * ほぼ下記URLのコードそのままです。
 * HTTP通信処理のみ追加しました。
 *  http://qiita.com/daxanya1/items/b9c2b971a946c8bf063e
 */
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define RST_PIN 16
#define SS_PIN  5

MFRC522 mfrc522(SS_PIN, RST_PIN); // MFRC522のインスタンスを作成

#define CARD_PRESENT_PIN 4 // MFRC522にカードが検知されたら光らせるGPIO NO
bool cardset;     // MFRC522にカードが検知されているかどうか
int timeoutcount; // MFRC522からカードが連続で離れた回数を記録

const char* ssid = "XXX";         // ESP-WROOM-02用 Wifi SSID(各自設定してください)
const char* password = "XXX"; // ESP-WROOM-02用 Wifi パスワード(各自設定してください)

void setup() {
  // UART接続初期化
  Serial.begin(115200); // UARTの通信速度を変更したい場合はこちらを変更
  delay(10);

  // カード検出用のGPIO初期化
  pinMode(CARD_PRESENT_PIN, OUTPUT);
  digitalWrite(CARD_PRESENT_PIN, LOW);

  // MFRC522用変数初期化
  cardset = false;
  timeoutcount = 0;

  // MFRC522のスケッチのDumpInfoを参考にした。ref) https://github.com/miguelbalboa/rfid
  SPI.begin();          // SPI初期化
  mfrc522.PCD_Init();   // MFRC522初期化
  ShowReaderDetails();  // MFRC522 Card Readerのバージョンを返す。00かFFなら配線間違いの可能性

  // Wifiにつなぐ
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Print the IP address
  Serial.println(WiFi.localIP());
}

void loop() {
  // カードの検出
  if (!mfrc522.PICC_IsNewCardPresent()) {
    // 検出されなかった場合の処理
    if (cardset) {
      // すでにカードが検出されていた場合で、連続４回未検出になったらLEDをLOWにする
      // 検出後、タイムアウトと検出を繰り返すのでその対策 
      if (timeoutcount > 4) {
        digitalWrite(CARD_PRESENT_PIN, LOW);
        Serial.println("LED LOW");
        cardset = false;
        timeoutcount = 0;
      } else {
        // ４回以内なら連続未検出回数を増やす
        timeoutcount++;
      }
    }
    delay(5);
    return;
  }

  // カードが初めてor改めてセットされた場合、LEDをHIGHにする
  if (!cardset) {
    // if(mfrc522.PICC_ReadCardSerial()
    if (mfrc522.PICC_ReadCardSerial()) {
      mfrc522.PICC_DumpToSerial(&(mfrc522.uid));

      /**
       * ここから流用
       * http://qiita.com/yuji_miyano/items/f00f9e3f2edc8025b704
       */
      String strBuf[mfrc522.uid.size];
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        strBuf[i] =  String(mfrc522.uid.uidByte[i], HEX);  // (E)using a constant integer
        if(strBuf[i].length() == 1){  // 1桁の場合は先頭に0を追加
          strBuf[i] = "0" + strBuf[i];
        }
      }
      String strUID = strBuf[0] + strBuf[1] + strBuf[2] + strBuf[3] + strBuf[4] + strBuf[5] + strBuf[6];
      Serial.println("UID:" + strUID);
      /* ここまで */

      /**
       * ここから追加
       */
      HTTPClient http;
      Serial.println("[HTTP] begin...\n");
      http.begin("http://XX.XX.XX.XX:3000/" + strUID); // UIDの送信先のサーバのアドレスを設定してください。
      int httpCode = http.GET();
      Serial.println("[HTTP] StatusCode: " + httpCode);
      http.end();
      /* ここまで */
    }
    
    digitalWrite(CARD_PRESENT_PIN, HIGH);
    Serial.println("LED HIGH");
    cardset = true;
  }
  timeoutcount = 0;
  delay(100);
}

 // MFRC522のスケッチのDumpInfoのまま。ref) https://github.com/miguelbalboa/rfid
 // LICENSE:https://github.com/miguelbalboa/rfid/blob/master/UNLICENSE
void ShowReaderDetails() {
  // Get the MFRC522 software version
  int v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (unknown)"));
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
  }
}
