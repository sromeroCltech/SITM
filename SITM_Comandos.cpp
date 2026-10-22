#include "SITM_Sistema.h"

// ==============================================================================
// PROCESAMIENTO DE COMANDOS DE HARDWARE CRUDO (TELNET / USB)
// ==============================================================================
void SITM_CNC::procesarComandoSerial(String comando) {
    comando.trim();
    if (comando.length() == 0) return;

    int separador = comando.indexOf(' ');
    String accion = comando.substring(0, separador);
    accion.toUpperCase();
    int valor = comando.substring(separador + 1).toInt();

    // --- DASHBOARDS DE HARDWARE (Streaming Directo) ---
    if (accion == "INFORPM") {
        redSITM.print("\r\n====== [ PERFIL DE VELOCIDAD Y ACELERACION ] ======\r\n");
        redSITM.print("Eje X - Vel: "); redSITM.print(String(velocidadX)); redSITM.print(" | Accel: "); redSITM.print(String(accelX)); redSITM.print("\r\n");
        redSITM.print("Eje Y - Vel: "); redSITM.print(String(velocidadY)); redSITM.print(" | Accel: "); redSITM.print(String(accelY)); redSITM.print("\r\n");
        redSITM.print("Eje Z - Vel: "); redSITM.print(String(velocidadZ)); redSITM.print(" | Accel: "); redSITM.print(String(accelZ)); redSITM.print("\r\n");
        redSITM.print("===================================================\r\n");
        return;
    }
    else if (accion == "INFOS") {
        redSITM.print("\r\n========= [ AUDITORIA DE SENSORES EN VIVO ] =========\r\n");
        redSITM.print("Sensor Eje X     : "); redSITM.print(digitalRead(X_LIMIT_PIN) == HIGH ? "TOCADO (HIGH)\r\n" : "LIBRE (LOW)\r\n");
        redSITM.print("Sensor Eje Z     : "); redSITM.print(digitalRead(Z_LIMIT_PIN) == HIGH ? "TOCADO (HIGH)\r\n" : "LIBRE (LOW)\r\n");
        redSITM.print("Sensor Eje Y     : "); redSITM.print(digitalRead(Y_LIMIT_PIN) == HIGH ? "TOCADO (HIGH)\r\n" : "LIBRE (LOW)\r\n");
        redSITM.print("TCRT 1 (Ingreso) : "); redSITM.print(digitalRead(PIN_TCRT1) == LOW ? "[DETECTADO]\r\n" : "[VACIO]\r\n");
        redSITM.print("TCRT 2 (Banda)   : "); redSITM.print(digitalRead(PIN_TCRT2) == LOW ? "[DETECTADO]\r\n" : "[VACIO]\r\n");
        redSITM.print("Servo (Libera)   : "); redSITM.print(String(minibanda.anguloLiberacion)); redSITM.print(" grados\r\n");
        redSITM.print("=====================================================\r\n");
        return;
    }

    // --- MOTORES PASO A PASO ---
    else if (accion == "VX") { velocidadX = valor; stepperX->setSpeedInHz(convertirRpmAHz(velocidadX)); redSITM.print("VX: "); redSITM.print(String(velocidadX)); redSITM.print(" RPM\r\n"); } 
    else if (accion == "VY") { velocidadY = valor; stepperY->setSpeedInHz(convertirRpmAHz(velocidadY)); redSITM.print("VY: "); redSITM.print(String(velocidadY)); redSITM.print(" RPM\r\n"); } 
    else if (accion == "VZ") { velocidadZ = valor; stepperZ->setSpeedInHz(convertirRpmAHz(velocidadZ)); redSITM.print("VZ: "); redSITM.print(String(velocidadZ)); redSITM.print(" RPM\r\n"); }
    
    else if (accion == "AX") { accelX = valor; stepperX->setAcceleration(accelX); redSITM.print("AX: "); redSITM.print(String(accelX)); redSITM.print("\r\n"); } 
    else if (accion == "AY") { accelY = valor; stepperY->setAcceleration(accelY); redSITM.print("AY: "); redSITM.print(String(accelY)); redSITM.print("\r\n"); } 
    else if (accion == "AZ") { accelZ = valor; stepperZ->setAcceleration(accelZ); redSITM.print("AZ: "); redSITM.print(String(accelZ)); redSITM.print("\r\n"); }

    // --- POSICIONAMIENTO MANUAL ---
    else if (accion == "PZ") { pasosRecogerZ = valor; redSITM.print("PZ: "); redSITM.print(String(pasosRecogerZ)); redSITM.print("\r\n"); }
    else if (accion == "PDZ") { pasosDejarZ = valor; redSITM.print("PDZ (Dejar): "); redSITM.print(String(pasosDejarZ)); redSITM.print("\r\n"); }
    else if (accion == "PX") { posPruebaX = valor; redSITM.print("PX: "); redSITM.print(String(posPruebaX)); redSITM.print("\r\n"); }
    else if (accion == "PY") { posPruebaY = valor; redSITM.print("PY: "); redSITM.print(String(posPruebaY)); redSITM.print("\r\n"); }
    
    else if (accion == "MV") { redSITM.print("Moviendo X/Y...\r\n"); moverXYEsperar(posPruebaX, posPruebaY); }
    else if (accion == "BAJAR") { redSITM.print("Bajando Z...\r\n"); moverZEsperar(pasosRecogerZ); }
    else if (accion == "SUBIR") { redSITM.print("Subiendo Z a 0...\r\n"); moverZEsperar(0); }
    else if (accion == "HOME") { homeAxis(); }

    // --- ACTUADORES FÍSICOS Y MINIBANDA ---
    else if (accion == "GA") { abrirGripper(); }
    else if (accion == "GC") { cerrarGripper(); }
    else if (accion == "VB") { minibanda.actualizarVelocidadRPM(valor); }
    else if (accion == "DB") { minibanda.detenerManual(); } 
    else if (accion == "EB") { minibanda.arrancarManual(); }
    else if (accion == "SL") { 
        minibanda.anguloLiberacion = valor; 
        minibanda.moverServoPrueba(minibanda.anguloLiberacion);
        redSITM.print("SL (Abierto): "); redSITM.print(String(minibanda.anguloLiberacion)); redSITM.print(" grados\r\n");
    }
    else if (accion == "SR") { 
        minibanda.anguloReposo = valor; 
        minibanda.moverServoPrueba(minibanda.anguloReposo);
        redSITM.print("SR (Cerrado): "); redSITM.print(String(minibanda.anguloReposo)); redSITM.print(" grados\r\n");
    }

    // --- MEMORIA Y SISTEMA ---
    else if (accion == "GUARDAR") { guardarConfiguracion(); }
    else if (accion == "FORMAT") { 
        memoria.begin("sitm_config", false);
        memoria.clear(); 
        memoria.end();
        redSITM.print("[SISTEMA] Memoria Flash FORMATEADA.\r\n-> Escribe 'REBOOT' para cargar valores de fabrica.\r\n"); 
    }
    else if (accion == "REBOOT") {
        redSITM.print("[SISTEMA] Reiniciando ESP32...\r\n");
        delay(500);
        ESP.restart(); 
    }
    else {
        redSITM.print("[ERROR] Comando de hardware invalido.\r\n");
    }
}