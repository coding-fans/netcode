#!/bin/sh

# Author: fasion
# Created time: 2022-03-07 18:14:00
# Last Modified by: fasion
# Last Modified time: 2022-03-12 14:24:42

SELF_PATH=`realpath "$0"`
DIR_PATH=`dirname "$SELF_PATH"`

sqlite3 "${DIR_PATH}/ideahub.db" < "${DIR_PATH}/ideahub.sql"

du -sh "${DIR_PATH}/ideahub.db"
