#!/bin/bash
cd $1
$1/bootstrap.sh
if [ $# -gt 1 ]
  then
    cat $2 >> $1/project-config.jam
fi
