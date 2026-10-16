# Barullo FM

Instrumento VST3 / AU / app independiente con el chip FM de 2 operadores emulado registro a registro, tal como lo calibramos con la grabación del chip real. No usa muestras: el chip se calcula muestra a muestra, así que **no tiene latencia** (latencia declarada al DAW: 0).

## Qué hace igual que el teclado

- **El chip**: el mismo motor que el emulador web, comprobado muestra a muestra: 28,9 millones de muestras sin una sola diferencia.
- **Los 100 sonidos**, con sus parámetros originales; los marcados "·2" usan dos canales por nota (izquierda + derecha).
- **8 canales**, con el mismo reparto que el teclado: una nota nueva roba el canal más antiguo.
- **Movimiento**: los registros que el teclado reescribe mientras suena la nota (vibratos, trémolos, barridos), grabados del teclado real. Aquí van a la muestra exacta. Se repiten desde los 6 s si mantienes la nota más de lo grabado.
- **Caída al soltar**: la de cada sonido, grabada con el espía.
- **Ecualizador "Altavoz"** (tu ajuste) o "Plano". Opcionalmente, **saturación de altavoz pequeño**.
- **MIDI**: notas, pedal de sostén (CC64), "all notes off" (CC120/123) y **cambio de programa = número de sonido** (0–99). Los 100 sonidos también aparecen como programas en el DAW.

## Novedades (versión 0.2)

- **Sin aliasing.** La web pasa el chip de 55 930 Hz a la frecuencia del DAW con interpolación lineal. Eso dobla hacia abajo las imágenes de lo que el chip tiene entre 8 y 28 kHz. En sonidos brillantes (sitar, órganos, metales) ese ruido inarmónico queda solo 23–39 dB por debajo de la nota.
  - Ahora se usa un filtro con el mismo tono que la web (así el ecualizador "Altavoz" sigue valiendo), pero con el aliasing a −88…−97 dB.
  - Es de **fase mínima**: solo usa muestras pasadas, así que no hay que esperar a nada. El retraso es de 0,07 ms, como estar 2 cm más lejos del altavoz.
- **Editor de sonidos**, el mismo que el del emulador web:
  - modulador y portadora: multiplicador, nivel, AR, DR, SL, RR, KSR y media onda;
  - canal: realimentación, enlace de frecuencia, izquierda y derecha;
  - partes A y B en los sonidos de dos canales.
  - Los cambios suenan al momento, se guardan con el proyecto y "Restaurar original" los deshace. Los sonidos editados llevan un punto naranja en la rejilla.
  - Si la película de un sonido reescribe registros mientras suena, el editor lo avisa.
- **Bend de tono** (de 0 a 12 semitonos; 2 por defecto). Reescribe la frecuencia del chip igual que hace el teclado con sus vibratos.
- **Rueda de modulación**: el vibrato por software del emulador (5,5 Hz, hasta ±50 cents, reescrito cada 12 ms).
- **Velocidad** (0 % por defecto = como el teclado, que no tiene). Baja el nivel de la portadora al tocar suave.
- **Ventana redimensionable**, entre el 60 % y el 200 %. El tamaño se guarda con el proyecto.

Con bend, rueda y velocidad a cero, las escrituras al chip son byte a byte las mismas que en la versión anterior, que coincide con el teclado.

## Novedades (versión 0.4)

- **Nuevo nombre: Barullo FM.**
- **Saturación domada.**
  - Antes subía el volumen de 12 a 20 dB y recortaba en seco. Ahora compensa el volumen, que queda parecido al encenderla, y la curva es suave.
  - Al final de todo hay un limitador con techo a −1 dBFS: nunca satura la salida, ni con 64 notas.
- **Sin límite** (en Funciones).
  - Apagado: 8 canales, como el teclado. Son 8 notas a la vez, o 4 con los sonidos dobles (×2); con una más, se corta la más antigua.
  - Encendido: chips virtuales iguales al original, hasta 64 canales, y ya no se corta ninguna nota. Los chips que no suenan no gastan CPU.
- **Sonidos dobles bien nombrados:**
  - 12 son estéreo: una parte sale por la izquierda (L) y otra por la derecha (R).
  - 6 son dos capas que salen las dos por los dos lados: Honky-tonk, Pipe organ, Brass ensemble 1 y 3, 12-string y Golf.
  - El editor y el display lo dicen según los registros de cada parte.

## Interfaz (versión 0.3)

- **Colores del teclado original:**
  - cuerpo negro y botones de goma gris con luz verde;
  - la lista de sonidos impresa en columnas, con los números alternando verde agua y ámbar;
  - rótulos en verde;
  - display de LED rojo de siete segmentos y faders con la raya roja.
- **La rejilla del altavoz** es el medidor de nivel: los agujeros se encienden desde el centro.
- **Página «Chip»:** el chip dibujado desde arriba, con sus 18 patas y sus nombres (A0, /WE, D0–D7, reloj, GND, +5 V; las 4 sin identificar llevan «?»).
  - Dentro está su memoria de registros, en vivo: cada casilla se enciende cuando el teclado la escribe.
  - Las patas D0–D7 muestran los bits del último byte que ha pasado por el bus.
- **Editor:** cada operador enseña su envolvente dibujada en LED, y el canal un esquema de la señal (modulador → portadora → salidas, con la realimentación).
- Tipografía Barlow, de licencia libre OFL, incluida en `Resources/`.

## Distribuir (Mac, Windows y Linux)

GitHub compila en sus propias máquinas. En cada subida comprueba que todo compila y pasa las pruebas: prueba de sonido, pluginval al nivel 10 y auval en Mac.

Para **publicar una versión**, sube una etiqueta:

```
git tag v0.4.0 && git push origin v0.4.0
```

La página *Releases* del repositorio se llena sola con:

| Sistema | Archivos |
|---|---|
| Mac (Apple Silicon e Intel) | `BarulloFM-…-mac.pkg` (instalador) y `…-mac.zip` |
| Windows x64 | `BarulloFM-…-windows-setup.exe` (instalador) y `…-windows-x64.zip` |
| Linux x86_64 | `BarulloFM-…-linux-x86_64.tar.gz` (VST3, LV2 y app, con `instalar.sh`) |

La versión sale de la línea `project(BarulloFM VERSION …)` de `CMakeLists.txt`. Súbela antes de etiquetar.

**Firma.** Sin certificados, los instaladores funcionan, pero Mac y Windows avisan la primera vez.
- Si algún día tienes cuenta de desarrollador de Apple, añade estos *secrets* en GitHub y el .pkg saldrá firmado y notarizado, sin avisos: `MACOS_CERT_P12` (el .p12 en base64), `MACOS_CERT_PASSWORD`, `MACOS_DEV_ID`, `MACOS_INSTALLER_ID`, `APPLE_ID`, `APPLE_TEAM_ID` y `APPLE_APP_PASSWORD`.

## Instalar en el Mac (Apple Silicon)

1. Abre **Terminal** en esta carpeta.
2. Escribe `./build_mac.sh` y pulsa Enter.
3. Si es la primera vez, puede pedirte instalar las herramientas de Apple: acepta y vuelve a ejecutar.

El script descarga JUCE, compila (versión universal) e instala:
- VST3 → `~/Library/Audio/Plug-Ins/VST3/Barullo FM.vst3`
- AU → `~/Library/Audio/Plug-Ins/Components/Barullo FM.component`
- App → `~/Applications/Barullo FM.app`

Después reabre el DAW y busca "Barullo FM" entre los instrumentos.

### Alternativa sin compilar en tu Mac

Sube esta carpeta a un repositorio de GitHub. El flujo `.github/workflows/build.yml` compila en un Mac de GitHub, pasa la prueba y valida el AU. Después deja un `BarulloFM-mac.zip` para descargar en la pestaña *Actions*.

Al descomprimirlo, el Mac lo marca como descargado de internet. Para quitar esa marca, ejecuta esto una vez sobre cada archivo (por ejemplo, el VST3):

```
xattr -cr "Barullo FM.vst3" && codesign --force --deep -s - "Barullo FM.vst3"
```

## Estructura

| Archivo | Qué es |
|---|---|
| `Source/Chip.h` | El chip: fases de 19 bits, seno logarítmico + tabla exponencial, envolvente de 9 bits, realimentación, media onda; corre a reloj/64 |
| `Source/Resampler.h` | Paso a la frecuencia del DAW sin aliasing (filtro de fase mínima, se diseña al arrancar) |
| `Source/Synth.h` | El "firmware": reparto de canales, carga de sonidos, películas, caída al soltar, pedal |
| `Source/Fx.h` | Filtro de salida, ecualizador (mismas fórmulas que el navegador), compensación, saturación y limitador sin anticipación |
| `Source/VoiceData.h` | Los 100 sonidos, las 36 741 escrituras de las películas y las caídas, exportadas del emulador web por `tools/export_data.js` |
| `Source/Plugin*.cpp` | El plugin JUCE y la interfaz |
| `packaging/` | Instaladores: `package_mac.sh` (.pkg universal), `windows.iss` (Inno Setup), `package_linux.sh` (.tar.gz) |
| `tools/RenderTest.cpp` | Prueba sin interfaz: con bloques de 512 y de 37 muestras el resultado tiene que ser idéntico |

## Pruebas hechas

- **Chip**: render del registro de escrituras completo (11 min) con el motor C++ y con el del emulador web. Diferencia: 0.
- **Firmware**: 260 s con 17 sonidos, acordes, pedal, una nota de 14 s (bucle de película) y robo de canales. Las escrituras al chip coinciden registro a registro con la web. La C++ es más precisa en el tiempo: va a la muestra, no cada 4 ms.
- **Ecualizador**: respuesta al impulso igual a la de Chromium (diferencia de 7·10⁻⁷).
- **Bloques**: con bloques de 512 y de 37 muestras la salida es idéntica.
- **CPU**: unas 115 veces más rápido que tiempo real con el filtro nuevo, es decir, menos del 1 % de un núcleo.
- **Estado**: se guardan el sonido, los sonidos editados y el tamaño de la ventana, y se recuperan iguales.
- **Validación**: pluginval al nivel máximo (10), con pruebas de interfaz. Compila sin avisos con gcc y con clang (el compilador del Mac).
