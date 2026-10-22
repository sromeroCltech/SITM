#include "SITM_Sistema.h"

// ==============================================================================
// PUENTES DE LÓGICA SENSORIAL
// ==============================================================================
extern bool sensorTocado(uint8_t pin);  
extern bool tuboDetectado(uint8_t pin); 

// ==============================================================================
// 1. ORQUESTADOR Y PANEL DE CALIBRACIÓN DINÁMICA (HMI Y AUDITORÍA)
// ==============================================================================
void SITM_Rutinas::procesarRutina(String comando) {
    comando.trim();
    comando.toUpperCase(); 

    // A. COMANDO DE FATIGA INDUSTRIAL
    if (comando == "TEST") {
        ejecutarTestCalibracion();
        return;
    }

    // B. DASHBOARD DE INFORMACIÓN MATRICIAL (Cero RAM, Máxima Velocidad)
    if (comando == "INFO") {
        redSITM.print("\r\n========= [ MAPA DE CALIBRACION ACTUAL ] =========\r\n");
        
        redSITM.print("Gradilla 1 (X) [C1 a C5] : ");
        for(int i=0; i<5; i++) { redSITM.print(String(coordenadasX_G1[i])); redSITM.print("\t"); }
        redSITM.print("\r\n");

        redSITM.print("Gradilla 2 (X) [C1 a C5] : ");
        for(int i=0; i<5; i++) { redSITM.print(String(coordenadasX_G2[i])); redSITM.print("\t"); }
        redSITM.print("\r\n");

        redSITM.print("Gradilla 3 (X) [C1 a C5] : ");
        for(int i=0; i<5; i++) { redSITM.print(String(coordenadasX_G3[i])); redSITM.print("\t"); }
        redSITM.print("\r\n");

        redSITM.print("Gradilla 4 (X) [C1 a C5] : ");
        for(int i=0; i<5; i++) { redSITM.print(String(coordenadasX_G4[i])); redSITM.print("\t"); }
        redSITM.print("\r\n");

        redSITM.print("Filas      (Y) [F1 a F10]: ");
        for(int i=0; i<10; i++) { redSITM.print(String(coordenadasY[i])); redSITM.print("\t"); }
        redSITM.print("\r\n==================================================\r\n");
        return;
    }

    // C. INTÉRPRETE DE CALIBRACIÓN HMI
    if (comando.startsWith("G1X") || comando.startsWith("G2X") || comando.startsWith("G3X") || comando.startsWith("G4X") || comando.startsWith("Y")) {
        int espacioIdx = comando.indexOf(' ');
        if (espacioIdx > -1) {
            int valorNuevo = comando.substring(espacioIdx + 1).toInt();
            
            if (comando.startsWith("G1X")) {
                int col = comando.substring(3, espacioIdx).toInt() - 1;
                if (col >= 0 && col < 5) {
                    coordenadasX_G1[col] = valorNuevo;        
                    redSITM.print("[CALIBRACION] G1-Columna "); redSITM.print(String(col+1)); redSITM.print(" (X) -> "); redSITM.print(String(valorNuevo)); redSITM.print("\r\n");
                }
            }
            else if (comando.startsWith("G2X")) {
                int col = comando.substring(3, espacioIdx).toInt() - 1;
                if (col >= 0 && col < 5) {
                    coordenadasX_G2[col] = valorNuevo;
                    redSITM.print("[CALIBRACION] G2-Columna "); redSITM.print(String(col+1)); redSITM.print(" (X) -> "); redSITM.print(String(valorNuevo)); redSITM.print("\r\n");
                }
            }
            else if (comando.startsWith("G3X")) { 
                int col = comando.substring(3, espacioIdx).toInt() - 1;
                if (col >= 0 && col < 5) {
                    coordenadasX_G3[col] = valorNuevo;
                    redSITM.print("[CALIBRACION] G3-Columna "); redSITM.print(String(col+1)); redSITM.print(" (X) -> "); redSITM.print(String(valorNuevo)); redSITM.print("\r\n");
                }
            }
            else if (comando.startsWith("G4X")) { 
                int col = comando.substring(3, espacioIdx).toInt() - 1;
                if (col >= 0 && col < 5) {
                    coordenadasX_G4[col] = valorNuevo;
                    redSITM.print("[CALIBRACION] G4-Columna "); redSITM.print(String(col+1)); redSITM.print(" (X) -> "); redSITM.print(String(valorNuevo)); redSITM.print("\r\n");
                }
            }
            else if (comando.startsWith("Y")) {
                int fila = comando.substring(1, espacioIdx).toInt() - 1;
                if (fila >= 0 && fila < 10) {
                    coordenadasY[fila] = valorNuevo;
                    redSITM.print("[CALIBRACION] Fila "); redSITM.print(String(fila+1)); redSITM.print(" (Y) -> "); redSITM.print(String(valorNuevo)); redSITM.print("\r\n");
                }
            }
            return;
        }
    }

    // D. LÓGICA DE PRODUCCIÓN (Tubos T1 al T200)
    comando.replace("T", "");
    int indexTubo = comando.toInt();
    
    if (indexTubo > 0 && indexTubo <= 200) { 
        ejecutarRecoleccion(indexTubo);
    } else {
        redSITM.print("[ERROR] Comando no reconocido. Opciones: T1-T200, TEST, INFO, INFORPM o G1X/G2X/G3X/G4X.\r\n");
    }
}
// ==============================================================================
// 2. OPERACIÓN DIARIA: RECOLECCIÓN Y CLASIFICACIÓN (200 Tubos)
// ==============================================================================
void SITM_Rutinas::ejecutarRecoleccion(int indexTubo) {
    // Seguro Inicial: Banda
    if (!minibanda.tuboListo) {
        redSITM.print("[ALERTA] No hay tubo detectado en TCRT2. Rutina abortada.\r\n");
        return; 
    }
    
    int destinoX = 0;
    int destinoY = 0;
    int idRelativo = 0; 
    String gradillaStr = ""; 

    if (indexTubo <= 50) {
        gradillaStr = "Gradilla 1";
        idRelativo = indexTubo - 1; 
        destinoX = coordenadasX_G1[idRelativo % 5];
    } else if (indexTubo <= 100) {
        gradillaStr = "Gradilla 2";
        idRelativo = indexTubo - 51; 
        destinoX = coordenadasX_G2[idRelativo % 5];
    } else if (indexTubo <= 150) {
        gradillaStr = "Gradilla 3";
        idRelativo = indexTubo - 101; 
        destinoX = coordenadasX_G3[idRelativo % 5];
    } else {
        gradillaStr = "Gradilla 4";
        idRelativo = indexTubo - 151; 
        destinoX = coordenadasX_G4[idRelativo % 5];
    }
    
    int fila = idRelativo / 5;      
    destinoY = coordenadasY[fila];

    redSITM.print("\r\n[RUTINA] Clasificando tubo #"); redSITM.print(String(indexTubo)); redSITM.print(" -> Destino: "); redSITM.print(gradillaStr); redSITM.print("\r\n");

    // 1. VIAJE AL PUNTO DE RECOGIDA
    estacionCNC.moverXYEsperar(estacionCNC.posPruebaX, estacionCNC.posPruebaY);
    
    // 2. BUCLE DE AUTO-RECUPERACIÓN (Verificación de Lazo Cerrado)
    int intentos = 0;
    bool tuboAtrapado = false;

    while (intentos < 3 && !tuboAtrapado) {
        estacionCNC.abrirGripper();
        estacionCNC.moverZEsperar(estacionCNC.pasosRecogerZ);
        estacionCNC.cerrarGripper();
        estacionCNC.moverZEsperar(0); 
        
        vTaskDelay(pdMS_TO_TICKS(150)); 

        if (!tuboDetectado(PIN_TCRT2)) { 
            tuboAtrapado = true;
        } else {
            intentos++;
            redSITM.print("[LAZO CERRADO] TCRT2 sigue detectando. Reintento ("); redSITM.print(String(intentos)); redSITM.print("/3)...\r\n");
        }
    }

    if (!tuboAtrapado) {
        redSITM.print("[ERROR CRITICO] Imposible extraer el tubo. Abortando rutina.\r\n");
        estacionCNC.abrirGripper();
        estacionCNC.homeAxis();
        return;
    }

    // 3. ÉXITO: ACTIVAR BANDA PARA EL SIGUIENTE TUBO
    minibanda.reanudarCiclo(); 
    
    // 4. VIAJE A LA GRADILLA CORRESPONDIENTE Y DESCARGA
    estacionCNC.moverXYEsperar(destinoX, destinoY);
    estacionCNC.moverZEsperar(estacionCNC.pasosDejarZ); // Usando pasosDejarZ para entrega
    estacionCNC.abrirGripper();

    // 5. RETORNO DE ALTA VELOCIDAD
    estacionCNC.homeAxis();
    
    redSITM.print("[RUTINA] Tubo "); redSITM.print(String(indexTubo)); redSITM.print(" en "); redSITM.print(gradillaStr); redSITM.print(" completado.\r\n");
}
// ==============================================================================
// 3. MANTENIMIENTO: BURN-IN TEST (Secuencia de Alta Velocidad Continua)
// ==============================================================================
void SITM_Rutinas::ejecutarTestCalibracion() {
    redSITM.print("\r\n[SISTEMA] Iniciando prueba de fatiga (10 CICLOS) - MODO CONTINUO MAXIMA VELOCIDAD.\r\n");
    redSITM.print("[SISTEMA] Apagando minibanda por seguridad y ahorro energetico...\r\n");
    minibanda.detener(); 

    bool abortoEmergencia = false; 

    // 🔄 SÚPER CICLO: 10 Vueltas
    for (int ciclo = 1; ciclo <= 10; ciclo++) {
        
        redSITM.print("\r\n--- [ INICIANDO CICLO "); redSITM.print(String(ciclo)); redSITM.print(" DE 10 ] ---\r\n");

        for (int i = 1; i <= 200; i++) { 
            
            // BOTÓN DE PÁNICO ASÍNCRONO
            redSITM.loop(); 
            String cmdPausa = redSITM.leerComandos();
            cmdPausa.trim();
            cmdPausa.toUpperCase();
            if (cmdPausa == "STOP") {
                redSITM.print("\r\n[ALERTA] Prueba de fatiga ABORTADA por el usuario.\r\n");
                abortoEmergencia = true;
                break; 
            }

            // Filtro de Telemetría: Hablamos solo cada 50 tubos para no asfixiar la red
            if (i % 50 == 0) {
                redSITM.print("[TEST] Ciclo "); redSITM.print(String(ciclo)); redSITM.print(" - Progreso: Tubo "); redSITM.print(String(i)); redSITM.print("/200 completado.\r\n");
            }

            // 2. RETORNO A HOME
            estacionCNC.homeAxis();
            
            // 3. MOVIMIENTO EN Z EN HOME
            estacionCNC.moverZEsperar(estacionCNC.pasosRecogerZ); 
            estacionCNC.abrirGripper(); 
            delay(250);
            estacionCNC.cerrarGripper(); 
            delay(250);
            estacionCNC.apagarGripper();
            estacionCNC.moverZEsperar(0); 

            // 4. CÁLCULO MATRICIAL INSTANTÁNEO
            int destinoX = 0;
            int destinoY = 0;
            int idRelativo = 0; 

            if (i <= 50) {
                idRelativo = i - 1; 
                destinoX = coordenadasX_G1[idRelativo % 5];
            } else if (i <= 100) {
                idRelativo = i - 51; 
                destinoX = coordenadasX_G2[idRelativo % 5];
            } else if (i <= 150) {
                idRelativo = i - 101; 
                destinoX = coordenadasX_G3[idRelativo % 5];
            } else {
                idRelativo = i - 151; 
                destinoX = coordenadasX_G4[idRelativo % 5];
            }
            
            int fila = idRelativo / 5;      
            destinoY = coordenadasY[fila];

            // 5. VIAJE A LA COORDENADA
            estacionCNC.moverXYEsperar(destinoX, destinoY);

            // 6. TEST EN DESTINO
            estacionCNC.cerrarGripper(); 
            estacionCNC.moverZEsperar(estacionCNC.pasosDejarZ); 
            estacionCNC.moverZEsperar(0); 
            estacionCNC.apagarGripper(); 
        } 

        if (abortoEmergencia) {
            break; 
        }

        redSITM.print("\r\n==========================================\r\n");
        redSITM.print("   [✓] CICLO "); redSITM.print(String(ciclo)); redSITM.print(" COMPLETADO SIN DEMORAS.\r\n");
        redSITM.print("==========================================\r\n");
    }

    // Retorno final seguro
    estacionCNC.homeAxis();
    
    // Reiniciamos estado minibanda
    minibanda.estadoBanda = 0;
    minibanda.tuboListo = false;
    minibanda.tiempoUltimaAccion = millis();
    minibanda.moverServoPrueba(minibanda.anguloLiberacion); 
    
    if (!abortoEmergencia) {
        redSITM.print("\r\n[SISTEMA] Burn-in Test superado. Maquina lista para 200 tubos a tope.\r\n");
    } else {
        redSITM.print("\r\n[SISTEMA] Maquina devuelta a Home. Minibanda reseteada.\r\n");
    }
}