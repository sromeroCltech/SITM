Aquí tienes el manual de sistema (Help/Dashboard) completamente actualizado para reflejar la expansión a 200 tubos y los nuevos comandos de telemetría de latencia cero.

# ================================================================================
S.I.T.M. - MANUAL DE COMANDOS INDUSTRIAL (HMI Y CNC)

---

1. COMANDOS DE RUTINAS Y PRODUCCIÓN (CAPACIDAD MÁXIMA: 200 TUBOS)

---

T[1-200]    : Ejecuta la recolección del tubo en la posición indicada.
Ejemplo: T1   (Busca en Gradilla 1, Columna 1, Fila 1)
Ejemplo: T151 (Busca en Gradilla 4, Columna 1, Fila 1)

---

2. COMANDOS DE DIAGNÓSTICO Y MANTENIMIENTO (CEREBRO - RUTINAS)

---

TEST        : Inicia el "Burn-in Test" (10 ciclos en Modo Continuo Alta Velocidad).
Clasifica las 200 posiciones sin pausas térmicas.
STOP        : Botón de pánico asíncrono. Aborta rutinas (T o TEST)
y regresa el CNC a la posición de Home de forma segura.
INFO        : Imprime el mapa geométrico guardado en RAM para las 4 gradillas.
HELP        : Muestra este menú de comandos.

---

3. COMANDOS DE CALIBRACIÓN MATRICIAL (AJUSTE FINO HMI)

---

G1X[1-5] [v]: Ajusta el eje X de una columna en la Gradilla 1. (Ej. G1X1 150)
G2X[1-5] [v]: Ajusta el eje X de una columna en la Gradilla 2. (Ej. G2X5 11800)
G3X[1-5] [v]: Ajusta el eje X de una columna en la Gradilla 3. (Ej. G3X1 14450)
G4X[1-5] [v]: Ajusta el eje X de una columna en la Gradilla 4. (Ej. G4X1 21100)
Y[1-10] [v] : Ajusta el eje Y de una fila (Afecta a las 4 gradillas). (Ej. Y1 13400)

---

4. TELEMETRÍA Y AUDITORÍA DE HARDWARE (MÚSCULO - CNC - LATENCIA CERO)

---

INFORPM     : Muestra el perfil cinemático en vivo (Velocidad y Aceleración X,Y,Z).
INFOS       : Auditoría de sensores. Muestra el estado físico de los pines limit
(X, Z) y la posición del servo sin frenar el procesador.

---

5. COMANDOS BÁSICOS CNC (CINEMÁTICA Y MOVIMIENTO MANUAL)

---

PX / PY / PZ: Define la posición de prueba del eje X, Y o Z. (Ej. PX 1000).
MV          : Viaja a las coordenadas cargadas previamente en PX y PY.
BAJAR       : Desciende el eje Z hasta los pasos definidos.
SUBIR       : Retorna el eje Z a la posición 0.
VX / VY / VZ: Ajusta la Velocidad máxima del eje en RPM. (Ej. VX 400).
AX / AY / AZ: Ajusta la Aceleración del eje. (Ej. AX 10000).
GC / GA     : Cierra (Activa relé/válvula) o Abre (Desactiva) el Gripper.
HOME        : Envía todos los ejes a su posición cero mecánica.

---

6. CONTROL DE MINIBANDA Y ACTUADORES

---

VB [val]    : Ajusta la velocidad de la minibanda en RPM.
DB          : Detiene el motor de la minibanda por completo.
SL [grados] : Mueve el servo al ángulo de Liberación (Abierto).
SR [grados] : Mueve el servo al ángulo de Reposo (Cerrado).

---

7. SISTEMA Y MEMORIA EEPROM (COSTO CERO)

---

# GUARDAR     : Graba velocidades, aceleraciones y pasos en la memoria Flash.
FORMAT      : Factory Reset. Destruye datos corruptos en la partición Flash.
REBOOT      : Reinicia el chip ESP32 por hardware.