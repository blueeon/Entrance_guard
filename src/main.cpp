#include <Arduino.h>
#include <SPI.h>         //RFID库文件
#include <MFRC522.h>     //RFID库文件
#include <ArduinoJson.h> //json
#include <SoftwareSerial.h>
/**
 * @brief 定义UNO主控板引脚
 *
 */
// RC522
#define RC522_RST_PIN 9 // RFID 的RST引脚
#define RC522_SS_PIN 10 // RFID 的SDA(SS)引脚
// ESP320CAM
#define ESP_UOR_PIN 2 // ESP320CAM 的读串口引脚
#define ESP_UOT_PIN 3 // ESP320CAM 的写串口引脚
//红绿灯
#define LED_RED 5   //红灯正极
#define LED_GREEN 4 //绿灯正极
//蜂鸣器
#define BEEP_PIN 6       //蜂鸣器正极引脚
#define BUZZER_CHANNEL 0 //
//继电器
#define RELAY_PIN 6 //继电器引脚

int DOOR_OPEN_DELAY = 5000; //门开了后多长时间关闭？

MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN); // Create MFRC522 instance.
MFRC522::MIFARE_Key key;
boolean g_boolSuccess = false; //刷卡成功标识

// ESP32软串口
SoftwareSerial esp32Serial(ESP_UOR_PIN, ESP_UOT_PIN); // RX, TX

/**
 * @brief 存储IC卡卡号
 *
 */
String CardInfo[4][2] = {
    {"532e8611", "阿强"},
    {"01048689", "lisi"},
    {"b076b156", "yahboom"},
    {"a075f1a2", "xiaojie"},
};

int MaxNum = 4; //卡数量
/**
 * @brief 上报开关门的事件
 *
 */
void doorEvent(char *action, char *content = NULL)
{
}
/**
 * @brief 蜂鸣控制
 *
 * @param time 响几声
 * @param duration 每声响多久
 * @param rest 每声中间停多久
 */
void beep(int time, int duration, int rest = 100)
{

  for (int i = 0; i < time; i++)
  {
    digitalWrite(BEEP_PIN, HIGH); //打开蜂鸣器
    delay(duration);
    digitalWrite(BEEP_PIN, LOW); //关闭蜂鸣器
    delay(rest);
  }
}

/**
 * @brief 关门操作
 *
 */
void doorClose()
{
  //亮红灯，灭绿灯
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, LOW);
  //继电器关
  //长鸣一次
  beep(1, 200, 100);
  //上报事件
  char action[] = "doorClose";
  char content[] = "";
  doorEvent(action, content);
}

/**
 * @brief 开门操作
 *
 */
void doorOpen()
{
  //亮绿灯，灭红灯
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
  beep(3, 200, 100);
  //继电器开
  //门常开一会
  delay(DOOR_OPEN_DELAY);
  doorClose();
  //上报事件
  char action[] = "doorOpen";
  char content[] = "";

  doorEvent(action, content);
}

/**
 * @brief 操作失败警告
 *
 */
void warningBeep()
{

  //短鸣两次
  beep(1, 1000, 100);
}

/**
 * @brief 监听并处理刷卡动作
 *
 */
void listenCard()
{

  /* 寻找新的卡片*/
  if (!mfrc522.PICC_IsNewCardPresent())
    return;
  /* 选择一张卡片*/
  if (!mfrc522.PICC_ReadCardSerial())
    return;

  /* 显示PICC的信息，将卡的信息写入temp */
  String temp, str; //定义字符串temp,str
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    str = String(mfrc522.uid.uidByte[i], HEX); // 将数据转换成16进制的字符
    if (str.length() == 1)                     //保证str的长度有两位
    {
      str = "0" + str;
    }
    temp += str; //将字符str放入temp
  }
  Serial.println("Your card ID is :" + temp); //这里打开可以查看实际的卡，方便填写数组

  /* 将temp的信息与存储的卡信息库CardInfo[4][2]进行比对*/
  for (int i = 0; i < MaxNum; i++)
  {
    if (CardInfo[i][0] == temp) //如果在CardInfo[i][0]中比对到卡片的信息
    {
      // Serial.print(CardInfo[i][1] + " Open door!\n");
      Serial.println("Verified success : " + CardInfo[i][0] + "," + CardInfo[i][1]); //将卡的信息打印到串口
      g_boolSuccess = true;                                                          //刷卡成功标识
    }
  }
  if (g_boolSuccess == true) //如果刷卡成功
  {
    //执行开门操作
    doorOpen();
  }
  else //刷卡失败
  {
    Serial.println("Verified failed.");
    //报警操作
    warningBeep();
  }
  g_boolSuccess = false;     //刷卡失败标识
  mfrc522.PICC_HaltA();      //停止读写
  mfrc522.PCD_StopCrypto1(); //  停止向PCD加密
}

/**
 * @brief 处理QR读取结果
 *
 */
void listenQRCode()
{
}
void listenRemote()
{
}

String rev;

// DynamicJsonDocument espMsg(1024);

/**
 * @brief 监听ESP串口信息，一次处理一条
 *
 */
void listenEspSerial()
{

  if (esp32Serial.available())
  {
    rev = esp32Serial.readString();
    Serial.print(rev);
    // DeserializationError error = deserializeJson(espMsg, esp32Serial);
    // if (error)
    // {
    //   Serial.print("deserializeJson() failed: ");
    //   Serial.println(error.f_str());
    //   return;
    // }
    // serializeJsonPretty(espMsg, Serial);
    // Serial.println();
  }
}
/**
 * @brief 初始化
 *
 */
void setup()
{
  Serial.begin(9600);
  pinMode(BEEP_PIN, OUTPUT);  //初始化蜂鸣器为输出模式
  pinMode(LED_RED, OUTPUT);   //初始化LED为输出模式
  pinMode(LED_GREEN, OUTPUT); //初始化LED为输出模式
  pinMode(LED_BUILTIN, OUTPUT);
  //读取ESP模块串口
  esp32Serial.begin(115200);

  // 如果没有打开串行端口，就什么也不做
  while (!Serial)
    ;
  SPI.begin();        //初始化SPI
  mfrc522.PCD_Init(); // 初始化 MFRC522

  digitalWrite(BEEP_PIN, LOW);    //关闭蜂鸣器
  digitalWrite(LED_RED, HIGH);    //打开红LED
  digitalWrite(LED_GREEN, LOW);   // 关闭绿LED
  digitalWrite(LED_BUILTIN, LOW); //打开主板自带灯
  Serial.println("UNO:im ready");
}
void loop()
{
  // listenCard();
  listenEspSerial();
  // listenRemote();
  delay(1);
}