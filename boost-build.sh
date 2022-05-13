#!/bin/bash
cd $1
./b2 $4 --stage-dir=$3 link=static runtime-link=static --with-program_options --with-filesystem --with-date_time --with-chrono --with-random --with-regex --with-thread
./b2 $4 --stage-dir=$3 link=static runtime-link=static --with-program_options --with-filesystem --with-date_time --with-chrono --with-random --with-regex --with-thread headers
