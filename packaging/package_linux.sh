#!/bin/bash
# Empaqueta Barullo FM para Linux (x86_64): un .tar.gz con VST3, LV2, la aplicación y un instalador.
# Uso: packaging/package_linux.sh <carpeta de artefactos Release> <versión> <carpeta de salida>
set -eo pipefail
trap 'echo "::error::$(basename "$0") línea $LINENO: $BASH_COMMAND"' ERR
ART="$1"; VER="$2"; OUT="$3"
NAME="Barullo FM"
mkdir -p "$OUT"
WORK="$(mktemp -d)"
D="$WORK/BarulloFM-$VER-linux-x86_64"
mkdir -p "$D"
cp -R "$ART/VST3/$NAME.vst3" "$D/"
cp -R "$ART/LV2/$NAME.lv2" "$D/"
cp "$ART/Standalone/$NAME" "$D/barullo-fm"
strip "$D/barullo-fm" "$D/$NAME.vst3/Contents/x86_64-linux/$NAME.so" "$D/$NAME.lv2/lib$NAME.so" || true

cat > "$D/instalar.sh" <<'EOS'
#!/bin/sh
# Instala Barullo FM para o teu usuario (sen sudo): VST3 en ~/.vst3, LV2 en ~/.lv2, a app en ~/.local/bin
set -e
cd "$(dirname "$0")"
mkdir -p "$HOME/.vst3" "$HOME/.lv2" "$HOME/.local/bin" "$HOME/.local/share/applications"
rm -rf "$HOME/.vst3/Barullo FM.vst3" "$HOME/.lv2/Barullo FM.lv2"
cp -R "Barullo FM.vst3" "$HOME/.vst3/"
cp -R "Barullo FM.lv2" "$HOME/.lv2/"
cp barullo-fm "$HOME/.local/bin/"
cat > "$HOME/.local/share/applications/barullo-fm.desktop" <<EOD
[Desktop Entry]
Type=Application
Name=Barullo FM
Comment=Sintetizador FM de 2 operadores (Escoita)
Exec=$HOME/.local/bin/barullo-fm
Categories=AudioVideo;Audio;Midi;
EOD
echo "Instalado. Volve abrir o teu DAW e busca «Barullo FM»; a app está no menú ou en ~/.local/bin/barullo-fm"
EOS
cat > "$D/desinstalar.sh" <<'EOS'
#!/bin/sh
rm -rf "$HOME/.vst3/Barullo FM.vst3" "$HOME/.lv2/Barullo FM.lv2" "$HOME/.local/bin/barullo-fm" "$HOME/.local/share/applications/barullo-fm.desktop"
echo "Barullo FM desinstalado."
EOS
chmod +x "$D/instalar.sh" "$D/desinstalar.sh" "$D/barullo-fm"
cat > "$D/LEEME.txt" <<'EOS'
BARULLO FM para Linux (x86_64)
Unha ferramenta de Escoita · https://www.escoita.link

Instalar:    ./instalar.sh      (para o teu usuario, sen sudo)
Desinstalar: ./desinstalar.sh

Formatos: VST3 (~/.vst3), LV2 (~/.lv2) e aplicación independente (barullo-fm).
A aplicación precisa ALSA ou JACK; o plugin funciona en calquera DAW con VST3 ou LV2
(Bitwig, Reaper, Ardour, Carla…).
EOS
tar -C "$WORK" -czf "$OUT/BarulloFM-$VER-linux-x86_64.tar.gz" "BarulloFM-$VER-linux-x86_64"
echo "Hecho: $OUT/BarulloFM-$VER-linux-x86_64.tar.gz"
