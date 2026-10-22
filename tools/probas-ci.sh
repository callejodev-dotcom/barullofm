#!/bin/bash
# As probas de Barullo FM na CI común (escoita-nucleo): chámase desde a raíz con
# mac, windows ou linux, coa compilación en build/
set -eo pipefail
EXE=""; [ "$1" = windows ] && EXE=".exe"
./build/BarulloRenderTest_artefacts/Release/BarulloRenderTest$EXE prueba.wav
