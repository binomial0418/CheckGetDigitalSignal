# CheckGetDigitalSignal
with wifi manager

0.請先在開發板上傳並執行一次uploadFileToFS.ino，建立需要的檔案。<br>
1.使用ESP8266（ESP-12F，CH9102X），MAC版驅動請連結https://github.com/Xinyuan-LilyGO/LilyGo-T-Call-SIM800/files/7037260/CH9102_Mac_Driver.zip 。<br> 
2.含WIFI設定介面，flash按三下可進入AP模式，請連接名稱『ESP-****』的AP後開啟192.168.4.1進行WIFI設定。<br> 
3.程式中可對接收器狀態傳送LINE通知，請自行申請token替換。<br> 
4.額外函式庫：<br> 
  ArduinoJson：內建程式庫管理員中下載。<br>
  ESPAsyncTCP：內建程式庫管理員中下載。<br>
  ESPAsyncWebServer：內建程式庫管理員中下載。<br>
  EasyButton：內建程式庫管理員中下載。<br>
  TridentTD_LineNotify內建程式庫管理員中下載。<br>
5.本程式將digital signal輸入至D7腳位，僅簡單判斷D7腳位的訊號作判斷，其餘需求自行替換掉感應器模組即可。  
