# Barullo FM

Instrumento VST3 / AU / app independente co chip FM de 2 operadores emulado rexistro a rexistro, tal como o calibramos coa gravación do chip real. Non usa mostras gravadas: o chip calcúlase mostra a mostra. A única latencia é a do limitador de seguridade, que mira 1,5 ms por diante para baixar suave antes dos picos (o DAW compénsaa só).

## Que fai igual ca o teclado

- **O chip**: o mesmo motor ca o emulador web, comprobado mostra a mostra: 28,9 millóns de mostras sen unha soa diferenza.
- **Os 100 sons**, cos seus parámetros orixinais; os marcados «×2» usan dúas canles por nota (esquerda + dereita).
- **8 canles**, co mesmo reparto ca o teclado: unha nota nova rouba a canle máis antiga.
- **Movemento**: os rexistros que o teclado reescribe mentres soa a nota (vibratos, trémolos, varridos), gravados do teclado real. Aquí van á mostra exacta. Repítense desde os 6 s se mantés a nota máis do gravado.
- **Caída ao soltar**: a de cada son, gravada co espía.
- **Ecualizador «Altofalante»** (o teu axuste) ou «Plano». Opcionalmente, **saturación de altofalante pequeno**.
- **MIDI**: notas, pedal de sostemento (CC64), «all notes off» (CC120/123) e **cambio de programa = número de son** (0–99). Os 100 sons tamén aparecen como programas no DAW.

## Novidades (na seguinte versión)

- **Presets propios**: a tecla PRESETS garda e carga o estado enteiro (o son escollido, os sons editados e os axustes) en ficheiros `.bfmpreset`, en Documentos/Barullo FM/Presets. No mesmo menú: abrir a carpeta e copiar entre A e B.
- **A/B**: dous xogos de axustes para comparar, cos sons editados incluídos. Cambiar de lado garda o actual.
- **Desfacer e refacer** (as frechas da cabeceira): os axustes e tamén as edicións dos sons e «Restaurar o orixinal». Cada clic é un paso.
- Os tres veñen do núcleo común de Escoita, igual ca en Barullo Tune; tamén o pedal de sostemento, a escolla da canle e o limitador de seguridade, sen cambiar o son nin nun bit.
- O aviso das versións novas vai agora debaixo de ESCOITA.

## Novidades (versión 0.4.1)

- **Aviso das versións novas**: unha vez ao día, pregúntase a GitHub cal é a última versión (sen mandar ningún dato) e, se hai unha nova, avísase á beira de ESCOITA. cunha ligazón ás descargas. Co clic dereito en ESCOITA. vese a versión que tes, ábrense as descargas e desactívase o aviso. En Linux pregúntase co curl ou o wget do sistema.

## Novidades (versión 0.2)

- **Sen aliasing.** A web pasa o chip de 55 930 Hz á frecuencia do DAW con interpolación lineal. Iso dobra cara abaixo as imaxes do que o chip ten entre 8 e 28 kHz. En sons brillantes (sitar, órganos, metais) ese ruído inarmónico queda só 23–39 dB por debaixo da nota.
  - Agora úsase un filtro co mesmo ton ca a web (así o ecualizador «Altofalante» segue valendo), pero co aliasing a −88…−97 dB.
  - É de **fase mínima**: só usa mostras pasadas, así que non hai que agardar por nada. O atraso é de 0,07 ms, coma estar 2 cm máis lonxe do altofalante.
- **Editor de sons**, o mesmo ca o do emulador web:
  - modulador e portadora: multiplicador, nivel, AR, DR, SL, RR, KSR e media onda;
  - canle: realimentación, enlace de frecuencia, esquerda e dereita;
  - partes A e B nos sons de dúas canles.
  - Os cambios soan ao momento, gárdanse co proxecto e «Restaurar orixinal» desfaios. Os sons editados levan un punto laranxa na grella.
  - Se a película dun son reescribe rexistros mentres soa, o editor avísao.
- **Bend de ton** (de 0 a 12 semitonos; 2 por defecto). Reescribe a frecuencia do chip igual ca fai o teclado cos seus vibratos.
- **Roda de modulación**: o vibrato por software do emulador (5,5 Hz, ata ±50 cents, reescrito cada 12 ms).
- **Velocidade** (0 % por defecto = coma o teclado, que non ten). Baixa o nivel da portadora ao tocar suave.
- **Xanela redimensionable**, entre o 60 % e o 200 %. O tamaño gárdase co proxecto.

Con bend, roda e velocidade a cero, as escrituras ao chip son byte a byte as mesmas ca na versión anterior, que coincide co teclado.

## Novidades (versión 0.4)

- **Nome novo: Barullo FM.**
- **Saturación domada.**
  - Antes subía o volume de 12 a 20 dB e recortaba en seco. Agora compensa o volume, que queda parecido ao acendela, e a curva é suave.
  - Ao final de todo hai un limitador con teito a −1 dBFS: nunca satura a saída, nin con 64 notas.
- **Sen límite** (en Funcións).
  - Apagado: 8 canles, coma o teclado. Son 8 notas á vez, ou 4 cos sons dobres (×2); con unha máis, córtase a máis antiga.
  - Aceso: chips virtuais iguais ao orixinal, ata 64 canles, e xa non se corta ningunha nota. Os chips que non soan non gastan CPU.
- **Sons dobres ben nomeados:**
  - 12 son estéreo: unha parte sae pola esquerda (L) e outra pola dereita (R).
  - 6 son dúas capas que saen as dúas polos dous lados: Honky-tonk, Pipe organ, Brass ensemble 1 e 3, 12-string e Golf.
  - O editor e o display dino segundo os rexistros de cada parte.

## Interface (versión 0.3)

- **Cores do teclado orixinal:**
  - corpo negro e botóns de goma gris con luz verde;
  - a lista de sons impresa en columnas, cos números alternando verde auga e ámbar;
  - rótulos en verde;
  - display de LED vermello de sete segmentos e faders coa raia vermella.
- **A grella do altofalante** é o medidor de nivel: os buracos acéndense desde o centro.
- **Páxina «Chip»:** o chip debuxado desde arriba, coas súas 18 patas e os seus nomes (A0, /WE, D0–D7, reloxo, GND, +5 V; as 4 sen identificar levan «?»).
  - Dentro está a súa memoria de rexistros, en vivo: cada casa acéndese cando o teclado a escribe.
  - As patas D0–D7 amosan os bits do último byte que pasou polo bus.
- **Editor:** cada operador amosa a súa envolvente debuxada en LED, e a canle un esquema do sinal (modulador → portadora → saídas, coa realimentación).
- Tipografía Barlow, de licenza libre OFL, incluída en `Resources/`.

## Distribuír (Mac, Windows e Linux)

GitHub compila nas súas propias máquinas. En cada subida comproba que todo compila e pasa as probas: proba de son, pluginval ao nivel 10 e auval no Mac.

Para **publicar unha versión**, sube unha etiqueta:

```
git tag v0.5.0 && git push origin v0.5.0
```

A páxina *Releases* do repositorio énchese soa con:

| Sistema | Ficheiros |
|---|---|
| Mac (Apple Silicon e Intel) | `BarulloFM-…-mac.pkg` (instalador) e `…-mac.zip` |
| Windows x64 | `BarulloFM-…-windows-setup.exe` (instalador) e `…-windows-x64.zip` |
| Linux x86_64 | `BarulloFM-…-linux-x86_64.tar.gz` (VST3, LV2 e app, con `instalar.sh`) |

A versión sae da liña `project(BarulloFM VERSION …)` de `CMakeLists.txt`. Sóbea antes de etiquetar.

**Sinatura.** Sen certificados, os instaladores funcionan, pero Mac e Windows avisan a primeira vez.
- Se algún día tes conta de desenvolvedor de Apple, engade estes *secrets* en GitHub e o .pkg sairá firmado e notarizado, sen avisos: `MACOS_CERT_P12` (o .p12 en base64), `MACOS_CERT_PASSWORD`, `MACOS_DEV_ID`, `MACOS_INSTALLER_ID`, `APPLE_ID`, `APPLE_TEAM_ID` e `APPLE_APP_PASSWORD`.

## Instalar no Mac (Apple Silicon)

1. Abre o **Terminal** neste cartafol.
2. Escribe `./build_mac.sh` e preme Intro.
3. Se é a primeira vez, pode pedirche instalar as ferramentas de Apple: acepta e volve executalo.

O script descarga JUCE, compila (versión universal) e instala:
- VST3 → `~/Library/Audio/Plug-Ins/VST3/Barullo FM.vst3`
- AU → `~/Library/Audio/Plug-Ins/Components/Barullo FM.component`
- App → `~/Applications/Barullo FM.app`

Despois volve abrir o DAW e busca «Barullo FM» entre os instrumentos.

### Sen compilar no teu Mac

Sube este cartafol a un repositorio de GitHub. O fluxo `.github/workflows/build.yml` compila nun Mac de GitHub, pasa a proba e valida o AU. Despois deixa o `BarulloFM-…-mac.pkg` e o `BarulloFM-…-mac.zip` para descargar na lapela *Actions* (no artefacto «mac»).

Ao descomprimilo, o Mac márcao como descargado de internet. Para quitar esa marca, executa isto unha vez sobre cada ficheiro (por exemplo, o VST3):

```
xattr -cr "Barullo FM.vst3" && codesign --force --deep -s - "Barullo FM.vst3"
```

## Estrutura

| Ficheiro | Que é |
|---|---|
| `Source/Chip.h` | O chip: fases de 19 bits, seno logarítmico + táboa exponencial, envolvente de 9 bits, realimentación, media onda; corre a reloxo/64 |
| `Source/Resampler.h` | Paso á frecuencia do DAW sen aliasing (filtro de fase mínima, deséñase ao arrancar) |
| `Source/Synth.h` | O «firmware»: reparto de canles, carga de sons, películas, caída ao soltar, pedal (o pedal e a escolla da canle, do núcleo común) |
| `Source/Fx.h` | Filtro de saída, ecualizador (as mesmas fórmulas ca o navegador), compensación, saturación e limitador sen anticipación e, ao final, o teito a −1 dBFS do núcleo común, con 1,5 ms de anticipación |
| `Source/VoiceData.h` | Os 100 sons, as 36 741 escrituras das películas e as caídas, exportadas do emulador web por `tools/export_data.js` |
| `Source/Plugin*.cpp` | O plugin JUCE e a interface |
| `packaging/` | Os datos do produto (`produto.env`), os LEEME dos instaladores e as notas da versión; os instaladores e a CI son do núcleo común, [escoita-nucleo](https://github.com/callejodev-dotcom/escoita-nucleo) (versión fixada en `CMakeLists.txt`, `ESCOITA_NUCLEO_VERSION`, e en `.github/workflows/build.yml`) |
| `tools/probas-ci.sh` | As probas que pasa a CI en cada sistema |
| `tools/RenderTest.cpp` | Proba sen interface: con bloques de 512 e de 37 mostras o resultado ten que ser idéntico; imprime a **pegada da saída** (un cambio de organización non pode cambiala nin nun bit); A/B, presets e desfacer |
| `tools/seqtest.cpp` | Toca un `seq.txt` co firmware e garda as escrituras ao chip e o audio, para comparar co emulador web (`BarulloSeqTest`) |

## Probas feitas

- **Chip**: render do rexistro de escrituras completo (11 min) co motor C++ e co do emulador web. Diferenza: 0.
- **Firmware**: 260 s con 17 sons, acordes, pedal, unha nota de 14 s (bucle de película) e roubo de canles. As escrituras ao chip coinciden rexistro a rexistro coa web. A C++ é máis precisa no tempo: vai á mostra, non cada 4 ms.
- **Ecualizador**: resposta ao impulso igual á de Chromium (diferenza de 7·10⁻⁷).
- **Bloques**: con bloques de 512 e de 37 mostras a saída é idéntica.
- **CPU**: unhas 115 veces máis rápido ca tempo real co filtro novo, é dicir, menos do 1 % dun núcleo.
- **Estado**: gárdanse o son, os sons editados e o tamaño da xanela, e recupéranse iguais.
- **A/B, presets e desfacer**: cada lado garda os seus axustes e sons editados; un preset gárdase e cárgase igual; desfacer e refacer un axuste, unha edición e «Restaurar o orixinal».
- **Pegada da saída** (`34c2be491b7c4300`) e **pegada do roubo** (acordes de doce notas dun son ×2 co pedal, `d69b1075164b256b`): iguais antes e despois de levar o pedal, a escolla da canle e o limitador ao núcleo común.
- **Validación**: pluginval ao nivel máximo (10), con probas de interface. Compila sen avisos con gcc e con clang (o compilador do Mac).

## Licenza

Barullo FM é software libre: publícase baixo a licenza **GNU Affero General Public License v3.0** (ficheiro `LICENSE`). Podes usalo, estudalo, modificalo e compartilo; se distribúes unha versión modificada, ten que ir co seu código e baixo a mesma licenza.

Usa JUCE 8 baixo a súa licenza AGPLv3. A tipografía Barlow vai coa súa licenza OFL (`Resources/Barlow-OFL.txt`).
