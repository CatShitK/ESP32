#include <Arduino.h>
#include "Audio.h"
#include <WiFi.h>
#include <HTTPClient.h>
// //测试SD卡
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <FSImpl.h>
#include <vfs_api.h>
#include "WiFiMulti.h"
#define SCK  18
#define MISO  19
#define MOSI  23
#define CS  5

#define I2S_DOUT      0
#define I2S_BCLK      2
#define I2S_LRC       15

Audio audio;

WiFiMulti wifiMulti;
// put function declarations here:

const int dataPin = 1;

HTTPClient http;
#define DHTTYPE DHT11
const char* ssid = "败家之眼";
const char* password = "Tgs200410";

// const char* ssid = "ASUS";
// const char* password = "12345555";

 #define HttpGet "http://192.168.31.225:3000/download"
//#define HttpGet "http://mp3.ffh.de/radioffh/hqlivestream.mp3"

SPIClass spi = SPIClass(VSPI);

/******容量转换*******/ 
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}
  /******查看SD卡文件列表*******/  
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);
  
    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }
  
    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}
/******SD卡创建文件夹*******/  
void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}
  /******删除SD卡中的文件夹*******/ 
void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}
  /******读取SD卡中的文件*******/ 
void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);
  
    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }
  
    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}
    /******向SD卡中的文件写数据*******/ 
void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);
  
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}
/******向SD卡中的文件中添加数据*******/  
void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);
  
    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}
      /******给SD卡中的文件重命名*******/ 
void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}
     /******删除SD卡中的文件*******/  
void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}
      /******测试SD卡读写*******/ 
void testFileIO(fs::FS &fs, const char * path){
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }
   
    file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
  
    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}
//访问http获取文件并下载到SD卡中
void downloadMp3FileToSD(const char* url, const char* filename) {
  HTTPClient http;
  http.begin(url);

  int httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      File file = SD.open(filename, FILE_WRITE);
      if (file) {
        http.writeToStream(&file);
        file.close();
        Serial.printf("MP3 文件 '%s' 已保存到 SD 卡\n", filename);
      } else {
        Serial.printf("无法打开文件 '%s'\n", filename);
      }
    } else {
      Serial.printf("HTTP 请求失败, 错误代码: %d\n", httpCode);
    }
  } else {
    Serial.printf("HTTP 请求失败, 错误: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}
 
 u8_t wifiFlag = 0;

void setup()
{
  // 开始串行通信
  Serial.begin(115200);

    //wifi连接
    WiFi.mode(WIFI_STA);
    wifiMulti.addAP(ssid, password);
    wifiMulti.run();
    if(WiFi.status() != WL_CONNECTED){
        WiFi.disconnect(true);
        wifiMulti.run();
        Serial.println("WifiNoconnected");
    }
    else
    {
       Serial.println("Wificonnected");
       wifiFlag = 1;
    }  
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(2); // 0...21    控制音量

  if(wifiFlag==1){
  Serial.println("123");
  audio.connecttohost(HttpGet); //  128k mp
  //速度太慢不能直接下载播放，需要先播放再下载
  audio.connecttoFS(SD, "/file.mp3");
  Serial.println("下载完毕的SD卡文件列表：");
  listDir(SD, "/", 0);
  }

}

void loop()
{

  audio.loop();

}

// put function definitions here:

