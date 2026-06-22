#!/bin/bash

PRINTER=/dev/usb/lp0

function Init () {
    printf "\x1B\x40" > $PRINTER      # reset
    printf "\x1B\x74\x13" > $PRINTER  # Codepage 858
}

function Print () {
    printf "$1\\n" | iconv -f UTF-8 -t CP858 > $PRINTER
}

#---------------------------------------------

function High () {
    printf "\x1B\xC1\x01" > $PRINTER  # CPI Mode
}

function Low () {
    printf "\x1B\xC1\x00" > $PRINTER
}

#---------------------------------------------

function FontA () {
    printf "\x1B\x4D\x00" > $PRINTER  # Font A
}

function FontB () {
    printf "\x1B\x4D\x01" > $PRINTER  # Font B
}

#---------------------------------------------

function Underline () {
    printf "\x1B\x2D\x01" > $PRINTER
}

function NoUnderline () {
    printf "\x1B\x2D\x00" > $PRINTER
}

#---------------------------------------------

function Nl0 () {
    printf "\x1B\x33\x10" > $PRINTER  # linefeed mini
}

function Nl1 () {
    printf "\x1B\x33\x28" > $PRINTER  # linefeed normal
}

#---------------------------------------------

function Bold () {
    printf "\x1B\x45\x01" > $PRINTER
}
function Normal () {
    printf "\x1B\x45\x00" > $PRINTER
}

#---------------------------------------------

function FontAx2Height () {
    printf "\x1B\x21\x10" > $PRINTER
}
function FontAx2Width () {
    printf "\x1B\x21\x20" > $PRINTER
}
function FontAx2 () {
    printf "\x1B\x21\x30" > $PRINTER
}

function LeftMarginX18 () {
    printf "\x1D\x4C\x18\x00" > $PRINTER
}

Init
Print "Hallo zusammen. Was ist hier los? Geht es noch so wie es soll?"
LeftMarginX18
Print "Hallo zusammen. Was ist hier los? Geht es noch so wie es soll?"

