#!/bin/sh

# This script is used to create the configure script
cd config
autoconf configure.in > ../configure
cd ..
chmod 755 configure
