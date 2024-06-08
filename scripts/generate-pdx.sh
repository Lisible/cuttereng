#!/bin/sh
mkdir Source
cp basic-app/pdex.elf Source/pdex.elf
pdc Source basic-app.pdx
rm -rf Source

