#include "SITM_Sistema.h"

// ==============================================================================
// INSTANCIACIÓN DE MÓDULOS GLOBALES (Memoria Principal)
// ==============================================================================
SITM_CNC estacionCNC;
SITM_Rutinas controlRutinas;
//SITM_Minibanda minibanda; // Creada aquí para satisfacer al 'extern' del .h
//SITM_OTA redSITM;         // Creada aquí para satisfacer al 'extern' del .h

// ==============================================================================
// SETUP: ARRANQUE DEL SISTEMA Y PROTECCIÓN MECÁNICA
// ==============================================================================
void setup() {
    Serial.begin(115200);
    //Serial.println("=======================================================");
    //Serial.println("  SITM V2.0 - NUCLEO CNC + BANDA CONTINUA (Lazo Cerrado)");
    //Serial.println("=======================================================");

    estacionCNC.init();
    
    // Inicia la banda girando al infinito en segundo plano (y el servo durmiente)
    minibanda.init(estacionCNC.obtenerEngine()); 
    
    redSITM.init();
    
    // --- LÓGICA DE PROTECCIÓN TÉRMICA Y MECÁNICA ---
    // Acercamiento suave del Eje Z por gravedad para evitar impactos
    estacionCNC.velocidadZ = estacionCNC.velocidadZ / 2; 
    estacionCNC.accelZ = estacionCNC.accelZ / 2;         
    estacionCNC.homeAxis();
    
    // Restaura las velocidades agresivas de producción desde la NVRAM
    estacionCNC.cargarConfiguracion(); 
    //Serial.println("[SISTEMA] Velocidades de produccion restauradas. Listo para operar.");
}

// ==============================================================================
// LOOP: ORQUESTADOR ASÍNCRONO Y ROUTER HMI
// ==============================================================================
void loop() {
    // 1. Mantiene vivo el servidor WiFi/Telnet
    redSITM.loop(); 
    
    // 2. VIGILANCIA DE LAZO CERRADO (Velocidad de la luz)
    minibanda.vigilarSensores(); 

    // 3. LECTURA DE COMANDOS DEL USUARIO
    String comando = redSITM.leerComandos();
    
    if (comando.length() > 0) {
        comando.trim();
        comando.toUpperCase();
        
        estacionCNC.maquinaEnReposo = false; 

        // ======================================================================
        // 🧠 ENRUTADOR INTELIGENTE (Routing Industrial Optimizado)
        // ======================================================================
        // INFOS e INFORPM ya no están aquí, así que caen al 'else' directo al hardware
        if (comando.startsWith("T") || comando == "TEST" || comando == "INFO" || 
            comando == "HELP" || comando == "STOP" || 
            comando.startsWith("G1X") || comando.startsWith("G2X") || comando.startsWith("G3X") || 
            comando.startsWith("G4X") || comando.startsWith("Y")) {
                
            // Va al Cerebro de Rutinas (Lógica matricial, bucles y producción)
            controlRutinas.procesarRutina(comando);
            
        } else {
            // Va al Músculo CNC (Telemetría de latencia cero, motores y memoria)
            estacionCNC.procesarComandoSerial(comando);
        }

        estacionCNC.maquinaEnReposo = true;  
    }
}