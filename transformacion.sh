#!/bin/bash

# Deben ser tres argumentos:
# 1. Nombre del archivo de salida MP3 (debes incluir la extensión .mp3)
# 2. Nombre del archivo de video de entrada (debe incluir la extensión, p. ej., .mp4)
# 3. Opcional: 's', 'S', 'y', o 'Y' para borrar el archivo MP3 de salida si ya existe

if [ "$#" -lt 2 ]; then
    echo "Uso: $0 <nombre_archivo.mp3> <nombre_video.mp4> [s para sobrescribir]"
    exit 1
fi

# Verificar si se debe sobrescribir el archivo MP3 si ya existe
if [ "$#" -eq 3 ]; then
    if [ "$3" == 's' ] || [ "$3" == 'S' ] || [ "$3" == 'y' ] || [ "$3" == 'Y' ]; then
        rm -f "$1"
    else
        echo "No se sobrescribirá el archivo MP3 existente."
        exit 1
    fi
fi

# Convertir el video en MP3
ffmpeg -i "$2" -vn -acodec libmp3lame "$1" 2> Errores/error.log
# la ultima parte es para que no salga por pantalla los "erroes" del comando

clear

echo "La conversion de $2 a $1 ha tenido exito"


