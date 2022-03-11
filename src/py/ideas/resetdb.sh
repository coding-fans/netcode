#!/bin/sh

# Author: fasion
# Created time: 2022-03-07 18:14:00
# Last Modified by: fasion
# Last Modified time: 2022-03-07 18:20:25

SELF_PATH=`realpath "$0"`
DIR_PATH=`dirname "$SELF_PATH"`

sqlite3 "${DIR_PATH}/ideas.db" < "${DIR_PATH}/ideas.sql"

du -sh "${DIR_PATH}/ideas.db"
