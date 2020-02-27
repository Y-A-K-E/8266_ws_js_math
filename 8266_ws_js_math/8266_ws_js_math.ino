/*
 Y.A.K.E
 http://www.yge.me
 2020/02/27
 MIT
*/


#include <ESP8266WebServer.h>
#include <FS.h>
#include <WebSocketsServer.h>


#include <ESP8266WiFi.h>

#include <ESP8266HTTPClient.h>

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
HTTPClient http;

//路由WIFI/密码
String wifi_ssid = "openwrt_546659";
String wifi_pass = "012345678900aabb";

//时间差
unsigned long time1;   //运算前
unsigned long time2;   //运算后
unsigned long time3;   //运算总时间

#define ALL_TIME (time2-time1);


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  
	String out_str = "";
	
	switch(type) {
		case WStype_DISCONNECTED: {
			out_str = "disconnect";
		};break;
		case WStype_CONNECTED: {
			out_str = "connect : " + num ;
		}; break;
		case WStype_TEXT: {


      //接受到网页客户端返回的数据时间点2
      time2 = millis(); 
      Serial.print("time2:");
      Serial.println(String(" ") + time2 );
      
      //总时间
      Serial.print("ALL_time:");
      time3 = ALL_TIME;
      Serial.println(String(" ") + time3 );
      
      //打印结果
			out_str = String("Result : ") + (char *)&payload[0];
      
		};break;
		case WStype_BIN: {
			out_str = "error type";
		};break;
	}

  //串口输出
  Serial.println(out_str);
  
  Serial.println("-----------------------");
  Serial.println();


}


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

String getContentType(String filename) {
  if (server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}

bool handleFileRead(String path) {

  if (path.endsWith("/")) {
    path += "index.htm";
  }
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {

    
    if (SPIFFS.exists(pathWithGz)) {
      path += ".gz";
    }
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileList() {
  if (!server.hasArg("dir")) {
    server.send(500, "text/plain", "BAD ARGS");
    return;
  }

  String path = server.arg("dir");

  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while (dir.next()) {
    File entry = dir.openFile("r");
    if (output != "[") {
      output += ',';
    }
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir) ? "dir" : "file";
    output += "\",\"name\":\"";
    if (entry.name()[0] == '/') {
      output += &(entry.name()[1]);
    } else {
      output += entry.name();
    }
    output += "\"}";
    entry.close();
  }

  output += "]";
  server.send(200, "text/json", output);
}


void init_web() {


    //调试模式多一个链接,查看文件列表

    server.on("/list", HTTP_GET, handleFileList);
   

   
    server.onNotFound([]() {
      if (!handleFileRead(server.uri())) {
        server.send(404, "text/plain", "FileNotFound");
      }
    });   


    
    server.begin(); 

}

void setup(){
  Serial.begin(115200);

  
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  for(uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
    Serial.flush();      
    delay(1000);
  }



    //调试输出

      //输出IPv4 
      Serial.print("Connected! IPv4: ");
      Serial.println(WiFi.localIP());
    



  
  init_web();



  
  if (SPIFFS.begin());
  {
    Dir dir = SPIFFS.openDir("/");
    
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();      
      //调试输出
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());     
    }
  }
      
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);


    http.setReuse(true);
  
}


void read_serial(){
  
  if (Serial.available() > 0)
  {
    String all_input = "";
    while (Serial.available() > 0)
    {
      all_input += char(Serial.read());
      delay(2);//延时等待响应
    }
    if (all_input.length() > 0)
    {
      time1 = millis(); //串口收到数据起为时间1
      Serial.print("user input:");
      Serial.println(all_input);
      Serial.print("time1:");
      Serial.println(String(" ") + time1 );

      //将串口数据推送给所有客户端
      webSocket.broadcastTXT(all_input);
    }
  }
}

void loop(){
  server.handleClient();  
  webSocket.loop();
	//取串口数学公式/
	//然后马上反馈给所有的websocket链接的客户端.
  read_serial();
}
