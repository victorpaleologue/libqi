#!/bin/bash
cd $1
$1/bootstrap.sh
cat $2 > $1/user-config.jam
cat $2 >> $1/project-config.jam