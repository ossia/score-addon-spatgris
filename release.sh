#!/bin/bash
rm -rf release
mkdir -p release

cp -rf SpatGRIS *.{hpp,cpp,txt,json} LICENSE release/

mv release score-addon-spatgris
7z a score-addon-spatgris.zip score-addon-spatgris
