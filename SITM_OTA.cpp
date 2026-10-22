#include "SITM_Sistema.h"

SITM_OTA redSITM;

void SITM_OTA::conectarWiFi() {
    IPAddress ipLocal(192, 168, 1, 203);      
    IPAddress gateway(192, 168, 1, 1);        
    IPAddress subnet(255, 255, 255, 0);       
    IPAddress primaryDNS(8, 8, 8, 8);         

    WiFi.config(ipLocal, gateway, subnet, primaryDNS);
    WiFi.begin("CLTECH", "cltech*123");

    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void SITM_OTA::initOTAServer() {
    server.on("/upload", HTTP_GET, [this]() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", serverIndex); 
    });

    server.on("/update", HTTP_POST, [this]() {
        server.sendHeader("Connection", "close");
        if (Update.hasError()) server.send(500, "text/plain", "FALLO");
        else {
            server.send(200, "text/plain", "OK - REINICIANDO");
            delay(1000);
            ESP.restart();
        } 
    }, [this]() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) Update.begin(UPDATE_SIZE_UNKNOWN);
        else if (upload.status == UPLOAD_FILE_WRITE) Update.write(upload.buf, upload.currentSize);
        else if (upload.status == UPLOAD_FILE_END) Update.end(true);
    });
    server.begin();
}

void SITM_OTA::init() {
    conectarWiFi();
    if (WiFi.status() == WL_CONNECTED) {
        initOTAServer();
        tcpServer.begin();
    }
}

void SITM_OTA::aceptarNuevasConexiones() {
    if (tcpServer.hasClient()) {
        WiFiClient nuevoCliente = tcpServer.accept();
        int slotLibre = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!tcpClients[i] || !tcpClients[i].connected()) { slotLibre = i; break; }
        }
        if (slotLibre != -1) {
            tcpClients[slotLibre] = nuevoCliente;
            clientCount++;
            delay(100); 
            tcpClients[slotLibre].print("\r\n=== SITM V2 CNC LOG ===\r\n> ");
            tcpClients[slotLibre].flush(); 
        } else nuevoCliente.stop();
    }
}

void SITM_OTA::limpiarClientesDesconectados() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (tcpClients[i] && !tcpClients[i].connected()) {
            tcpClients[i].stop();
            clientCount--;
        }
    }
}

void SITM_OTA::loop() {
    if (WiFi.status() == WL_CONNECTED) {
        server.handleClient();
        aceptarNuevasConexiones();
        limpiarClientesDesconectados();
    }
}

String SITM_OTA::leerComandos() {
    if (Serial.available()) return Serial.readStringUntil('\n');
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (tcpClients[i] && tcpClients[i].connected() && tcpClients[i].available()) {
            return tcpClients[i].readStringUntil('\n');
        }
    }
    return "";
}

void SITM_OTA::print(String msg) {
    Serial.print(msg);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (tcpClients[i] && tcpClients[i].connected()) {
            tcpClients[i].print(msg);
            tcpClients[i].flush();
        }
    }
}

void SITM_OTA::println(String msg) {
    //Serial.println(msg);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (tcpClients[i] && tcpClients[i].connected()) {
            tcpClients[i].print(msg + "\r\n");
            tcpClients[i].flush();
        }
    }
}