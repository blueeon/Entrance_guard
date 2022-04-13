# 基于Arduino和esp32cam的无线智能门禁
## Features
1. 支持四种开门方式：
   1. 刷卡开门(NFC读写)
   2. 二维码扫描开门（摄像头读和识别）
   3. 远程开门（基于MQTT）
   4. 蓝牙开门(基于BLE)
2. 支持远程管理：
   1. 基于MQTT协议
   2. 设备联网后自动注册
   3. 远程下发二维码、IC卡卡号
   4. 事件上报：日志上报
3. 对接腾讯云IOT平台
4. 传感器控制：
   1. 开门亮绿灯
   2. 关门亮红灯
   3. 控制一个继电器，继电器控制锁
   4. 操作蜂鸣

## 架构
### 硬件部分
#### 材料
1. 开发板
   1. Arduino UNO r3开发板，做为主控板
   2. Esp32-cam，提供网络和二维码读取
   3. RFID-RC522，读写NFC
2. 传感器
   1. 红色led * 1：表示门禁的关状态
   2. 绿色led * 1：表示门禁的开状态
   3. 有源蜂鸣器 * 1：声音提醒
   4. RFID-RC522 * 1 ：用作IC卡卡号读取
   5. 继电器 * 1：用作开合电磁锁
   6. 200Ω电阻 * 2
#### 接线
|模块A|pinA|pinB|模块B
|---|---|---|---|
|rc522|RST|9|uno|
|rc522|SDA|10|uno|
|rc522|MOSI|11|uno|
|rc522|MISO|12|uno|
|rc522|3.3v|3.3v|uno|
|rc522|GND|GND|uno|
||||
|esp32-cam|5v|5v|uno|
|esp32-cam|GND|GND|uno|
|esp32-cam|UOR|2|uno|
|esp32-cam|UOT|3|uno|
||||
|red-led|3.3v|5|uno|
|red-led|GND|GND|uno|
|green-led|3.3v|4|uno|
|green-led|GND|GND|uno|
|蜂鸣器|3.3v|6|uno|
|蜂鸣器|GND|GND|uno|
|继电器| + | 5v |uno|
|继电器| - | - |uno|
|继电器| s | - |uno|

### 软件部分
#### UNO主控
工程：`Entrance_guard`

主控负责：
1. 完成设备初始化，并蜂鸣
2. 接收RFID读到的卡号，验证，控制开门，5秒后关门
3. 接收esp32发过来的二维码，验证，控制开门，5秒后关门
4. 接收esp32发过来的mqtt消息：
   1. 远程开门
   3. 存储下发的RFID卡号操作
   4. 存储下发的二维码操作
   6. 重启
   7. ota（TODO）
5. 开门后发送事件，通过esp32发给云端

#### esp32-cam模组
工程：`Entrance_guard_esp32-cam`

Esp32模组负责：
1. 初始化wifi和mqtt，并返回注册结果给主控
2. 读取二维码，并返回给主控
3. 接收Mqtt消息，并返回给主控
4. 接收主控的Mqtt消息，发送给腾讯云

### 模块间串口通信
ESP模块和UNO通过UART进行通信，波特率`115200`。每个指令包装在一个json结构中。

用`ArduinoJson`实现串口之间的json数据包。数据结构定义：
```json
{
    "sensor":"esp32-cam", //发送方
    "send_time":12312312312,
    "data":"XXXX",
    
}
```
## 主控过程
SETUP:
1. 设置UNO的各个端口
2. 启动RFC读卡器
3. 等待EPS模块就绪
4. 对时

Loop：

1. 监听并处理：是否有刷卡动作
2. 监听串口（serialEvent）并路由，ESP模块的串口是否有消息需要处理
   1. 收到扫码开门指令：调用扫码开门逻辑
   2. 收到远程开门指令：调用远程开门
   3. 收到其他...
3. delay(200)