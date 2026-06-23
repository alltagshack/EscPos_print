#!/bin/bash

# Variablen definieren
WORT="Mega Kackscheiße!"

# Datum und Uhrzeit abrufen
DATUM=$(date +"%d.%m.%Y")
UHRZEIT=$(date +"%H:%M")

# Status seite drucken
#printf "\x1D\x28\x41" > /dev/usb/lp0

printf "\x1B\x40" > /dev/usb/lp0      # reset
printf "\x1B\x74\x13" > /dev/usb/lp0  # Codepage 858
printf "\x1B\x4D\x01" > /dev/usb/lp0  # Font B
printf "\x1B\x45\x01" > /dev/usb/lp0  # Fett an
printf "\x1B\x33\x10" > /dev/usb/lp0  # linefeed mini
printf "Verstoß\\n" | iconv -f UTF-8 -t CP858 > /dev/usb/lp0
printf "\x1B\x33\x28" > /dev/usb/lp0  # linefeed normal
printf "verbales Moralit\x84tsstatut\\n" > /dev/usb/lp0
printf "\x1B\xC1\x01" > /dev/usb/lp0  # CPI Mode
printf "\x1B\x45\x00" > /dev/usb/lp0  # Fett aus
printf "\x1B\x33\x10" > /dev/usb/lp0  # linefeed mini
# Normale Höhe
printf "Datum:   $DATUM\\n" > /dev/usb/lp0
printf "Uhrzeit: $UHRZEIT\\n" > /dev/usb/lp0

# WORT groß ausgeben
printf "\x1D\x21\x11" > /dev/usb/lp0  # Textgröße ändern für groß
printf "$WORT\\n" | iconv -f UTF-8 -t CP858  > /dev/usb/lp0
printf "\\n\\n\\n" > /dev/usb/lp0 
printf "\x1B\x21\x00" > /dev/usb/lp0  # Textgröße zurücksetzen
