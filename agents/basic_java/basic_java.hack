#!/bin/bash
CANNONPATH=`readlink -f "$0"`
cd "`dirname "$CANNONPATH"`"
exec java BasicAI
